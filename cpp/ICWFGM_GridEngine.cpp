/**
 * WISE_Grid_Module: ICWFGM_GridEngine.h
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
#include <cpl_string.h>
#include "CoordinateConverter.h"

#include <errno.h>
#include <float.h>
#include <stdio.h>

#include "propsysreplacement.h"

#ifdef DEBUG
#include <assert.h>
#endif

using namespace CONSTANTS_NAMESPACE;


#ifndef DOXYGEN_IGNORE_CODE


ICWFGM_GridEngine::ICWFGM_GridEngine() {
}


ICWFGM_GridEngine::~ICWFGM_GridEngine() {
}


boost::intrusive_ptr<ICWFGM_GridEngine> ICWFGM_GridEngine::m_gridEngine(Layer *layerThread, std::uint32_t **cnt) const {
	boost::intrusive_ptr<ICWFGM_GridEngine> engine;
	if (!layerThread)
		return m_rootEngine;
	m_layerManager->GetGridEngine(layerThread, this, cnt, &engine);
	return engine;
}

#endif


HRESULT ICWFGM_GridEngine::GetGridEngine(Layer *layerThread, boost::intrusive_ptr<ICWFGM_GridEngine> *pVal) const {
	if (!pVal)									return E_POINTER;
	if (!m_layerManager)						{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

	*pVal = m_gridEngine(layerThread);
	if (!(*pVal))								return ERROR_GRID_UNINITIALIZED;
	return S_OK;
}


HRESULT ICWFGM_GridEngine::PutGridEngine(Layer *layerThread, ICWFGM_GridEngine *newVal) {
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
	if (!m_layerManager) { weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	return m_layerManager->PutGridEngine(layerThread, this, newVal);
}


HRESULT ICWFGM_GridEngine::get_LayerManager(CCWFGM_LayerManager **pVal) {
	if (!pVal)						return E_POINTER;
	*pVal = m_layerManager.get();
	return S_OK;
}


HRESULT ICWFGM_GridEngine::put_LayerManager(CCWFGM_LayerManager *newVal) {
	if ((m_layerManager) && (newVal))
		return ERROR_NOT_SUPPORTED | ERROR_SEVERITY_WARNING;

	if (!newVal) {
		m_layerManager = nullptr;
		return S_OK;
	}
	HRESULT retval;
	boost::intrusive_ptr <CCWFGM_LayerManager> manager;
	manager = dynamic_cast<CCWFGM_LayerManager *>(newVal);
	if (manager) {
		m_layerManager = manager;
		retval = S_OK;
	}
	else
		retval = E_FAIL;
	return retval;
}


HRESULT ICWFGM_GridEngine::Valid(Layer *layerThread, const HSS_Time::WTime &start_time, const HSS_Time::WTimeSpan &duration, std::uint32_t option, std::vector<uint16_t> *application_count) {
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);

	if (!gridEngine) { weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	HRESULT hr = gridEngine->Valid(layerThread, start_time, duration, option, application_count);
	return hr;
}


HRESULT ICWFGM_GridEngine::GetEventTime(Layer *layerThread, const XY_Point& pt, std::uint32_t flags, const HSS_Time::WTime &from_time, HSS_Time::WTime *next_event, bool* event_valid) {
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine) { weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	HRESULT hr = gridEngine->GetEventTime(layerThread, pt, flags, from_time, next_event, event_valid);
	return hr;
}


HRESULT ICWFGM_GridEngine::GetCommonData(Layer* layerThread, ICWFGM_CommonData** pVal) {
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine) { weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	return gridEngine->GetCommonData(layerThread, pVal);
}


HRESULT ICWFGM_GridEngine::PutCommonData(Layer* layerThread, ICWFGM_CommonData* pVal) {
	return E_NOTIMPL;
}


HRESULT ICWFGM_GridEngine::GetDimensions(Layer *layerThread, std::uint16_t *x_dim, std::uint16_t *y_dim) {
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine) { weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	return gridEngine->GetDimensions(layerThread, x_dim, y_dim);
}


HRESULT ICWFGM_GridEngine::GetFuelData(Layer *layerThread, const XY_Point &pt, const HSS_Time::WTime &time, ICWFGM_Fuel **fuel, bool *fuel_valid, XY_Rectangle *cache_bbox)
{
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine) { weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	return gridEngine->GetFuelData(layerThread, pt, time, fuel, fuel_valid, cache_bbox);

}

HRESULT ICWFGM_GridEngine::GetFuelIndexData(Layer *layerThread, const XY_Point &pt, const HSS_Time::WTime &time, std::uint8_t *fuel_index, bool *fuel_valid, XY_Rectangle *cache_bbox) {
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine) { weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	return gridEngine->GetFuelIndexData(layerThread, pt, time, fuel_index, fuel_valid, cache_bbox);
}


HRESULT ICWFGM_GridEngine::GetFuelDataArray(Layer *layerThread, const XY_Point &min_pt, const XY_Point &max_pt, double scale, const HSS_Time::WTime &time, ICWFGM_Fuel_2d *fuel, bool_2d *fuel_valid)
{
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine) { weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	return gridEngine->GetFuelDataArray(layerThread, min_pt, max_pt, scale, time, fuel, fuel_valid);
}


HRESULT ICWFGM_GridEngine::GetFuelIndexDataArray(Layer *layerThread, const XY_Point &min_pt, const XY_Point &max_pt, double scale,
	const HSS_Time::WTime &time, uint8_t_2d *fuel_index, bool_2d *fuel_valid) {
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine) { weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	return gridEngine->GetFuelIndexDataArray(layerThread, min_pt, max_pt, scale, time, fuel_index, fuel_valid);
}


HRESULT ICWFGM_GridEngine::GetElevationData(Layer *layerThread, const XY_Point &pt, bool allow_defaults_returned, double *elevation, double *slope_factor, double *slope_azimuth, grid::TerrainValue *elev_valid, grid::TerrainValue *terrain_valid, XY_Rectangle *cache_bbox) {
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine) { weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	return gridEngine->GetElevationData(layerThread, pt, allow_defaults_returned, elevation, slope_factor, slope_azimuth, elev_valid, terrain_valid, cache_bbox);
}


HRESULT ICWFGM_GridEngine::GetElevationDataArray(Layer *layerThread, const XY_Point &min_pt, const XY_Point &max_pt, double scale, bool allow_defaults_returned,
	double_2d *elevation, double_2d *slope_factor, double_2d *slope_azimuth, terrain_t_2d *elev_valid, terrain_t_2d *terrain_valid) {

	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine) { weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	return gridEngine->GetElevationDataArray(layerThread, min_pt, max_pt, scale, allow_defaults_returned, elevation, slope_factor, slope_azimuth, elev_valid, terrain_valid);
}


HRESULT ICWFGM_GridEngine::GetWeatherData(Layer *layerThread, const XY_Point &pt, const HSS_Time::WTime &time, std::uint64_t interpolate_method,
	IWXData *wx, IFWIData *ifwi, DFWIData *dfwi, bool *wx_valid, XY_Rectangle *cache_bbox) {

	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine) { weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	return gridEngine->GetWeatherData(layerThread, pt, time, interpolate_method, wx, ifwi, dfwi, wx_valid, cache_bbox);
}


HRESULT ICWFGM_GridEngine::GetWeatherDataArray(Layer *layerThread, const XY_Point &min_pt, const XY_Point &max_pt, double scale, const HSS_Time::WTime &time, std::uint64_t interpolate_method,
		IWXData_2d *wx, IFWIData_2d *ifwi, DFWIData_2d *dfwi, bool_2d *wx_valid) {

	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine) { weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	return gridEngine->GetWeatherDataArray(layerThread, min_pt, max_pt, scale, time, interpolate_method, wx, ifwi, dfwi, wx_valid);
}


HRESULT ICWFGM_GridEngine::GetAttributeData(Layer *layerThread, const XY_Point &pt, const HSS_Time::WTime &time, const HSS_Time::WTimeSpan& timeSpan, std::uint16_t option, std::uint64_t optionFlags, NumericVariant *attribute, grid::AttributeValue *attribute_valid, XY_Rectangle *cache_bbox) {
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine) { weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	return gridEngine->GetAttributeData(layerThread, pt, time, timeSpan, option, optionFlags, attribute, attribute_valid, cache_bbox);
}


HRESULT ICWFGM_GridEngine::GetAttributeDataArray(Layer *layerThread, const XY_Point &min_pt, const XY_Point &max_pt, double scale, const HSS_Time::WTime &time, const HSS_Time::WTimeSpan& timeSpan,
	std::uint16_t option, std::uint64_t optionFlags, NumericVariant_2d *attribute, attribute_t_2d *attribute_valid) {
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine) { weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	return gridEngine->GetAttributeDataArray(layerThread, min_pt, max_pt, scale, time, timeSpan, option, optionFlags, attribute, attribute_valid);
}


HRESULT ICWFGM_GridEngine::PreCalculationEvent(Layer *layerThread, const HSS_Time::WTime &time, std::uint32_t mode, CalculationEventParms *parms) {
	std::uint32_t *cnt;
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread, &cnt);
	if (!gridEngine) { weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

	if ((mode & (~(1 << CWFGM_SCENARIO_OPTION_WEATHER_ALTERNATE_CACHE))) == 1) {
		(*cnt)++;
	}
	HRESULT hr = gridEngine->PreCalculationEvent(layerThread, time, mode, parms);
	return hr;
}


HRESULT ICWFGM_GridEngine::PostCalculationEvent(Layer *layerThread, const HSS_Time::WTime &time, std::uint32_t mode, CalculationEventParms *parms) {
	std::uint32_t *cnt;
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread, &cnt);
	if (!gridEngine) { weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	if ((mode & (~(1 << CWFGM_SCENARIO_OPTION_WEATHER_ALTERNATE_CACHE))) == 1) {
		(*cnt)--;
	}
	HRESULT hr = gridEngine->PostCalculationEvent(layerThread, time, mode, parms);
	return hr;
}
