/**
 * WISE_Grid_Module: CWFGM_PolyReplaceGridFilter.Serialize.h
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
#include "CWFGM_ReplaceGridFilter.h"
#include "angles.h"
#include "points.h"

#include <errno.h>
#include <stdio.h>

#ifdef DEBUG
#include <assert.h>
#endif


#ifndef DOXYGEN_IGNORE_CODE

CCWFGM_ReplaceGridFilter::CCWFGM_ReplaceGridFilter() : m_fromFuel(nullptr), m_toFuel(nullptr) {
	m_fromIndex = (std::uint8_t)-1;
	m_x1 = m_y1 = m_x2 = m_y2 = (std::uint16_t)-1;
	m_resolution = -1.0;
	m_xllcorner = m_yllcorner = -999999999.0;
	m_allCombustible = m_allNoData = false;

	m_bRequiresSave = false;
}


CCWFGM_ReplaceGridFilter::CCWFGM_ReplaceGridFilter(const CCWFGM_ReplaceGridFilter &toCopy) : m_fromFuel(toCopy.m_fromFuel), m_toFuel(toCopy.m_toFuel) {
	CRWThreadSemaphoreEngage engage(*(CRWThreadSemaphore *)&toCopy.m_lock, SEM_FALSE);

	m_fromIndex = toCopy.m_fromIndex;
	m_x1 = toCopy.m_x1;
	m_y1 = toCopy.m_y1;
	m_x2 = toCopy.m_x2;
	m_y2 = toCopy.m_y2;

	m_resolution = toCopy.m_resolution;
	m_xllcorner = toCopy.m_xllcorner;
	m_yllcorner = toCopy.m_yllcorner;

	m_allCombustible = toCopy.m_allCombustible;
	m_allNoData = toCopy.m_allNoData;

	m_bRequiresSave = false;
}


CCWFGM_ReplaceGridFilter::~CCWFGM_ReplaceGridFilter() {
}

#endif


HRESULT CCWFGM_ReplaceGridFilter::MT_Lock(Layer *layerThread, bool exclusive, std::uint16_t obtain) {
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine)	{ weak_assert(0); return ERROR_GRID_UNINITIALIZED; }

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

		hr = gridEngine->MT_Lock(layerThread, exclusive, obtain);

	}
	else {
		hr = gridEngine->MT_Lock(layerThread, exclusive, obtain);

		if (exclusive)	m_lock.Unlock();
		else		m_lock.Unlock(1000000LL);
	}
	return S_OK;
}


HRESULT CCWFGM_ReplaceGridFilter::GetFuelData(Layer *layerThread, const XY_Point &pt, const HSS_Time::WTime &time, ICWFGM_Fuel **fuel, bool *fuel_valid, XY_Rectangle *cache_bbox) {
	if (!fuel)								return E_POINTER;
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine)							{ weak_assert(0); return ERROR_GRID_UNINITIALIZED; }

	HRESULT hr;

	hr = gridEngine->GetFuelData(layerThread, pt, time, fuel, fuel_valid, cache_bbox);
	std::uint16_t x = convertX(pt.x, cache_bbox);
	std::uint16_t y = convertY(pt.y, cache_bbox);
	if (m_fromIndex == (std::uint8_t)-1) {
		if (SUCCEEDED(hr)) {
			if ((*fuel_valid) && (*fuel)) {						// NODATA areas stay as NODATA even through the filters
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
					if (((x >= m_x1) && (x <= m_x2) && (y >= m_y1) && (y <= m_y2)) ||
					    ((m_x1 == (std::uint16_t)(-1)) && (m_y1 == (std::uint16_t)(-1)) && (m_x2 == (std::uint16_t)(-1)) && (m_y2 == (std::uint16_t)(-1))))
						*fuel = m_toFuel.get();
				}
			} else if ((!(*fuel_valid)) && (m_allNoData)) {
				if (((x >= m_x1) && (x <= m_x2) && (y >= m_y1) && (y <= m_y2)) ||
					((m_x1 == (std::uint16_t)(-1)) && (m_y1 == (std::uint16_t)(-1)) && (m_x2 == (std::uint16_t)(-1)) && (m_y2 == (std::uint16_t)(-1)))) {
					*fuel = m_toFuel.get();
					*fuel_valid = true;
				}
			}
		} else if (hr == ERROR_FUELS_FUEL_UNKNOWN) {
			if ((!(*fuel_valid)) && (m_allNoData)) {
				if (((x >= m_x1) && (x <= m_x2) && (y >= m_y1) && (y <= m_y2)) ||
					((m_x1 == (std::uint16_t)(-1)) && (m_y1 == (std::uint16_t)(-1)) && (m_x2 == (std::uint16_t)(-1)) && (m_y2 == (std::uint16_t)(-1)))) {
					*fuel = m_toFuel.get();
					*fuel_valid = true;
					hr = S_OK;
				}
			}
		}
	} else {
		std::uint8_t fuel_index;
		bool f_valid;
		hr = gridEngine->GetFuelIndexData(layerThread, pt, time, &fuel_index, &f_valid, cache_bbox);
		if (SUCCEEDED(hr)) {
			if ((f_valid) && (fuel_index == m_fromIndex)) {
				if (((x >= m_x1) && (x <= m_x2) && (y >= m_y1) && (y <= m_y2)) ||
					((m_x1 == (std::uint16_t)(-1)) && (m_y1 == (std::uint16_t)(-1)) && (m_x2 == (std::uint16_t)(-1)) && (m_y2 == (std::uint16_t)(-1)))) {
					*fuel = m_toFuel.get();
					*fuel_valid = true;
				}
			}
		}
	}
	return hr;
}


HRESULT CCWFGM_ReplaceGridFilter::GetFuelDataArray(Layer *layerThread, const XY_Point &min_pt, const XY_Point &max_pt, double scale, const HSS_Time::WTime &time, ICWFGM_Fuel_2d *fuel, bool_2d *fuel_valid) {
	HRESULT hr;
	if (!fuel)						return E_POINTER;

	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine)							{ weak_assert(0); return ERROR_GRID_UNINITIALIZED; }

	bool complete_area = (m_x1 == (std::uint16_t)-1) && (m_y1 == (std::uint16_t)-1) && (m_x2 == (std::uint16_t)-1) && (m_y2 == (std::uint16_t)-1);
	
	std::uint16_t x_min = convertX(min_pt.x, nullptr), y_min = convertY(min_pt.y, nullptr);
	std::uint16_t x_max = convertX(max_pt.x, nullptr), y_max = convertY(max_pt.y, nullptr);
	hr = gridEngine->GetFuelDataArray(layerThread, min_pt, max_pt, scale, time, fuel, fuel_valid);
	if (m_fromIndex == (std::uint8_t)-1) {
		if (SUCCEEDED(hr)) {
			const ICWFGM_Fuel_2d::size_type *dims = fuel->shape();
			if (dims[0] < (x_max - x_min + 1))		return E_INVALIDARG;
			if (dims[1] < (y_max - y_min + 1))		return E_INVALIDARG;

			dims = fuel_valid->shape();
			if (dims[0] < (x_max - x_min + 1))		return E_INVALIDARG;
			if (dims[1] < (y_max - y_min + 1))		return E_INVALIDARG;

			std::uint16_t x, y;
			ICWFGM_Fuel *fff;
			bool bff;

			for (y = y_min; y <= y_max; y++)
			{
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
						if (((fff == fromFuel) || (!fromFuel && m_toFuel)))
							if (complete_area || ((x >= m_x1) && (x <= m_x2) && (y >= m_y1) && (y <= m_y2)))
								(*fuel)[x - x_min][y - y_min] = m_toFuel.get();
					}
					else {
						if (m_allNoData) {
							if (complete_area || ((x >= m_x1) && (x <= m_x2) && (y >= m_y1) && (y <= m_y2))) {
								(*fuel_valid)[x - x_min][y - y_min] = true;
								(*fuel)[x - x_min][y - y_min] = m_toFuel.get();
							}
						}
					}
				}
			}
		}
	} else {
		if (SUCCEEDED(hr)) {
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
							if (complete_area || ((x >= m_x1) && (x <= m_x2) && (y >= m_y1) && (y <= m_y2))) {
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


HRESULT CCWFGM_ReplaceGridFilter::Clone(boost::intrusive_ptr<ICWFGM_CommonBase> *newObject) const {
	if (!newObject)						return E_POINTER;

	CRWThreadSemaphoreEngage engage(*(CRWThreadSemaphore *)&m_lock, SEM_FALSE);

	try {
		CCWFGM_ReplaceGridFilter *f = new CCWFGM_ReplaceGridFilter(*this);
		*newObject = f;
		return S_OK;
	}
	catch (std::exception &e) {
	}
	return E_FAIL;
}


HRESULT CCWFGM_ReplaceGridFilter::SetRelationship(ICWFGM_Fuel *from_fuel, std::uint8_t from_index, ICWFGM_Fuel *to_fuel) {
	if (from_fuel == (ICWFGM_Fuel *)(-2)) {
		m_allCombustible = false;
		m_allNoData = true;
		from_fuel = nullptr;
	} else if (from_fuel == (ICWFGM_Fuel *)~0) {
		m_allCombustible = true;
		m_allNoData = false;
		from_fuel = nullptr;
	} else {
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
	m_fromIndex = from_index;
	m_fromFuel.reset(from_fuel);
	m_toFuel.reset(to_fuel);
	m_bRequiresSave = true;
	if ((m_x1 == (std::uint16_t)-1) && (m_y1 == (std::uint16_t)-1) && (m_x2 == (std::uint16_t)-1) && (m_y2 == (std::uint16_t)-1))
		return SUCCESS_GRID_TOTAL_AREA;
	return S_OK;
}


HRESULT CCWFGM_ReplaceGridFilter::GetRelationship(ICWFGM_Fuel **from_fuel, std::uint8_t *from_index, ICWFGM_Fuel **to_fuel) {
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
	if ((m_x1 == (std::uint16_t)-1) && (m_y1 == (std::uint16_t)-1) && (m_x2 == (std::uint16_t)-1) && (m_y2 == (std::uint16_t)-1))
		return SUCCESS_GRID_TOTAL_AREA;
	return S_OK;
}


HRESULT CCWFGM_ReplaceGridFilter::SetArea(const XY_Point &pt1,const XY_Point &pt2) {
	SEM_BOOL engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)								return ERROR_SCENARIO_SIMULATION_RUNNING;

	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(nullptr);
	if (!gridEngine)							{ weak_assert(0); return ERROR_GRID_UNINITIALIZED; }

	std::uint16_t x_size, y_size;
	HRESULT hr = gridEngine->GetDimensions(0, &x_size, &y_size);
	if (FAILED(hr))								return hr;

	if (pt1.x > pt2.x)								return E_INVALIDARG;
	if (pt1.y > pt2.y)								return E_INVALIDARG;
	if (!((pt1.x == -1.0) && (pt1.y == -1.0) && (pt2.x == -1.0) && (pt2.y == -1.0))) {
		if (pt2.x >= x_size)					return ERROR_GRID_LOCATION_OUT_OF_RANGE;
		if (pt2.y >= y_size)					return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	}
	if ((pt1.x == -1.0) && (pt1.y == -1.0) && (pt2.x == -1.0) && (pt2.y == -1.0)) {
		m_x1 = m_y1 = m_x2 = m_y2 = -1;
	} else {
		m_x1 = convertX(pt1.x, nullptr);
		m_y1 = convertY(pt1.y, nullptr);
		m_x2 = convertX(pt2.x, nullptr);
		m_y2 = convertY(pt2.y, nullptr);
	}
	m_bRequiresSave = true;
	if ((m_x1 == (std::uint16_t)-1) && (m_y1 == (std::uint16_t)-1) && (m_x2 == (std::uint16_t)-1) && (m_y2 == (std::uint16_t)-1))
		return SUCCESS_GRID_TOTAL_AREA;
	return S_OK;
}


HRESULT CCWFGM_ReplaceGridFilter::GetArea( XY_Point* pt1, XY_Point* pt2) {
	if ((!pt1) || (!pt2))					return E_POINTER;

	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);

	if ((m_x1 == (uint16_t)-1) && (m_y1 == (uint16_t)-1) && (m_x2 == (uint16_t)-1) && (m_y2 == (uint16_t)-1)) {
		pt1->x = -1.0;
		pt1->y = -1.0;
		pt2->x = -1.0;
		pt2->y = -1.0;
		return SUCCESS_GRID_TOTAL_AREA;
	}

	pt1->x = invertX(m_x1);
	pt1->y = invertY(m_y1);
	pt2->x = invertX(m_x2);
	pt2->y = invertY(m_y2);

	return S_OK;
}


HRESULT CCWFGM_ReplaceGridFilter::GetAttribute(Layer *layerThread, std::uint16_t option, PolymorphicAttribute *value) {
	if (!layerThread) {
		HRESULT hr = GetAttribute(option, value);
		if (SUCCEEDED(hr))
			return hr;
	}

	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine)							return ERROR_GRID_UNINITIALIZED;
	return gridEngine->GetAttribute(layerThread, option, value);
}


HRESULT CCWFGM_ReplaceGridFilter::GetAttribute(std::uint16_t option, PolymorphicAttribute *value) {
	if (!value)								return E_POINTER;

	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);

	switch (option) {
		case CWFGM_ATTRIBUTE_LOAD_WARNING: {
							*value = m_loadWarning;
							return S_OK;
						   }
	}
	return E_INVALIDARG;
}


/*! Polymorphic.  This routine sets an attribute/option value given the attribute/option index.  Currently does not
	perform any operation and is simply reserved for future functionality.
	\param option Reserved for future functionality.
	\param value Value for the attribute/option index
	\sa ICWFGM_ReplaceGridFilter::SetAttribute

	\retval E_NOTIMPL This function is reserved for future functionality.
*/
HRESULT CCWFGM_ReplaceGridFilter::SetAttribute(std::uint16_t option, const PolymorphicAttribute &value) {
	return E_NOTIMPL;
}


