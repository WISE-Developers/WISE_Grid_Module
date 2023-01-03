/**
 * WISE_Grid_Module: CWFGM_PolyReplaceGridFilter.h
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
#include "GridCom_ext.h"
#include "FireEngine_ext.h"
#include "CWFGM_PolyReplaceGridFilter.h"
#include "angles.h"
#include "Thread.h"

#include <errno.h>
#include <stdio.h>
#include <omp.h>

#include "CoordinateConverter.h"

#ifdef DEBUG
#include <assert.h>
#endif



#ifndef DOXYGEN_IGNORE_CODE

CCWFGM_PolyReplaceGridFilter::CCWFGM_PolyReplaceGridFilter() : m_fromFuel(nullptr), m_toFuel(nullptr) {
	m_bRequiresSave = false;
	m_fromIndex = (std::uint8_t)-1;
	m_resolution = -1.0;
	m_xllcorner = m_yllcorner = -999999999.0;
	m_replaceArray = nullptr;
	m_allCombustible = m_allNoData = false;
	m_flags = 0;
}


CCWFGM_PolyReplaceGridFilter::CCWFGM_PolyReplaceGridFilter(const CCWFGM_PolyReplaceGridFilter &toCopy) :
	m_fromFuel(toCopy.m_fromFuel), m_toFuel(toCopy.m_toFuel), m_polySet(toCopy.m_polySet) {
	CRWThreadSemaphoreEngage engage(*(CRWThreadSemaphore *)&toCopy.m_lock, SEM_FALSE);

	m_fromIndex = toCopy.m_fromIndex;
	m_resolution = toCopy.m_resolution;
	m_xllcorner = toCopy.m_xllcorner;
	m_yllcorner = toCopy.m_yllcorner;

	m_gisURL = toCopy.m_gisURL;
	m_gisLayer = toCopy.m_gisLayer;
	m_gisUID = toCopy.m_gisUID;
	m_gisPWD = toCopy.m_gisPWD;

	m_replaceArray = nullptr;

	m_allCombustible = toCopy.m_allCombustible;
	m_allNoData = toCopy.m_allNoData;
	m_flags = toCopy.m_flags;

	m_bRequiresSave = false;
}


CCWFGM_PolyReplaceGridFilter::~CCWFGM_PolyReplaceGridFilter() {
	if (m_replaceArray)
		delete m_replaceArray;
}

#endif


HRESULT CCWFGM_PolyReplaceGridFilter::MT_Lock(Layer *layerThread, bool exclusive, std::uint16_t obtain) {
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine)	{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

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

		m_calcLock.Lock_Write();
		if (!m_replaceArray)
			calculateReplaceArray();
		m_calcLock.Unlock();

		hr = gridEngine->MT_Lock(layerThread, exclusive, obtain);
	}
	else {
		hr = gridEngine->MT_Lock(layerThread, exclusive, obtain);

		if (exclusive)	m_lock.Unlock();
		else		m_lock.Unlock(1000000LL);
	}
	return hr;
}


HRESULT CCWFGM_PolyReplaceGridFilter::GetFuelData(Layer *layerThread, const XY_Point &pt, const HSS_Time::WTime &time, ICWFGM_Fuel **fuel, bool *fuel_valid, XY_Rectangle *cache_bbox) {
	if (!fuel)								return E_POINTER;
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine)							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

	HRESULT hr;

	if (!m_replaceArray)
		calculateReplaceArray();

	hr = gridEngine->GetFuelData(layerThread, pt, time, fuel, fuel_valid, cache_bbox);
	std::uint16_t x = convertX(pt.x, cache_bbox);
	std::uint16_t y = convertY(pt.y, cache_bbox);
	if (m_fromIndex == (std::uint8_t)-1) {
		if (SUCCEEDED(hr)) {
			if ((*fuel_valid) && (*fuel)) {
				ICWFGM_Fuel *fromFuel = m_fromFuel.get();
				if (m_allCombustible) {				// special new rule - if fromIndex == -1 (which means check the fuel pointer) then
										// if the fromFuel == NULL then change all valid fuels
										// if the m_allCombustible == TRUE then change all non-nonfuel fuels (all combustible fuels)
					bool result;			// else if fromFuel == the fuel in that grid cell, then change it (only)
					(*fuel)->IsNonFuel(&result);
					if (!result)
						fromFuel = NULL;
					else
						fromFuel = (ICWFGM_Fuel *)~0;
				} else if (m_allNoData)
					fromFuel = (ICWFGM_Fuel *)~0;
				if ((!fromFuel && m_toFuel) || (*fuel == fromFuel)) {
					bool_2d_ref r = *m_replaceArray;
					if (r[x][y])
						*fuel = m_toFuel.get();
				}
			}
			else if ((!(*fuel_valid)) && (m_allNoData)) {
				const_bool_2d_ref r = *m_replaceArray;
				if (r[x][y]) {
					*fuel = m_toFuel.get();
					*fuel_valid = true;
				}
			}
		}
		else if (hr == ERROR_FUELS_FUEL_UNKNOWN) {
			if ((!(*fuel_valid)) && (m_allNoData)) {
				const_bool_2d_ref r = *m_replaceArray;
				if (r[x][y]) {
					*fuel = m_toFuel.get();
					*fuel_valid = true;
					hr = S_OK;
				}
			}
		}
	}
	else {
		std::uint8_t fuel_index;
		bool f_valid;
		hr = gridEngine->GetFuelIndexData(layerThread, pt, time, &fuel_index, &f_valid, cache_bbox);
		if (SUCCEEDED(hr)) {
			if ((f_valid) && ((std::uint8_t)fuel_index == m_fromIndex)) {
				bool_2d_ref r = *m_replaceArray;
				if (r[x][y]) {
					*fuel = m_toFuel.get();
					*fuel_valid = true;
				}
			}
		}
	}
	return hr;
}


HRESULT CCWFGM_PolyReplaceGridFilter::GetFuelDataArray(Layer *layerThread, const XY_Point &min_pt, const XY_Point &max_pt, double scale,
    const HSS_Time::WTime &time, ICWFGM_Fuel_2d *fuel, bool_2d *fuel_valid) {
	HRESULT hr;
	if (!fuel)										return E_POINTER;

	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine)							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

	if (!m_replaceArray)
		calculateReplaceArray();

	hr = gridEngine->GetFuelDataArray(layerThread, min_pt, max_pt, scale, time, fuel, fuel_valid);
	if (m_fromIndex == (std::uint8_t)(-1)) {
		if (SUCCEEDED(hr)) {
			std::uint16_t x_min = convertX(min_pt.x, nullptr), y_min = convertY(min_pt.y, nullptr);
			std::uint16_t x_max = convertX(max_pt.x, nullptr), y_max = convertY(max_pt.y, nullptr);

			const ICWFGM_Fuel_2d::size_type *dims = fuel->shape();
			if (dims[0] < (x_max - x_min + 1))		return E_INVALIDARG;
			if (dims[1] < (y_max - y_min + 1))		return E_INVALIDARG;

			dims = fuel_valid->shape();
			if (dims[0] < (x_max - x_min + 1))		return E_INVALIDARG;
			if (dims[1] < (y_max - y_min + 1))		return E_INVALIDARG;

			std::uint16_t x, y;
			ICWFGM_Fuel *fff;
			bool bff = false;
			for (y = y_min; y <= y_max; y++) {
				for (x = x_min; x <= x_max; x++) {
					bff = (*fuel_valid)[x - x_min][y - y_min];
					if ((bff) && (fff = (*fuel)[x - x_min][y - y_min])) {

						ICWFGM_Fuel *fromFuel = m_fromFuel.get();
						if (m_allCombustible) {				// special new rule - if fromIndex == -1 (which means check the fuel pointer) then
												// if the fromFuel == NULL then change all valid fuels
												// if the m_allCombustible == TRUE then change all non-nonfuel fuels (all combustible fuels)
							bool result;			// else if fromFuel == the fuel in that grid cell, then change it (only)
							fff->IsNonFuel(&result);
							if (!result)
								fromFuel = nullptr;
							else
								fromFuel = (ICWFGM_Fuel *)~0;
						}
						else if (m_allNoData)
							fromFuel = (ICWFGM_Fuel *)~0;
						if ((fff == fromFuel) || (!fromFuel && m_toFuel)) {
							bool_2d_ref r = *m_replaceArray;
							if (r[x][y])
								(*fuel)[x - x_min][y - y_min] = m_toFuel.get();
						}
					}
					else {
						if ((!bff) && (m_allNoData)) {
							bool_2d_ref r = *m_replaceArray;
							if (r[x][y]) {
								(*fuel_valid)[x - x_min][y - y_min] = true;
								(*fuel)[x - x_min][y - y_min] = m_toFuel.get();
							}
						}
					}
				}
			}
		}
	}
	else {
		if (SUCCEEDED(hr)) {
			std::uint16_t x_min = convertX(min_pt.x, nullptr), y_min = convertY(min_pt.y, nullptr);
			std::uint16_t x_max = convertX(max_pt.x, nullptr), y_max = convertY(max_pt.y, nullptr);

			const ICWFGM_Fuel_2d::size_type *dims = fuel->shape();
			if (dims[0] < (x_max - x_min + 1))		return E_INVALIDARG;
			if (dims[1] < (y_max - y_min + 1))		return E_INVALIDARG;

			dims = fuel_valid->shape();
			if (dims[0] < (x_max - x_min + 1))		return E_INVALIDARG;
			if (dims[1] < (y_max - y_min + 1))		return E_INVALIDARG;

			const ICWFGM_Fuel_2d::index *bases = fuel->index_bases();
			uint8_t_2d bb(boost::extents[dims[0]][dims[1]]);
			bool_2d bv(boost::extents[dims[0]][dims[1]]);
			boost::array<uint8_t_2d::index, 2> bbases = { bases[0], bases[1] };
			boost::array<bool_2d::index, 2> vbases = { bases[0], bases[1] };
			bb.reindex(bbases);
			bv.reindex(vbases);

			hr = gridEngine->GetFuelIndexDataArray(layerThread, min_pt, max_pt, scale, time, &bb, &bv);

			if (SUCCEEDED(hr)) {
				std::uint16_t x, y;
				for (y = y_min; y <= y_max; y++)			// for every point that was requested...
				{
					for (x = x_min; x <= x_max; x++)
						if ((bv[x - x_min][y - y_min]) && (bb[x - x_min][y - y_min] == m_fromIndex)) {
							bool_2d_ref r = *m_replaceArray;
							if (r[x][y]) {
								(*fuel_valid)[x - x_min][y - y_min] = true;
								(*fuel)[x - x_min][y - y_min] = m_toFuel.get();
							}
						}
				}
			}
		}
	}
	return hr;
}


HRESULT CCWFGM_PolyReplaceGridFilter::Clone(boost::intrusive_ptr<ICWFGM_CommonBase> *newObject) const {
	if (!newObject)						return E_POINTER;

	CRWThreadSemaphoreEngage engage(*(CRWThreadSemaphore *)&m_lock, SEM_FALSE);

	try {
		CCWFGM_PolyReplaceGridFilter *f = new CCWFGM_PolyReplaceGridFilter(*this);
		*newObject = f;
		return S_OK;
	}
	catch (std::exception &e) {
	}
	return E_FAIL;
}


HRESULT CCWFGM_PolyReplaceGridFilter::SetRelationship(ICWFGM_Fuel *from_fuel, std::uint8_t from_index, ICWFGM_Fuel *to_fuel) {
	if (from_fuel == (ICWFGM_Fuel *)(-2)) {
		m_allCombustible = false;
		m_allNoData = true;
		from_fuel = nullptr;
	}
	else if (from_fuel == (ICWFGM_Fuel *)~0) {
		m_allCombustible = true;
		m_allNoData = false;
		from_fuel = nullptr;
	}
	else {
		m_allCombustible = false;
		m_allNoData = false;
	}
	if ((from_index == (std::uint8_t)-1) && (!from_fuel) && (!to_fuel))		return E_INVALIDARG;

	SEM_BOOL engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)								return ERROR_SCENARIO_SIMULATION_RUNNING;

	ICWFGM_Fuel *fuelPtr;
	if (from_fuel) {
		fuelPtr = dynamic_cast<ICWFGM_Fuel *>(from_fuel);
		if (!fuelPtr)
			return E_NOINTERFACE;
		fuelPtr = NULL;
	}
	if (to_fuel) {
		fuelPtr = dynamic_cast<ICWFGM_Fuel *>(to_fuel);
		if (!fuelPtr)
			return E_NOINTERFACE;
	}
	m_fromIndex = (std::uint8_t)from_index;
	m_fromFuel.reset(from_fuel);
	m_toFuel.reset(to_fuel);
	m_bRequiresSave = true;
	return S_OK;
}


HRESULT CCWFGM_PolyReplaceGridFilter::GetRelationship(ICWFGM_Fuel **from_fuel, std::uint8_t *from_index, ICWFGM_Fuel **to_fuel) {
	if (!from_fuel)								return E_POINTER;
	if (!from_index)							return E_POINTER;
	if (!to_fuel)								return E_POINTER;

	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);
	if (m_allNoData) {
		*from_fuel = (ICWFGM_Fuel *)(-2);
		*from_index = (std::uint8_t)-1;
	} else if (m_allCombustible) {
		*from_fuel = (ICWFGM_Fuel *)~0;
		*from_index = (std::uint8_t)-1;
	} else {
		*from_fuel = m_fromFuel.get();
		*from_index = m_fromIndex;
	}
	*to_fuel = m_toFuel.get();
	return S_OK;
}


HRESULT CCWFGM_PolyReplaceGridFilter::GetAttribute(Layer *layerThread, std::uint16_t option, PolymorphicAttribute *value) {
	if (!layerThread) {
		HRESULT hr = GetAttribute(option, value);
		if (SUCCEEDED(hr))
			return hr;
	}

	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine)							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	return gridEngine->GetAttribute(layerThread, option, value);
}


HRESULT CCWFGM_PolyReplaceGridFilter::GetAttribute(std::uint16_t option, PolymorphicAttribute *value) {
	if (!value)								return E_POINTER;

	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);

	switch (option) {
	case CWFGM_ATTRIBUTE_LOAD_WARNING: {
		*value = m_loadWarning;
		return S_OK;
	}
	case CWFGM_GRID_ATTRIBUTE_GIS_CANRESIZE:
		*value = (m_flags & CCWFGMGRID_ALLOW_GIS) ? true : false;
		return S_OK;

	case CWFGM_GRID_ATTRIBUTE_GIS_URL: {
		*value = m_gisURL;
		return S_OK;
	}
	case CWFGM_GRID_ATTRIBUTE_GIS_LAYER: {
		*value = m_gisLayer;
		return S_OK;
	}
	case CWFGM_GRID_ATTRIBUTE_GIS_UID: {
		*value = m_gisUID;
		return S_OK;
	}
	case CWFGM_GRID_ATTRIBUTE_GIS_PWD: {
		*value = m_gisPWD;
		return S_OK;
	}
	}
	return E_INVALIDARG;
}


HRESULT CCWFGM_PolyReplaceGridFilter::AddPolygon(const XY_PolyConst &xy_pairs, std::uint32_t *index) {
	if (!index)								return E_POINTER;

#ifdef _DEBUG
	std::uint32_t point_count = xy_pairs.NumPoints();
#endif

	SEM_BOOL engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)										return ERROR_SCENARIO_SIMULATION_RUNNING;

	XY_PolyLL *poly = (XY_PolyLL *)m_polySet.NewCopy(xy_pairs);
	if (!poly)										return E_OUTOFMEMORY;

	poly->m_publicFlags |= XY_PolyLL::Flags::INTERPRET_POLYGON;
	poly->CleanPoly(0.0, XY_PolyLL::Flags::INTERPRET_POLYGON);
	if (poly->NumPoints() > 2) {
		m_polySet.AddPoly(poly);
		*index = m_polySet.NumPolys() - 1;
	}
	else {
		m_polySet.Delete(poly);
		return E_FAIL;
	}
	clearReplaceArray();
	m_bRequiresSave = true;

	return S_OK;
}


HRESULT CCWFGM_PolyReplaceGridFilter::ClearPolygon(std::uint32_t index) {
	SEM_BOOL engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)										return ERROR_SCENARIO_SIMULATION_RUNNING;

	if (index == (std::uint32_t)-1) {
		XY_PolyLL *poly;
		while (poly = m_polySet.RemHead())
			m_polySet.Delete(poly);
	}
	else {
		if (index >= m_polySet.NumPolys())							return ERROR_FIREBREAK_NOT_FOUND;

		XY_PolyLL *pn = (XY_PolyLL *)m_polySet.GetPoly(index);
		m_polySet.RemovePoly(pn);
		m_polySet.Delete(pn);
	}
	clearReplaceArray();
	m_bRequiresSave = true;
	return S_OK;
}


HRESULT CCWFGM_PolyReplaceGridFilter::GetPolygonRange(std::uint32_t index, XY_Point* min_pt, XY_Point* max_pt) {
	if ((!min_pt) || (!max_pt))					return E_POINTER;

	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);

	if (index >= m_polySet.NumPolys())							return ERROR_FIREBREAK_NOT_FOUND;

	XY_PolyLL *pn = (XY_PolyLL *)m_polySet.GetPoly(index);

	XY_Rectangle bbox;
	if (pn->BoundingBox(bbox)) {
		min_pt->x = bbox.m_min.x;
		min_pt->y = bbox.m_min.y;
		max_pt->x = bbox.m_max.x;
		max_pt->y = bbox.m_max.y;
		return S_OK;
	}
	return ERROR_NO_DATA | ERROR_SEVERITY_WARNING;
}


HRESULT CCWFGM_PolyReplaceGridFilter::GetPolygon(std::uint32_t index, XY_Poly *xy_pairs) {
	if (!xy_pairs)						return E_POINTER;

	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);

	if (index >= m_polySet.NumPolys())							return ERROR_FIREBREAK_NOT_FOUND;

	XY_PolyLL *pn = (XY_PolyLL *)m_polySet.GetPoly(index);
	
	XY_PolyNode *n;
	xy_pairs->SetNumPoints(pn->NumPoints());

	n = pn->LH_Head();
	std::uint32_t cnt = 0;
	while (n->LN_Succ()) {
		xy_pairs->SetPoint(cnt++, *n);
		n = n->LN_Succ();
	}
	return S_OK;
}


HRESULT CCWFGM_PolyReplaceGridFilter::GetPolygonCount(std::uint32_t *count) {
	if (!count)										return E_POINTER;

	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);

	*count = m_polySet.NumPolys();
	return S_OK;
}


HRESULT CCWFGM_PolyReplaceGridFilter::GetPolygonSize(std::uint32_t index, std::uint32_t *size) {
	if (!size)										return E_POINTER;
	*size = 0;

	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);

	if (index == (std::uint32_t)-1) {
		XY_PolyLL *pn = (XY_PolyLL *)m_polySet.LH_Head();
		while (pn->LN_Succ()) {
			if (pn->NumPoints() > *size)
				*size = pn->NumPoints();
			pn = pn->LN_Succ();
		}
	}
	else {
		if (index >= m_polySet.NumPolys())						return ERROR_FIREBREAK_NOT_FOUND;

		XY_PolyLL *pn = (XY_PolyLL *)m_polySet.GetPoly(index);
		weak_assert(pn);
		if (pn)
			*size = pn->NumPoints();
	}
	return S_OK;
}


HRESULT CCWFGM_PolyReplaceGridFilter::GetArea(double *area) {
	if (!area)										return E_POINTER;

	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);

	*area = m_polySet.Area();
	return S_OK;
}


#ifndef DOXYGEN_IGNORE_CODE

void CCWFGM_PolyReplaceGridFilter::calculateReplaceArray() {
	std::uint16_t x_dim, y_dim;

	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(nullptr);
	if (!gridEngine)							return;
	if (FAILED(fixResolution()))				return;
	if (FAILED(gridEngine->GetDimensions(0, &x_dim, &y_dim)))	return;

	if (m_replaceArray) {
		weak_assert(false);
		delete m_replaceArray;
	}
	m_replaceArray = new bool_2d(boost::extents[x_dim][y_dim]);

	m_polySet.SetCacheScale(m_resolution);
	m_polySet.RescanRanges(false, false);
	bool_2d_ref r = *m_replaceArray;
	
	int thread_id = -1;
#pragma omp parallel for num_threads(CWorkerThreadPool::NumberIdealProcessors()) firstprivate(thread_id)
	for (std::int32_t xy = 0; xy < ((std::int32_t)x_dim*(std::int32_t)y_dim); ++xy) {
		if (thread_id == -1) {
			thread_id = omp_get_thread_num();
			CWorkerThread::native_handle_type thread = CWorkerThreadPool::GetCurrentThread();
			CWorkerThreadPool::SetThreadAffinityToMask(thread, thread_id);
		}
		std::uint16_t x = xy / y_dim;
		std::uint16_t y = xy % y_dim;
			XY_Point test(invertX(((double)x) + 0.5), invertY(((double)y) + 0.5));
			if (m_polySet.PointInArea(test))
				r[x][y] = true;
			else
				r[x][y] = false;
		}
}


void CCWFGM_PolyReplaceGridFilter::clearReplaceArray() {
	if (m_replaceArray) {
		delete m_replaceArray;
		m_replaceArray = NULL;
	}
}

#endif


/*! Polymorphic.  This routine sets an attribute/option value given the attribute/option index.  Currently does not
	perform any operation and is simply reserved for future functionality.
	\param option Reserved for future functionality.
	\param value Value for the attribute/option index
	\sa ICWFGM_PolyReplaceGridFilter::SetAttribute

	\retval E_NOTIMPL This function is reserved for future functionality.
*/
HRESULT CCWFGM_PolyReplaceGridFilter::SetAttribute(std::uint16_t option, const PolymorphicAttribute &var) {
	std::uint32_t *cnt, old;
	bool bval;
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(nullptr, &cnt);
	if (!gridEngine)							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

	switch (option) {
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
									m_gisURL = str;
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
									m_gisLayer = str;
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
								m_gisUID = str;
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
								m_gisPWD = str;
								m_bRequiresSave = true;
								}
								return S_OK;
	}

	return E_INVALIDARG;
}


