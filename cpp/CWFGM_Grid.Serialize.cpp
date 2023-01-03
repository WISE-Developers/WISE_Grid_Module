/**
 * WISE_Grid_Module: CWFGM_Grid.Serialize.h
 * Copyright (C) 2023  WISE
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef DEBUG
#include <cassert>
#endif
#include <exception>
#include "ICWFGM_Fuel.h"
#include "ICWFGM_GridEngine.h"
#include "GridCom_ext.h"
#include "FireEngine_ext.h"
#include "CWFGM_Grid.h"
#include "CWFGM_FuelMap.h"
#include "angles.h"
#include "comcodes.h"
#include <cpl_string.h>
#include "CoordinateConverter.h"
#include "gdalclient.h"
#include "doubleBuilder.h"
#include "GDALextras.h"
#include "boost_compression.h"
#include "str_printf.h"
#include <errno.h>
#include <float.h>
#include <stdio.h>
#include <GDALExporter.h>
#include <ctime>
#include <fstream>
#include "GDALImporter.h"
#include "filesystem.hpp"
#include <boost/algorithm/string/predicate.hpp>

using namespace GDALExtras;
using namespace CONSTANTS_NAMESPACE;
#if __cplusplus<201700 || (GCC_VERSION > NO_GCC && GCC_VERSION < GCC_8)
namespace fs = std::experimental::filesystem;
#else
namespace fs = std::filesystem;
#endif

HRESULT CCWFGM_Grid::ImportGrid(const std::string & prj_file_name, const std::string & grid_file_name, bool forcePrj, long *fail_index) {
	SEM_BOOL engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)								return ERROR_SCENARIO_SIMULATION_RUNNING;

	if (!m_fuelMap)								{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }		// we need a fuelMap to load correctly

	double scale;
	std::string grid(grid_file_name), prj(prj_file_name);

	GDALImporter importer;
	if (importer.Import(grid.c_str(), nullptr) != GDALImporter::ImportResult::OK) {
		return E_FAIL;
	}

	CSemaphoreEngage lock(GDALClient::GDALClient::getGDALMutex(), SEM_TRUE);

	const char* projection;
	OGRSpatialReferenceH sourceSRS;
	if (forcePrj || (importer.projection()[0] == '\0')) {
		try {
			fs::path p(prj_file_name.c_str());
			if (boost::iequals(p.extension().string(), _T(".prj"))) {
				std::ifstream t(prj);
				std::stringstream buffer;
				buffer << t.rdbuf();
				prj = buffer.str();
				sourceSRS = CCoordinateConverter::CreateSpatialReferenceFromStr(prj.c_str());
			}
			else
				sourceSRS = CCoordinateConverter::CreateSpatialReferenceFromWkt(prj.c_str());
		}
		catch (...) {
			sourceSRS = CCoordinateConverter::CreateSpatialReferenceFromWkt(prj.c_str());
		}

		projection = prj.c_str();
	}
	else {
		projection = importer.projection();
		sourceSRS = CCoordinateConverter::CreateSpatialReferenceFromWkt(projection);
	}

	if (m_sourceSRS) {
		if (!sourceSRS)
			return ERROR_GRID_LOCATION_OUT_OF_RANGE;
		if (!OSRIsSame(m_sourceSRS, sourceSRS, false)) {
			OSRDestroySpatialReference(sourceSRS);
			return ERROR_GRID_LOCATION_OUT_OF_RANGE;
		}
		OSRDestroySpatialReference(sourceSRS);
	}
	else if (!sourceSRS)
		return E_FAIL;
	else
		m_sourceSRS = sourceSRS;
	m_projectionContents = projection;

	char *units = nullptr;
	scale = OSRGetLinearUnits(m_sourceSRS, &units);
	m_units = units;

	std::uint16_t		xsize,
			ysize;					// size of our plots
	double		xllcorner, yllcorner;
	double		resolution;
	std::int32_t		noData;

	xsize = importer.xSize();
	ysize = importer.ySize();
	xllcorner = importer.lowerLeftX();
	yllcorner = importer.lowerLeftY();
	resolution = importer.xPixelSize();
	noData = importer.nodata();

	//if initialized
	if (m_baseGrid.m_xsize != (std::uint16_t)-1) {
		if ((m_baseGrid.m_xsize != xsize) ||
			(m_baseGrid.m_ysize != ysize))
			return ERROR_GRID_SIZE_INCORRECT;
		if (fabs(m_baseGrid.m_resolution - resolution * scale) > 0.000001)
			return ERROR_GRID_UNSUPPORTED_RESOLUTION;
		if ((fabs(m_baseGrid.m_xllcorner - xllcorner) > 0.001) ||
			(fabs(m_baseGrid.m_yllcorner - yllcorner) > 0.001))
			return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	}

	GDALImporter::ImportType data = importer.importType();
	if ((data != GDALImporter::ImportType::LONG) &&
		(data != GDALImporter::ImportType::SHORT) &&
		(data != GDALImporter::ImportType::UCHAR) &&
		(data != GDALImporter::ImportType::USHORT) &&
		(data != GDALImporter::ImportType::ULONG))
		return E_FAIL;

	HRESULT error = S_OK;
	std::uint8_t *fuelArray, *fa1;
	bool *fuelValidArray, *fd1;
	std::int32_t i, index = xsize * ysize;
	try {
		fuelArray = new std::uint8_t[index];
		fuelValidArray = new bool[index];
		memset(fuelArray, -1, index * sizeof(std::uint8_t));	// only really necessary in debug version but will leave it here anyway
		for (i = 0; i < index; i++)
			fuelValidArray[i] = false;
	}
	catch(std::bad_alloc &cme) {
		return E_OUTOFMEMORY;
	}
							//-------- Read Fuel Data ------------------------
	std::int32_t last_index = noData;
	std::uint8_t internal_index;
	for (i = 0, fa1 = fuelArray, fd1 = fuelValidArray; i < index; i++, fa1++, fd1++) {
		std::int32_t fuel_index;
		long export_index;
		ICWFGM_Fuel *fuel;
			fuel_index = importer.integerData(1, i);
		if (fuel_index != noData) {
			if (fuel_index != last_index)
				if (FAILED(m_fuelMap->FuelAtFileIndex(fuel_index, &internal_index, &export_index, &fuel))) {
					delete[] fuelArray;
					delete[] fuelValidArray;
					if (fail_index)
						*fail_index = fuel_index;
					return ERROR_FUELS_FUEL_UNKNOWN;
				}
			*fa1 = internal_index;
			*fd1 = true;
		}
		else
			*fd1 = false;
		last_index = fuel_index;
	}

	if (m_baseGrid.m_fuelArray) {
		delete [] m_baseGrid.m_fuelArray;
		error = SUCCESS_GRID_DATA_UPDATED;
	}
	if (m_baseGrid.m_fuelValidArray)
		delete [] m_baseGrid.m_fuelValidArray;

	m_baseGrid.m_fuelArray = fuelArray;
	m_baseGrid.m_fuelValidArray = fuelValidArray;
	m_flags |= CCWFGMGRID_VALID;
	m_baseGrid.m_xsize = xsize;
	m_baseGrid.m_ysize = ysize;
	m_baseGrid.m_xllcorner = xllcorner;
	m_baseGrid.m_yllcorner = yllcorner;
	m_baseGrid.m_resolution = resolution * scale;
	m_baseGrid.m_iresolution = 1.0 / m_baseGrid.m_resolution;

	bool success = fixWorldLocation();
	if (!success) {
		m_defaultFMC = 120.0;
		m_flags &= ~(CCWFGMGRID_SPECIFIED_FMC_ACTIVE);
		error = ERROR_PROJECTION_UNKNOWN;
	}
	else if (m_worldLocation.InsideNewZealand()) {
		m_flags |= CCWFGMGRID_SPECIFIED_FMC_ACTIVE;
		m_defaultFMC = 145.0;
	}
	else if (m_worldLocation.InsideTasmania()) {
		m_flags |= CCWFGMGRID_SPECIFIED_FMC_ACTIVE;
		m_defaultFMC = 145.0;
	}
	else if (m_worldLocation.InsideAustraliaMainland()) {
		m_flags |= CCWFGMGRID_SPECIFIED_FMC_ACTIVE;
		m_defaultFMC = 145.0;
	}
	else if (!m_worldLocation.InsideCanada()) {
		m_flags |= CCWFGMGRID_SPECIFIED_FMC_ACTIVE;
		m_defaultFMC = 120.0;
	}
	else {
		m_flags &= ~(CCWFGMGRID_SPECIFIED_FMC_ACTIVE);
		m_defaultFMC = 120.0;
	}

	m_bRequiresSave = true;
	return error;
}


HRESULT CCWFGM_Grid::ImportGridWCS(const std::string & url, const std::string & layer, const std::string & username, const std::string & password, XY_Point lowerleft, XY_Point upperright) {
	return E_NOTIMPL;
}


HRESULT CCWFGM_Grid::ImportElevationWCS(const std::string & url, const std::string & layer, const std::string & username, const std::string & password) {
	return E_NOTIMPL;
}


HRESULT CCWFGM_Grid::sizeGrid(Layer *layerThread, CalculationEventParms *parms) {
	bool canGrow = (m_flags & CCWFGMGRID_ALLOW_GIS) ? true : false;
	if (canGrow) {
		if (m_gisGridURL.length()) {
		}
	}

	GridData *gd = m_gridData(layerThread);

	if (!canGrow) {
		if (parms) {
			parms->TargetGridMin.x = parms->CurrentGridMin.x = gd->m_xllcorner;
			parms->TargetGridMin.y = parms->CurrentGridMin.y = gd->m_yllcorner;
			parms->TargetGridMax.x = parms->CurrentGridMax.x = gd->m_xllcorner + gd->m_xsize * gd->m_resolution;
			parms->TargetGridMax.y = parms->CurrentGridMax.y = gd->m_yllcorner + gd->m_ysize * gd->m_resolution;
		}
		return S_OK;
	}

	if (layerThread == 0) {
		if (!parms)
			return E_INVALIDARG;

		SEM_BOOL engaged;
		CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
		if (!engaged)								return ERROR_SCENARIO_SIMULATION_RUNNING;

		// ***** need to set m_resolution here before the below code

		m_initialSize = ceil(m_initialSize * gd->m_iresolution) * gd->m_resolution;
		m_growSize = ceil(m_initialSize * gd->m_iresolution) * gd->m_resolution;

		XY_Rectangle bounds;
		bounds.m_min.x = parms->SimulationMin.x - m_initialSize;
		bounds.m_min.y = parms->SimulationMin.y - m_initialSize;
		bounds.m_max.x = parms->SimulationMax.x + m_initialSize;
		bounds.m_max.y = parms->SimulationMax.y + m_initialSize;
		bounds.m_min *= gd->m_iresolution;
		bounds.m_max *= gd->m_iresolution;
		bounds.m_min.x = floor(bounds.m_min.x);
		bounds.m_min.y = floor(bounds.m_min.y);
		bounds.m_max.x = ceil(bounds.m_max.x);
		bounds.m_max.y = ceil(bounds.m_max.y);
		bounds.m_min *= gd->m_resolution;
		bounds.m_max *= gd->m_resolution;

		if (!(m_flags & CCWFGMGRID_VALID)) {		// not yet initialized for the first time
			parms->TargetGridMin.x = parms->CurrentGridMin.x = bounds.m_min.x;
			parms->TargetGridMin.y = parms->CurrentGridMin.y = bounds.m_min.y;
			parms->TargetGridMax.x = parms->CurrentGridMax.x = bounds.m_max.x;
			parms->TargetGridMax.y = parms->CurrentGridMax.y = bounds.m_max.y;
				// ***** load the data based on CurrentGridMin, CurrentGridMax
			m_flags |= CCWFGMGRID_VALID;
			gd->m_xllcorner = bounds.m_min.x;
			gd->m_yllcorner = bounds.m_min.y;
			gd->m_xsize = (bounds.m_max.x - bounds.m_min.x) * gd->m_iresolution;
			gd->m_ysize = (bounds.m_max.y - bounds.m_min.y) * gd->m_iresolution;
				// ***** need to use copy over code for m_fuelArray, m_elevationArray
			bool success = fixWorldLocation();
			m_bRequiresSave = true;
		}
		else { // need to grow the main base grid
			XY_Rectangle currentBounds, targetBounds;
			currentBounds.m_min.x = parms->CurrentGridMin.x = gd->m_xllcorner;
			currentBounds.m_min.y = parms->CurrentGridMin.y = gd->m_yllcorner;
			currentBounds.m_max.x = parms->CurrentGridMax.x = gd->m_xllcorner + gd->m_xsize * gd->m_resolution;
			currentBounds.m_max.y = parms->CurrentGridMax.y = gd->m_yllcorner + gd->m_ysize * gd->m_resolution;

			targetBounds = currentBounds;
			targetBounds.EncompassRectangle(bounds);
			parms->TargetGridMin.x = currentBounds.m_min.x;
			parms->TargetGridMin.y = currentBounds.m_min.y;
			parms->TargetGridMax.x = currentBounds.m_max.x;
			parms->TargetGridMax.y = currentBounds.m_max.y;
			if (!targetBounds.Equals(currentBounds)) {
				// ***** the object is locked above so you should be able to safely load the new data based on targetBounds, set up the values like above
				gd->m_xllcorner = targetBounds.m_min.x;
				gd->m_yllcorner = targetBounds.m_min.y;
				gd->m_xsize = (targetBounds.m_max.x - targetBounds.m_min.x) * gd->m_iresolution;
				gd->m_ysize = (targetBounds.m_max.y - targetBounds.m_min.y) * gd->m_iresolution;
			}
			bool success = fixWorldLocation();
			m_bRequiresSave = true;
		}
	}
	else {
		XY_Rectangle bounds;
		XY_Rectangle currentBounds, targetBounds, reactionBounds;
		bounds.m_min.x = parms->SimulationMin.x - m_reactionSize;
		bounds.m_min.y = parms->SimulationMin.y - m_reactionSize;
		bounds.m_max.x = parms->SimulationMax.x + m_reactionSize;
		bounds.m_max.y = parms->SimulationMax.y + m_reactionSize;

		currentBounds.m_min.x = parms->CurrentGridMin.x = gd->m_xllcorner;
		currentBounds.m_min.y = parms->CurrentGridMin.y = gd->m_yllcorner;
		currentBounds.m_max.x = parms->CurrentGridMax.x = gd->m_xllcorner + gd->m_xsize * gd->m_resolution;
		currentBounds.m_max.y = parms->CurrentGridMax.y = gd->m_yllcorner + gd->m_ysize * gd->m_resolution;
		reactionBounds = currentBounds;
		currentBounds.EncompassRectangle(reactionBounds);
		if (currentBounds.Equals(reactionBounds)) {		// no change is needed yet, so it's all good
			parms->TargetGridMin.x = parms->CurrentGridMin.x;
			parms->TargetGridMin.y = parms->CurrentGridMin.y;
			parms->TargetGridMax.x = parms->CurrentGridMax.x;
			parms->TargetGridMax.y = parms->CurrentGridMax.y;
			return S_OK;
		}

		GridData *gd0 = m_gridData(nullptr);
		if (gd0 == gd) {
			gd = new GridData();
			assignGridData(layerThread, gd);

		}
		if (currentBounds.m_min.x != reactionBounds.m_min.x)
			parms->TargetGridMin.x = targetBounds.m_min.x = currentBounds.m_min.x - m_growSize;
		if (currentBounds.m_min.y != reactionBounds.m_min.y)
			parms->TargetGridMin.y = targetBounds.m_min.y = currentBounds.m_min.y - m_growSize;
		if (currentBounds.m_max.x != reactionBounds.m_max.x)
			parms->TargetGridMax.x = targetBounds.m_max.x = currentBounds.m_max.x + m_growSize;
		if (currentBounds.m_max.y != reactionBounds.m_max.y)
			parms->TargetGridMax.y = targetBounds.m_max.y = currentBounds.m_max.y + m_growSize;

		// ***** data can be (re)loaded into gd, like was done above
	}
	return S_OK;
}


HRESULT CCWFGM_Grid::ImportElevation(const std::string & prj_file_name, const std::string & grid_file_name, bool forcePrj, std::uint8_t* calc_bits) {
	double scale;
	std::string grid(grid_file_name), prj(prj_file_name);

	SEM_BOOL engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)								return ERROR_SCENARIO_SIMULATION_RUNNING;

	GDALImporter importer;
	if (importer.Import(grid.c_str(), nullptr) != GDALImporter::ImportResult::OK) {
		return E_FAIL;
	}

	CSemaphoreEngage lock(GDALClient::GDALClient::getGDALMutex(), TRUE);

	const char* projection;
	OGRSpatialReferenceH sourceSRS;
	if (forcePrj || (importer.projection()[0] == '\0')) {
		try {
			fs::path p(prj_file_name.c_str());
			if (boost::iequals(p.extension().string(), _T(".prj"))) {
				std::ifstream t(prj);
				std::stringstream buffer;
				buffer << t.rdbuf();
				prj = buffer.str();
				sourceSRS = CCoordinateConverter::CreateSpatialReferenceFromStr(prj.c_str());
			}
			else
				sourceSRS = CCoordinateConverter::CreateSpatialReferenceFromWkt(prj.c_str());
		}
		catch (...) {
			sourceSRS = CCoordinateConverter::CreateSpatialReferenceFromWkt(prj.c_str());
		}
		projection = prj.c_str();
	}
	else {
		projection = importer.projection();
		sourceSRS = CCoordinateConverter::CreateSpatialReferenceFromWkt(projection);
	}

	if (m_sourceSRS) {
		if (!sourceSRS)
			return ERROR_GRID_LOCATION_OUT_OF_RANGE;
		if (!OSRIsSame(m_sourceSRS, sourceSRS, false)) {
			OSRDestroySpatialReference(sourceSRS);
			return ERROR_GRID_LOCATION_OUT_OF_RANGE;
		}
		OSRDestroySpatialReference(sourceSRS);
	}
	else if (!sourceSRS)
		return E_FAIL;
	else
		m_sourceSRS = sourceSRS;
	m_projectionContents = projection;

	char *units = nullptr;
	scale = OSRGetLinearUnits(m_sourceSRS, &units);
	m_units = units;

	GDALImporter::ImportType data = importer.importType();
	if ((data != GDALImporter::ImportType::LONG) &&
		(data != GDALImporter::ImportType::SHORT) &&
		(data != GDALImporter::ImportType::USHORT) &&
		(data != GDALImporter::ImportType::ULONG) &&
		(data != GDALImporter::ImportType::FLOAT32) &&
		(data != GDALImporter::ImportType::FLOAT64))
		return E_FAIL;

	std::uint16_t	xsize,
					ysize;					// size of our plots
	double			xllcorner, yllcorner;
	double			resolution;
	double			noData;

	xsize = importer.xSize();
	ysize = importer.ySize();
	xllcorner = importer.lowerLeftX();
	yllcorner = importer.lowerLeftY();
	resolution = importer.xPixelSize();
	noData = importer.nodata();

	//if initialized
	if (m_baseGrid.m_xsize != (std::uint16_t)-1) {
		if ((m_baseGrid.m_xsize != xsize) ||
			(m_baseGrid.m_ysize != ysize))
			return ERROR_GRID_SIZE_INCORRECT;
		if (fabs(m_baseGrid.m_resolution - resolution * scale) > 0.0001)
			return ERROR_GRID_UNSUPPORTED_RESOLUTION;
		if ((fabs(m_baseGrid.m_xllcorner - xllcorner) > 0.001) ||
			(fabs(m_baseGrid.m_yllcorner - yllcorner) > 0.001))
			return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	}

	*calc_bits = 0;

	HRESULT error = S_OK;
	std::int16_t *elevationArray = NULL, *ea1;
	bool *elevationValid = NULL, *ev1;
	std::uint32_t *elevationFrequency = NULL;
	std::uint32_t i, j;
	const std::uint32_t index = xsize * ysize;
	std::uint32_t index_possible = index;
	std::int16_t e_min = -9999, e_max = -9999;

	try {
		elevationArray = new std::int16_t[index];
		elevationValid = new bool[index];
		elevationFrequency = new std::uint32_t[65536];
		memset(elevationFrequency, 0, sizeof(std::uint32_t) * 65536);
	}
	catch(std::bad_alloc &cme) {
		if (elevationArray)	delete [] elevationArray;
		if (elevationValid) delete [] elevationValid;
		if (elevationFrequency)	delete [] elevationFrequency;
		return E_OUTOFMEMORY;
	}
							//-------- Read Elevation Data ------------------------
	double elev_acc = 0.0;
	for (i = 0, j = 0, ea1 = elevationArray, ev1 = elevationValid; i < index; i++, ea1++, ev1++) {
		double elevation = importer.doubleData(1, i);
		if (elevation == noData) {
			*ev1 = false;
			index_possible--;
			*calc_bits |= 1;
			m_flags |= CCWFGMGRID_ELEV_NODATA_EXISTS;
		}
		else {
			std::int16_t s_elev = (std::int16_t)floor(elevation + 0.5);
			elevationFrequency[(std::uint16_t)s_elev]++;
			if ((!j) || (s_elev > e_max))	e_max = s_elev;
			if ((!j) || (s_elev < e_min))	e_min = s_elev;
			elev_acc += elevation;
			*ea1 = s_elev;
			*ev1 = true;
			j++;
		}
	}
	if (m_baseGrid.m_elevationArray) {
		delete [] m_baseGrid.m_elevationArray;
		error = SUCCESS_GRID_DATA_UPDATED;
	}
	m_baseGrid.m_elevationArray = elevationArray;

	if (m_baseGrid.m_elevationValidArray)
		delete[] m_baseGrid.m_elevationValidArray;
	m_baseGrid.m_elevationValidArray = elevationValid;

	memcpy(m_baseGrid.m_elevationFrequency, elevationFrequency, sizeof(std::uint32_t) * 65536);
	delete [] elevationFrequency;

	m_baseGrid.m_minElev = e_min;
	m_baseGrid.m_maxElev = e_max;
	if (j)	m_baseGrid.m_meanElev = elev_acc / j;
	else	m_baseGrid.m_meanElev = 0.0;

	i = 0;
	if (index_possible) {
		j = 0;
		index_possible /= 2;
		while (j < index_possible) {
			j += m_baseGrid.m_elevationFrequency[i++];

#ifdef DEBUG
			weak_assert(i < 65535);
#endif
		}
	}
	else
		*calc_bits |= 1 << 3;

	if (i > 0)
		i--;

	m_baseGrid.m_medianElev = (std::int16_t)i;
	m_defaultElevation = i;
	m_flags |= CCWFGMGRID_VALID | CCWFGMGRID_DEFAULT_ELEV_SET;
	m_baseGrid.m_xsize = xsize;
	m_baseGrid.m_ysize = ysize;
	m_baseGrid.m_xllcorner = xllcorner;
	m_baseGrid.m_yllcorner = yllcorner;
	m_baseGrid.m_resolution = resolution * scale;
	m_baseGrid.m_iresolution = 1.0 / m_baseGrid.m_resolution;
	m_bRequiresSave = true;

	if (!((*calc_bits) & 1 << 3))
		error = calculateSlopeFactorAndAzimuth(nullptr, calc_bits);

	return error;
}


HRESULT CCWFGM_Grid::ExportGrid(const std::string & grid_file_name, std::uint32_t compression) {
	std::string grid(grid_file_name);

	if (grid.length() == 0)
		return E_INVALIDARG;
	
	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);

	int* l_array = new int[m_baseGrid.m_xsize * m_baseGrid.m_ysize];
	int* l_pointer = l_array;
	std::uint8_t fuel_value;
	bool f_valid;
	for(std::uint16_t i = m_baseGrid.m_ysize - 1; i < m_baseGrid.m_ysize; i--) {
		for(std::uint16_t j = 0; j < m_baseGrid.m_xsize; j++) {
			XY_Point pt;
			pt.x = m_baseGrid.invertX(((double)j) + 0.5);
			pt.y = m_baseGrid.invertY(((double)i) + 0.5);
			HRESULT hr = GetFuelIndexData(0, pt, WTime(0, &m_timeManager), &fuel_value, &f_valid, nullptr);
			long file_index, export_index;
			if ((hr == ERROR_FUELS_FUEL_UNKNOWN) || (!f_valid))
				export_index = -9999;
			else {
				ICWFGM_Fuel *fuel;
				m_fuelMap->FuelAtIndex(fuel_value, &file_index, &export_index, &fuel);
			}

			*l_pointer = export_index;
			l_pointer++;
		}
	}

	GDALExporter exporter;
	exporter.AddTag("TIFFTAG_SOFTWARE", "Prometheus");
	exporter.AddTag("TIFFTAG_GDAL_NODATA", "-9999");
	char mbstr[100];
	std::time_t t = std::time(nullptr);
	std::strftime(mbstr, sizeof(mbstr), "%Y-%m-%d %H:%M:%S %Z", std::localtime(&t));
	exporter.AddTag("TIFFTAG_DATETIME", mbstr);
	PolymorphicAttribute v;
	GetAttribute(CWFGM_GRID_ATTRIBUTE_SPATIALREFERENCE, &v);
	std::string ref;

	/*POLYMORPHIC CHECK*/
	try { ref = std::get<std::string>(v); } catch (std::bad_variant_access &) { weak_assert(false); return ERROR_PROJECTION_UNKNOWN; };

	exporter.setExportCompress((GDALExporter::CompressionType)compression);
	exporter.setProjection(ref.c_str());
	exporter.setSize(m_baseGrid.m_xsize, m_baseGrid.m_ysize);
	exporter.setPixelResolution(m_baseGrid.m_resolution, m_baseGrid.m_resolution);
	exporter.setLowerLeft(m_baseGrid.m_xllcorner, m_baseGrid.m_yllcorner);
	GDALExporter::ExportResult res = exporter.Export(l_array, grid.c_str(), "Fuel");
	
	if (l_array)
		delete [] l_array;
	
	if (res == GDALExporter::ExportResult::ERROR_ACCESS)
		return E_ACCESSDENIED;
	
	return S_OK;
}