HRESULT CCWFGM_ReplaceGridFilter::PutGridEngine(Layer *layerThread, ICWFGM_GridEngine *newVal) {
	HRESULT hr = ICWFGM_GridEngine::PutGridEngine(layerThread, newVal);
	if (SUCCEEDED(hr) && m_gridEngine(nullptr)) {
		HRESULT hr = fixResolution();
		weak_assert(SUCCEEDED(hr));
	}
	return hr;
}


HRESULT CCWFGM_ReplaceGridFilter::fixResolution() {
	HRESULT hr;
	double gridResolution, gridXLL, gridYLL;
	PolymorphicAttribute var;

	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine;
	if (!(gridEngine = m_gridEngine(nullptr)))					{ weak_assert(0); return ERROR_GRID_UNINITIALIZED; }

	/*POLYMORPHIC CHECK*/
	try{
		if (FAILED(hr = gridEngine->GetAttribute(nullptr, CWFGM_GRID_ATTRIBUTE_PLOTRESOLUTION, &var))) return hr; VariantToDouble_(var, &gridResolution);
		if (FAILED(hr = gridEngine->GetAttribute(nullptr, CWFGM_GRID_ATTRIBUTE_XLLCORNER, &var))) return hr; VariantToDouble_(var, &gridXLL);
		if (FAILED(hr = gridEngine->GetAttribute(nullptr, CWFGM_GRID_ATTRIBUTE_YLLCORNER, &var))) return hr; VariantToDouble_(var, &gridYLL);
	}
	catch (std::bad_variant_access &) {
		weak_assert(0);
		return ERROR_GRID_UNINITIALIZED;
	}

	m_resolution = gridResolution;
	m_xllcorner = gridXLL;
	m_yllcorner = gridYLL;

	return S_OK;
}


std::uint16_t CCWFGM_ReplaceGridFilter::convertX(double x, XY_Rectangle* bbox) {
	double lx = x - m_xllcorner;
	double cx = floor(lx / m_resolution);
	if (bbox) {
		bbox->m_min.x = cx * m_resolution + m_xllcorner;
		bbox->m_max.x = bbox->m_min.x + m_resolution;
	}
	return (std::uint16_t)cx;
}


std::uint16_t CCWFGM_ReplaceGridFilter::convertY(double y, XY_Rectangle* bbox) {
	double ly = y - m_yllcorner;
	double cy = floor(ly / m_resolution);
	if (bbox) {
		bbox->m_min.y = cy * m_resolution + m_yllcorner;
		bbox->m_max.y = bbox->m_min.y + m_resolution;
	}
	return (std::uint16_t)cy;
}
