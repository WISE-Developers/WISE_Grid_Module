/**
 * WISE_Grid_Module: CWFGM_Grid.h
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

#include "ICWFGM_Fuel.h"
#include "CWFGM_Grid.h"
#include "GridCom_ext.h"
#include "FireEngine_ext.h"
#include "CWFGM_Grid.h"
#include "CWFGM_FuelMap.h"
#include "angles.h"
#include "CoordinateConverter.h"
#include <gdal_alg.h>
#include <cpl_string.h>
#include "linklist.h"
#include "objectcache.h"
#include <errno.h>
#include <float.h>
#include <stdio.h>
#include "gdalclient.h"

#ifdef DEBUG
#include <assert.h>
#endif

using namespace CONSTANTS_NAMESPACE;


#ifndef DOXYGEN_IGNORE_CODE


CCWFGM_Grid::CCWFGM_Grid() : m_timeManager(m_worldLocation) {
	m_bRequiresSave = false;

	m_fuelMap = nullptr;
	m_sourceSRS = nullptr;

	m_defaultElevation = 0.0;
	m_defaultFMC = 120.0;

	m_flags = 0;
	m_initialSize = 0.0;
	m_reactionSize = 0.0;
	m_growSize = 0.0;

	m_commonData.m_timeManager = &m_timeManager;
}


CCWFGM_Grid::CCWFGM_Grid(const CCWFGM_Grid &toCopy) : m_timeManager(m_worldLocation), m_baseGrid(toCopy.m_baseGrid) {
	CRWThreadSemaphoreEngage engage(*(CRWThreadSemaphore *)&toCopy.m_lock, SEM_FALSE);

	m_projectionContents = toCopy.m_projectionContents;
	m_header = toCopy.m_header;
	m_units = toCopy.m_units;
	m_loadWarning = toCopy.m_loadWarning;
	m_sourceSRS = OSRClone(toCopy.m_sourceSRS);
	m_worldLocation = toCopy.m_worldLocation;

	m_defaultElevation = toCopy.m_defaultElevation;
	m_defaultFMC = toCopy.m_defaultFMC;

	m_flags = toCopy.m_flags;
	m_initialSize = toCopy.m_initialSize;
	m_growSize = toCopy.m_growSize;
	m_reactionSize = toCopy.m_reactionSize;

	m_gisGridURL = toCopy.m_gisGridURL;
	m_gisGridLayer = toCopy.m_gisGridLayer;
	m_gisGridUID = toCopy.m_gisGridUID;
	m_gisGridPWD = toCopy.m_gisGridPWD;
	m_gisElevURL = toCopy.m_gisElevURL;
	m_gisElevLayer = toCopy.m_gisElevLayer;
	m_gisElevUID = toCopy.m_gisElevUID;
	m_gisElevPWD = toCopy.m_gisElevPWD;

	m_commonData.m_timeManager = &m_timeManager;

	m_bRequiresSave = false;
}


GridData::GridData() {
	m_xsize = m_ysize = (std::uint16_t)-1;
	m_resolution = -1.0;
	m_xllcorner = m_yllcorner = -999999999.0;
	m_fuelArray = nullptr;
	m_fuelValidArray = nullptr;
	m_elevationArray = nullptr;
	m_elevationValidArray = nullptr;
	m_terrainValidArray = nullptr;
	m_slopeFactor = nullptr;
	m_slopeAzimuth = nullptr;
	m_maxElev = m_minElev = m_medianElev = m_meanElev = -1;
	m_maxSlopeFactor = m_minSlopeFactor = (std::uint16_t)-1;
	m_maxAzimuth = m_minAzimuth = (std::uint16_t)-1;
	memset(m_elevationFrequency, 0, sizeof(m_elevationFrequency));
}


GridData::GridData(const GridData &toCopy) {
	m_xllcorner = toCopy.m_xllcorner;
	m_yllcorner = toCopy.m_yllcorner;
	m_resolution = toCopy.m_resolution;
	m_xsize = toCopy.m_xsize;
	m_ysize = toCopy.m_ysize;
	m_minElev = toCopy.m_minElev;
	m_maxElev = toCopy.m_maxElev;
	m_medianElev = toCopy.m_medianElev;
	m_meanElev = toCopy.m_meanElev;
	m_maxSlopeFactor = toCopy.m_maxSlopeFactor;
	m_minSlopeFactor = toCopy.m_minSlopeFactor;
	m_maxAzimuth = toCopy.m_maxAzimuth;
	m_minAzimuth = toCopy.m_minAzimuth;
	memcpy(m_elevationFrequency, toCopy.m_elevationFrequency, sizeof(m_elevationFrequency));

	if (toCopy.m_fuelArray) {
		m_fuelArray = new std::uint8_t[m_xsize * m_ysize];
		memcpy(m_fuelArray, toCopy.m_fuelArray, (size_t)m_xsize * (size_t)m_ysize * sizeof(std::uint8_t));
	} else
		m_fuelArray = nullptr;

	if (toCopy.m_fuelValidArray) {
		m_fuelValidArray = new bool[m_xsize * m_ysize];
		memcpy(m_fuelValidArray, toCopy.m_fuelValidArray, (size_t)m_xsize * (size_t)m_ysize * sizeof(bool));
	} else
		m_fuelValidArray = nullptr;

	if (toCopy.m_elevationArray) {
		m_elevationArray = new std::int16_t[m_xsize * m_ysize];
		memcpy(m_elevationArray, toCopy.m_elevationArray, (size_t)m_xsize * (size_t)m_ysize * sizeof(std::int16_t));
	} else
		m_elevationArray = nullptr;

	if (toCopy.m_elevationValidArray) {
		m_elevationValidArray = new bool[m_xsize * m_ysize];
		memcpy(m_elevationValidArray, toCopy.m_elevationValidArray, (size_t)m_xsize * (size_t)m_ysize * sizeof(bool));
	} else
		m_elevationValidArray = nullptr;

	if (toCopy.m_terrainValidArray) {
		m_terrainValidArray = new bool[m_xsize * m_ysize];
		memcpy(m_terrainValidArray, toCopy.m_terrainValidArray, (size_t)m_xsize * (size_t)m_ysize * sizeof(bool));
	} else
		m_terrainValidArray = nullptr;

	if (toCopy.m_slopeFactor) {
		m_slopeFactor = new std::uint16_t[m_xsize * m_ysize];
		memcpy(m_slopeFactor, toCopy.m_slopeFactor, (size_t)m_xsize * (size_t)m_ysize * sizeof(std::uint16_t));
	} else
		m_slopeFactor = nullptr;

	if (toCopy.m_slopeAzimuth) {
		m_slopeAzimuth = new std::uint16_t[m_xsize * m_ysize];
		memcpy(m_slopeAzimuth, toCopy.m_slopeFactor, (size_t)m_xsize * (size_t)m_ysize * sizeof(std::uint16_t));
	} else
		m_slopeAzimuth = nullptr;
}


CCWFGM_Grid::~CCWFGM_Grid() {
	CSemaphoreEngage lock(GDALClient::GDALClient::getGDALMutex(), TRUE);

	if (m_sourceSRS)	OSRDestroySpatialReference(m_sourceSRS);
}


GridData::~GridData() {
	if (m_fuelArray)			delete [] m_fuelArray;
	if (m_fuelValidArray)		delete [] m_fuelValidArray;
	if (m_elevationArray)		delete [] m_elevationArray;
	if (m_elevationValidArray)	delete [] m_elevationValidArray;
	if (m_terrainValidArray)	delete [] m_terrainValidArray;
	if (m_slopeFactor)			delete [] m_slopeFactor;
	if (m_slopeAzimuth)			delete [] m_slopeAzimuth;
}

#endif


HRESULT CCWFGM_Grid::MT_Lock(Layer * /*layerThread*/, bool exclusive, std::uint16_t obtain) {
	HRESULT hr;
	if (obtain == (std::uint16_t)-1) {
		std::int64_t state = m_lock.CurrentState();
		if (!state)				return SUCCESS_STATE_OBJECT_UNLOCKED;
		if (state < 0)			return SUCCESS_STATE_OBJECT_LOCKED_WRITE;
		if (state >= 1000000LL)	return SUCCESS_STATE_OBJECT_LOCKED_SCENARIO;
		return						   SUCCESS_STATE_OBJECT_LOCKED_READ;
	}
	else if (obtain) {
		if (exclusive)	m_lock.Lock_Write();
		else			m_lock.Lock_Read(1000000LL);

		hr = m_fuelMap->MT_Lock(exclusive, obtain);
	}
	else {
		hr = m_fuelMap->MT_Lock(exclusive, obtain);

		if (exclusive)	m_lock.Unlock();
		else		m_lock.Unlock(1000000LL);
	}
	return hr;
}


#ifndef DOXYGEN_IGNORE_CODE

GridData *CCWFGM_Grid::m_gridData(Layer *layerThread) {
	if (!layerThread)
		return &m_baseGrid;
	PolymorphicUserData v;
	HRESULT hr;
	if (SUCCEEDED(hr = m_layerManager->GetUserData(layerThread, this, &v)) && std::holds_alternative<void*>(v)) {
		GridData *gd;

		/*POLYMORPHIC CHECK*/
		try { gd = (GridData *)std::get<void *>(v); } catch (std::bad_variant_access &) { weak_assert(false); return &m_baseGrid; };

		if (gd)
			return gd;
	}
	return &m_baseGrid;
}


void CCWFGM_Grid::assignGridData(Layer *layerThread, GridData *gd) {
	PolymorphicUserData v = gd;
	HRESULT hr = m_layerManager->PutUserData(layerThread, this, v);
	weak_assert(SUCCEEDED(hr));
}

#endif