HRESULT CCWFGM_Grid::ExportGridWCS(const std::string & url, const std::string & layer, const std::string & username, const std::string & password) {
	return E_NOTIMPL;
}


HRESULT CCWFGM_Grid::ExportElevationWCS(const std::string & url, const std::string & layer, const std::string & username, const std::string & password) {
	return E_NOTIMPL;
}


HRESULT CCWFGM_Grid::ExportSlopeWCS(const std::string & url, const std::string & layer, const std::string & username, const std::string & password) {
	return E_NOTIMPL;
}


HRESULT CCWFGM_Grid::ExportAspectWCS(const std::string & url, const std::string & layer, const std::string & username, const std::string & password) {
	return E_NOTIMPL;
}


HRESULT CCWFGM_Grid::WriteProjection(const std::string & szPath) {
	if (szPath.length() == 0)
		return E_INVALIDARG;

	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);

	FILE *f;
#ifdef _MSC_VER
	errno_t err = _tfopen_s(&f, szPath.c_str(), _T("w"));
	if (f) {
		_ftprintf(f, "%s", m_projectionContents.c_str());
		fclose(f);
	} else
		return ComError(err);
#else
    bool success = _tfopen_s(&f, szPath.c_str(), _T("w"));
    if (f) {
        _ftprintf(f, "%s", m_projectionContents.c_str());
        fclose(f);
    } else
        return ERROR_FILE_NOT_FOUND | ERROR_SEVERITY_WARNING;
