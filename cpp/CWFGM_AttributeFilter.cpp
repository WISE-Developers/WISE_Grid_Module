/**
 * WISE_Grid_Module: CWFGM_AttributeFilter.cpp
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
#include "CWFGM_AttributeFilter.h"
#include "lines.h"
#include "GridCom_ext.h"
#include "FireEngine_ext.h"
#include <errno.h>
#include <stdio.h>
#include <cpl_string.h>
#include "CoordinateConverter.h"

/////////////////////////////////////////////////////////////////////////////
// CWFGM_AttributeFilter


#ifndef DOXYGEN_IGNORE_CODE

CCWFGM_AttributeFilter::CCWFGM_AttributeFilter() {
	m_xsize = m_ysize = (std::uint16_t)-1;
	m_iresolution = m_resolution = -1.0;
	m_xllcorner = m_yllcorner = -999999999.0;
	m_flags = 0;
	m_optionKey = (std::uint16_t)-1;
	m_optionType = VT_EMPTY;
	m_array_i1 = nullptr;
	m_array_nodata = nullptr;
	m_bRequiresSave = false;
}


CCWFGM_AttributeFilter::CCWFGM_AttributeFilter(const CCWFGM_AttributeFilter &toCopy) {
	CRWThreadSemaphoreEngage engage(*(CRWThreadSemaphore *)&toCopy.m_lock, SEM_FALSE);

	m_xsize = toCopy.m_xsize;
	m_ysize = toCopy.m_ysize;
	m_resolution = toCopy.m_resolution;
	m_iresolution = toCopy.m_iresolution;
	m_xllcorner = toCopy.m_xllcorner;
	m_yllcorner = toCopy.m_yllcorner;
	m_flags = toCopy.m_flags;
	m_optionKey = toCopy.m_optionKey;
	m_optionType = toCopy.m_optionType;

	m_gisURL = toCopy.m_gisURL;
	m_gisLayer = toCopy.m_gisLayer;
	m_gisUID = toCopy.m_gisUID;
	m_gisPWD = toCopy.m_gisPWD;

	std::uint16_t size = 0;
	if (toCopy.m_array_i1) {
		switch (m_optionType) {
		case VT_BOOL:
		case VT_I1:
		case VT_UI1:	size = 1; break;
		case VT_I2:
		case VT_UI2:	size = 2; break;
		case VT_I4:
		case VT_UI4:
		case VT_R4:		size = 4; break;
		case VT_I8:
		case VT_UI8:
		case VT_R8:		size = 8; break;
		}
	}
	if (size) {
		m_array_i1 = (int8_t *)malloc((size_t)m_xsize * (size_t)m_ysize * (size_t)size);
		if (m_array_i1) {
			memcpy(m_array_i1, toCopy.m_array_i1, (size_t)m_xsize * (size_t)m_ysize * (size_t)size);
			if (toCopy.m_array_nodata) {
				m_array_nodata = (bool*)malloc((size_t)m_xsize * (size_t)m_ysize * sizeof(bool));
				if (m_array_nodata)
					memcpy(m_array_nodata, toCopy.m_array_nodata, (size_t)m_xsize * (size_t)m_ysize * sizeof(bool));
			} else
				m_array_nodata = nullptr;
		}
	}
	else {
		m_array_i1 = nullptr;
		m_array_nodata = nullptr;
	}
	m_bRequiresSave = false;
}


CCWFGM_AttributeFilter::~CCWFGM_AttributeFilter() {
	if (m_array_i1)
		free(m_array_i1);
	if (m_array_nodata)
		free(m_array_nodata);
}

#endif


HRESULT CCWFGM_AttributeFilter::get_FuelMap(CCWFGM_FuelMap **pVal) {
	if (!pVal)
		return E_POINTER;

	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);

	*pVal = m_fuelMap.get();
	if (!m_fuelMap) {
		weak_assert(false);
		return ERROR_GRID_UNINITIALIZED;
	}
	return S_OK;
}


HRESULT CCWFGM_AttributeFilter::put_FuelMap(CCWFGM_FuelMap *newVal) {
	if (!newVal)
		return E_POINTER;

	SEM_BOOL engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)
		return ERROR_SCENARIO_SIMULATION_RUNNING;

	if (m_fuelMap)
		return ERROR_GRID_INITIALIZED;
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


HRESULT CCWFGM_AttributeFilter::SetAttributePoint(const XY_Point &pt, const NumericVariant &value) {
	SEM_BOOL engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)								return ERROR_SCENARIO_SIMULATION_RUNNING;

	std::uint16_t x = convertX(pt.x, nullptr);
	std::uint16_t y = convertY(pt.y, nullptr);
	if (!m_gridEngine(nullptr))					{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	if (x >= m_xsize)							return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	if (y >= m_ysize)							return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	if (!m_array_i1)							return ERROR_SEVERITY_WARNING;

	HRESULT hr = setPoint(x, y, value);
	if (SUCCEEDED(hr))
		m_bRequiresSave = true;
	return hr;
}


#ifndef DOXYGEN_IGNORE_CODE

HRESULT CCWFGM_AttributeFilter::setPoint(const XY_Point &pt, const NumericVariant &value) {
	std::uint16_t x = convertX(pt.x, nullptr);
	std::uint16_t y = convertY(pt.y, nullptr);
	return setPoint(x, y, value);
}


HRESULT CCWFGM_AttributeFilter::setPoint(const std::uint16_t x, const std::uint16_t y, const NumericVariant &value) {
	std::uint32_t index = arrayIndex(x, y);
	return setPoint(index, value);
}


HRESULT CCWFGM_AttributeFilter::setPoint(const std::uint32_t index, const NumericVariant &value) {
	bool hr;
	switch (m_optionType) {
		case VT_I1:		hr = variantToInt8(value, &m_array_i1[index]); break;
		case VT_I2:		hr = variantToInt16(value, &m_array_i2[index]); break;
		case VT_I4:		hr = variantToInt32(value, &m_array_i4[index]); break;
		case VT_I8:		hr = variantToInt64(value, &m_array_i8[index]); break;
		case VT_UI1:	hr = variantToUInt8(value, &m_array_ui1[index]); break;
		case VT_UI2:	hr = variantToUInt16(value, &m_array_ui2[index]); break;
		case VT_UI4:	hr = variantToUInt32(value, &m_array_ui4[index]); break;
		case VT_UI8:	hr = variantToUInt64(value, &m_array_ui8[index]); break;
		case VT_R4:		hr = variantToFloat(value, &m_array_r4[index]); break;
		case VT_R8:		hr = variantToDouble(value, &m_array_r8[index]); break;
		case VT_BOOL: { bool b; hr = variantToBoolean(value, &b); if (hr) m_array_i1[index] = b ? 1 : 0; } break;
		default:		return E_UNEXPECTED;
	}
	if (m_array_nodata)
		m_array_nodata[index] = false;
	return hr ? S_OK : S_FALSE;
}


HRESULT CCWFGM_AttributeFilter::getPoint(const XY_Point &pt, NumericVariant *value, grid::AttributeValue *value_valid) {
	std::uint16_t x = convertX(pt.x, nullptr);
	std::uint16_t y = convertY(pt.y, nullptr);
	return getPoint(x, y, value, value_valid);
}


HRESULT CCWFGM_AttributeFilter::getPoint(const std::uint16_t x, const std::uint16_t y, NumericVariant *value, grid::AttributeValue *value_valid) {
	std::uint32_t index = arrayIndex(x, y);
	return getPoint(index, value, value_valid);
}


HRESULT CCWFGM_AttributeFilter::getPoint(const std::uint32_t index, NumericVariant *value, grid::AttributeValue *value_valid) {
	if (m_array_nodata) {
		if (m_array_nodata[index]) {
			NumericVariant va;
			*value = va;
			*value_valid = grid::AttributeValue::NOT_SET;
			return ERROR_GRID_NO_DATA;
		}
	}
	if (m_array_i1) {
		switch (m_optionType) {
			case VT_I1:	*value_valid = grid::AttributeValue::SET; *value = m_array_i1[index]; return S_OK;
			case VT_I2:	*value_valid = grid::AttributeValue::SET; *value = m_array_i2[index]; return S_OK;
			case VT_I4:	*value_valid = grid::AttributeValue::SET; *value = m_array_i4[index]; return S_OK;
			case VT_I8:	*value_valid = grid::AttributeValue::SET; *value = m_array_i8[index]; return S_OK;
			case VT_UI1:*value_valid = grid::AttributeValue::SET; *value = m_array_ui1[index]; return S_OK;
			case VT_UI2:*value_valid = grid::AttributeValue::SET; *value = m_array_ui2[index]; return S_OK;
			case VT_UI4:*value_valid = grid::AttributeValue::SET; *value = m_array_ui4[index]; return S_OK;
			case VT_UI8:*value_valid = grid::AttributeValue::SET; *value = m_array_ui8[index]; return S_OK;
			case VT_R4:	*value_valid = grid::AttributeValue::SET; *value = m_array_r4[index]; return S_OK;
			case VT_R8:	*value_valid = grid::AttributeValue::SET; *value = m_array_r8[index]; return S_OK;
			case VT_BOOL:*value_valid = grid::AttributeValue::SET;*value = m_array_i1[index] ? true : false; return S_OK;
			default:	return E_UNEXPECTED;
		}
	}
	return ERROR_GRID_NO_DATA;
}


struct br_callback {
	CCWFGM_AttributeFilter *_this;
	NumericVariant value;
	XY_Point prev_pt;
};


bool __cdecl break_fcn(APTR parameter, const XY_Point *loc) {
	struct br_callback *br = (struct br_callback *)parameter;
	
	XY_Point pt;
	pt.x = loc->x;
	pt.y = loc->y;
	if (FAILED(br->_this->setPoint(pt, br->value)))
		return false;
	if (((int)loc->x != (int)br->prev_pt.x) && ((int)loc->y != (int)br->prev_pt.y)) {
		pt.y = br->prev_pt.y;
		if (FAILED(br->_this->setPoint(pt, br->value)))
			return false;
	}
	br->prev_pt = *loc;
	return true;
}

#endif


HRESULT CCWFGM_AttributeFilter::SetAttributeLine(XY_Point pt1,XY_Point pt2, const NumericVariant &value) {
	SEM_BOOL engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)								return ERROR_SCENARIO_SIMULATION_RUNNING;

	std::uint16_t x1 = convertX(pt1.x, nullptr); std::uint16_t y1 = convertY(pt1.y, nullptr);
	std::uint16_t x2 = convertX(pt2.x, nullptr); std::uint16_t y2 = convertY(pt2.y, nullptr);
	if (!m_gridEngine(nullptr))					{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	if (x1 >= m_xsize)							return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	if (y1 >= m_ysize)							return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	if (x2 >= m_xsize)							return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	if (y2 >= m_ysize)							return ERROR_GRID_LOCATION_OUT_OF_RANGE;

	XY_Line l(pt1.x, pt1.y, pt2.x, pt2.y);
	struct br_callback br;
	br._this = this;
	br.value = value;
	br.prev_pt = l.p1;
	if (!l.Draw(1.0, break_fcn, &br))
		return E_FAIL;
	m_bRequiresSave = true;
	return S_OK;
}


HRESULT CCWFGM_AttributeFilter::GetAttributePoint(const XY_Point &pt, NumericVariant *value, grid::AttributeValue *value_valid) {
	if (!value)								return E_POINTER;

	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);

	std::uint16_t x = convertX(pt.x, nullptr);
	std::uint16_t y = convertY(pt.y, nullptr);
	if (!m_gridEngine(nullptr))					{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	if (x >= m_xsize)							return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	if (y >= m_ysize)							return ERROR_GRID_LOCATION_OUT_OF_RANGE;
	if (!m_array_i1)							return ERROR_SEVERITY_WARNING;

	return getPoint(pt, value, value_valid);
}


HRESULT CCWFGM_AttributeFilter::GetAttribute(std::uint16_t option,  /*unsigned*/ PolymorphicAttribute *value) {
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


HRESULT CCWFGM_AttributeFilter::ResetAttribute(const NumericVariant &value) {
	SEM_BOOL engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)								return ERROR_SCENARIO_SIMULATION_RUNNING;

	bool hr;
	boost::intrusive_ptr<ICWFGM_GridEngine> ge;
	if (!(ge = m_gridEngine(nullptr)))			{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

	std::uint32_t size;
	union {
		std::int8_t vt_c;
		std::uint8_t vt_b;
		std::int16_t vt_s;
		std::uint16_t vt_us;
		std::int32_t vt_l;
		std::uint32_t vt_ul;
		std::int64_t vt_ll;
		std::uint64_t vt_ull;
		float vt_f;
		double vt_d;
	} u;

	switch (m_optionType) {
		case VT_BOOL:	{ bool s; if (!(hr = variantToBoolean(value, &s))) return E_FAIL; u.vt_c = s ? 1 : 0; size = 1; break; }
		case VT_I1:		if (!(hr = variantToInt8(value, &u.vt_c))) return E_FAIL; size = 1; break;
		case VT_UI1:	if (!(hr = variantToUInt8(value, &u.vt_b))) return E_FAIL; size = 1; break;
		case VT_I2:		if (!(hr = variantToInt16(value, &u.vt_s))) return E_FAIL; size = 2; break;
		case VT_UI2:	if (!(hr = variantToUInt16(value, &u.vt_us))) return E_FAIL; size = 2; break;
		case VT_I4:		if (!(hr = variantToInt32(value, &u.vt_l))) return E_FAIL; size = 4; break;
		case VT_UI4:	if (!(hr = variantToUInt32(value, &u.vt_ul))) return E_FAIL; size = 4; break;
		case VT_R4:		if (!(hr = variantToFloat(value, &u.vt_f))) return E_FAIL; size = 4; break;
		case VT_I8:		if (!(hr = variantToInt64(value, &u.vt_ll))) return E_FAIL; size = 8; break;
		case VT_UI8:	if (!(hr = variantToUInt64(value, &u.vt_ull))) return E_FAIL; size = 8; break;
		case VT_R8:		if (!(hr = variantToDouble(value, &u.vt_d))) return E_FAIL; size = 8; break;
		default:		return E_UNEXPECTED;
	}

	std::uint16_t x, y;
	HRESULT hr1;
	if (FAILED(hr1 = ge->GetDimensions(0, &x, &y)))				return hr1;

	APTR mem = malloc((size_t)x * (size_t)y * (size_t)size);
	bool *mem2 = (bool *)malloc((size_t)x * (size_t)y * sizeof(bool));
	if ((!mem) || (!mem2)) {
		if (mem) free(mem);
		if (mem2) free(mem2);
		return E_OUTOFMEMORY;
	}

	m_xsize = x;
	m_ysize = y;

	if (m_array_i1)
		free(m_array_i1);
	if (m_array_nodata) {
		free(m_array_nodata);
		m_array_nodata = nullptr;
	}
	m_array_i1 = (std::int8_t *)mem;
	m_array_nodata = mem2;

	std::uint32_t i, array_size = x * y;

	if (size == 1) {
		for (i = 0; i < array_size; i++)
			m_array_i1[i] = u.vt_c;
	}
	else if (size == 2) {
		for (i = 0; i < array_size; i++)
			m_array_i2[i] = u.vt_s;
	}
	else if (size == 4) {
		for (i = 0; i < array_size; i++)
			m_array_i4[i] = u.vt_l;
	}
	else if (size == 8) {
		for (i = 0; i < array_size; i++)
			m_array_i8[i] = u.vt_ll;
	}

	memset(m_array_nodata, 0, (size_t)x * (size_t)y * sizeof(bool));

	m_bRequiresSave = true;
	return S_OK;
}


HRESULT CCWFGM_AttributeFilter::MT_Lock(Layer *layerThread, bool exclusive, std::uint16_t obtain) {
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

		hr = gridEngine->MT_Lock(layerThread, exclusive, obtain);

		if (SUCCEEDED(hr) && (m_fuelMap))
			hr = m_fuelMap->MT_Lock(exclusive, obtain);
	} else {
		hr = gridEngine->MT_Lock(layerThread, exclusive, obtain);

		if (SUCCEEDED(hr) && (m_fuelMap))
			hr = m_fuelMap->MT_Lock(exclusive, obtain);

		if (exclusive)	m_lock.Unlock();
		else		m_lock.Unlock(1000000LL);
	}
	return S_OK;
}