HRESULT CCWFGM_Grid::PutGridEngine(Layer *layerThread, ICWFGM_GridEngine *newVal) {
	if (!layerThread) {
		if (newVal) {
			boost::intrusive_ptr<ICWFGM_GridEngine> pGridEngine;
			pGridEngine = dynamic_cast<ICWFGM_GridEngine *>(const_cast<ICWFGM_GridEngine *>(newVal));
			if (pGridEngine.get()) {
				m_rootEngine = pGridEngine;
				return S_OK;
			}
			return E_FAIL;
		}
		else {
			m_rootEngine = nullptr;
			return S_OK;
		}
	}
	if (!m_layerManager)							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

	HRESULT hr;
	if (newVal) {
		hr = m_layerManager->PutGridEngine(layerThread, this, newVal);
		if (SUCCEEDED(hr)) {
			m_layerManager->PutUserData(layerThread, this, 0ULL);
		}
	}
	else {
		PolymorphicUserData v;
		m_layerManager->GetUserData(layerThread, this, &v);
		GridData *gd;
		try {
			if (std::holds_alternative<void *>(v)) {
				gd = (GridData *)std::get<void *>(v);
				if (gd)
					delete gd;
			}
		}
		catch (std::bad_variant_access &) {
			weak_assert(false);
			gd = nullptr;
		}
		hr = m_layerManager->PutGridEngine(layerThread, this, newVal);
	}
	return hr;
}


HRESULT CCWFGM_Grid::Valid(Layer * /*layerThread*/, const HSS_Time::WTime &start_time, const HSS_Time::WTimeSpan &duration, std::uint32_t option, std::vector<uint16_t> *diurnal_application_count) {
	if (!(m_flags & CCWFGMGRID_VALID))					return ERROR_SEVERITY_WARNING;
	if (!m_fuelMap)										return ERROR_SEVERITY_WARNING;
	if ((start_time.GetTotalMicroSeconds() == 0) && (duration.GetTotalMicroSeconds() == 0))				return S_OK;
									// not asking for weather too in the valid check
	return ERROR_GRID_WEATHER_NOT_IMPLEMENTED;			// this layer doesn't implement weather, another grid "stacked" on top of
}									// this may, though, so this error code may not get back to the scenario


HRESULT CCWFGM_Grid::GetEventTime(Layer * /*layerThread*/, const XY_Point& pt, std::uint32_t /*flags*/, const HSS_Time::WTime & /*from_time*/, HSS_Time::WTime * /*next_event*/, bool* /*event_valid*/) {
	if (!(m_flags & CCWFGMGRID_VALID))					return ERROR_SEVERITY_WARNING;
	if (!m_fuelMap)										return ERROR_SEVERITY_WARNING;

	return S_OK;
}


HRESULT CCWFGM_Grid::PreCalculationEvent(Layer *layerThread, const HSS_Time::WTime &time, std::uint32_t mode, CalculationEventParms *parms) {
										// if mode == 0, then it's a simulation being initialized (and you're likely getting the bounding box of the ignitions)
										//				, or if layerThread is 0, then you're initializing the main grid
										// if mode == 1, then it's a time step being initialized (and you're getting the bounding box of the fires at that state)
	std::uint32_t *cnt;
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread, &cnt);
	if (!gridEngine.get())							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	if ((mode & (~(1 << CWFGM_SCENARIO_OPTION_WEATHER_ALTERNATE_CACHE))) == 1) {
		(*cnt)++;
	}

	GridData *gd = m_gridData(layerThread);
	if (parms) {
		parms->TargetGridMin.x = parms->CurrentGridMin.x = gd->m_xllcorner;
		parms->TargetGridMin.y = parms->CurrentGridMin.y = gd->m_yllcorner;
		parms->TargetGridMax.x = parms->CurrentGridMax.x = gd->m_xllcorner + gd->m_xsize * gd->m_resolution;
		parms->TargetGridMax.y = parms->CurrentGridMax.y = gd->m_yllcorner + gd->m_ysize * gd->m_resolution;
	}

	return S_OK;
}


HRESULT CCWFGM_Grid::PostCalculationEvent(Layer *layerThread, const HSS_Time::WTime &time, std::uint32_t mode, CalculationEventParms *parms) {
	std::uint32_t *cnt;
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread, &cnt);
	if (!gridEngine)							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	if ((mode & (~(1 << CWFGM_SCENARIO_OPTION_WEATHER_ALTERNATE_CACHE))) == 1) {
		(*cnt)--;
	}

	return sizeGrid(layerThread, parms);
}


HRESULT CCWFGM_Grid::GetCommonData(Layer* layerThread, ICWFGM_CommonData** pVal) {
	if (!pVal)									return E_POINTER;

	*pVal = &m_commonData;
	return S_OK;
}


HRESULT CCWFGM_Grid::GetDimensions(Layer *layerThread, std::uint16_t *x_dim, std::uint16_t *y_dim) {
	if (!(m_flags & CCWFGMGRID_VALID))			return ERROR_GRID_UNINITIALIZED;
	if (!x_dim)									return E_POINTER;
	if (!y_dim)									return E_POINTER;

	GridData *gd = m_gridData(layerThread);
	*x_dim = gd->m_xsize;
	*y_dim = gd->m_ysize;
	return S_OK;
}