#endif
	return S_OK;
}


#ifndef DOXYGEN_IGNORE_CODE

bool CCWFGM_Grid::fixWorldLocation() {
	std::string str;
	char *tokens = NULL;
	CSemaphoreEngage lock(GDALClient::GDALClient::getGDALMutex(), TRUE);

	if ((m_sourceSRS) && (OSRExportToWkt(m_sourceSRS, &tokens) == OGRERR_NONE)) {
		str = tokens;
		CPLFree(tokens);
	}
	if ((!str.length()) && (m_projectionContents.length()))
		str = m_projectionContents;
	if (str.length()) {
		CCoordinateConverter cc;
		cc.SetSourceProjection(str.c_str());
		double	x = m_baseGrid.m_xllcorner,
				y = m_baseGrid.m_yllcorner;
		if (!cc.SourceToLatlon(1, &x, &y, nullptr))
			return false;
		m_worldLocation.m_latitude(DEGREE_TO_RADIAN(y));
		m_worldLocation.m_longitude(DEGREE_TO_RADIAN(x));
		return true;
	}
	return false;
}

#endif


HRESULT CCWFGM_Grid::ExportElevation(const std::string & grid_file_name) {
	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);

	std::string grid;

	if (!m_baseGrid.m_elevationArray) {
		weak_assert(false);
		return ERROR_GRID_UNINITIALIZED;
	}
	grid = grid_file_name;

	if (grid.length() == 0)
		return E_INVALIDARG;
	
	int* l_array = new int[m_baseGrid.m_xsize * m_baseGrid.m_ysize];
	int* l_pointer = l_array;
	for(std::uint16_t i = m_baseGrid.m_ysize - 1; i < m_baseGrid.m_ysize; i--) {
		for(std::uint16_t j = 0; j < m_baseGrid.m_xsize; j++) {
			*l_pointer = (!m_baseGrid.m_elevationValidArray[m_baseGrid.arrayIndex(j, i)]) ? -9999 : m_baseGrid.m_elevationArray[m_baseGrid.arrayIndex(j, i)];
			l_pointer++;
		}
	}

	GDALExporter exporter;
	exporter.AddTag("TIFFTAG_SOFTWARE", "Prometheus");
	exporter.AddTag("TIFFTAG_GDAL_NODATA", "-9999");
	char mbstr[100];
	std::time_t t = std::time(nullptr);
	std::strftime(mbstr, sizeof(mbstr), "%Y-%m-%d %H:%M:%S %Z", std::localtime(&t));
	exporter.AddTag("TIFFTAG_DATETIME", mbstr);
	PolymorphicAttribute v;
	GetAttribute(CWFGM_GRID_ATTRIBUTE_SPATIALREFERENCE, &v);
	std::string ref;

	/*POLYMORPHIC CHECK*/
	try { ref = std::get<std::string>(v); } catch (std::bad_variant_access &) { weak_assert(false); return ERROR_PROJECTION_UNKNOWN; };

	exporter.setProjection(ref.c_str());
	exporter.setSize(m_baseGrid.m_xsize, m_baseGrid.m_ysize);
	exporter.setPixelResolution(m_baseGrid.m_resolution, m_baseGrid.m_resolution);
	exporter.setLowerLeft(m_baseGrid.m_xllcorner, m_baseGrid.m_yllcorner);
	GDALExporter::ExportResult res = exporter.Export(l_array, grid.c_str(), "Elevation");
	
	if (l_array)
		delete [] l_array;
	
	if (res == GDALExporter::ExportResult::ERROR_ACCESS)
		return E_ACCESSDENIED;

	return S_OK;
}