HRESULT CCWFGM_AttributeFilter::GetFuelData(Layer *layerThread, const XY_Point &pt, const HSS_Time::WTime &time, ICWFGM_Fuel **fuel, bool *fuel_valid, XY_Rectangle *cache_bbox) {
	if (!fuel)									return E_POINTER;
	if (!fuel_valid)							return E_POINTER;
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine)							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	if (m_optionKey == (std::uint16_t)-1) {
		std::uint16_t x = convertX(pt.x, cache_bbox);
		std::uint16_t y = convertY(pt.y, cache_bbox);
		if (!m_fuelMap)							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
		if (m_xsize == (std::uint16_t)-1)		{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
		if (!m_array_ui1)						{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
		if (m_optionType != VT_UI1)				{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
		if (x >= m_xsize)						return ERROR_GRID_LOCATION_OUT_OF_RANGE;
		if (y >= m_ysize)						return ERROR_GRID_LOCATION_OUT_OF_RANGE;

		uint32_t index = arrayIndex(x, y);
		long idx, export_index;
		if (m_array_nodata[index]) {
			*fuel = nullptr;
			*fuel_valid = false;
			return ERROR_FUELS_FUEL_UNKNOWN;
		}
		*fuel_valid = true;
		return m_fuelMap->FuelAtIndex(m_array_ui1[index], &idx, &export_index, fuel);
	}
	return gridEngine->GetFuelData(layerThread, pt, time, fuel, fuel_valid, cache_bbox);
}


HRESULT CCWFGM_AttributeFilter::GetFuelIndexData(Layer *layerThread, const XY_Point &pt, const HSS_Time::WTime &time, std::uint8_t *fuel_index, bool *fuel_valid, XY_Rectangle *cache_bbox) {
	if (!fuel_index)							return E_POINTER;
	if (!fuel_valid)							return E_POINTER;
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine)							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	if (m_optionKey == (std::uint16_t)-1) {
		std::uint16_t x = convertX(pt.x, cache_bbox);
		std::uint16_t y = convertY(pt.y, cache_bbox);
		if (!m_fuelMap)							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
		if (m_xsize == (std::uint16_t)-1)		{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
		if (!m_array_ui1)						{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
		if (m_optionType != VT_UI1)				{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
		if (x >= m_xsize)						return ERROR_GRID_LOCATION_OUT_OF_RANGE;
		if (y >= m_ysize)						return ERROR_GRID_LOCATION_OUT_OF_RANGE;

		uint32_t index = arrayIndex(x, y);
		*fuel_valid = !m_array_nodata[index];
		if (m_array_nodata[index])
			*fuel_index = (std::uint8_t)-1;
		else
			*fuel_index = m_array_ui1[index];
		if (m_array_nodata[index])					return ERROR_FUELS_FUEL_UNKNOWN;
		return S_OK;
	}
	return gridEngine->GetFuelIndexData(layerThread, pt, time, fuel_index, fuel_valid, cache_bbox);
}


HRESULT CCWFGM_AttributeFilter::GetFuelDataArray(Layer *layerThread, const XY_Point &min_pt, const XY_Point &max_pt, double scale,
    const HSS_Time::WTime &time, ICWFGM_Fuel_2d *fuel, bool_2d *fuel_valid) {

	std::uint16_t x_min = convertX(min_pt.x, nullptr), y_min = convertY(min_pt.y, nullptr);
	std::uint16_t x_max = convertX(max_pt.x, nullptr), y_max = convertY(max_pt.y, nullptr);
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine)							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	if (m_optionKey == (std::uint16_t)-1) {
		if (!m_fuelMap)							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
		if (m_xsize == (std::uint16_t)-1)		{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
		if (!m_array_ui1)						{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
		if (m_optionType != VT_UI1)				{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
		if (x_min >= m_xsize)					return ERROR_GRID_LOCATION_OUT_OF_RANGE;
		if (y_min >= m_ysize)					return ERROR_GRID_LOCATION_OUT_OF_RANGE;
		if (x_max >= m_xsize)					return ERROR_GRID_LOCATION_OUT_OF_RANGE;
		if (y_max >= m_ysize)					return ERROR_GRID_LOCATION_OUT_OF_RANGE;
		if (x_min > x_max)						return E_INVALIDARG;
		if (y_min > y_max)						return E_INVALIDARG;

		if (!fuel)								return E_POINTER;
		if (!fuel_valid)						return E_POINTER;

		const boost::multi_array_types::size_type *dims = fuel->shape();
		if (dims[0] < (x_max - x_min + 1))		return E_INVALIDARG;
		if (dims[1] < (y_max - y_min + 1))		return E_INVALIDARG;

		dims = fuel_valid->shape();
		if (dims[0] < (x_max - x_min + 1))		return E_INVALIDARG;
		if (dims[1] < (y_max - y_min + 1))		return E_INVALIDARG;

		std::int32_t index, index2;
		long idx, export_index;
		std::uint16_t x, y, xx, yy;
		HRESULT hr;
		ICWFGM_Fuel *fff = nullptr;
		bool bff = false;

		for (y = y_min; y <= y_max; y++)				// for every point that was requested...
		{
			for (x = x_min; x <= x_max; x++) {
				(*fuel)[x - x_min][y - y_min] = fff;	// first clear it out
				(*fuel_valid)[x - x_min][y - y_min] = bff;
			}
		}

		// for every point that was requested... 
		for (y = y_min; y <= y_max; y++) {
			for (x = x_min, index = arrayIndex(x, y); x <= x_max; x++, index++) {
				if ((*fuel_valid)[x - x_min][y - y_min])
					continue;				// if we have taken care of this point, then skip it and move on
				if (m_array_nodata[index]) {
					weak_assert((*fuel)[x - x_min][y - y_min] == nullptr);
					fff = nullptr;
					bff = false;
					(*fuel)[x - x_min][y - y_min] = fff;
					(*fuel_valid)[x - x_min][y - y_min] = bff;
					continue;				// hit a noData so just continue on to the next cell
				}
				else {
					hr = m_fuelMap->FuelAtIndex(m_array_ui1[index], &idx, &export_index, &fff);
					(*fuel)[x - x_min][y - y_min] = fff;
					bff = true;
					(*fuel_valid)[x - x_min][y - y_min] = bff;
				}
				if (FAILED(hr))					// find out what fuel is here
					return hr;
				for (yy = y; yy <= y_max; yy++) {
					for (xx = ((yy == y) ? x : x_min), index2 = arrayIndex(xx, yy); xx <= x_max; xx++, index2++) {
						if ((*fuel_valid)[xx - x_min][yy - y_min])
							continue;
						if ((!m_array_nodata[index]) && (!m_array_nodata[index2]) && (m_array_ui1[index] == m_array_ui1[index2])) {
							(*fuel)[xx - x_min][yy - y_min] = fff;
							(*fuel_valid)[xx - x_min][yy - y_min] = bff;
						}
					}
				}
			}
		}
		return S_OK;
	}
	return gridEngine->GetFuelDataArray(layerThread, min_pt, max_pt, scale, time, fuel, fuel_valid);
}


HRESULT CCWFGM_AttributeFilter::GetFuelIndexDataArray(Layer *layerThread, const XY_Point &min_pt, const XY_Point &max_pt, double scale,
    const HSS_Time::WTime &time, uint8_t_2d *fuel, bool_2d *fuel_valid) {

	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine)							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	if (m_optionKey == (std::uint16_t)-1) {
		std::uint16_t x_min = convertX(min_pt.x, nullptr), y_min = convertY(min_pt.y, nullptr);
		std::uint16_t x_max = convertX(max_pt.x, nullptr), y_max = convertY(max_pt.y, nullptr);
		if (!m_fuelMap)							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
		if (m_xsize == (std::uint16_t)-1)		{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
		if (!m_array_ui1)						{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
		if (m_optionType != VT_UI1)				{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
		if (x_min >= m_xsize)					return ERROR_GRID_LOCATION_OUT_OF_RANGE;
		if (y_min >= m_ysize)					return ERROR_GRID_LOCATION_OUT_OF_RANGE;
		if (x_max >= m_xsize)					return ERROR_GRID_LOCATION_OUT_OF_RANGE;
		if (y_max >= m_ysize)					return ERROR_GRID_LOCATION_OUT_OF_RANGE;
		if (min_pt.x > max_pt.x)				return E_INVALIDARG;
		if (min_pt.y > max_pt.y)				return E_INVALIDARG;

		if (!fuel)								return E_POINTER;
		if (!fuel_valid)						return E_POINTER;

		const boost::multi_array_types::size_type *dims = fuel->shape();
		if (dims[0] < (x_max - x_min + 1))		return E_INVALIDARG;
		if (dims[1] < (y_max - y_min + 1))		return E_INVALIDARG;

		dims = fuel_valid->shape();
		if (dims[0] < (x_max - x_min + 1))		return E_INVALIDARG;
		if (dims[1] < (y_max - y_min + 1))		return E_INVALIDARG;
		std::int32_t index;
		std::uint16_t x, y;

		for (y = y_min; y <= y_max; y++) {
			for (x = x_min, index = arrayIndex(x, y); x <= x_max; x++, index++) {
				(*fuel_valid)[x - x_min][y - y_min] = !m_array_nodata[index];
				if (m_array_nodata[index])
					(*fuel)[x - x_min][y - y_min] = (std::uint8_t)-1;
				else
					(*fuel)[x - x_min][y - y_min] = m_array_ui1[index];
			}
		}
		return S_OK;
	}
	return gridEngine->GetFuelIndexDataArray(layerThread, min_pt, max_pt, scale, time, fuel, fuel_valid);
}


HRESULT CCWFGM_AttributeFilter::GetAttributeData(Layer *layerThread, const XY_Point &pt, const HSS_Time::WTime &time, const HSS_Time::WTimeSpan& timeSpan, std::uint16_t option, std::uint64_t optionFlags, NumericVariant *attribute, grid::AttributeValue *attribute_valid, XY_Rectangle *cache_bbox) {
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine)							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	if (option == m_optionKey)
		return getPoint(pt, attribute, attribute_valid);
	return gridEngine->GetAttributeData(layerThread, pt, time, timeSpan, option, optionFlags, attribute, attribute_valid, cache_bbox);
}


HRESULT CCWFGM_AttributeFilter::GetAttributeDataArray(Layer *layerThread, const XY_Point &min_pt, const XY_Point &max_pt, double scale, const HSS_Time::WTime &time, const HSS_Time::WTimeSpan& timeSpan,
    std::uint16_t option, std::uint64_t optionFlags, NumericVariant_2d *attribute, attribute_t_2d *attribute_valid) {
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine)							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

	if (option == m_optionKey) {
		std::uint16_t x_min = convertX(min_pt.x, nullptr), y_min = convertY(min_pt.y, nullptr);
		std::uint16_t x_max = convertX(max_pt.x, nullptr), y_max = convertY(max_pt.y, nullptr);
		if (!attribute)							return E_POINTER;
		if (!attribute_valid)					return E_POINTER;

		const ICWFGM_Fuel_2d::size_type *dims = attribute->shape();
		if (dims[0] < (x_max - x_min + 1))		return E_INVALIDARG;
		if (dims[1] < (y_max - y_min + 1))		return E_INVALIDARG;

		dims = attribute_valid->shape();
		if (dims[0] < (x_max - x_min + 1))		return E_INVALIDARG;
		if (dims[1] < (y_max - y_min + 1))		return E_INVALIDARG;
		
		std::uint16_t x, y;
		NumericVariant v;
		grid::AttributeValue v_valid;
		HRESULT hr;
		for (y = y_min; y <= y_max; y++) {				// for every point that was requested...
			for (x = x_min; x <= x_max; x++) {
				(*attribute)[x - x_min][y - y_min] = v;
				(*attribute_valid)[x - x_min][y - y_min] = grid::AttributeValue::NOT_SET;
			}
		}

		for (y = y_min; y <= y_max; y++) {				// for every point that was requested...
			for (x = x_min; x <= x_max; x++) {
				XY_Point pt;
				pt.x = x;
				pt.y = y;
				if (FAILED(hr = getPoint(pt, &v, &v_valid))) {
					if (hr != ERROR_GRID_NO_DATA)
						break;
				}
				(*attribute)[x - x_min][y - y_min] = v;
				(*attribute_valid)[x - x_min][y - y_min] = v_valid;
			}
		}
		return S_OK;
	}
	return gridEngine->GetAttributeDataArray(layerThread, min_pt, max_pt, scale, time, timeSpan, option, optionFlags, attribute, attribute_valid);
}


HRESULT CCWFGM_AttributeFilter::Clone(boost::intrusive_ptr<ICWFGM_CommonBase> *newObject) const {
	if (!newObject)						return E_POINTER;

	CRWThreadSemaphoreEngage engage(*(CRWThreadSemaphore *)&m_lock, SEM_FALSE);

	try {
		CCWFGM_AttributeFilter *f = new CCWFGM_AttributeFilter(*this);
		*newObject = f;
		return S_OK;
	}
	catch (std::exception &e) {
	}
	return E_FAIL;
}


HRESULT CCWFGM_AttributeFilter::get_OptionKey(std::uint16_t *pVal) {
	if (!pVal)						return E_POINTER;
	*pVal = m_optionKey;
	return S_OK;
}


HRESULT CCWFGM_AttributeFilter::put_OptionKey(std::uint16_t newVal) {
	m_optionKey = newVal;
	m_bRequiresSave = true;
	return S_OK;
}


HRESULT CCWFGM_AttributeFilter::get_OptionType(std::uint16_t *pVal) {
	if (!pVal)						return E_POINTER;
	*pVal = m_optionType;
	return S_OK;
}


HRESULT CCWFGM_AttributeFilter::put_OptionType(std::uint16_t newVal) {
	switch (newVal) {
		case VT_I1:
		case VT_I2:
		case VT_I4:
		case VT_I8:
		case VT_UI1:
		case VT_UI2:
		case VT_UI4:
		case VT_UI8:
		case VT_R4:
		case VT_R8:
		case VT_BOOL:	m_optionType = newVal;
				if (m_array_i1) {
					free(m_array_i1);
					m_array_i1 = NULL;
				}
				if (m_array_nodata) {
					free(m_array_nodata);
					m_array_nodata = NULL;
				}
				m_bRequiresSave = true;
				return S_OK;
		default:	return E_INVALIDARG;
	}
}


HRESULT CCWFGM_AttributeFilter::GetAttribute(Layer *layerThread, std::uint16_t option,  /*unsigned*/ PolymorphicAttribute *value) {
	if (!layerThread) {
		HRESULT hr = GetAttribute(option, value);
		if (SUCCEEDED(hr))
			return hr;
	}

	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine)							return ERROR_GRID_UNINITIALIZED;
	return gridEngine->GetAttribute(layerThread, option, value);
}


/*! Polymorphic.  This routine sets an attribute/option value given the attribute/option index.  Currently does not
	perform any operation and is simply reserved for future functionality.
	\param option Reserved for future functionality.
	\param value Value for the attribute/option index
	\sa ICWFGM_AttributeFilter::SetAttribute

	\retval E_NOTIMPL This function is reserved for future functionality.
*/
HRESULT CCWFGM_AttributeFilter::SetAttribute(std::uint16_t option, const PolymorphicAttribute &var) {
	std::uint32_t *cnt, old;
	bool bval;
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(0, &cnt);
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


HRESULT CCWFGM_AttributeFilter::PutGridEngine(Layer *layerThread, ICWFGM_GridEngine *newVal) {
	HRESULT hr = ICWFGM_GridEngine::PutGridEngine(layerThread, newVal);
	if (SUCCEEDED(hr) && m_gridEngine(nullptr))
		fixResolution(nullptr, "");
	return hr;
}


HRESULT CCWFGM_AttributeFilter::fixResolution(std::shared_ptr<validation::validation_object> valid, const std::string& name) {
	HRESULT hr;
	double gridResolution, gridXLL, gridYLL;
	PolymorphicAttribute var;

	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine;
	if (!(gridEngine = m_gridEngine(nullptr)))				{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

	/*POLYMORPHIC CHECK*/
	if (FAILED(hr = gridEngine->GetDimensions(0, &m_xsize, &m_ysize)))
		return hr;
	if (FAILED(hr = gridEngine->GetAttribute(nullptr, CWFGM_GRID_ATTRIBUTE_PLOTRESOLUTION, &var))) {
		if (valid)
			/// <summary>
			/// The plot resolution is not readable but should be by this time in deserialization.
			/// </summary>
			/// <type>internal</type>
			valid->add_child_validation("WISE.GridProto.CwfgmAttributeFilter", name, validation::error_level::SEVERE,
				validation::id::initialization_incomplete, "resolution");
		return hr;
	}
	try { gridResolution = std::get<double>(var); } catch (std::bad_variant_access &) { weak_assert(false); return E_FAIL; };

	if (FAILED(hr = gridEngine->GetAttribute(nullptr, CWFGM_GRID_ATTRIBUTE_XLLCORNER, &var))) {
		if (valid)
			/// <summary>
			/// The plot lower left corner is not readable but should be by this time in deserialization.
			/// </summary>
			/// <type>internal</type>
			valid->add_child_validation("WISE.GridProto.CwfgmAttributeFilter", name, validation::error_level::SEVERE,
				validation::id::initialization_incomplete, "xllcorner");
		return hr;
	}
	try { gridXLL = std::get<double>(var); } catch (std::bad_variant_access &) { weak_assert(false); return E_FAIL; };

	if (FAILED(hr = gridEngine->GetAttribute(nullptr, CWFGM_GRID_ATTRIBUTE_YLLCORNER, &var))) {
		if (valid)
			/// <summary>
			/// The plot lower left corner is not readable but should be by this time in deserialization.
			/// </summary>
			/// <type>internal</type>
			valid->add_child_validation("WISE.GridProto.CwfgmAttributeFilter", name, validation::error_level::SEVERE,
				validation::id::initialization_incomplete, "yllcorner");
		return hr;
	}
	try { gridYLL = std::get<double>(var); } catch (std::bad_variant_access &) { weak_assert(false); return E_FAIL; };

	m_resolution = gridResolution;
	m_xllcorner = gridXLL;
	m_yllcorner = gridYLL;

	return S_OK;
}


std::uint16_t CCWFGM_AttributeFilter::convertX(double x, XY_Rectangle* bbox) {
	double lx = x - m_xllcorner;
	double cx = floor(lx / m_resolution);
	if (bbox) {
		bbox->m_min.x = cx * m_resolution + m_xllcorner;
		bbox->m_max.x = bbox->m_min.x + m_resolution;
	}
	return (std::uint16_t)cx;
}


std::uint16_t CCWFGM_AttributeFilter::convertY(double y, XY_Rectangle* bbox) {
	double ly = y - m_yllcorner;
	double cy = floor(ly / m_resolution);
	if (bbox) {
		bbox->m_min.y = cy * m_resolution + m_yllcorner;
		bbox->m_max.y = bbox->m_min.y + m_resolution;
	}
	return (std::uint16_t)cy;
}