HRESULT CCWFGM_Grid::GetFuelData(Layer *layerThread, const XY_Point &pt, const HSS_Time::WTime &/*time*/, ICWFGM_Fuel **fuel, bool *fuel_valid, XY_Rectangle *cache_bbox) {
	if (!m_fuelMap.get())						{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	if (!(m_flags & CCWFGMGRID_VALID))			{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

	GridData *gd = m_gridData(layerThread);
	std::uint16_t x = gd->convertX(pt.x, cache_bbox);
	std::uint16_t y = gd->convertY(pt.y, cache_bbox);
	if (!gd->m_fuelArray)						{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	if (x >= gd->m_xsize)						return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	if (y >= gd->m_ysize)						return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	if (!fuel)									return E_POINTER;
	if (!fuel_valid)							return E_POINTER;

	std::uint32_t index = gd->arrayIndex(x, y);
	long idx, export_index;
	if (!gd->m_fuelValidArray[index]) {
		*fuel = nullptr;
		*fuel_valid = false;
		return ERROR_FUELS_FUEL_UNKNOWN;
	}

#ifdef _DEBUG
	if (gd->m_fuelArray[index] == (std::uint8_t)-1) {
		*fuel = nullptr;
		*fuel_valid = false;
		return ERROR_FUELS_FUEL_UNKNOWN;
	}
#endif

	*fuel_valid = true;
	return m_fuelMap->FuelAtIndex(gd->m_fuelArray[index], &idx, &export_index, fuel);
}


HRESULT CCWFGM_Grid::GetFuelIndexData(Layer *layerThread, const XY_Point &pt, const HSS_Time::WTime &/*time*/, std::uint8_t *fuel_index, bool *fuel_valid, XY_Rectangle *cache_bbox) {
	if (!m_fuelMap)								{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	if (!(m_flags & CCWFGMGRID_VALID))			{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

	GridData *gd = m_gridData(layerThread);
	std::uint16_t x = gd->convertX(pt.x, cache_bbox);
	std::uint16_t y = gd->convertY(pt.y, cache_bbox);
	if (!gd->m_fuelArray)						{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	if (x >= gd->m_xsize)						return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	if (y >= gd->m_ysize)						return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	if (!fuel_index)							return E_POINTER;
	if (!fuel_valid)							return E_POINTER;

	std::uint32_t index = gd->arrayIndex(x, y);
	*fuel_valid = gd->m_fuelValidArray[index];
	if (*fuel_valid)
		*fuel_index = gd->m_fuelArray[index];
	else
		*fuel_index = (std::uint8_t)(-1);

	if (!(*fuel_valid))							return ERROR_FUELS_FUEL_UNKNOWN;

#ifdef _DEBUG
	if (gd->m_fuelArray[index] == (std::uint8_t)(-1)) {
		weak_assert(false);
		return ERROR_FUELS_FUEL_UNKNOWN;
	}
#endif
	return S_OK;
}


HRESULT CCWFGM_Grid::GetFuelDataArray(Layer *layerThread, const XY_Point &min_pt, const XY_Point &max_pt, double scale,
	const HSS_Time::WTime & /*time*/, ICWFGM_Fuel_2d *fuel, bool_2d *fuel_valid) {

	if (!m_fuelMap)									{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	if (!(m_flags & CCWFGMGRID_VALID))				{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

	GridData *gd = m_gridData(layerThread);
	std::uint16_t x_min = gd->convertX(min_pt.x, nullptr), y_min = gd->convertY(min_pt.y, nullptr);
	std::uint16_t x_max = gd->convertX(max_pt.x, nullptr), y_max = gd->convertY(max_pt.y, nullptr);
	if (!gd->m_fuelArray)							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	if (x_min >= gd->m_xsize)						return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	if (y_min >= gd->m_ysize)						return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	if (x_max >= gd->m_xsize)						return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	if (y_max >= gd->m_ysize)						return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	if (min_pt.x > max_pt.x)						return E_INVALIDARG;
	if (min_pt.y > max_pt.y)						return E_INVALIDARG;
	if (scale != gd->m_resolution)					return ERROR_GRID_UNSUPPORTED_RESOLUTION;
	if (!fuel)										return E_POINTER;
	if (!fuel_valid)								return E_POINTER;

	const ICWFGM_Fuel_2d::size_type *dims = fuel->shape();
	if (dims[0] < (x_max - x_min + 1))		return E_INVALIDARG;
	if (dims[1] < (y_max - y_min + 1))		return E_INVALIDARG;
	
	dims = fuel_valid->shape();
	if (dims[0] < (x_max - x_min + 1))		return E_INVALIDARG;
	if (dims[1] < (y_max - y_min + 1))		return E_INVALIDARG;

	std::uint32_t index, index2;
	long idx, export_index;
	std::uint16_t x, y, x2, y2;
	HRESULT hr;
	ICWFGM_Fuel *fff = nullptr;
	bool bff = false;

	for (y = y_min; y <= y_max; y++) {
		for (x = x_min; x <= x_max; x++) {

#ifdef _DEBUG
			(*fuel)[x - x_min][y - y_min] = nullptr;
#endif

			(*fuel_valid)[x - x_min][y - y_min] = bff;
		}
	}

	for (y = y_min; y <= y_max; y++) {
		for (x = x_min, index = gd->arrayIndex(x, y); x <= x_max; x++, index++) {
			if ((*fuel_valid)[x - x_min][y - y_min])
				continue;				// if we have taken care of this point, then skip it and move on
			if (!gd->m_fuelValidArray[index]) {
				weak_assert((*fuel)[x - x_min][y - y_min] == nullptr);
				fff = nullptr;
				bff = false;
				(*fuel)[x - x_min][y - y_min] = fff;
				(*fuel_valid)[x - x_min][y - y_min] = bff;
				continue;
			}
			else
#ifdef _DEBUG
			if (gd->m_fuelArray[index] == (std::uint8_t)-1)
			{
				weak_assert((*fuel)[x - x_min][y - y_min] == nullptr);
				fff = nullptr;
				bff = false;
				(*fuel)[x - x_min][y - y_min] = fff;//*f = NULL;
				(*fuel_valid)[x - x_min][y - y_min] = bff;
				continue;				// hit a noData so just continue on to the next cell
			}
			else
#endif
			{
				hr = m_fuelMap->FuelAtIndex(gd->m_fuelArray[index], &idx, &export_index, &fff);
				bff = true;
				(*fuel)[x - x_min][y - y_min] = fff;
				(*fuel_valid)[x - x_min][y - y_min] = bff;
			}
			if (FAILED(hr))					// find out what fuel is here
				return hr;
			for (y2 = y; y2 <= y_max; y2++)		// now that we've got the fuel, see if it's used anywhere else in
			{
				x2 = ((y2 == y) ? x + 1 : x_min);
				if (x2 > x_max)
					continue;
				for (index2 = gd->arrayIndex(x2, y2); x2 <= x_max; x2++, index2++)
				{
					if ((*fuel_valid)[x2 - x_min][y2 - y_min])
						continue;

#ifdef _DEBUG
					if ((*fuel)[x2 - x_min][y2 - y_min]) {
						weak_assert(false);
						continue;
					}
#endif

					if ((gd->m_fuelValidArray[index]) && (gd->m_fuelValidArray[index2]) && (gd->m_fuelArray[index] == gd->m_fuelArray[index2])) {
						(*fuel)[x2 - x_min][y2 - y_min] = fff;
						(*fuel_valid)[x2 - x_min][y2 - y_min] = bff;
					}
				}
			}
		}
	}
	return S_OK;
}


HRESULT CCWFGM_Grid::GetFuelIndexDataArray(Layer *layerThread, const XY_Point &min_pt, const XY_Point &max_pt, double scale,
    const HSS_Time::WTime & /*time*/, uint8_t_2d *fuel, bool_2d *fuel_valid) {

	if (!m_fuelMap)									{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	if (!(m_flags & CCWFGMGRID_VALID))				{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

	GridData *gd = m_gridData(layerThread);
	std::uint16_t x_min = gd->convertX(min_pt.x, nullptr), y_min = gd->convertY(min_pt.y, nullptr);
	std::uint16_t x_max = gd->convertX(max_pt.x, nullptr), y_max = gd->convertY(max_pt.y, nullptr);
	if (!gd->m_fuelArray)							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	if (x_min >= gd->m_xsize)						return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	if (y_min >= gd->m_ysize)						return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	if (x_max >= gd->m_xsize)						return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	if (y_max >= gd->m_ysize)						return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	if (min_pt.x > max_pt.x)						return E_INVALIDARG;
	if (min_pt.y > max_pt.y)						return E_INVALIDARG;
	if (scale != gd->m_resolution)					return ERROR_GRID_UNSUPPORTED_RESOLUTION;

	if (!fuel)										return E_POINTER;
	if (!fuel_valid)								return E_POINTER;

	const ICWFGM_Fuel_2d::size_type *dims = fuel->shape();
	if (dims[0] < (x_max - x_min + 1))		return E_INVALIDARG;
	if (dims[1] < (y_max - y_min + 1))		return E_INVALIDARG;

	dims = fuel_valid->shape();
	if (dims[0] < (x_max - x_min + 1))		return E_INVALIDARG;
	if (dims[1] < (y_max - y_min + 1))		return E_INVALIDARG; 
	
	std::uint32_t index;
	std::uint16_t x, y;

	for (y = y_min; y <= y_max; y++)			// for every point that was requested...
	{
		for (x = x_min, index = gd->arrayIndex(x, y); x <= x_max; x++, index++) {
			(*fuel_valid)[x - x_min][y - y_min] = gd->m_fuelValidArray[index];
			if (gd->m_fuelValidArray[index])
				(*fuel)[x - x_min][y - y_min] = gd->m_fuelArray[index];
			else
				(*fuel)[x - x_min][y - y_min] = (std::uint8_t)(-1);
		}
	}
	return S_OK;
}

/*!
Get the elevation data at a given grid location.
\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
\param pt The grid cell location.
\param allow_defaults_returned Flag for allowing defaults to be returned
	\param	elevation	Elevation (m) of the requested location.
	\param	slope_factor	Percentage ground slope specified as a decimal value (0 - 1)
	\param	slope_azimuth	Direction of up-slope, Cartesian radians.
	\param	elev_valid	bit 0 = value set, bit 1 = value is a default (1) or not (0)
	\param	terrain_valid	bit 0 = value set, bit 1 = value is a default (1) or not (0)
	\param  bbox_cache: May be NULL. If provided, then the rectangle is filled or adjusted to indicate the maximum size of the cell containing the requested point which is constant and uniform in content and value.
	\sa ICWFGM_GridEngine::GetElevationData
	\retval E_POINTER	One or more of elevation, slope_factor, slope_azimuth is NULL
	\retval ERROR_GRID_UNINITIALIZED	Object hasn't been initialized; no gridded data has been loaded
	\retval ERROR_GRID_LOCATION_OUT_OF_RANGE x and/or y is invalid
	\retval S_OK success
*/
HRESULT CCWFGM_Grid::GetElevationData(Layer *layerThread, const XY_Point &pt, bool allow_defaults_returned,
	double *elevation, double *slope_factor, double *slope_azimuth, grid::TerrainValue *elev_valid, grid::TerrainValue *terrain_valid, XY_Rectangle *bbox_cache) {
	if (!elevation)								return E_POINTER;
	if (!slope_factor)							return E_POINTER;
	if (!slope_azimuth)							return E_POINTER;
	if (!elev_valid)							return E_POINTER;
	if (!terrain_valid)							return E_POINTER;

	GridData *gd = m_gridData(layerThread);
	std::uint16_t x = gd->convertX(pt.x, bbox_cache);
	std::uint16_t y = gd->convertY(pt.y, bbox_cache);
	if (!(m_flags & CCWFGMGRID_VALID))				{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	if (x >= gd->m_xsize)							return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	if (y >= gd->m_ysize)							return ERROR_GRID_LOCATION_OUT_OF_RANGE;

	std::uint32_t index = gd->arrayIndex(x, y);
	if (gd->m_elevationArray) {
		if (gd->m_elevationValidArray[index]) {
			*elevation = gd->m_elevationArray[index];
			*elev_valid = grid::TerrainValue::SET;
		}

#ifdef _DEBUG
		else if (gd->m_elevationArray[index] != -9999) {
			weak_assert(false);
			*elevation = gd->m_elevationArray[index];
			*elev_valid = grid::TerrainValue::SET;
		}
#endif

		if ((gd->m_terrainValidArray) && (gd->m_terrainValidArray[index])) {
				*slope_factor = ((double)(gd->m_slopeFactor[index])) / 100.0;	// if the value in file is percentage, we should use this.
				*slope_azimuth = NORMALIZE_ANGLE_RADIAN(DEGREE_TO_RADIAN(COMPASS_TO_CARTESIAN_DEGREE((double)gd->m_slopeAzimuth[index])) + Pi<double>());
				*terrain_valid = grid::TerrainValue::SET;
		}
		else {
			if (allow_defaults_returned) {
				*slope_factor = 0.0;
				*slope_azimuth = HalfPi<double>();	// 90 degrees cartesian is 0 north
				*terrain_valid = grid::TerrainValue::DEFAULT;
			}
			else {
				*slope_factor = -1.0;					// to say that slope percent is invalid
				*slope_azimuth = -1.0;					// to say that slope aspect is invalid
				*terrain_valid = grid::TerrainValue::NOT_SET;
			}
		}
	}
	else {
		if (allow_defaults_returned) {
			*elevation = m_defaultElevation;
			*slope_factor = 0.0;
			*slope_azimuth = HalfPi<double>();	// 90 degrees cartesian is 0 north
			*elev_valid = grid::TerrainValue::DEFAULT;
			*terrain_valid = grid::TerrainValue::DEFAULT;					// lowest bit for successfully set, next bit for being a default
		}
		else {
			*elevation = -9999.0;					// to say that elevation isn't available
			*slope_factor = -1.0;					// to say that slope percent is invalid
			*slope_azimuth = -1.0;					// to say that slope aspect is invalid
			*elev_valid = grid::TerrainValue::NOT_SET;
			*terrain_valid = grid::TerrainValue::NOT_SET;
		}
	}
	return S_OK;
}

/*!
Get the elevation data for all grid cells inside a specified rectangle.
\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
\param min_pt The minimum of the bounding rectangle.
\param max_pt The maximum (inclusive) of the bounding rectangle.
\param allow_defaults_returned Flag for allowing defaults to be returned
\param elevation An array of elevation data for the given bounding rectangle.
\param slope_factor An array of slope factor data for the given bounding rectangle.
\param slope_azimuth An array of slope azimuth data for the given bounding rectangle.
\sa ICWFGM_GridEngine::GetElevationDataArray
	\retval E_POINTER	One or more of elevation, slope_factor, slope_azimuth is NULL
	\retval E_INVALIDARG	The array is not 2D, or is insufficient in size to contain the requested data
	\retval ERROR_GRID_UNINITIALIZED	Object hasn't been initialized; no gridded data has been loaded
	\retval ERROR_GRID_LOCATION_OUT_OF_RANGE x and/or y is invalid
	\retval S_OK success
*/
HRESULT CCWFGM_Grid::GetElevationDataArray(Layer *layerThread, const XY_Point &min_pt, const XY_Point &max_pt, double scale, bool allow_defaults_returned, 
	double_2d *elevation, double_2d *slope_factor, double_2d *slope_azimuth, terrain_t_2d *elev_valid, terrain_t_2d *terrain_valid) {

	if ((!elevation) && (!slope_factor) && (!slope_azimuth))	return E_POINTER;

	GridData *gd = m_gridData(layerThread);
	std::uint16_t x_min = gd->convertX(min_pt.x, nullptr), y_min = gd->convertY(min_pt.y, nullptr);
	std::uint16_t x_max = gd->convertX(max_pt.x, nullptr), y_max = gd->convertY(max_pt.y, nullptr);
	if (x_min >= gd->m_xsize)								return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	if (y_min >= gd->m_ysize)								return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	if (x_max >= gd->m_xsize)								return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	if (y_max >= gd->m_ysize)								return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	if (min_pt.x > max_pt.x)								return E_INVALIDARG;
	if (min_pt.y > max_pt.y)								return E_INVALIDARG;
	if (scale != gd->m_resolution)							return ERROR_GRID_UNSUPPORTED_RESOLUTION;

	std::int32_t xsize = (x_max - x_min + 1);
	std::int32_t ysize = (y_max - y_min + 1);
	if (elevation) {
		const ICWFGM_Fuel_2d::size_type *dims = elevation->shape();
		if (dims[0] < xsize)								return E_INVALIDARG;
		if (dims[1] < ysize)								return E_INVALIDARG;
	}
	if (slope_factor) {
		const ICWFGM_Fuel_2d::size_type *dims = slope_factor->shape();
		if (dims[0] < xsize)								return E_INVALIDARG;
		if (dims[1] < ysize)								return E_INVALIDARG;
	}
	if (slope_azimuth) {
		const ICWFGM_Fuel_2d::size_type *dims = slope_azimuth->shape();
		if (dims[0] < xsize)								return E_INVALIDARG;
		if (dims[1] < ysize)								return E_INVALIDARG;
	}
	if (elev_valid) {
		const ICWFGM_Fuel_2d::size_type *dims = elev_valid->shape();
		if (dims[0] < xsize)								return E_INVALIDARG;
		if (dims[1] < ysize)								return E_INVALIDARG;
	}
	if (terrain_valid) {
		const ICWFGM_Fuel_2d::size_type *dims = terrain_valid->shape();
		if (dims[0] < xsize)								return E_INVALIDARG;
		if (dims[1] < ysize)								return E_INVALIDARG;
	}

	std::uint32_t index;
	std::uint16_t x, y;


	for (y = y_min; y <= y_max; y++) {
		for (x = x_min, index = gd->arrayIndex(x, y); x <= x_max; x++, index++) {
			if ((!gd->m_elevationValidArray) || (!gd->m_elevationValidArray[index])) {
				if (allow_defaults_returned) {
					if (elevation)		(*elevation)[x - x_min][y - y_min] = m_defaultElevation;
					if (slope_azimuth)	(*slope_azimuth)[x - x_min][y - y_min] = HalfPi<double>();
					if (slope_factor)	(*slope_factor)[x - x_min][y - y_min] = 0.0;
					if (elev_valid)		(*elev_valid)[x - x_min][y - y_min] = grid::TerrainValue::DEFAULT;
					if (terrain_valid)	(*terrain_valid)[x - x_min][y - y_min] = grid::TerrainValue::DEFAULT;
				}
				else {
					if (elevation)		(*elevation)[x - x_min][y - y_min] = -9999.0;
					if (slope_factor)	(*slope_factor)[x - x_min][y - y_min] = -1.0;
					if (slope_azimuth)	(*slope_azimuth)[x - x_min][y - y_min] = -1.0;
					if (elev_valid)		(*elev_valid)[x - x_min][y - y_min] = grid::TerrainValue::NOT_SET;
					if (terrain_valid)	(*terrain_valid)[x - x_min][y - y_min] = grid::TerrainValue::NOT_SET;
				}
			}
			else {
				if (elevation)
					(*elevation)[x - x_min][y - y_min] = gd->m_elevationArray[index];
				if (elev_valid)
					(*elev_valid)[x - x_min][y - y_min] = grid::TerrainValue::SET;

				if ((!gd->m_terrainValidArray) || (!gd->m_terrainValidArray[index])) {

#if defined(DEBUG) || defined(_DEBUG)
					double __slope = ((double)(gd->m_slopeFactor[index])) / 100.0;
#endif
					if (allow_defaults_returned) {
						if (slope_azimuth)	(*slope_azimuth)[x - x_min][y - y_min] = HalfPi<double>();
						if (slope_factor)	(*slope_factor)[x - x_min][y - y_min] = 0.0;
						if (terrain_valid)	(*terrain_valid)[x - x_min][y - y_min] = grid::TerrainValue::DEFAULT;
					}
					else {
						if (slope_azimuth)	(*slope_azimuth)[x - x_min][y - y_min] = -1.0;
						if (slope_factor)	(*slope_factor)[x - x_min][y - y_min] = -1.0;
						if (terrain_valid)	(*terrain_valid)[x - x_min][y - y_min] = grid::TerrainValue::NOT_SET;
					}
				}
				else {
					if (gd->m_slopeAzimuth[index] != (std::uint16_t)(-1)) {
						if (slope_factor)	(*slope_factor)[x - x_min][y - y_min] = ((double)(gd->m_slopeFactor[index])) / 100.0;	// if the value in file is percentage, we should use this.
						if (slope_azimuth)	(*slope_azimuth)[x - x_min][y - y_min] = NORMALIZE_ANGLE_RADIAN(DEGREE_TO_RADIAN(COMPASS_TO_CARTESIAN_DEGREE((double)gd->m_slopeAzimuth[index])) + Pi<double>());
						if (terrain_valid)	(*terrain_valid)[x - x_min][y - y_min] = grid::TerrainValue::SET;
					}
					else {
						if (allow_defaults_returned) {
							if (slope_azimuth)	(*slope_azimuth)[x - x_min][y - y_min] = HalfPi<double>();
							if (slope_factor)	(*slope_factor)[x - x_min][y - y_min] = 0.0;
							if (terrain_valid)	(*terrain_valid)[x - x_min][y - y_min] = grid::TerrainValue::DEFAULT;
						}
						else {
							if (slope_azimuth)	(*slope_azimuth)[x - x_min][y - y_min] = -1.0;
							if (slope_factor)	(*slope_factor)[x - x_min][y - y_min] = -1.0;
							if (terrain_valid)	(*terrain_valid)[x - x_min][y - y_min] = grid::TerrainValue::NOT_SET;
						}
					}
				}
			}
		}
	}
	return S_OK;
}


HRESULT CCWFGM_Grid::GetWeatherData(Layer *layerThread, const XY_Point &pt, const HSS_Time::WTime &time, std::uint64_t interpolate_method,
    IWXData *wx, IFWIData *ifwi, DFWIData *dfwi, bool *wx_valid, XY_Rectangle *bbox_cache) {

	if (interpolate_method & (CWFGM_GETEVENTTIME_QUERY_PRIMARY_WX_STREAM | CWFGM_GETEVENTTIME_QUERY_ANY_WX_STREAM)) {
		boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
		if (!gridEngine.get())							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

		return gridEngine->GetWeatherData(layerThread, pt, time, interpolate_method, wx, ifwi, dfwi, wx_valid, bbox_cache);
	}

	return E_NOTIMPL;
}


HRESULT CCWFGM_Grid::GetWeatherDataArray(Layer *layerThread, const XY_Point &min_pt, const XY_Point &max_pt, double scale, const HSS_Time::WTime &time, std::uint64_t interpolate_method,
    IWXData_2d * wx, IFWIData_2d * ifwi, DFWIData_2d * dfwi, bool_2d *wx_valid) {

	if (interpolate_method & (CWFGM_GETEVENTTIME_QUERY_PRIMARY_WX_STREAM | CWFGM_GETEVENTTIME_QUERY_ANY_WX_STREAM)) {
		boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
		if (!gridEngine.get())							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

		return gridEngine->GetWeatherDataArray(layerThread, min_pt, max_pt, scale, time, interpolate_method, wx, ifwi, dfwi, wx_valid);
	}

	return E_NOTIMPL;
}


HRESULT CCWFGM_Grid::GetAttributeData(Layer * /*layerThread*/, const XY_Point &pt, const HSS_Time::WTime & /*time*/, const HSS_Time::WTimeSpan& timeSpan, std::uint16_t option, std::uint64_t /*optionFlags*/, NumericVariant *attribute, grid::AttributeValue *attribute_valid, XY_Rectangle *cache_bbox) {
	GridData* gd = m_gridData(nullptr);

	switch (option) {
		case CWFGM_FUELGRID_ATTRIBUTE_X_START:
		case CWFGM_FUELGRID_ATTRIBUTE_X_MID:
		case CWFGM_FUELGRID_ATTRIBUTE_X_END:
		{
			if (!attribute)							return E_POINTER;
			if (!attribute_valid)					return E_POINTER;
			double x = pt.x;
			x -= gd->m_xllcorner;
			x /= gd->m_resolution;
			x = floor(x);
			if (option == CWFGM_FUELGRID_ATTRIBUTE_X_MID)
				x += 0.5;
			else if (option == CWFGM_FUELGRID_ATTRIBUTE_X_END)
				x++;
			x *= gd->m_resolution;
			x += gd->m_xllcorner;
			*attribute = x;
			*attribute_valid = grid::AttributeValue::SET;
			return S_OK;
		}
		case CWFGM_FUELGRID_ATTRIBUTE_Y_START:
		case CWFGM_FUELGRID_ATTRIBUTE_Y_MID:
		case CWFGM_FUELGRID_ATTRIBUTE_Y_END:
		{
			if (!attribute)							return E_POINTER;
			if (!attribute_valid)					return E_POINTER;
			double y = pt.y;
			y -= gd->m_yllcorner;
			y /= gd->m_resolution;
			y = floor(y);
			if (option == CWFGM_FUELGRID_ATTRIBUTE_Y_MID)
				y += 0.5;
			else if (option == CWFGM_FUELGRID_ATTRIBUTE_Y_END)
				y++;
			y *= gd->m_resolution;
			y += gd->m_yllcorner;
			*attribute = y;
			*attribute_valid = grid::AttributeValue::SET;
			return S_OK;
		}
	}
	return E_INVALIDARG;
}


HRESULT CCWFGM_Grid::GetAttributeDataArray(Layer * /*layerThread*/, const XY_Point & /*min_pt*/, const XY_Point & /*max_pt*/, double /*scale*/, const HSS_Time::WTime & /*time*/, const HSS_Time::WTimeSpan& timeSpan,
    std::uint16_t /*option*/, std::uint64_t /*optionFlags*/, NumericVariant_2d * /*attribute*/, attribute_t_2d *attribute_valid) {
	return E_NOTIMPL;
}


HRESULT CCWFGM_Grid::IsFuelUsed(ICWFGM_Fuel *fuel) {
	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);

	if (!(m_flags & CCWFGMGRID_VALID))				{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	if (!m_fuelMap)									{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	GridData *gd = m_gridData(nullptr);
	if (!gd->m_fuelArray)							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

	std::uint8_t f = (std::uint8_t)-1;
	long i, export_index, index = gd->m_xsize * gd->m_ysize;
	while (1) {
		HRESULT hr = m_fuelMap->IndexOfFuel(fuel, &i, &export_index, &f);
		if (FAILED(hr))							return hr;	// didn't find one
		std::uint8_t *ff = gd->m_fuelArray;
		bool *fv = gd->m_fuelValidArray;
		for (i = 0; i < index; i++, ff++)
			if ((*fv) && (*ff == (std::uint8_t)f))
				return S_OK;				// found one
	}								// didn't find one
	return ERROR_SEVERITY_WARNING;
}


HRESULT CCWFGM_Grid::get_FuelMap(CCWFGM_FuelMap **pVal) {
	if (!pVal)								return E_POINTER;

	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);

	*pVal = m_fuelMap.get();
	if (!m_fuelMap)								{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	return S_OK;
}


HRESULT CCWFGM_Grid::put_FuelMap(CCWFGM_FuelMap *newVal) {
	if (!newVal)								return E_POINTER;

	SEM_BOOL engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)								return ERROR_SCENARIO_SIMULATION_RUNNING;

	if (m_fuelMap)								{ weak_assert(false); return ERROR_GRID_INITIALIZED; }
	boost::intrusive_ptr <CCWFGM_FuelMap> pFuelMap;
	pFuelMap = dynamic_cast<CCWFGM_FuelMap *>(newVal);
	HRESULT retval;
	if (pFuelMap) {
		m_fuelMap = pFuelMap;
		m_bRequiresSave = true;
		retval = S_OK;
	}
	else
		retval = E_FAIL;

	return retval;
}


HRESULT CCWFGM_Grid::Clone(boost::intrusive_ptr<ICWFGM_CommonBase> *newObject) const {
	if (!newObject)						return E_POINTER;

	CRWThreadSemaphoreEngage engage(*(CRWThreadSemaphore *)&m_lock, SEM_FALSE);

	try {
		CCWFGM_Grid *f = new CCWFGM_Grid(*this);
		*newObject = f;
		return S_OK;
	}
	catch (std::exception &e) {
	}
	return E_FAIL;
}


#ifndef DOXYGEN_IGNORE_CODE

//must be in [1, 4]
#define MAX_INTERP_NEIGHBOUR_DEPTH 2
//cap the number of valid nodes to check
#define MAX_TOTAL_NODES 4
//if this many valid nodes can't be found within the neighbour depth, set NODATA
#define MIN_TOTAL_NODES 4

static_assert(MAX_INTERP_NEIGHBOUR_DEPTH > 0 && MAX_INTERP_NEIGHBOUR_DEPTH < 5, "Invalid interpolation neighbour depth.");
static_assert(MAX_TOTAL_NODES >= 4, "Too few interpolation nodes.");
static_assert(MAX_TOTAL_NODES >= MIN_TOTAL_NODES, "Max nodes must be >= to min total nodes.");

#define NEIGHBOURS_FIRST_SIZE 4
#define NEIGHBOURS_FIRST_WEIGHT 1
static std::int8_t neighbours_first_x[4] = { 1, 0, -1, 0 };
static std::int8_t neighbours_first_y[4] = { 0, 1, 0, -1 };

#if MAX_INTERP_NEIGHBOUR_DEPTH > 1
#define NEIGHBOURS_SECOND_SIZE 4
#define NEIGHBOURS_SECOND_WEIGHT (Constants::Sqrt2<double>() * 0.5);
static std::int8_t neighbours_second_x[4] = { 1, -1, -1, 1 };
static std::int8_t neighbours_second_y[4] = { 1, 1, -1, -1 };
#endif
#if MAX_INTERP_NEIGHBOUR_DEPTH > 2
#define NEIGHBOURS_THIRD_SIZE 4
#define NEIGHBOURS_THIRD_WEIGHT 0.5
static std::int8_t neighbours_third_x[4] = { 1, -1, -1, 1 };
static std::int8_t neighbours_third_y[4] = { 1, 1, -1, -1 };
#endif
#if MAX_INTERP_NEIGHBOUR_DEPTH > 3
#define NEIGHBOURS_FOURTH_SIZE 8
#define NEIGHBOURS_FOURTH_WEIGHT 0.4472135955
static std::int8_t neighbours_fourth_x[8] = { 2, 1, -1, -2, -2, -1, 1, 2 };
static std::int8_t neighbours_fourth_y[8] = { 1, 2, 2, 1, -1, -2, -2, -1 };
#endif

bool CCWFGM_Grid::interpolateElevation(GridData *gd, std::uint16_t i, std::uint16_t j, std::uint16_t *elev) {
	double numer = 0;
	std::uint16_t count = 0;
	std::uint16_t indx, indy;
	double tmp;
	double denom = 0;
	std::uint32_t a_index;

	for (int x = 0; x < NEIGHBOURS_FIRST_SIZE; x++) {
		indx = i + neighbours_first_x[x];
		indy = j + neighbours_first_y[x];
		if (indx < gd->m_xsize && indy < gd->m_ysize) {
			a_index = gd->arrayIndex(indx, indy);
			if (gd->m_elevationValidArray[a_index]) {
				tmp = static_cast<double>(gd->m_elevationArray[a_index]);
				count++;
				numer += tmp * NEIGHBOURS_FIRST_WEIGHT;
				denom += NEIGHBOURS_FIRST_WEIGHT;
			}
		}
		if (count >= MAX_TOTAL_NODES)
			break;
	}

#if MAX_INTERP_NEIGHBOUR_DEPTH > 1
	if (count < MAX_TOTAL_NODES) {
		for (int x = 0; x < NEIGHBOURS_SECOND_SIZE; x++) {
			indx = i + neighbours_second_x[x];
			indy = j + neighbours_second_y[x];
			if (indx < gd->m_xsize && indy < gd->m_ysize) {
				a_index = gd->arrayIndex(indx, indy);
				if (gd->m_elevationValidArray[a_index]) {
					tmp = static_cast<double>(gd->m_elevationArray[a_index]);
					count++;
					numer += tmp * NEIGHBOURS_SECOND_WEIGHT;
					denom += NEIGHBOURS_SECOND_WEIGHT;
				}
			}
			if (count >= MAX_TOTAL_NODES)
				break;
		}

#if MAX_INTERP_NEIGHBOUR_DEPTH > 2
		if (count < MAX_TOTAL_NODES) {
			for (int x = 0; x < NEIGHBOURS_THIRD_SIZE; x++) {
				indx = i + neighbours_third_x[x];
				indy = j + neighbours_third_y[x];
				if (indx >= 0 && indy >= 0 && indx < m_xsize && indy < m_ysize) {
					a_index = gd->arrayIndex(indx, indy);
					if (gd->m_elevationValidArray[a_index]) {
						tmp = static_cast<double>(gd->m_elevationArray[a_index]);
						count++;
						numer += tmp * NEIGHBOURS_THIRD_WEIGHT;
						denom += NEIGHBOURS_THIRD_WEIGHT;
					}
				}
				if (count >= MAX_TOTAL_NODES)
					break;
			}

#if MAX_INTERP_NEIGHBOUR_DEPTH > 3
			if (count < MAX_TOTAL_NODES) {
				for (int x = 0; x < NEIGHBOURS_FOURTH_SIZE; x++) {
					indx = i + neighbours_fourth_x[x];
					indy = j + neighbours_fourth_y[x];
					if (indx >= 0 && indy >= 0 && indx < m_xsize && indy < m_ysize) {
						a_index = gd->arrayIndex(indx, indy);
						if (gd->m_elevationValidArray[a_index]) {
							tmp = static_cast<double>(gd->m_elevationArray[a_index]);
							count++;
							numer += tmp * NEIGHBOURS_FOURTH_WEIGHT;
							denom += NEIGHBOURS_FOURTH_WEIGHT;
						}
					}
					if (count >= MAX_TOTAL_NODES)
						break;
				}
			}
#endif
		}
#endif
	}
#endif

	if (count >= MIN_TOTAL_NODES) {
		*elev = (std::uint16_t)(numer / denom);
		return true;
	}
	else {
		*elev = (std::uint16_t) -9999;
		return false;
	}
}


class TraceNode : public MinNode {
public:
	TraceNode(std::uint16_t _x, std::uint16_t _y, std::uint32_t _index) { x = _x; y = _y; index = _index; }
	std::uint16_t x, y;
	std::uint32_t index;

	DECLARE_OBJECT_CACHE(TraceNode, TraceNode)
};

IMPLEMENT_OBJECT_CACHE(TraceNode, TraceNode, 4 * 1024 * 1024 / sizeof(TraceNode), false, 16)


void CCWFGM_Grid::determineElevationAreas(std::uint8_t *outside) {
	std::uint16_t i;
	std::uint32_t index;
	MinListTempl<TraceNode> nodes;
	for (i = 0; i < m_baseGrid.m_xsize; i++) {
		index = m_baseGrid.arrayIndex(i, 0);
		if (!m_baseGrid.m_elevationValidArray[index])
			nodes.AddTail(new TraceNode(i, 0, index));
		index = m_baseGrid.arrayIndex(i, m_baseGrid.m_ysize - 1);
		if (!m_baseGrid.m_elevationValidArray[index])
			nodes.AddTail(new TraceNode(i, m_baseGrid.m_ysize - 1, index));
	}
	for (i = 0; i < m_baseGrid.m_ysize; i++) {
		index = m_baseGrid.arrayIndex(0, i);
		if (!m_baseGrid.m_elevationValidArray[index])
			nodes.AddTail(new TraceNode(0, i, index));
		index = m_baseGrid.arrayIndex(m_baseGrid.m_xsize - 1, i);
		if (!m_baseGrid.m_elevationValidArray[index])
			nodes.AddTail(new TraceNode(m_baseGrid.m_xsize - 1, i, index));
	}

	while (!nodes.IsEmpty()) {
		TraceNode *tn = nodes.RemHead();
		index = tn->index;
		if (!outside[index]) {
			if (!m_baseGrid.m_elevationValidArray[index]) {
				outside[index] = 0x3;	// visited AND set as outside
				if (((std::uint16_t)(tn->x - 1)) < m_baseGrid.m_xsize) {
					index = m_baseGrid.arrayIndex(tn->x - 1, tn->y);
					if ((!outside[index]) && (!m_baseGrid.m_elevationValidArray[index]))
						nodes.AddHead(new TraceNode(tn->x - 1, tn->y, index));
				}
				if (((std::uint16_t)(tn->x + 1)) < m_baseGrid.m_xsize) {
					index = m_baseGrid.arrayIndex(tn->x + 1, tn->y);
					if ((!outside[index]) && (!m_baseGrid.m_elevationValidArray[index]))
						nodes.AddHead(new TraceNode(tn->x + 1, tn->y, index));
				}
				if (((std::uint16_t)(tn->y - 1)) < m_baseGrid.m_ysize) {
					index = m_baseGrid.arrayIndex(tn->x, tn->y - 1);
					if ((!outside[index]) && (!m_baseGrid.m_elevationValidArray[index]))
						nodes.AddHead(new TraceNode(tn->x, tn->y - 1, index));
				}
				if (((std::uint16_t)(tn->y + 1)) < m_baseGrid.m_ysize) {
					index = m_baseGrid.arrayIndex(tn->x, tn->y + 1);
					if ((!outside[index]) && (!m_baseGrid.m_elevationValidArray[index]))
						nodes.AddHead(new TraceNode(tn->x, tn->y + 1, index));
				}
			}
			else
				outside[index] = 0x2;	// visited, not set as outside
		}
		delete tn;
	}
}


/*!
Calculates the slope factor and azimuth from the elevation data.  The method is based on the nearest neighbour method from:\n
Dunn, M. and R. Hickey, Cartography <b>9-15</b> 27 (1998)
*/
HRESULT CCWFGM_Grid::calculateSlopeFactorAndAzimuth(Layer *layerThread, std::uint8_t *calc_bits) {
	GridData *gd = m_gridData(layerThread);
	double slope_factor, sEW, sNS;
	double z1, z2, z3, z4, z5, z6, z7, z8, z9;
	bool v1, v2, v3, v4, v5, v6, v7, v8, v9;
	std::uint16_t iM1, iP1, jM1, jP1, interp;
	double denom = 100.0 / (8.0 * gd->m_resolution);
	HRESULT error = S_OK;
	bool only_nodata = true;
	std::uint32_t a_index;

	std::int32_t index = gd->m_xsize * gd->m_ysize;

	std::uint8_t *outside = new std::uint8_t[index];
	memset(outside, 0, index);
	determineElevationAreas(outside);

	if (gd->m_slopeFactor) {
		delete [] gd->m_slopeFactor;
		error = SUCCESS_GRID_DATA_UPDATED;
	}
	gd->m_slopeFactor = new std::uint16_t[index];
	if (gd->m_slopeAzimuth)
		delete [] gd->m_slopeAzimuth;
	gd->m_slopeAzimuth = new std::uint16_t[index];

	if (gd->m_terrainValidArray)
		delete [] gd->m_terrainValidArray;
	gd->m_terrainValidArray = new bool[index];

	int arrInd;
	std::uint16_t a_min = (std::uint16_t)-1, a_max = 0;
	std::uint16_t b_min = (std::uint16_t)-1, b_max = 0;

	std::uint64_t lastmissing = 0;
	std::uint64_t missing = 0;
	do {
		lastmissing = missing;
		missing = 0;
		for (std::uint16_t i = 0; i < gd->m_xsize; i++) {
			for (std::uint16_t j = 0; j < gd->m_ysize; j++) {
				a_index = gd->arrayIndex(i, j);

				if ((!gd->m_elevationValidArray[a_index]) && ((!(outside[a_index] & 0x1)) || (gd->m_fuelValidArray[a_index]))) {
					if (interpolateElevation(gd, i, j, &interp)) {
						gd->m_elevationArray[a_index] = interp;
						gd->m_elevationValidArray[a_index] = true;
						*calc_bits |= 0x2;
					} else
						missing++;
				}
			}
		}
	} while (missing != 0 && missing != lastmissing);

	if (missing != 0) {
		*calc_bits |= 0x4;
	}

	for (std::uint16_t i = 0; i < gd->m_xsize; i++) {
		iM1 = i > 0 ? i - 1 : i;
		iP1 = (i + 1) < gd->m_xsize ? i + 1 : i;
		for (std::uint16_t j = 0; j < gd->m_ysize; j++) {
			arrInd = gd->arrayIndex(i,j);
			jM1 = j > 0 ? j - 1 : j;
			jP1 = (j + 1) < gd->m_ysize ? j + 1 : j;
			z1 = gd->m_elevationArray[gd->arrayIndex(iP1,jP1)];
			z2 = gd->m_elevationArray[gd->arrayIndex(i,jP1)];
			z3 = gd->m_elevationArray[gd->arrayIndex(iM1,jP1)];
			z4 = gd->m_elevationArray[gd->arrayIndex(iM1,j)];
			z5 = gd->m_elevationArray[gd->arrayIndex(iM1,jM1)];
			z6 = gd->m_elevationArray[gd->arrayIndex(i,jM1)];
			z7 = gd->m_elevationArray[gd->arrayIndex(iP1,jM1)];
			z8 = gd->m_elevationArray[gd->arrayIndex(iP1,j)];
			z9 = gd->m_elevationArray[gd->arrayIndex(i,j)];

			v1 = gd->m_elevationValidArray[gd->arrayIndex(iP1, jP1)];
			v2 = gd->m_elevationValidArray[gd->arrayIndex(i, jP1)];
			v3 = gd->m_elevationValidArray[gd->arrayIndex(iM1, jP1)];
			v4 = gd->m_elevationValidArray[gd->arrayIndex(iM1, j)];
			v5 = gd->m_elevationValidArray[gd->arrayIndex(iM1, jM1)];
			v6 = gd->m_elevationValidArray[gd->arrayIndex(i, jM1)];
			v7 = gd->m_elevationValidArray[gd->arrayIndex(iP1, jM1)];
			v8 = gd->m_elevationValidArray[gd->arrayIndex(iP1, j)];
			v9 = gd->m_elevationValidArray[gd->arrayIndex(i, j)];

			if (!v1) {
				if (v2)
					z1 = z2;
				else if (v8)
					z1 = z8;
				else if (v9)
					z1 = z9;
				else {
					gd->m_terrainValidArray[arrInd] = false;
					gd->m_slopeFactor[arrInd] = (std::uint16_t)-1;
					gd->m_slopeAzimuth[arrInd] = (std::uint16_t)-1;
					continue;
				}
			}
			if (!v2) {
				if (v3)
					z2 = z3;
				else if (v9)
					z2 = z9;
				else if (v1)
					z2 = z1;
				else {
					gd->m_terrainValidArray[arrInd] = false;
					gd->m_slopeFactor[arrInd] = (std::uint16_t)-1;
					gd->m_slopeAzimuth[arrInd] = (std::uint16_t)-1;
					continue;
				}
			}
			if (!v3) {
				if (v4)
					z3 = z4;
				else if (v9)
					z3 = z9;
				else if (v2)
					z3 = z2;
				else {
					gd->m_terrainValidArray[arrInd] = false;
					gd->m_slopeFactor[arrInd] = (std::uint16_t)-1;
					gd->m_slopeAzimuth[arrInd] = (std::uint16_t)-1;
					continue;
				}
			}
			if (!v4) {
				if (v5)
					z4 = z5;
				else if (v9)
					z4 = z9;
				else if (v3)
					z4 = z3;
				else {
					gd->m_terrainValidArray[arrInd] = false;
					gd->m_slopeFactor[arrInd] = (std::uint16_t)-1;
					gd->m_slopeAzimuth[arrInd] = (std::uint16_t)-1;
					continue;
				}
			}
			if (!v5) {
				if (v6)
					z5 = z6;
				else if (v9)
					z5 = z9;
				else if (v4)
					z5 = z4;
				else {
					gd->m_terrainValidArray[arrInd] = false;
					gd->m_slopeFactor[arrInd] = (std::uint16_t)-1;
					gd->m_slopeAzimuth[arrInd] = (std::uint16_t)-1;
					continue;
				}
			}
			if (!v6) {
				if (v7)
					z6 = z7;
				else if (v9)
					z6 = z9;
				else if (v5)
					z6 = z5;
				else {
					gd->m_terrainValidArray[arrInd] = false;
					gd->m_slopeFactor[arrInd] = (std::uint16_t)-1;
					gd->m_slopeAzimuth[arrInd] = (std::uint16_t)-1;
					continue;
				}
			}
			if (!v7) {
				if (v8)
					z7 = z8;
				else if (v9)
					z7 = z9;
				else if (v6)
					z7 = z6;
				else {
					gd->m_terrainValidArray[arrInd] = false;
					gd->m_slopeFactor[arrInd] = (std::uint16_t)-1;
					gd->m_slopeAzimuth[arrInd] = (std::uint16_t)-1;
					continue;
				}
			}
			if (!v8) {
				if (v9)
					z8 = z9;
				else if (v7)
					z8 = z7;
				else if (v1)
					z8 = z1;
				else {
					gd->m_terrainValidArray[arrInd] = false;
					gd->m_slopeFactor[arrInd] = (std::uint16_t)-1;
					gd->m_slopeAzimuth[arrInd] = (std::uint16_t)-1;
					continue;
				}
			}
			only_nodata = false;

			sEW = ((z3 + (2.0 * z4) + z5) - (z1 + (2.0 * z8) + z7));
			sNS = ((z1 + (2.0 * z2) + z3) - (z7 + (2.0 * z6) + z5));

			gd->m_terrainValidArray[arrInd] = true;
			slope_factor = sqrt(pow(sEW,2.0) + pow(sNS,2.0)) * denom;
			gd->m_slopeFactor[arrInd] = (std::uint16_t)slope_factor;
			if (gd->m_slopeFactor[arrInd] > a_max)  a_max = gd->m_slopeFactor[arrInd];
			if (gd->m_slopeFactor[arrInd] < a_min)  a_min = gd->m_slopeFactor[arrInd];

			if (gd->m_slopeFactor[arrInd] == 0)
			{
				gd->m_slopeAzimuth[arrInd] = (std::uint16_t)-1;
				continue;
			}

			gd->m_slopeAzimuth[arrInd] = (std::uint16_t)(CARTESIAN_TO_COMPASS_DEGREE(RADIAN_TO_DEGREE(atan2(sNS, -sEW) + Pi<double>())));

			if (gd->m_slopeAzimuth[arrInd] > b_max) b_max = gd->m_slopeAzimuth[arrInd];
			if (gd->m_slopeAzimuth[arrInd] < b_min) b_min = gd->m_slopeAzimuth[arrInd];
		}
	}

	gd->m_minSlopeFactor = a_min;
	gd->m_maxSlopeFactor = a_max;
	gd->m_minAzimuth = b_min;
	gd->m_maxAzimuth = b_max;
	m_bRequiresSave = true;

	delete [] outside;
	return error;
}

#endif


HRESULT CCWFGM_Grid::SetAttribute(std::uint16_t option, const PolymorphicAttribute &var) {
	SEM_BOOL engaged;
	bool bval;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)								return ERROR_SCENARIO_SIMULATION_RUNNING;

	double value;
	std::uint32_t lValue;
	WTimeSpan llvalue;
	std::uint32_t old;

	HRESULT hr = E_INVALIDARG;

	switch (option) {
		case CWFGM_GRID_ATTRIBUTE_LATITUDE:		
								if (FAILED(hr = VariantToDouble_(var, &value)))				break;
								if (value < DEGREE_TO_RADIAN(-90.0))					{ weak_assert(false); return E_INVALIDARG; }
								if (value > DEGREE_TO_RADIAN(90.0))					{ weak_assert(false); return E_INVALIDARG; }
								m_worldLocation.m_latitude(value);
								m_bRequiresSave = true;
								return S_OK;

		case CWFGM_GRID_ATTRIBUTE_LONGITUDE:
								if (FAILED(hr = VariantToDouble_(var, &value)))			break;
								if (value < DEGREE_TO_RADIAN(-180.0))					{ weak_assert(false); return E_INVALIDARG; }
								if (value > DEGREE_TO_RADIAN(180.0))					{ weak_assert(false); return E_INVALIDARG; }
								m_worldLocation.m_longitude(value);
								m_bRequiresSave = true;
								return S_OK;

		case CWFGM_GRID_ATTRIBUTE_DEFAULT_ELEVATION:
								if (FAILED(hr = VariantToDouble_(var, &value)))		break;
								if (value < 0.0) if (value != -99.0)					{ weak_assert(false); return E_INVALIDARG; }
								if (value > 7000.0)							{ weak_assert(false); return E_INVALIDARG; }
								m_defaultElevation = value;
								m_flags |= CCWFGMGRID_DEFAULT_ELEV_SET;
								m_bRequiresSave = true;
								return S_OK;

		case CWFGM_GRID_ATTRIBUTE_DEFAULT_FMC_ACTIVE:
								if (FAILED(hr = VariantToBoolean_(var, &bval)))								break;
								if (bval)
									m_flags |= CCWFGMGRID_SPECIFIED_FMC_ACTIVE;
								else
									m_flags &= ~(CCWFGMGRID_SPECIFIED_FMC_ACTIVE);
								m_bRequiresSave = true;
								return S_OK;

		case CWFGM_GRID_ATTRIBUTE_DEFAULT_FMC:
								if (FAILED(hr = VariantToDouble_(var, &value)))								break;
								if (value > 300.0)							return E_INVALIDARG;
								if (value < 0.0)							{ weak_assert(false);  return E_INVALIDARG; }
								m_defaultFMC = value;
								m_bRequiresSave = true;
								return S_OK;

		case CWFGM_GRID_ATTRIBUTE_SPATIALREFERENCE: {
								std::string str;
								try {
									str = std::get<std::string>(var);
								}
								catch (std::bad_variant_access &) {
									weak_assert(false);
									break;
								}
								if (str.length()) {
									CSemaphoreEngage lock(GDALClient::GDALClient::getGDALMutex(), TRUE);

									OGRSpatialReferenceH sourceSRS = CCoordinateConverter::CreateSpatialReferenceFromStr(str.c_str());
									if (sourceSRS) {
										if (m_sourceSRS)
											OSRDestroySpatialReference(m_sourceSRS);
										m_sourceSRS = sourceSRS;
										m_projectionContents = str;
										m_bRequiresSave = true;
									} else {
										m_loadWarning += "Grid: Could not re-parse originally provided projection data.\n";
									}
								}
								return S_OK;
							    }

		case CWFGM_GRID_ATTRIBUTE_GIS_CANRESIZE:
								try {
									bval = std::get<bool>(var);
								}
								catch (std::bad_variant_access &) {
									weak_assert(false);
									break;
								}
								old = m_flags;
								if (bval)
									m_flags |= CCWFGMGRID_ALLOW_GIS;
								else
									m_flags &= (~(CCWFGMGRID_ALLOW_GIS));
								if (old != m_flags)
									m_bRequiresSave = true;
								return S_OK;

		case CWFGM_GRID_ATTRIBUTE_GIS_URL: {
								std::string str;
								try {
									str = std::get<std::string>(var);
								}
								catch (std::bad_variant_access &) {
									weak_assert(false);
									break;
								}
								if (str.length()) {
									m_gisGridURL = str;
									m_bRequiresSave = true;
								}
								}
								return S_OK;

		case CWFGM_GRID_ATTRIBUTE_GIS_LAYER: {
								std::string str;
								try {
									str = std::get<std::string>(var);
								}
								catch (std::bad_variant_access &) {
									weak_assert(false);
									break;
								}
								if (str.length()) {
									m_gisGridLayer = str;
									m_bRequiresSave = true;
								}
								}
								return S_OK;

		case CWFGM_GRID_ATTRIBUTE_GIS_UID: {
								std::string str;
								try {
									str = std::get<std::string>(var);
								}
								catch (std::bad_variant_access &) {
									weak_assert(false);
									break;
								}
								m_gisGridUID = str;
								m_bRequiresSave = true;
								}
								return S_OK;

		case CWFGM_GRID_ATTRIBUTE_GIS_PWD: {
								std::string str;
								try {
									str = std::get<std::string>(var);
								}
								catch (std::bad_variant_access &) {
									weak_assert(false);
									break;
								}
								m_gisGridPWD = str;
								m_bRequiresSave = true;
								}
								return S_OK;

		case CWFGM_GRID_ATTRIBUTE_INITIALSIZE:
								try {
									value = std::get<double>(var);
								}
								catch (std::bad_variant_access &) {
									weak_assert(false);
									break;
								}
								if (value < 100.0)									{ weak_assert(false); return E_INVALIDARG; }		// a 100m initial size is pretty small!
								if (value > 100000.0)								{ weak_assert(false); return E_INVALIDARG; }		// a 100km initial size is pretty big!
								if (m_flags & CCWFGMGRID_VALID)
									value = ceil(value / m_baseGrid.m_resolution) * m_baseGrid.m_resolution;
								m_initialSize = value;
								m_bRequiresSave = true;
								return S_OK;

		case CWFGM_GRID_ATTRIBUTE_BUFFERSIZE:
								try {
									value = std::get<double>(var);
								}
								catch (std::bad_variant_access &) {
									weak_assert(false);
									break;
								}
								if (value < 100.0)									{ weak_assert(false); return E_INVALIDARG; }		// a 100m buffer size is pretty small!
								if (value > 100000.0)								{ weak_assert(false); return E_INVALIDARG; }		// a 100km buffer size is pretty big!
								m_reactionSize = value;
								m_bRequiresSave = true;
								return S_OK;

		case CWFGM_GRID_ATTRIBUTE_GROWTHSIZE:
								try {
									value = std::get<double>(var);
								}
								catch (std::bad_variant_access &) {
									weak_assert(false);
									break;
								}
								if (value < 100.0)									{ weak_assert(false); return E_INVALIDARG; }		// a 100m grow size is pretty small!
								if (value > 100000.0)								{ weak_assert(false); return E_INVALIDARG; }		// a 100km grow size is pretty big!
								if (m_flags & CCWFGMGRID_VALID)
									value = ceil(value / m_baseGrid.m_resolution) * m_baseGrid.m_resolution;
								m_reactionSize = value;
								m_bRequiresSave = true;
								return S_OK;
	}

	weak_assert(false);
	return hr;
}


HRESULT CCWFGM_Grid::GetAttribute(std::uint16_t option, PolymorphicAttribute *value) {
	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);
	return GetAttribute(0, option, value);
}


HRESULT CCWFGM_Grid::GetAttribute(Layer * /*layerThread*/, std::uint16_t option, PolymorphicAttribute *value) {
	if (!(m_flags & CCWFGMGRID_VALID))					return ERROR_GRID_UNINITIALIZED;
	if (!value)											return E_POINTER;

	GridData *gd = m_gridData(nullptr);
	switch (option) {
		case CWFGM_GRID_ATTRIBUTE_LATITUDE:				*value = m_worldLocation.m_latitude(); return S_OK;
		case CWFGM_GRID_ATTRIBUTE_LONGITUDE:			*value = m_worldLocation.m_longitude(); return S_OK;
		case CWFGM_GRID_ATTRIBUTE_XLLCORNER:				*value = gd->m_xllcorner; return S_OK;
		case CWFGM_GRID_ATTRIBUTE_YLLCORNER:				*value = gd->m_yllcorner; return S_OK;
		case CWFGM_GRID_ATTRIBUTE_XURCORNER:				*value = gd->m_xllcorner + gd->m_xsize * gd->m_resolution; return S_OK;
		case CWFGM_GRID_ATTRIBUTE_YURCORNER:				*value = gd->m_yllcorner + gd->m_ysize * gd->m_resolution; return S_OK;
		case CWFGM_GRID_ATTRIBUTE_PLOTRESOLUTION:			*value = gd->m_resolution; return S_OK;

		case CWFGM_GRID_ATTRIBUTE_DEFAULT_ELEVATION:		*value = m_defaultElevation; if (!(m_flags & CCWFGMGRID_DEFAULT_ELEV_SET)) return ERROR_SEVERITY_WARNING; return S_OK;
		case CWFGM_GRID_ATTRIBUTE_DEFAULT_ELEVATION_SET:	*value = (m_flags & CCWFGMGRID_DEFAULT_ELEV_SET) ? true : false; return S_OK;
		case CWFGM_GRID_ATTRIBUTE_DEFAULT_FMC_ACTIVE:		*value = (m_flags & CCWFGMGRID_SPECIFIED_FMC_ACTIVE) ? true : false; return S_OK;
		case CWFGM_GRID_ATTRIBUTE_DEFAULT_FMC:				*value = m_defaultFMC; return S_OK;

		case CWFGM_GRID_ATTRIBUTE_MIN_ELEVATION:			if (gd->m_elevationArray) { *value = (double)gd->m_minElev; return S_OK; } *value = false; return S_FALSE;
		case CWFGM_GRID_ATTRIBUTE_MAX_ELEVATION:			if (gd->m_elevationArray) { *value = (double)gd->m_maxElev; return S_OK; } *value = false; return S_FALSE;
		case CWFGM_GRID_ATTRIBUTE_MEDIAN_ELEVATION:			if (gd->m_elevationArray) { *value = (double)gd->m_medianElev; return S_OK; } *value = false; return S_FALSE;
		case CWFGM_GRID_ATTRIBUTE_MEAN_ELEVATION:			if (gd->m_elevationArray) { *value = (double)gd->m_meanElev; return S_OK; } *value = false; return S_FALSE;
		case CWFGM_GRID_ATTRIBUTE_MIN_SLOPE:				if (gd->m_slopeFactor) { *value = (double)gd->m_minSlopeFactor; return S_OK; } *value = false; return S_FALSE;
		case CWFGM_GRID_ATTRIBUTE_MAX_SLOPE:				if (gd->m_slopeFactor) { *value = (double)gd->m_maxSlopeFactor; return S_OK; } *value = false; return S_FALSE;
		case CWFGM_GRID_ATTRIBUTE_MIN_AZIMUTH:				if (gd->m_slopeFactor) { *value = (double)gd->m_minAzimuth; return S_OK; } *value = false; return S_FALSE;
		case CWFGM_GRID_ATTRIBUTE_MAX_AZIMUTH:				if (gd->m_slopeFactor) { *value = (double)gd->m_maxAzimuth; return S_OK; } *value = false; return S_FALSE;

		case CWFGM_GRID_ATTRIBUTE_FUELS_PRESENT:
			*value = true;
														return S_OK;
		case CWFGM_GRID_ATTRIBUTE_DEM_PRESENT:			*value = (gd->m_elevationArray) ? true : false; return S_OK;
		case CWFGM_GRID_ATTRIBUTE_DEM_NODATA_EXISTS:	*value = (m_flags & CCWFGMGRID_ELEV_NODATA_EXISTS) ? true : false; return S_OK;
		case CWFGM_ATTRIBUTE_LOAD_WARNING: {
														std::string warning(m_loadWarning);
														if (!m_sourceSRS)
															warning += "Grid: No valid projection data could be located and parsed in this project.\n";
														*value = warning;
														return S_OK;
													}
		case CWFGM_GRID_ATTRIBUTE_ASCII_GRIDFILE_HEADER: {
														*value = m_header;
														return S_OK;
													}
		case CWFGM_GRID_ATTRIBUTE_SPATIALREFERENCE: {
														CThreadSemaphoreEngage(&m_tokenlock, SEM_TRUE);
														if (!m_tokens.length()) {
															if (m_sourceSRS) {
																char* tokens = nullptr;
																CSemaphoreEngage lock(GDALClient::GDALClient::getGDALMutex(), SEM_TRUE);

																if (OSRExportToWkt(m_sourceSRS, &tokens) == OGRERR_NONE) {
																	m_tokens = tokens;
																	CPLFree(tokens);
																}
															}
														}
														*value = m_tokens;
														return S_OK;
													}
		case CWFGM_GRID_ATTRIBUTE_PROJECTION_UNITS: {
														*value = m_units;
														return S_OK;
													}
		case CWFGM_GRID_ATTRIBUTE_GIS_CANRESIZE:
													*value = m_flags & CCWFGMGRID_ALLOW_GIS ? true : false;
													return S_OK;

		case CWFGM_GRID_ATTRIBUTE_GIS_URL:			*value = m_gisGridURL; return S_OK;
		case CWFGM_GRID_ATTRIBUTE_GIS_LAYER:		*value = m_gisGridLayer; return S_OK;
		case CWFGM_GRID_ATTRIBUTE_GIS_UID:			*value = m_gisGridUID; return S_OK;
		case CWFGM_GRID_ATTRIBUTE_GIS_PWD:			*value = m_gisGridPWD; return S_OK;
		case CWFGM_GRID_ATTRIBUTE_INITIALSIZE:		*value = m_initialSize; return S_OK;
		case CWFGM_GRID_ATTRIBUTE_BUFFERSIZE:		*value = m_reactionSize; return S_OK;
		case CWFGM_GRID_ATTRIBUTE_GROWTHSIZE:		*value = m_growSize; return S_OK;
	}

	weak_assert(false);
	return E_INVALIDARG;
}


HRESULT CCWFGM_Grid::CreateGrid(std::uint16_t xsize, std::uint16_t ysize, const double xllcorner, const double yllcorner, const double resolution, std::uint8_t BasicFuel) {
	if ((xsize < 1))			return E_INVALIDARG;
	if ((ysize < 1))			return E_INVALIDARG;
	if (BasicFuel == (std::uint8_t)-1)				return E_INVALIDARG;
	if (resolution < 0.0)							return E_INVALIDARG;

	SEM_BOOL engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)									return ERROR_SCENARIO_SIMULATION_RUNNING;

	if (!m_fuelMap)									{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	GridData *gd = m_gridData(nullptr);
	if (gd->m_xsize != (std::uint16_t)-1)			{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	if (gd->m_fuelArray)							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

	long internal_index, export_index;
	ICWFGM_Fuel *fuel;
	if (FAILED(m_fuelMap->FuelAtIndex(BasicFuel, &internal_index, &export_index, &fuel)))
		return ERROR_FUELS_FUEL_UNKNOWN;

	std::int32_t total = xsize * ysize;

	try {
		gd->m_fuelArray = new std::uint8_t[total];
		gd->m_fuelValidArray = new bool[total];
	} catch(std::bad_alloc &cme) {
		return E_OUTOFMEMORY;
	}
	m_flags |= CCWFGMGRID_VALID;
	gd->m_xsize = xsize;
	gd->m_ysize = ysize;
	gd->m_xllcorner = xllcorner;
	gd->m_yllcorner = yllcorner;
	gd->m_resolution = resolution;
	memset(gd->m_fuelArray, BasicFuel, total);
	for (std::int32_t i = 0; i < total; i++)
		gd->m_fuelValidArray[i] = true;
	m_bRequiresSave = true;
	fixWorldLocation();

	if (m_worldLocation.InsideNewZealand()) {
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
	return S_OK;
}


HRESULT CCWFGM_Grid::CreateSlopeElevationGrid(std::int16_t elevation, std::uint16_t slope, std::uint16_t aspect) {
	SEM_BOOL engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)			return ERROR_SCENARIO_SIMULATION_RUNNING;

	GridData *gd = m_gridData(nullptr);
	if (gd->m_elevationArray)		return ERROR_GRID_INITIALIZED;
	if (gd->m_slopeFactor)		return ERROR_GRID_INITIALIZED;
	if (gd->m_slopeAzimuth)		return ERROR_GRID_INITIALIZED;
	if (gd->m_elevationValidArray)	return ERROR_GRID_INITIALIZED;

	std::int32_t total = gd->m_xsize * gd->m_ysize;

	try {
		gd->m_elevationArray		= new std::int16_t[total];
		gd->m_elevationValidArray	= new bool[total];
		gd->m_slopeFactor			= new std::uint16_t[total];
		gd->m_slopeAzimuth			= new std::uint16_t[total];
		gd->m_terrainValidArray		= new bool[total];
	} catch (std::bad_alloc &cme) {
		return E_OUTOFMEMORY;
	}

	std::fill_n(gd->m_elevationArray, total, elevation);
	std::fill_n(gd->m_elevationValidArray, total, true);
	std::fill_n(gd->m_slopeFactor, total, slope);
	std::fill_n(gd->m_slopeAzimuth, total, aspect);
	std::fill_n(gd->m_terrainValidArray, total, true);

	gd->m_maxElev = gd->m_minElev = elevation;
	gd->m_minSlopeFactor = gd->m_maxSlopeFactor = slope;
	gd->m_minAzimuth =gd-> m_maxAzimuth = aspect;

	m_bRequiresSave = true;
	return S_OK;
}


std::uint16_t GridData::convertX(double x, XY_Rectangle *bbox) {
	double lx = x - m_xllcorner;
	double cx = floor(lx / m_resolution);
	if (bbox) {
		bbox->m_min.x = cx * m_resolution + m_xllcorner;
		bbox->m_max.x = bbox->m_min.x + m_resolution;
	}
	return (std::uint16_t)cx;
}


std::uint16_t GridData::convertY(double y, XY_Rectangle *bbox) {
	double ly = y - m_yllcorner;
	double cy = floor(ly / m_resolution);
	if (bbox) {
		bbox->m_min.y = cy * m_resolution + m_yllcorner;
		bbox->m_max.y = bbox->m_min.y + m_resolution;
	}
	return (std::uint16_t)cy;
}