HRESULT CCWFGM_Grid::ExportSlope(const std::string & grid_file_name) {
	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);

	if (!m_baseGrid.m_slopeAzimuth) {
		weak_assert(false);
		return ERROR_GRID_UNINITIALIZED;
	}

	if (grid_file_name.length() == 0)
		return E_INVALIDARG;
	
	int* l_array = new int[m_baseGrid.m_xsize * m_baseGrid.m_ysize];
	int* l_pointer = l_array;
	for(std::uint16_t i=m_baseGrid.m_ysize-1;i<m_baseGrid.m_ysize;i--) {
		for(std::uint16_t j = 0; j < m_baseGrid.m_xsize; j++) {
			*l_pointer = (!m_baseGrid.m_terrainValidArray[m_baseGrid.arrayIndex(j,i)]) ? -9999 : m_baseGrid.m_slopeFactor[m_baseGrid.arrayIndex(j,i)];
			l_pointer++;
		}
	}

	GDALExporter exporter;
	exporter.AddTag("TIFFTAG_SOFTWARE", "Prometheus");
	exporter.AddTag("TIFFTAG_GDAL_NODATA", "-9999");
	char mbstr[100];
	std::time_t t = std::time(nullptr);
	std::strftime(mbstr, sizeof(mbstr), "%Y-%m-%d %H:%M:%S %Z", std::localtime(&t));
	exporter.AddTag("TIFFTAG_DATETIME", mbstr);
	PolymorphicAttribute v;
	GetAttribute(CWFGM_GRID_ATTRIBUTE_SPATIALREFERENCE, &v);
	std::string ref;

	/*POLYMORPHIC CHECK*/
	try { ref = std::get<std::string>(v); } catch (std::bad_variant_access &) { weak_assert(false); return ERROR_PROJECTION_UNKNOWN; };

	exporter.setProjection(ref.c_str());
	exporter.setSize(m_baseGrid.m_xsize, m_baseGrid.m_ysize);
	exporter.setPixelResolution(m_baseGrid.m_resolution, m_baseGrid.m_resolution);
	exporter.setLowerLeft(m_baseGrid.m_xllcorner, m_baseGrid.m_yllcorner);
	GDALExporter::ExportResult res = exporter.Export(l_array, grid_file_name.c_str(), "Slope");
	
	if (l_array)
		delete [] l_array;
	
	if (res == GDALExporter::ExportResult::ERROR_ACCESS)
		return E_ACCESSDENIED;

	return S_OK;
}