HRESULT CCWFGM_PolyReplaceGridFilter::PutGridEngine(Layer *layerThread, ICWFGM_GridEngine *newVal) {
	HRESULT hr = ICWFGM_GridEngine::PutGridEngine(layerThread, newVal);
	if (SUCCEEDED(hr) && m_gridEngine(nullptr)) {
		auto hr = fixResolution();
		weak_assert(SUCCEEDED(hr));
	}
	return hr;
}


HRESULT CCWFGM_PolyReplaceGridFilter::fixResolution() {
	HRESULT hr;
	double gridResolution, gridXLL, gridYLL;
	PolymorphicAttribute var;

	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine;
	if (!(gridEngine = m_gridEngine(nullptr)))					{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

	/*POLYMORPHIC CHECK*/
	try {
		if (FAILED(hr = gridEngine->GetAttribute(nullptr, CWFGM_GRID_ATTRIBUTE_PLOTRESOLUTION, &var))) return hr; gridResolution = std::get<double>(var);
		if (FAILED(hr = gridEngine->GetAttribute(nullptr, CWFGM_GRID_ATTRIBUTE_XLLCORNER, &var))) return hr; gridXLL = std::get<double>(var);
		if (FAILED(hr = gridEngine->GetAttribute(nullptr, CWFGM_GRID_ATTRIBUTE_YLLCORNER, &var))) return hr; gridYLL = std::get<double>(var);
	} catch (std::bad_variant_access &) {
		weak_assert(false);
		return ERROR_GRID_UNINITIALIZED;
	}

	m_resolution = gridResolution;
	m_xllcorner = gridXLL;
	m_yllcorner = gridYLL;

	return S_OK;
}


std::uint16_t CCWFGM_PolyReplaceGridFilter::convertX(double x, XY_Rectangle* bbox) {
	double lx = x - m_xllcorner;
	double cx = floor(lx / m_resolution);
	if (bbox) {
		bbox->m_min.x = cx * m_resolution + m_xllcorner;
		bbox->m_max.x = bbox->m_min.x + m_resolution;
	}
	return (std::uint16_t)cx;
}


std::uint16_t CCWFGM_PolyReplaceGridFilter::convertY(double y, XY_Rectangle* bbox) {
	double ly = y - m_yllcorner;
	double cy = floor(ly / m_resolution);
	if (bbox) {
		bbox->m_min.y = cy * m_resolution + m_yllcorner;
		bbox->m_max.y = bbox->m_min.y + m_resolution;
	}
	return (std::uint16_t)cy;
}