HRESULT CCWFGM_Grid::ExportAspect(const std::string & grid_file_name) {
	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);

	if (!m_baseGrid.m_slopeFactor) {
		weak_assert(false);
		return ERROR_GRID_UNINITIALIZED;
	}

	if(grid_file_name.length() == 0)
		return E_INVALIDARG;
	
	int* l_array = new int[m_baseGrid.m_xsize * m_baseGrid.m_ysize];
	int* l_pointer = l_array;
	for(std::uint16_t i = m_baseGrid.m_ysize - 1; i < m_baseGrid.m_ysize; i--) {
		for(std::uint16_t j = 0; j < m_baseGrid.m_xsize; j++) {
			std::uint32_t idx = m_baseGrid.arrayIndex(j, i);
			if (!m_baseGrid.m_terrainValidArray[idx])
				*l_pointer = -9999.0;
			else if (m_baseGrid.m_slopeAzimuth[idx] == (std::uint16_t)-1)
				*l_pointer = 0.0;
			else
				*l_pointer = m_baseGrid.m_slopeAzimuth[idx];
			l_pointer++;
		}
	}

	GDALExporter exporter;
	exporter.AddTag("TIFFTAG_SOFTWARE", "Prometheus");
	exporter.AddTag("TIFFTAG_GDAL_NODATA", "-9999");
	char mbstr[100];
	std::time_t t = std::time(nullptr);
	std::strftime(mbstr, sizeof(mbstr), "%Y-%m-%d %H:%M:%S %Z", std::localtime(&t));
	exporter.AddTag("TIFFTAG_DATETIME", mbstr);
	PolymorphicAttribute v;
	GetAttribute(CWFGM_GRID_ATTRIBUTE_SPATIALREFERENCE, &v);
	std::string ref;

	/*POLYMORPHIC CHECK*/
	try { ref = std::get<std::string>(v); } catch (std::bad_variant_access &) { weak_assert(false); return ERROR_PROJECTION_UNKNOWN; };

	exporter.setProjection(ref.c_str());
	exporter.setSize(m_baseGrid.m_xsize, m_baseGrid.m_ysize);
	exporter.setPixelResolution(m_baseGrid.m_resolution, m_baseGrid.m_resolution);
	exporter.setLowerLeft(m_baseGrid.m_xllcorner, m_baseGrid.m_yllcorner);
	GDALExporter::ExportResult res = exporter.Export(l_array, grid_file_name.c_str(), "Aspect");
	
	if (l_array)
		delete [] l_array;
	
	if (res == GDALExporter::ExportResult::ERROR_ACCESS)
		return E_ACCESSDENIED;

	return S_OK;
}


std::int32_t CCWFGM_Grid::serialVersionUid(const SerializeProtoOptions& options) const noexcept {
	return options.fileVersion();
}


WISE::GridProto::CwfgmGrid* CCWFGM_Grid::serialize(const SerializeProtoOptions& options)
{
	auto grid = new WISE::GridProto::CwfgmGrid();
	grid->set_version(serialVersionUid(options));

	grid->set_allocated_xsize(createProtobufObject((std::uint32_t)m_baseGrid.m_xsize));
	grid->set_allocated_ysize(createProtobufObject((std::uint32_t)m_baseGrid.m_ysize));
	grid->set_allocated_xllcorner(DoubleBuilder().withValue(m_baseGrid.m_xllcorner).forProtobuf(options.useVerboseFloats()));
	grid->set_allocated_yllcorner(DoubleBuilder().withValue(m_baseGrid.m_yllcorner).forProtobuf(options.useVerboseFloats()));
	grid->set_allocated_resolution(DoubleBuilder().withValue(m_baseGrid.m_resolution).forProtobuf(options.useVerboseFloats()));

	auto location = new Math::Coordinate();
	location->set_allocated_latitude(DoubleBuilder().withValue(RADIAN_TO_DEGREE(m_worldLocation.m_latitude())).forProtobuf(options.useVerboseFloats()));
	location->set_allocated_longitude(DoubleBuilder().withValue(RADIAN_TO_DEGREE(m_worldLocation.m_longitude())).forProtobuf(options.useVerboseFloats()));
	grid->set_allocated_lllocation(location);

	std::uint64_t size = m_baseGrid.m_xsize * m_baseGrid.m_ysize;

	//fuel map
	{
		auto fuelmap = new WISE::GridProto::CwfgmGrid_FuelMapFile();
		auto wcs = new WISE::GridProto::wcsData();
		wcs->set_version(1);
		wcs->set_xsize(m_baseGrid.m_xsize);
		wcs->set_ysize(m_baseGrid.m_ysize);
		auto binary = new WISE::GridProto::wcsData_binaryData();
		if (options.useVerboseOutput() || !options.zipOutput()) {
			binary->set_data(m_baseGrid.m_fuelArray, size);
			binary->set_datavalid(m_baseGrid.m_fuelValidArray, size);
		}
		else {
			binary->set_allocated_iszipped(createProtobufObject(true));
			binary->set_data(Compress::compress(reinterpret_cast<const char*>(m_baseGrid.m_fuelArray), size));
			binary->set_datavalid(Compress::compress(reinterpret_cast<const char*>(m_baseGrid.m_fuelValidArray), size));
		}
		wcs->set_allocated_binary(binary);
		fuelmap->set_allocated_contents(wcs);
		if (m_header.length() > 0)
			fuelmap->set_allocated_header(createProtobufObject(m_header));
		grid->set_allocated_fuelmap(fuelmap);
	}
	//elevation
	{
		auto wcs = new WISE::GridProto::wcsData();
		wcs->set_version(1);
		wcs->set_xsize(m_baseGrid.m_xsize);
		wcs->set_ysize(m_baseGrid.m_ysize);
		auto binary = new WISE::GridProto::wcsData_binaryData();
		if (options.useVerboseOutput() || !options.zipOutput()) {
			binary->set_data(m_baseGrid.m_elevationArray, size * sizeof(std::int16_t));
			binary->set_datavalid(m_baseGrid.m_elevationValidArray, size);
		}
		else {
			binary->set_allocated_iszipped(createProtobufObject(true));
			binary->set_data(Compress::compress(reinterpret_cast<const char*>(m_baseGrid.m_elevationArray), size * sizeof(std::uint16_t)));
			binary->set_datavalid(Compress::compress(reinterpret_cast<const char*>(m_baseGrid.m_elevationValidArray), size));
		}
		wcs->set_allocated_binary(binary);
		auto elevation = new WISE::GridProto::CwfgmGrid_ElevationFile();
		elevation->set_allocated_contents(wcs);
		grid->set_allocated_elevation(elevation);
	}

	auto projection = new WISE::GridProto::CwfgmGrid_ProjectionFile();
	if (m_projectionContents.length() > 0)
		projection->set_allocated_contents(createProtobufObject(m_projectionContents));
	if (m_sourceSRS) {
		CSemaphoreEngage lock(GDALClient::GDALClient::getGDALMutex(), true);
		char *tokens;
		if (OSRExportToWkt(m_sourceSRS, &tokens) == OGRERR_NONE) {
			std::string wkt = tokens;
			projection->set_allocated_wkt(createProtobufObject(wkt));
			CPLFree(tokens);
		}
	}
	if (m_units.length() > 0)
		projection->set_allocated_units(createProtobufObject(m_units));
	grid->set_allocated_projection(projection);

	return grid;
}


CCWFGM_Grid *CCWFGM_Grid::deserialize(const google::protobuf::Message& proto, std::shared_ptr<validation::validation_object> valid, const std::string& name) {
	auto grid = dynamic_cast_assert<const WISE::GridProto::CwfgmGrid*>(&proto);
	double value;

	if (!grid) {
		if (valid)
			/// <summary>
			/// The object passed as a grid is invalid. An incorrect object type was passed to the parser.
			/// </summary>
			/// <type>internal</type>
			valid->add_child_validation("WISE.GridProto.CwfgmGrid", name, validation::error_level::SEVERE, validation::id::object_invalid, proto.GetDescriptor()->name());
		weak_assert(false);
		throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmGrid: Protobuf object invalid", ERROR_PROTOBUF_OBJECT_INVALID);
	}
	if ((grid->version() != 1) && (grid->version() != 2)) {
		if (valid)
			/// <summary>
			/// The object version is not supported. The grid is not supported by this version of Prometheus.
			/// </summary>
			/// <type>user</type>
			valid->add_child_validation("WISE.GridProto.CwfgmGrid", name, validation::error_level::SEVERE, validation::id::version_mismatch, std::to_string(grid->version()));
		weak_assert(false);
		throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmGrid: Version is invalid", ERROR_PROTOBUF_OBJECT_VERSION_INVALID);
	}

	/// <summary>
	/// Child validations for grid objects.
	/// </summary>
	auto vt = validation::conditional_make_object(valid, "WISE.GridProto.CwfgmGrid", name);
	auto myValid = vt.lock();

	m_flags |= CCWFGMGRID_VALID;
	if (grid->has_xsize())
		m_baseGrid.m_xsize = grid->xsize().value();
	if (grid->has_ysize())
		m_baseGrid.m_ysize = grid->ysize().value();
	if (grid->has_xllcorner())
		m_baseGrid.m_xllcorner = DoubleBuilder().withProtobuf(grid->xllcorner(), myValid, "xllcorner").getValue();
	if (grid->has_yllcorner())
		m_baseGrid.m_yllcorner = DoubleBuilder().withProtobuf(grid->yllcorner(), myValid, "yllcorner").getValue();
	if (grid->has_resolution()) {
		m_baseGrid.m_resolution = DoubleBuilder().withProtobuf(grid->resolution(), myValid, "resolution").getValue();
		m_baseGrid.m_iresolution = 1.0 / m_baseGrid.m_resolution;
	}

	if (grid->has_lllocation()) {
		value = DEGREE_TO_RADIAN(DoubleBuilder().withProtobuf(grid->lllocation().latitude(), myValid, "ll_latitude").getValue());
		if (value < DEGREE_TO_RADIAN(-90.0)) {
			if (myValid)
				/// <summary>
				/// The specified latitude is not valid. The latitude must be between -90 and +90 degrees.
				/// </summary>
				/// <type>user</type>
				myValid->add_child_validation("WISE.GridProto.CwfgmGrid", "ll_latitude", validation::error_level::SEVERE, "Location.Latitude:Invalid", std::to_string(value));
			weak_assert(false);
			throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmGrid: Invalid latitude value");
		}
		if (value > DEGREE_TO_RADIAN(90.0)) {
			if (myValid)
				/// <summary>
				/// The specified latitude is not valid. The latitude must be between -90 and +90 degrees.
				/// </summary>
				/// <type>user</type>
				myValid->add_child_validation("WISE.GridProto.CwfgmGrid", "ll_latitude", validation::error_level::SEVERE, "Location.Latitude:Invalid", std::to_string(value));
			weak_assert(false);
			throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmGrid: Invalid latitude value");
		}
		m_worldLocation.m_latitude(value);

		value = DEGREE_TO_RADIAN(DoubleBuilder().withProtobuf(grid->lllocation().longitude(), myValid, "ll_longitude").getValue());
		if (value < DEGREE_TO_RADIAN(-180.0)) {
			if (myValid)
				/// <summary>
				/// The specified longitude is not valid. The longitude must be between -180 and +180 degrees.
				/// </summary>
				/// <type>user</type>
				myValid->add_child_validation("WISE.GridProto.CwfgmGrid", "ll_longitude", validation::error_level::SEVERE, "Location.Longitude:Invalid", std::to_string(value));
			weak_assert(false);
			throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmGrid: Invalid longitude value");
		}
		if (value > DEGREE_TO_RADIAN(180.0)) { 
			if (myValid)
				/// <summary>
				/// The specified longitude is not valid. The longitude must be between -180 and +180 degrees.
				/// </summary>
				/// <type>user</type>
				myValid->add_child_validation("WISE.GridProto.CwfgmGrid", "ll_longitude", validation::error_level::SEVERE, "Location.Longitude:Invalid", std::to_string(value));
			weak_assert(false);
			throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmGrid: Invalid longitude value");
		}
		m_worldLocation.m_longitude(value);
	}

	fixWorldLocation();
	m_flags |= CCWFGMGRID_SPECIFIED_FMC_ACTIVE;
	//I guess we don't actually care what the serialized FMC was, just that it existed
	if (m_worldLocation.InsideNewZealand()) {
		m_defaultFMC = 145.0;
	}
	else if (m_worldLocation.InsideTasmania()) {
		m_defaultFMC = 145.0;
	}
	else if (m_worldLocation.InsideAustraliaMainland()) {
		m_defaultFMC = 145.0;
	}
	else if (!m_worldLocation.InsideCanada()) {
		m_defaultFMC = 120.0;
	}
	else {
		m_flags &= ~(CCWFGMGRID_SPECIFIED_FMC_ACTIVE);
		m_defaultFMC = 120.0;
	}

	std::string projectionFile;
	if (grid->has_projection()) {
		if (grid->projection().has_contents())
			m_projectionContents = grid->projection().contents().value();
		if (grid->projection().has_wkt()) {
			CSemaphoreEngage lock(GDALClient::GDALClient::getGDALMutex(), TRUE);
			if (m_sourceSRS)
				OSRDestroySpatialReference(m_sourceSRS);

			m_sourceSRS = CCoordinateConverter::CreateSpatialReferenceFromWkt(grid->projection().wkt().value().c_str());

			char *units = nullptr;
			OSRGetLinearUnits(m_sourceSRS, &units);
			m_units = units;
		}
		else if (grid->projection().has_filename())
			projectionFile = grid->projection().filename().value();
		if (grid->projection().has_units())
			m_units = grid->projection().units().value();
	}

	std::uint64_t size = m_baseGrid.m_xsize * m_baseGrid.m_ysize;
	if (grid->has_fuelmap()) {
		auto fuelmap = grid->fuelmap();
		if (fuelmap.has_contents()) {
			if (fuelmap.contents().binary().has_iszipped() && fuelmap.contents().binary().iszipped().value()) {
				std::string data = Compress::decompress(fuelmap.contents().binary().data());
				std::string valid = Compress::decompress(fuelmap.contents().binary().datavalid());
				if (data.length() != valid.length() || data.length() != size) {
					if (myValid)
						/// <summary>
						/// The size of the fuelmap valid archive doesn't match the size of the fuelmap archive.
						/// </summary>
						/// <type>user</type>
						myValid->add_child_validation("WISE.GridProto.CwfgmGrid", "fuelmap.contents", validation::error_level::SEVERE, "Archive.Decompress:Invalid",
							strprintf("%d != %d", (int)data.length(), (int)valid.length()));
					m_loadWarning = "Error: WISE.GridProto.CwfgmGrid: Invalid fuel grid in imported file.";
					throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmGrid: Invalid fuel grid in imported file.");
				}
				m_baseGrid.m_fuelArray = new std::uint8_t[size];
				m_baseGrid.m_fuelValidArray = new bool[size];
				std::copy(data.begin(), data.end(), m_baseGrid.m_fuelArray);
				std::copy(valid.begin(), valid.end(), m_baseGrid.m_fuelValidArray);
			}
			else {
				m_baseGrid.m_fuelArray = new std::uint8_t[size];
				m_baseGrid.m_fuelValidArray = new bool[size];
				std::copy(fuelmap.contents().binary().data().begin(), fuelmap.contents().binary().data().end(), m_baseGrid.m_fuelArray);
				std::copy(fuelmap.contents().binary().datavalid().begin(), fuelmap.contents().binary().datavalid().end(), m_baseGrid.m_fuelValidArray);
			}
		}
		else if (fuelmap.has_filename() && projectionFile.length() > 0) {
#if GCC_VERSION > NO_GCC && GCC_VERSION < GCC_8
			if (fs::exists(fs::current_path() / fuelmap.filename().value()))
#else
			if (fs::exists(fs::relative(fuelmap.filename().value())))
#endif
			{
				long fail_index;
				HRESULT hr;
				if (FAILED(hr = ImportGrid(projectionFile, fuelmap.filename().value(), false, &fail_index))) {
					if (myValid) {
						switch (hr) {
						case E_FAIL:
							/// <summary>
							/// The specified fuelmap is not a valid file, it cannot be imported by GDAL.
							/// </summary>
							/// <type>user</type>
							myValid->add_child_validation("WISE.GridProto.CwfgmGrid", "fuelmap.file", validation::error_level::SEVERE,
								"Import.Fuelmap:Invalid", fuelmap.filename().value());
							break;
						case ERROR_GRID_LOCATION_OUT_OF_RANGE:
							/// <summary>
							/// The projection of the specified fuelmap does not match that of the elevation grid.
							/// </summary>
							/// <type>user</type>
							myValid->add_child_validation("WISE.GridProto.CwfgmGrid", "fuelmap.file", validation::error_level::SEVERE,
								"Import.Fuelmap:Projection", fuelmap.filename().value());
							break;
						case ERROR_GRID_SIZE_INCORRECT:
							/// <summary>
							/// The specified fuelmap is a different size than the elevation grid.
							/// </summary>
							/// <type>user</type>
							myValid->add_child_validation("WISE.GridProto.CwfgmGrid", "fuelmap.file", validation::error_level::SEVERE,
								"Import.Fuelmap:Size", fuelmap.filename().value());
							break;
						case ERROR_GRID_UNSUPPORTED_RESOLUTION:
							/// <summary>
							/// The specified fuelmap is a different resolution than the elevation grid.
							/// </summary>
							/// <type>user</type>
							myValid->add_child_validation("WISE.GridProto.wcsData.locationFile", "file", validation::error_level::SEVERE,
								validation::id::grid_resolution_mismatch, fuelmap.filename().value());
							break;
						case ERROR_FUELS_FUEL_UNKNOWN:
							/// <summary>
							/// The specified fuelmap contains fuels that are not in the fuel lookup table.
							/// </summary>
							/// <type>user</type>
							myValid->add_child_validation("WISE.GridProto.CwfgmGrid", "fuelmap.file", validation::error_level::SEVERE,
								"Import.Fuelmap:Fuels", fuelmap.filename().value());
							break;
						case ERROR_PROJECTION_UNKNOWN:
							/// <summary>
							/// Prometheus is unable to handle the projection of the specified fuelmap.
							/// </summary>
							/// <type>user</type>
							myValid->add_child_validation("WISE.GridProto.CwfgmGrid", "fuelmap.file", validation::error_level::SEVERE,
								"Import.Fuelmap:Location", fuelmap.filename().value());
							break;
						}
					}
					m_loadWarning = "Error: WISE.GridProto.CwfgmGrid: Fuel grid could not be imported.";
					switch (hr) {
					case E_FAIL:
						/// <summary>
						/// The specified fuelmap is not a valid file, it cannot be imported by GDAL.
						/// </summary>
						/// <type>user</type>
						m_loadWarning += "  The specified fuelmap is not a valid file, it cannot be imported by GDAL.";
						break;
					case ERROR_GRID_LOCATION_OUT_OF_RANGE:
						/// <summary>
						/// The projection of the specified fuelmap does not match that of the elevation grid.
						/// </summary>
						/// <type>user</type>
						m_loadWarning += "  The projection of the specified fuelmap does not match that of the elevation grid.";
						break;
					case ERROR_GRID_SIZE_INCORRECT:
						/// <summary>
						/// The specified fuelmap is a different size than the elevation grid.
						/// </summary>
						/// <type>user</type>
						m_loadWarning += "  The specified fuelmap is a different size than the elevation grid.";
						break;
					case ERROR_GRID_UNSUPPORTED_RESOLUTION:
						/// <summary>
						/// The specified fuelmap is a different resolution than the elevation grid.
						/// </summary>
						/// <type>user</type>
						m_loadWarning += "  The specified fuelmap is a different resolution than the elevation grid.";
						break;
					case ERROR_FUELS_FUEL_UNKNOWN:
						/// <summary>
						/// The specified fuelmap contains fuels that are not in the fuel lookup table.
						/// </summary>
						/// <type>user</type>
						m_loadWarning += "  The specified fuelmap contains fuels that are not in the fuel lookup table.";
						break;
					case ERROR_PROJECTION_UNKNOWN:
						/// <summary>
						/// Prometheus is unable to handle the projection of the specified fuelmap.
						/// </summary>
						/// <type>user</type>
						m_loadWarning += "  Prometheus is unable to handle the projection of the specified fuelmap.";
						break;
					}
					throw ISerializeProto::DeserializeError(m_loadWarning, hr);
				}
				size = m_baseGrid.m_xsize * m_baseGrid.m_ysize;
			}
			else {
				myValid->add_child_validation("WISE.GridProto.CwfgmGrid", "fuelmap.file", validation::error_level::SEVERE,
					validation::id::missing_fuel_grid, fuelmap.filename().value());
			}
		}
		else {
			myValid->add_child_validation("WISE.GridProto.CwfgmGrid", "fuelmap.file", validation::error_level::SEVERE,
				validation::id::missing_fuel_grid, fuelmap.filename().value());
		}
		if (fuelmap.has_header())
			m_header = fuelmap.header().value();
	}

	if (grid->has_elevation()) {
		if (grid->elevation().has_contents()) {
			WISE::GridProto::wcsData data = grid->elevation().contents();
			if (grid->has_nodataelevation()) {
				value = DoubleBuilder().withProtobuf(grid->nodataelevation()).getValue();

				if (value < 0.0) if (value != -99.0) {
					m_loadWarning = "Error: WISE.GridProto.CwfgmGrid: Invalid default elevation value";
					throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmGrid: Invalid default elevation value");
				}
				if (value > 7000.0) {
					m_loadWarning = "Error: WISE.GridProto.CwfgmGrid: Invalid default elevation value";
					throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmGrid: Invalid default elevation value");
				}

				m_flags |= CCWFGMGRID_DEFAULT_ELEV_SET;
				m_defaultElevation = value;
			}
			if (data.binary().has_iszipped() && data.binary().iszipped().value()) {
				std::string arr = Compress::decompress(data.binary().data());
				std::string valid = Compress::decompress(data.binary().datavalid());
				if (arr.length() != (valid.length() * sizeof(std::uint16_t)) || valid.length() != size) {
					if (myValid)
						/// <summary>
						/// The size of the elevation grid valid archive doesn't match the size of the elevation grid archive.
						/// </summary>
						/// <type>user</type>
						myValid->add_child_validation("WISE.GridProto.CwfgmGrid", "elevation.contents", validation::error_level::SEVERE, "Archive.Decompress:Invalid",
							strprintf("%d != %d", (int)arr.length(), (int)valid.length()));
					m_loadWarning = "Error: WISE.GridProto.CwfgmGrid: Invalid elevation grid in imported file.";
					throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmGrid: Invalid elevation grid in imported file.");
				}
				m_baseGrid.m_elevationArray = new std::int16_t[size];
				m_baseGrid.m_elevationValidArray = new bool[size];
				std::copy(arr.begin(), arr.end(), reinterpret_cast<std::uint8_t*>(m_baseGrid.m_elevationArray));
				std::copy(valid.begin(), valid.end(), m_baseGrid.m_elevationValidArray);
			}
			else {
				m_baseGrid.m_elevationArray = new std::int16_t[size];
				m_baseGrid.m_elevationValidArray = new bool[size];
				std::copy(data.binary().data().begin(), data.binary().data().end(), reinterpret_cast<std::uint8_t*>(m_baseGrid.m_elevationArray));
				std::copy(data.binary().datavalid().begin(), data.binary().datavalid().end(), m_baseGrid.m_elevationValidArray);
			}
		}
		else if (grid->elevation().has_filename()) {
#if GCC_VERSION > NO_GCC && GCC_VERSION < GCC_8
			if (fs::exists(fs::current_path() / grid->elevation().filename().value()) && projectionFile.length() > 0)
#else
			if (fs::exists(fs::relative(grid->elevation().filename().value())) && projectionFile.length() > 0)
#endif
			{
				HRESULT hr;
				std::uint8_t calc_bits;
				if (FAILED(hr = ImportElevation(projectionFile, grid->elevation().filename().value(), false, &calc_bits))) {
					if (myValid) {
						switch (hr) {
						case E_FAIL:
							/// <summary>
							/// The specified elevation grid is not a valid file, it cannot be imported by GDAL.
							/// </summary>
							/// <type>user</type>
							myValid->add_child_validation("WISE.GridProto.CwfgmGrid", "elevation.file", validation::error_level::SEVERE,
								"Import.Elevation:Invalid", grid->elevation().filename().value());
							break;
						case ERROR_GRID_LOCATION_OUT_OF_RANGE:
							/// <summary>
							/// The projection of the specified elevation grid does not match that of the fuelmap.
							/// </summary>
							/// <type>user</type>
							myValid->add_child_validation("WISE.GridProto.CwfgmGrid", "elevation.file", validation::error_level::SEVERE,
								"Import.Elevation:Projection", grid->elevation().filename().value());
							break;
						case ERROR_GRID_SIZE_INCORRECT:
							/// <summary>
							/// The specified elevation grid is a different size than the fuelmap.
							/// </summary>
							/// <type>user</type>
							myValid->add_child_validation("WISE.GridProto.CwfgmGrid", "elevation.file", validation::error_level::SEVERE,
								"Import.Elevation:Size", grid->elevation().filename().value());
							break;
						case ERROR_GRID_UNSUPPORTED_RESOLUTION:
							/// <summary>
							/// The specified elevation is a different resolution than the fuelmap.
							/// </summary>
							/// <type>user</type>
							myValid->add_child_validation("WISE.GridProto.wcsData.locationFile", "file", validation::error_level::SEVERE,
								validation::id::grid_resolution_mismatch, grid->elevation().filename().value());
							break;
						}
					}
					m_loadWarning = "Error: WISE.GridProto.CwfgmGrid: Elevation could not be imported.";
					switch (hr) {
					case E_FAIL:
						/// <summary>
						/// The specified elevation grid is not a valid file, it cannot be imported by GDAL.
						/// </summary>
						/// <type>user</type>
						m_loadWarning += "  The specified elevation grid is not a valid file, it cannot be imported by GDAL.";
						break;
					case ERROR_GRID_LOCATION_OUT_OF_RANGE:
						/// <summary>
						/// The projection of the specified elevation grid does not match that of the fuelmap.
						/// </summary>
						/// <type>user</type>
						m_loadWarning += "  The projection of the specified elevation grid does not match that of the fuelmap.";
						break;
					case ERROR_GRID_SIZE_INCORRECT:
						/// <summary>
						/// The specified elevation grid is a different size than the fuelmap.
						/// </summary>
						/// <type>user</type>
						m_loadWarning += "The specified elevation grid is a different size than the fuelmap.";
						break;
					case ERROR_GRID_UNSUPPORTED_RESOLUTION:
						/// <summary>
						/// The specified elevation is a different resolution than the fuelmap.
						/// </summary>
						/// <type>user</type>
						m_loadWarning += "The specified elevation is a different resolution than the fuelmap.";
						break;
					}
					throw ISerializeProto::DeserializeError(m_loadWarning, hr);
				}
				size = m_baseGrid.m_xsize * m_baseGrid.m_ysize;
				calcWarnings(calc_bits);
			}
			else {
				myValid->add_child_validation("WISE.GridProto.CwfgmGrid", "elevation.file", validation::error_level::SEVERE,
					validation::id::missing_file, grid->elevation().filename().value());
			}
		}
	}

	if ((m_baseGrid.m_xsize == (std::uint16_t) - 1) || (m_baseGrid.m_ysize == (std::uint16_t) - 1)) {
		if (myValid)
			/// <summary>
			/// The grid size has not been specified and was not able to be loaded from the imported grids.
			/// </summary>
			/// <type>user</type>
			myValid->add_child_validation("WISE.GridProto.CwfgmGrid", "grid", validation::error_level::SEVERE, "Grid.Size:Invalid", "unset");
		m_loadWarning = "Error: WISE.GridProto.CwfgmGrid: No grid size has been specified";
		throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmGrid: No grid size has been specified");
	}

	if (m_baseGrid.m_elevationArray) {
		std::uint8_t calc_bits;
		calculateSlopeFactorAndAzimuth(nullptr, &calc_bits);
		calcWarnings(calc_bits);

		m_baseGrid.m_minElev = 32767;
		m_baseGrid.m_maxElev = -32768;
		std::uint32_t j, index_possible = size;
		m_baseGrid.m_meanElev = 0;
		memset(m_baseGrid.m_elevationFrequency, 0, sizeof(m_baseGrid.m_elevationFrequency));
		std::int16_t s_elev = 0;
		std::int16_t *ea1;
		bool *ea2;
		double elev_acc = 0.0;
		int i;
		for (i = 0, j = 0, ea1 = m_baseGrid.m_elevationArray, ea2 = m_baseGrid.m_elevationValidArray; i < size; i++, ea1++, ea2++) {
			if (*ea2) {
				s_elev = *ea1;
				m_baseGrid.m_elevationFrequency[(std::uint16_t)s_elev]++;
				if ((s_elev > m_baseGrid.m_maxElev) || (!j))	m_baseGrid.m_maxElev = s_elev;
				if ((s_elev < m_baseGrid.m_minElev) || (!j))	m_baseGrid.m_minElev = s_elev;
				elev_acc += s_elev;
				j++;
			}
			else {
				index_possible--;
				m_flags |= CCWFGMGRID_ELEV_NODATA_EXISTS;
			}
		}

		if (j)	m_baseGrid.m_meanElev = (std::int16_t)(elev_acc / j);
		else	m_baseGrid.m_meanElev = 0;

		index_possible /= 2;
		std::int16_t ii = 0;
		j = 0;
		while (j < index_possible)
			j += m_baseGrid.m_elevationFrequency[ii++];

		if (ii > m_baseGrid.m_minElev)
			ii--;
		m_baseGrid.m_medianElev = ii;

		if (!(m_flags & CCWFGMGRID_DEFAULT_ELEV_SET)) {
			m_flags |= CCWFGMGRID_DEFAULT_ELEV_SET;
			m_defaultElevation = m_baseGrid.m_medianElev;
		}
	}

	return this;
}


void CCWFGM_Grid::calcWarnings(const std::uint8_t calc_bits) {
	if (calc_bits & 0x1) {
		std::string msg = "The elevation grid file contained NODATA entries.\n";
		if (calc_bits & 0x4) {
			msg += "Some NODATA entries could not be interpolated.  The median elevation will be used.\n";
			if (calc_bits & 0x2)
				msg += "Other ";
		}
		if (calc_bits & 0x2)
			msg += "NODATA entries have been filled using bilinear interpolation.\n";
		m_loadWarning += msg;
	}
}
