/**
 * WISE_Grid_Module: CWFGM_TemporalAttributeFilter.h
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

#include "GridCOM.h"
#include "CWFGM_TemporalAttributeFilter.h"
#include "FuelCom_ext.h"
#include "GridCom_ext.h"
#include "FireEngine_ext.h"
#include <errno.h>
#include <stdio.h>
#include "CoordinateConverter.h"
#include "propsysreplacement.h"


IMPLEMENT_OBJECT_CACHE_MT_NO_TEMPLATE(DailyAttribute, DailyAttribute, 16 * 1024 * 1024 / sizeof(DailyAttribute), false, 16)
IMPLEMENT_OBJECT_CACHE_MT_NO_TEMPLATE(SeasonalAttribute, SeasonalAttribute, 8 * 1024 * 1024 / sizeof(SeasonalAttribute), false, 16)


CCWFGM_TemporalAttributeFilter::CCWFGM_TemporalAttributeFilter() : m_timeManager(nullptr) {
	m_converter.setGrid(-1.0, -1.0, -1.0);
	m_bRequiresSave = false;

	SeasonalAttribute *def = new SeasonalAttribute();
	m_conditions.AddHead(def);
}


CCWFGM_TemporalAttributeFilter::CCWFGM_TemporalAttributeFilter(const CCWFGM_TemporalAttributeFilter &toCopy) : m_timeManager(toCopy.m_timeManager) {
	CRWThreadSemaphoreEngage engage(*(CRWThreadSemaphore *)&toCopy.m_lock, SEM_FALSE);

	m_converter.setGrid(toCopy.m_converter.resolution(), toCopy.m_converter.xllcorner(), toCopy.m_converter.yllcorner());
	DailyAttribute *da = toCopy.m_attributes.LH_Head();
	while (da->LN_Succ()) {
		DailyAttribute *da_c = new DailyAttribute(*da);
		da_c->m_localStartTime.SetTimeManager(m_timeManager);
		m_attributes.AddTail(da_c);
		da = da->LN_Succ();
	}

	SeasonalAttribute *sa = toCopy.m_conditions.LH_Head();
	while (sa->LN_Succ()) {
		SeasonalAttribute *sa_c = new SeasonalAttribute(*sa);
		m_conditions.AddTail(sa_c);
		sa = sa->LN_Succ();
	}

	m_bRequiresSave = false;
}


CCWFGM_TemporalAttributeFilter::~CCWFGM_TemporalAttributeFilter() {
	DailyAttribute *o;
	while (o = m_attributes.RemHead())
		delete o;
	SeasonalAttribute *s;
	while (s = m_conditions.RemHead())
		delete s;
}


DailyAttribute::DailyAttribute(WTimeManager *tm) : m_localStartTime(0ULL, tm), m_startTime(0LL), m_endTime(0, 23, 59, 59) {
	m_MinRH = 1.0;
	m_MaxWS = 0.0;
	m_MinFWI = 0.0;
	m_MinISI = 8.0;
	m_startTimeRelative = ::WISE::GridProto::TemporalCondition_DailyAttribute_TimeRelative::TemporalCondition_DailyAttribute_TimeRelative_LOCAL_MIDNIGHT;
	m_endTimeRelative = ::WISE::GridProto::TemporalCondition_DailyAttribute_TimeRelative::TemporalCondition_DailyAttribute_TimeRelative_LOCAL_MIDNIGHT;
	m_bits = 0;
}


SeasonalAttribute::SeasonalAttribute() {
	m_localStartDate = WTimeSpan(0LL);
	m_optionFlags = 0;
	m_optionFlagsSet = 0;
	m_bits = 0;
	m_curingDegree = 60.0;
}


HRESULT CCWFGM_TemporalAttributeFilter::GetAttribute(unsigned short option, PolymorphicAttribute *value) {
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


HRESULT CCWFGM_TemporalAttributeFilter::MT_Lock(Layer *layerThread, bool exclusive, std::uint16_t obtain) {
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine) { weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

	HRESULT hr;
	if (obtain == (std::uint16_t)-1) {
		std::int64_t state = m_lock.CurrentState();
		if (!state)				return SUCCESS_STATE_OBJECT_UNLOCKED;
		if (state < 0)			return SUCCESS_STATE_OBJECT_LOCKED_WRITE;
		if (state >= 1000000LL)	return SUCCESS_STATE_OBJECT_LOCKED_SCENARIO;
		return						   SUCCESS_STATE_OBJECT_LOCKED_READ;
	} else if (obtain) {
		if (exclusive)	m_lock.Lock_Write();
		else			m_lock.Lock_Read(1000000LL);

		hr = gridEngine->MT_Lock(layerThread, exclusive, obtain);
	} else {
		hr = gridEngine->MT_Lock(layerThread, exclusive, obtain);

		if (exclusive)	m_lock.Unlock();
		else		m_lock.Unlock(1000000LL);
	}
	return S_OK;
}


HRESULT CCWFGM_TemporalAttributeFilter::SetWorldLocation(ICWFGM_GridEngine *gridEngine, Layer *layerThread) {
	PolymorphicAttribute var;
	HRESULT hr;
	double temp;

	ICWFGM_CommonData *data;
	if (FAILED(hr = gridEngine->GetCommonData(layerThread, &data)))									return hr;
	m_timeManager = data->m_timeManager;

	gridEngine->GetAttribute(nullptr, CWFGM_GRID_ATTRIBUTE_SPATIALREFERENCE, &var);
	std::string csProject;
	try { csProject = std::get<std::string>(var); }
	catch (std::bad_variant_access&) { return ERROR_PROJECTION_UNKNOWN; };	// exception can be thrown during start-up (deserialization)

	m_converter.SetSourceProjection(csProject.c_str());

	double gridResolution, gridXLL, gridYLL;
	if (FAILED(hr = gridEngine->GetAttribute(nullptr, CWFGM_GRID_ATTRIBUTE_PLOTRESOLUTION, &var))) return hr;
	VariantToDouble_(var, &gridResolution);
	if (FAILED(hr = gridEngine->GetAttribute(nullptr, CWFGM_GRID_ATTRIBUTE_XLLCORNER, &var))) return hr;
	VariantToDouble_(var, &gridXLL);
	if (FAILED(hr = gridEngine->GetAttribute(nullptr, CWFGM_GRID_ATTRIBUTE_YLLCORNER, &var))) return hr;
	VariantToDouble_(var, &gridYLL);
	m_converter.setGrid(gridResolution, gridXLL, gridYLL);

	return hr;
}


HRESULT CCWFGM_TemporalAttributeFilter::PutGridEngine(Layer *layerThread, ICWFGM_GridEngine *newVal) {
	HRESULT hr = ICWFGM_GridEngine::PutGridEngine(layerThread, newVal);
	if (SUCCEEDED(hr) && m_gridEngine(nullptr)) {
		hr = SetWorldLocation(m_rootEngine.get(), nullptr);
	}
	return hr;
}


HRESULT CCWFGM_TemporalAttributeFilter::PutCommonData(Layer* layerThread, ICWFGM_CommonData* pVal) {
	if (!pVal)
		return E_POINTER;
	m_timeManager = pVal->m_timeManager;
	DailyAttribute* da = m_attributes.LH_Head();
	while (da->LN_Succ()) {
		da->m_localStartTime.SetTimeManager(m_timeManager);
		da = da->LN_Succ();
	}
	return S_OK;
}


HRESULT CCWFGM_TemporalAttributeFilter::Valid(Layer *layerThread, const HSS_Time::WTime &start_time, const HSS_Time::WTimeSpan &duration, std::uint32_t option, std::vector<uint16_t> *application_count) {
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);

	if (!gridEngine)							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	HRESULT hr = gridEngine->Valid(layerThread, start_time, duration, option, application_count);

	if (m_rootEngine)
		SetWorldLocation(m_rootEngine.get(), nullptr);

	return hr;
}					


std::uint16_t CCWFGM_TemporalAttributeFilter::solarTimes(const XY_Point &pt, const WTime& from_time, WTime& rise, WTime& set, WTime& noon) const {
	WTime day(from_time, m_timeManager);
	day.PurgeToDay(WTIME_FORMAT_AS_LOCAL | WTIME_FORMAT_WITHDST);
	day += WTimeSpan(0, 12, 0, 0);
	double latitude = pt.y, longitude = pt.x;
	m_converter.SourceToLatlon(1, &longitude, &latitude, nullptr);
	std::uint16_t retval = m_timeManager->m_worldLocation.m_sun_rise_set(DEGREE_TO_RADIAN(latitude), DEGREE_TO_RADIAN(longitude), day, &rise, &set, &noon);
	return retval;
}

void CCWFGM_TemporalAttributeFilter::calculatedTimes(const DailyAttribute* day, const XY_Point &pt, WTime& start, bool& startEffective, WTime& end, bool& endEffective) const {
	WTime localday(day->m_localStartTime);
	WTime daystart(localday, WTIME_FORMAT_AS_LOCAL | WTIME_FORMAT_WITHDST, -1);

	if ((day->m_startTimeRelative == WISE::GridProto::TemporalCondition_DailyAttribute_TimeRelative::TemporalCondition_DailyAttribute_TimeRelative_LOCAL_MIDNIGHT) ||
		(day->m_startTimeRelative == WISE::GridProto::TemporalCondition_DailyAttribute_TimeRelative::TemporalCondition_DailyAttribute_TimeRelative_LOCAL_NOON))
	{
		start = daystart;

		if (day->m_startTimeRelative == WISE::GridProto::TemporalCondition_DailyAttribute_TimeRelative::TemporalCondition_DailyAttribute_TimeRelative_LOCAL_NOON) {
			start += WTimeSpan(0, 12, 0, 0);
		}

		if (day->m_bits & TEMPORAL_BITS_STIME_EFFECTIVE) {
			start += day->m_startTime;
			startEffective = true;
		}
		else
			startEffective = false;
	}
	else if (day->m_startTimeRelative == WISE::GridProto::TemporalCondition_DailyAttribute_TimeRelative::TemporalCondition_DailyAttribute_TimeRelative_SUN_RISE_SET) {
		WTime rise(daystart), set(daystart), noon(daystart);
		std::uint16_t calced = 0;

		WTime solar = daystart + WTimeSpan(0, 12, 0, 0);	// make sure the sun is in the sky
		calced = solarTimes(pt, solar, rise, set, noon);

#ifdef _DEBUG
		std::string rtime = rise.ToString(WTIME_FORMAT_AS_LOCAL | WTIME_FORMAT_WITHDST | WTIME_FORMAT_ABBREV | WTIME_FORMAT_DATE | WTIME_FORMAT_TIME);
		std::string ntime = noon.ToString(WTIME_FORMAT_AS_LOCAL | WTIME_FORMAT_WITHDST | WTIME_FORMAT_ABBREV | WTIME_FORMAT_DATE | WTIME_FORMAT_TIME);
		std::string stime = set.ToString(WTIME_FORMAT_AS_LOCAL | WTIME_FORMAT_WITHDST | WTIME_FORMAT_ABBREV | WTIME_FORMAT_DATE | WTIME_FORMAT_TIME);
#endif

		if ((day->m_bits & TEMPORAL_BITS_STIME_EFFECTIVE) && (!(calced & NO_SUNRISE))) {
			start = rise + day->m_startTime;
			startEffective = true;
		}
		else
			startEffective = false;
	}

	if ((day->m_endTimeRelative == WISE::GridProto::TemporalCondition_DailyAttribute_TimeRelative::TemporalCondition_DailyAttribute_TimeRelative_LOCAL_MIDNIGHT) ||
		(day->m_endTimeRelative == WISE::GridProto::TemporalCondition_DailyAttribute_TimeRelative::TemporalCondition_DailyAttribute_TimeRelative_LOCAL_NOON))
	{
		end = daystart;

		if (day->m_startTimeRelative == WISE::GridProto::TemporalCondition_DailyAttribute_TimeRelative::TemporalCondition_DailyAttribute_TimeRelative_LOCAL_NOON) {
			end += WTimeSpan(0, 12, 0, 0);
		}

		if (day->m_bits & TEMPORAL_BITS_ETIME_EFFECTIVE) {
			end += day->m_endTime;
			endEffective = true;
		}
		else
			endEffective = false;
	}
	else if (day->m_endTimeRelative == WISE::GridProto::TemporalCondition_DailyAttribute_TimeRelative::TemporalCondition_DailyAttribute_TimeRelative_SUN_RISE_SET) {
		WTime rise(daystart), set(daystart), noon(daystart);
		std::uint16_t calced = 0;

		WTime solar = daystart + WTimeSpan(0, 12, 0, 0);	// make sure the sun is in the sky
		calced = solarTimes(pt, solar, rise, set, noon);

#ifdef _DEBUG
		std::string rtime = rise.ToString(WTIME_FORMAT_AS_LOCAL | WTIME_FORMAT_WITHDST | WTIME_FORMAT_ABBREV | WTIME_FORMAT_DATE | WTIME_FORMAT_TIME);
		std::string ntime = noon.ToString(WTIME_FORMAT_AS_LOCAL | WTIME_FORMAT_WITHDST | WTIME_FORMAT_ABBREV | WTIME_FORMAT_DATE | WTIME_FORMAT_TIME);
		std::string stime = set.ToString(WTIME_FORMAT_AS_LOCAL | WTIME_FORMAT_WITHDST | WTIME_FORMAT_ABBREV | WTIME_FORMAT_DATE | WTIME_FORMAT_TIME);
#endif

		if ((day->m_bits & TEMPORAL_BITS_ETIME_EFFECTIVE) && (!(calced & NO_SUNSET))) {
			end = set + day->m_endTime;
			endEffective = true;
		}
		else
			endEffective = false;
	}

#ifdef _DEBUG
	std::string _stime = start.ToString(WTIME_FORMAT_AS_LOCAL | WTIME_FORMAT_WITHDST | WTIME_FORMAT_ABBREV | WTIME_FORMAT_DATE | WTIME_FORMAT_TIME);
	std::string _etime = end.ToString(WTIME_FORMAT_AS_LOCAL | WTIME_FORMAT_WITHDST | WTIME_FORMAT_ABBREV | WTIME_FORMAT_DATE | WTIME_FORMAT_TIME);
#endif

}


HRESULT CCWFGM_TemporalAttributeFilter::GetEventTime(Layer *layerThread, const XY_Point& pt, std::uint32_t flags, const HSS_Time::WTime &from_time,  HSS_Time::WTime *next_event, bool *event_valid) {
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine)							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	HRESULT hr = gridEngine->GetEventTime(layerThread, pt, flags, from_time, next_event, event_valid);

	if (flags & (CWFGM_GETEVENTTIME_FLAG_SEARCH_SUNRISE | CWFGM_GETEVENTTIME_FLAG_SEARCH_SUNSET)) {
		WTime rise(from_time), set(from_time), noon(from_time);
		std::uint16_t calced = solarTimes(pt, from_time, rise, set, noon);

#ifdef _DEBUG
		std::string rtime = rise.ToString(WTIME_FORMAT_AS_LOCAL | WTIME_FORMAT_WITHDST | WTIME_FORMAT_ABBREV | WTIME_FORMAT_DATE | WTIME_FORMAT_TIME);
		std::string ntime = noon.ToString(WTIME_FORMAT_AS_LOCAL | WTIME_FORMAT_WITHDST | WTIME_FORMAT_ABBREV | WTIME_FORMAT_DATE | WTIME_FORMAT_TIME);
		std::string stime = set.ToString(WTIME_FORMAT_AS_LOCAL | WTIME_FORMAT_WITHDST | WTIME_FORMAT_ABBREV | WTIME_FORMAT_DATE | WTIME_FORMAT_TIME);
#endif

		if ((flags & (CWFGM_GETEVENTTIME_FLAG_SEARCH_SUNRISE | CWFGM_GETEVENTTIME_FLAG_SEARCH_SUNSET)) == CWFGM_GETEVENTTIME_FLAG_SEARCH_SUNRISE) {
			*next_event = rise;
			*event_valid = !(calced & NO_SUNRISE);
		}
		else if ((flags & (CWFGM_GETEVENTTIME_FLAG_SEARCH_SUNRISE | CWFGM_GETEVENTTIME_FLAG_SEARCH_SUNSET)) == CWFGM_GETEVENTTIME_FLAG_SEARCH_SUNSET) {
			*next_event = set;
			*event_valid = !(calced & NO_SUNSET);
		}
		else {
			*next_event = noon;
			*event_valid = true;
		}
	}
	else if (!(flags & (CWFGM_GETEVENTTIME_QUERY_PRIMARY_WX_STREAM | CWFGM_GETEVENTTIME_QUERY_PRIMARY_WX_STREAM))) {
		DailyAttribute *day = m_attributes.LH_Head();// , *default_day = NULL;

		const WTime ft(from_time, m_timeManager);
		WTime n_e(*next_event, m_timeManager);
		WTime inday(from_time, m_timeManager);
		inday.PurgeToDay(WTIME_FORMAT_AS_LOCAL | WTIME_FORMAT_WITHDST);

		while (day->LN_Succ()) {
			if (day->m_localStartTime.IsValid()) {
				if ((day->m_bits & TEMPORAL_BITS_TIMES_EFFECTIVE) == TEMPORAL_BITS_TIMES_EFFECTIVE) {
					WTime start(day->m_localStartTime), end(day->m_localStartTime);
					bool s_effective, e_effective;
					calculatedTimes(day, pt, start, s_effective, end, e_effective);

					if (flags & CWFGM_GETEVENTTIME_FLAG_SEARCH_BACKWARD) {
						if ((s_effective) && ((start < ft) && (start > n_e)))
							n_e = start;
						else if ((s_effective) && ((end < ft) && (end > n_e)))
							n_e = end;
					}
					else {
						if ((e_effective) && ((start > ft) && (start < n_e)))
							n_e = start;
						else if ((e_effective) && ((end > ft) && (end < n_e)))
							n_e = end;
					}
				}
			}
			else {
				weak_assert(false);
			}
			day = day->LN_Succ();
		}

		SeasonalAttribute *season = m_conditions.LH_Head();
		while (season->LN_Succ()) {
			if ((season->m_bits & TEMPORAL_BITS_CURING_DEGREE_EFFECTIVE) ||
				(season->m_optionFlagsSet & ((1 << CWFGM_SCENARIO_OPTION_GRASSPHENOLOGY) | (1 << CWFGM_SCENARIO_OPTION_GREENUP)))) {		// skip over null-value season items
				WTime daystart(ft);
				daystart.PurgeToYear(WTIME_FORMAT_AS_LOCAL | WTIME_FORMAT_WITHDST);
				daystart += season->m_localStartDate;
				if (flags & CWFGM_GETEVENTTIME_FLAG_SEARCH_BACKWARD) {
					if ((daystart < ft) && (daystart > n_e))
						n_e = daystart;
				}
				else {
					if ((daystart > ft) && (daystart < n_e))
						n_e = daystart;
				}
			}
			season = season->LN_Succ();
		}
		next_event->SetTime(n_e);
	}

	return hr;
}


HRESULT CCWFGM_TemporalAttributeFilter::GetAttributeData(Layer *layerThread, const XY_Point &pt,const WTime &time, const HSS_Time::WTimeSpan& timeSpan, std::uint16_t option,
	std::uint64_t optionFlags, NumericVariant *attribute, grid::AttributeValue *attribute_valid, XY_Rectangle *cache_bbox) {
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine)							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

	DailyAttribute *found = NULL;
	SeasonalAttribute *sfound = NULL;
	
	weak_assert(time.IsValid());

	if ((option == CWFGM_SCENARIO_OPTION_GRASSPHENOLOGY) ||
		(option == CWFGM_SCENARIO_OPTION_GREENUP) ||
		(option == FUELCOM_ATTRIBUTE_CURINGDEGREE)) {
		try {
			WTimeSpan ws = time.GetWTimeSpanIntoYear(WTIME_FORMAT_AS_LOCAL | WTIME_FORMAT_WITHDST);
			sfound = findSeasonOption(ws, option, false, true);
		}
		catch (std::bad_variant_access &) {
			weak_assert(false);
			return E_FAIL;
		}
	} else if ((option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_RH) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_FWI) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_ISI) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MAX_WS) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START_INTERPRET) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END_INTERPRET) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START_COMPUTED) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END_COMPUTED)) {
		try {
			WTime _time(time, m_timeManager);
			found = findOption(_time, false);
		}
		catch (std::bad_variant_access &) {
			weak_assert(false);
			return E_FAIL;
		}
	}

	HRESULT hr;
	if (found) {
		switch (option) {
		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_RH:
			*attribute = found->m_MinRH;
			*attribute_valid = (found->m_bits & TEMPORAL_BITS_RH_EFFECTIVE) ? grid::AttributeValue::SET : grid::AttributeValue::NOT_SET;
			return S_OK;

		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_FWI:
			*attribute = found->m_MinFWI;
			*attribute_valid = (found->m_bits & TEMPORAL_BITS_FWI_EFFECTIVE) ? grid::AttributeValue::SET : grid::AttributeValue::NOT_SET;
			return S_OK;

		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_ISI:
			*attribute = found->m_MinISI;
			*attribute_valid = (found->m_bits & TEMPORAL_BITS_ISI_EFFECTIVE) ? grid::AttributeValue::SET : grid::AttributeValue::NOT_SET;
			return S_OK;

		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MAX_WS:
			*attribute = found->m_MaxWS;
			*attribute_valid = (found->m_bits & TEMPORAL_BITS_WIND_EFFECTIVE) ? grid::AttributeValue::SET : grid::AttributeValue::NOT_SET;
			return S_OK;

		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START:
			*attribute = found->m_startTime.GetTotalSeconds();
			*attribute_valid = (found->m_bits & TEMPORAL_BITS_STIME_EFFECTIVE) ? grid::AttributeValue::SET : grid::AttributeValue::NOT_SET;
			return S_OK;

		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END:
			*attribute = found->m_endTime.GetTotalSeconds();
			*attribute_valid = (found->m_bits & TEMPORAL_BITS_ETIME_EFFECTIVE) ? grid::AttributeValue::SET : grid::AttributeValue::NOT_SET;
			return S_OK;

		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START_INTERPRET:
			*attribute = found->m_startTimeRelative;
			*attribute_valid = (found->m_bits & TEMPORAL_BITS_STIME_EFFECTIVE) ? grid::AttributeValue::SET : grid::AttributeValue::NOT_SET;
			return S_OK;

		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END_INTERPRET:
			*attribute = found->m_endTimeRelative;
			*attribute_valid = (found->m_bits & TEMPORAL_BITS_ETIME_EFFECTIVE) ? grid::AttributeValue::SET : grid::AttributeValue::NOT_SET;
			return S_OK;

		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START_COMPUTED:
		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END_COMPUTED:
			{
				WTime localday(found->m_localStartTime);
				WTime daystart(localday, WTIME_FORMAT_AS_LOCAL | WTIME_FORMAT_WITHDST, -1);
				WTime start(found->m_localStartTime), end(found->m_localStartTime);
				bool s_effective, e_effective;
				calculatedTimes(found, pt, start, s_effective, end, e_effective);
				if (option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START_COMPUTED) {
					*attribute = (start - daystart).GetTotalSeconds();
					*attribute_valid = s_effective ? grid::AttributeValue::SET : grid::AttributeValue::NOT_SET;
				}
				else {
					*attribute = (end - daystart).GetTotalSeconds();
					*attribute_valid = e_effective ? grid::AttributeValue::SET : grid::AttributeValue::NOT_SET;
				}
				return S_OK;
			}
		}
	} else if (sfound) {
		switch (option) {
		case CWFGM_SCENARIO_OPTION_GRASSPHENOLOGY:
		case CWFGM_SCENARIO_OPTION_GREENUP:
			if (SUCCEEDED(hr = gridEngine->GetAttributeData(layerThread, pt, time, timeSpan, option, optionFlags, attribute, attribute_valid, cache_bbox)))
				return hr;
			*attribute = (sfound->m_optionFlags & (1 << option)) ? true : false;
			*attribute_valid = (sfound->m_optionFlagsSet & (1 << option)) ? grid::AttributeValue::SET : grid::AttributeValue::NOT_SET;
			return S_OK;

		case FUELCOM_ATTRIBUTE_CURINGDEGREE:
			if (SUCCEEDED(hr = gridEngine->GetAttributeData(layerThread, pt, time, timeSpan, option, optionFlags, attribute, attribute_valid, cache_bbox)))
				return hr;
			*attribute = sfound->m_curingDegree;
			*attribute_valid = (sfound->m_bits & TEMPORAL_BITS_CURING_DEGREE_EFFECTIVE) ? grid::AttributeValue::SET : grid::AttributeValue::NOT_SET;
			return S_OK;
		}
	}
	return gridEngine->GetAttributeData(layerThread, pt, time, timeSpan, option, optionFlags, attribute, attribute_valid, cache_bbox);
}


HRESULT CCWFGM_TemporalAttributeFilter::GetAttributeDataArray(Layer * layerThread, const XY_Point &min_pt, const XY_Point &max_pt, double scale, const WTime &time, const HSS_Time::WTimeSpan& timeSpan,
    std::uint16_t option, std::uint64_t optionFlags, NumericVariant_2d *attribute, attribute_t_2d *attribute_valid) {
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine(layerThread);
	if (!gridEngine)							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

	return gridEngine->GetAttributeDataArray(layerThread, min_pt, max_pt, scale, time, timeSpan, option, optionFlags, attribute, attribute_valid);
}


HRESULT CCWFGM_TemporalAttributeFilter::Clone(boost::intrusive_ptr<ICWFGM_CommonBase> *newObject) const {
	if (!newObject)						return E_POINTER;

	CRWThreadSemaphoreEngage engage(*(CRWThreadSemaphore *)&m_lock, SEM_FALSE);

	try {
		CCWFGM_TemporalAttributeFilter *f = new CCWFGM_TemporalAttributeFilter(*this);
		*newObject = f;
		return S_OK;
	}
	catch (std::exception &e) {
	}
	return E_FAIL;
}


HRESULT CCWFGM_TemporalAttributeFilter::GetAttribute(Layer *layerThread, std::uint16_t option,  /*unsigned*/ PolymorphicAttribute *value) {
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
HRESULT CCWFGM_TemporalAttributeFilter::SetAttribute(unsigned short /*option*/, const PolymorphicAttribute& /*value*/) {
	return E_NOTIMPL;
}


HRESULT CCWFGM_TemporalAttributeFilter::GetOptionKey(unsigned short option, const WTimeVariant &start_time, PolymorphicAttribute *value, /*out]*/bool *value_valid) {
	if (!value)
		return E_POINTER;
	if (!value_valid)
		return E_POINTER;

	DailyAttribute *found = NULL;
	SeasonalAttribute *sfound = NULL;

	if ((option == CWFGM_SCENARIO_OPTION_GRASSPHENOLOGY) ||
		(option == CWFGM_SCENARIO_OPTION_GREENUP) ||
		(option == FUELCOM_ATTRIBUTE_CURINGDEGREE)) {
		try {
			WTimeSpan ws = std::get<WTimeSpan>(start_time);
			if (ws.GetTotalMicroSeconds() == (INTNM::int64_t)-1)
				return E_INVALIDARG;
			sfound = findSeasonOption(ws, option, false, false);
		}
		catch (std::bad_variant_access &) {
			weak_assert(false);
			return E_FAIL;
		}
	} else if ((option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_RH) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_FWI) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_ISI) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MAX_WS) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START_INTERPRET) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END_INTERPRET) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START_COMPUTED) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END_COMPUTED)) {
		try {
			WTime wt = std::get<WTime>(start_time);
			if (!wt.IsValid())
				return E_INVALIDARG;
			WTime _time(wt, m_timeManager);
			found = findOption(_time, false);
		}
		catch (std::bad_variant_access &) {
			weak_assert(false);
			return E_FAIL;
		}
	}

	if (found) {
		switch (option) {
		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_RH:
			*value = found->m_MinRH;
			*value_valid = (found->m_bits & TEMPORAL_BITS_RH_EFFECTIVE) ? true : false;
			break;

		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_FWI:
			*value = found->m_MinFWI;
			*value_valid = (found->m_bits & TEMPORAL_BITS_FWI_EFFECTIVE) ? true : false;
			break;

		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_ISI:
			*value = found->m_MinISI;
			*value_valid = (found->m_bits & TEMPORAL_BITS_ISI_EFFECTIVE) ? true : false;
			break;

		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MAX_WS:
			*value = found->m_MaxWS;
			*value_valid = (found->m_bits & TEMPORAL_BITS_WIND_EFFECTIVE) ? true : false;
			break;

		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START:
			*value = found->m_startTime;
			*value_valid = (found->m_bits & TEMPORAL_BITS_STIME_EFFECTIVE) ? true : false;
			break;

		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END:
			*value = found->m_endTime;
			*value_valid = (found->m_bits & TEMPORAL_BITS_ETIME_EFFECTIVE) ? true : false;
			break;

		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START_INTERPRET:
			*value = (std::uint16_t)found->m_startTimeRelative;
			*value_valid = (found->m_bits & TEMPORAL_BITS_STIME_EFFECTIVE) ? true : false;
			return S_OK;

		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END_INTERPRET:
			*value = (std::uint16_t)found->m_endTimeRelative;
			*value_valid = (found->m_bits & TEMPORAL_BITS_ETIME_EFFECTIVE) ? true : false;
			return S_OK;

		default:
			return E_INVALIDARG;
		}
	} else if (sfound) {
		switch (option) {
		case CWFGM_SCENARIO_OPTION_GRASSPHENOLOGY:
		case CWFGM_SCENARIO_OPTION_GREENUP:
			*value = (sfound->m_optionFlags & (1 << option)) ? true : false;
			*value_valid = (sfound->m_optionFlagsSet & (1 << option)) ? 1 : 0;
			return S_OK;

		case FUELCOM_ATTRIBUTE_CURINGDEGREE:
			*value = sfound->m_curingDegree;
			*value_valid = (sfound->m_bits & TEMPORAL_BITS_CURING_DEGREE_EFFECTIVE) ? true : false;
			return S_OK;

		default:
			return E_INVALIDARG;
		}
	} else
		return ERROR_BURNCONDITION_INVALID_TIME;

	return S_OK;
}


HRESULT CCWFGM_TemporalAttributeFilter::SetOptionKey(unsigned short option, const WTimeVariant &start_time, const PolymorphicAttribute &value, bool value_valid) {
	DailyAttribute *found = NULL;
	SeasonalAttribute *sfound = NULL;

	if ((option == CWFGM_SCENARIO_OPTION_GRASSPHENOLOGY) ||
		(option == CWFGM_SCENARIO_OPTION_GREENUP) ||
		(option == FUELCOM_ATTRIBUTE_CURINGDEGREE)) {
		try {
			WTimeSpan ws = std::get<WTimeSpan>(start_time);
			if (ws.GetTotalMicroSeconds() == (INTNM::int64_t)-1)
				return E_INVALIDARG;
			sfound = findSeasonOption(ws, option, true, false);
		}
		catch (std::bad_variant_access &) {
			weak_assert(false);
			return E_FAIL;
		}
	} else if ((option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_RH) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_FWI) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_ISI) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MAX_WS) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START_INTERPRET) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END_INTERPRET) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START_COMPUTED) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END_COMPUTED)) {
		try {
			WTime wt = std::get<WTime>(start_time);
			if (!wt.IsValid())
				return E_INVALIDARG;
			WTime _time(wt, m_timeManager);
			found = findOption(_time, true);
		}
		catch (std::bad_variant_access &) {
			weak_assert(false);
			return E_FAIL;
		}
	}

	double dval;
	WTimeSpan lval;
	HRESULT hr;
	bool bValue;
	std::uint16_t uval;

	if (found) {
		switch (option) {
		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_RH:
			if (FAILED(hr = VariantToDouble_(value, &dval)))
				return hr;
			if ((dval < 0.0) || (dval > 1.0))
				return E_INVALIDARG;
			found->m_MinRH = dval;
			if (value_valid)
				found->m_bits |= TEMPORAL_BITS_RH_EFFECTIVE;
			else
				found->m_bits &= (~(TEMPORAL_BITS_RH_EFFECTIVE));
			break;

		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_FWI:
			if (FAILED(hr = VariantToDouble_(value, &dval)))
				return hr;
			if (dval < 0.0)
				return E_INVALIDARG;
			found->m_MinFWI = dval;
			if (value_valid)
				found->m_bits |= TEMPORAL_BITS_FWI_EFFECTIVE;
			else
				found->m_bits &= (~(TEMPORAL_BITS_FWI_EFFECTIVE));
			break;

		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_ISI:
			if (FAILED(hr = VariantToDouble_(value, &dval)))
				return hr;
			if (dval < 0.0)
				return E_INVALIDARG;
			found->m_MinISI = dval;
			if (value_valid)
				found->m_bits |= TEMPORAL_BITS_ISI_EFFECTIVE;
			else
				found->m_bits &= (~(TEMPORAL_BITS_ISI_EFFECTIVE));
			break;

		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MAX_WS:
			if (FAILED(hr = VariantToDouble_(value, &dval)))
				return hr;
			if ((dval < 0.0) || (dval > 200.0))
				return E_INVALIDARG;
			found->m_MaxWS = dval;
			if (value_valid)
				found->m_bits |= TEMPORAL_BITS_WIND_EFFECTIVE;
			else
				found->m_bits &= (~(TEMPORAL_BITS_WIND_EFFECTIVE));
			break;

		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START:
			if (FAILED(hr = VariantToTimeSpan_(value, &lval)))
				return hr;
			if ((lval.GetTotalSeconds() < 0) || (lval.GetTotalSeconds() >= (24 * 60 * 60)))
				return E_INVALIDARG;
			found->m_startTime = WTimeSpan(lval);
			if (value_valid)
				found->m_bits |= TEMPORAL_BITS_STIME_EFFECTIVE;
			else
				found->m_bits &= (~(TEMPORAL_BITS_STIME_EFFECTIVE));
			break;

		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END:
			if (FAILED(hr = VariantToTimeSpan_(value, &lval)))
				return hr;
			if ((lval.GetTotalSeconds() < 0) || (lval.GetTotalSeconds() >= (36 * 60 * 60)))
				return E_INVALIDARG;
			found->m_endTime = WTimeSpan(lval);
			if (value_valid)
				found->m_bits |= TEMPORAL_BITS_ETIME_EFFECTIVE;
			else
				found->m_bits &= (~(TEMPORAL_BITS_ETIME_EFFECTIVE));
			break;

		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START_INTERPRET:
			if (FAILED(hr = VariantToUInt16_(value, &uval)))
				return hr;
			if (uval >= 3)
				return E_INVALIDARG;
			found->m_startTimeRelative = (::WISE::GridProto::TemporalCondition_DailyAttribute_TimeRelative)uval;
			break;

		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END_INTERPRET:
			if (FAILED(hr = VariantToUInt16_(value, &uval)))
				return hr;
			if (uval >= 3)
				return E_INVALIDARG;
			found->m_endTimeRelative = (::WISE::GridProto::TemporalCondition_DailyAttribute_TimeRelative)uval;
			break;

		default:
			return E_INVALIDARG;
		}
	} else if (sfound) {
		switch (option) {
		case CWFGM_SCENARIO_OPTION_GRASSPHENOLOGY:
		case CWFGM_SCENARIO_OPTION_GREENUP:
			if (FAILED(hr = VariantToBoolean_(value, &bValue)))
				return hr;
			if (bValue)
				sfound->m_optionFlags |= (1 << option);
			else
				sfound->m_optionFlags &= (~(1 << option));
			if (value_valid)
				sfound->m_optionFlagsSet |= (1 << option);
			else
				sfound->m_optionFlagsSet &= (~(1 << option));
			return S_OK;

		case FUELCOM_ATTRIBUTE_CURINGDEGREE:
			if (FAILED(hr = VariantToDouble_(value, &dval)))
				return hr;
			if ((dval < 0.0) || (dval > 100.0))
				return E_INVALIDARG;
			sfound->m_curingDegree = dval;
			if (value_valid)
				sfound->m_bits |= TEMPORAL_BITS_CURING_DEGREE_EFFECTIVE;
			else
				sfound->m_bits &= (~(TEMPORAL_BITS_CURING_DEGREE_EFFECTIVE));
			return S_OK;

		default:	
			return E_INVALIDARG;
		}
	} else
		return ERROR_BURNCONDITION_INVALID_TIME;

	m_bRequiresSave = TRUE;
	return S_OK;
}


HRESULT CCWFGM_TemporalAttributeFilter::ClearOptionKey(unsigned short option, const WTimeVariant &start_time) {
	DailyAttribute *found = NULL;
	SeasonalAttribute *sfound = NULL;

	if ((option == CWFGM_SCENARIO_OPTION_GRASSPHENOLOGY) ||
		(option == CWFGM_SCENARIO_OPTION_GREENUP) ||
		(option == FUELCOM_ATTRIBUTE_CURINGDEGREE)) {
		try {
			WTimeSpan ws = std::get<WTimeSpan>(start_time);
			if (ws.GetTotalMicroSeconds() == (INTNM::int64_t)-1)
				return E_INVALIDARG;
			sfound = findSeasonOption(ws, option, false, false);
		}
		catch (std::bad_variant_access &) {
			weak_assert(false);
			return E_FAIL;
		}
	} else if ((option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_RH) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_FWI) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_ISI) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MAX_WS) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START_INTERPRET) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END_INTERPRET)) {
		try {
			WTime wt = std::get<WTime>(start_time);
			if (!wt.IsValid())
				return E_INVALIDARG;
			WTime _time(wt, m_timeManager);
			found = findOption(_time, false);
		}
		catch (std::bad_variant_access &) {
			weak_assert(false);
			return E_FAIL;
		}
	}

	if (found) {
		switch (option) {
		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_RH:					found->m_bits &= (~(TEMPORAL_BITS_RH_EFFECTIVE)); break;
		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_FWI:					found->m_bits &= (~(TEMPORAL_BITS_FWI_EFFECTIVE)); break;
		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_ISI:					found->m_bits &= (~(TEMPORAL_BITS_ISI_EFFECTIVE)); break;
		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MAX_WS:					found->m_bits &= (~(TEMPORAL_BITS_WIND_EFFECTIVE)); break;
		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START:			found->m_bits &= (~(TEMPORAL_BITS_STIME_EFFECTIVE)); break;
		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END:				found->m_bits &= (~(TEMPORAL_BITS_ETIME_EFFECTIVE)); break;
		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START_INTERPRET:	found->m_startTimeRelative = ::WISE::GridProto::TemporalCondition_DailyAttribute_TimeRelative::TemporalCondition_DailyAttribute_TimeRelative_LOCAL_MIDNIGHT; break;
		case CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END_INTERPRET:	found->m_startTimeRelative = ::WISE::GridProto::TemporalCondition_DailyAttribute_TimeRelative::TemporalCondition_DailyAttribute_TimeRelative_LOCAL_MIDNIGHT; break;
		default:												return E_INVALIDARG;
		}
	} else if (sfound) {
		switch (option) {
		case CWFGM_SCENARIO_OPTION_GRASSPHENOLOGY:
		case CWFGM_SCENARIO_OPTION_GREENUP:
			sfound->m_optionFlagsSet &= (~(1 << option));
			return S_OK;

		case FUELCOM_ATTRIBUTE_CURINGDEGREE:
			sfound->m_bits &= (~(TEMPORAL_BITS_CURING_DEGREE_EFFECTIVE));
			return S_OK;

		default:
			return E_INVALIDARG;
		}

		if ((!sfound->m_bits) && (!sfound->m_optionFlagsSet)) {
			m_conditions.Remove(sfound);
			delete sfound;
		}
	} else
		return ERROR_BURNCONDITION_INVALID_TIME;

	m_bRequiresSave = TRUE;
	return S_OK;
}


SeasonalAttribute *CCWFGM_TemporalAttributeFilter::findSeasonOption(const WTimeSpan &wtime, unsigned short option, bool autocreate, bool accept_earlier) {
	weak_assert(wtime <= WTimeSpan(366 * 24 * 60 * 60));

	SeasonalAttribute *found = nullptr, *lastfound = nullptr;
	WTimeSpan wtime2(wtime);
	wtime2.PurgeToDay();

	SeasonalAttribute *ao = m_conditions.LH_Head();
	while (ao->LN_Succ()) {
		if ((ao->m_localStartDate == wtime) || (ao->m_localStartDate == wtime2)) {
			found = ao;
			break;
		}

		if (ao->m_localStartDate > wtime2) {

#ifdef _DEBUG
			while (ao->LN_Succ()) {
				weak_assert(ao->m_localStartDate != wtime2);
				ao = ao->LN_Succ();
			}
#endif

			break;
		}

		if ((option == CWFGM_SCENARIO_OPTION_GRASSPHENOLOGY) || (option == CWFGM_SCENARIO_OPTION_GREENUP)) {
			if (ao->m_optionFlagsSet & (1 << option))
				lastfound = ao;
		} else if (option == FUELCOM_ATTRIBUTE_CURINGDEGREE) {
			if (ao->m_bits & TEMPORAL_BITS_CURING_DEGREE_EFFECTIVE)
				lastfound = ao;
		}

		ao = ao->LN_Succ();
	}

	if (found) {
		if (autocreate)
			return found;
		if ((option == CWFGM_SCENARIO_OPTION_GRASSPHENOLOGY) || (option == CWFGM_SCENARIO_OPTION_GREENUP))
			if (found->m_optionFlagsSet & (1 << option))
				return found;
		if (option == FUELCOM_ATTRIBUTE_CURINGDEGREE)
			if (found->m_bits & TEMPORAL_BITS_CURING_DEGREE_EFFECTIVE)
				return found;		// make sure that this entry has data set in there that we want
	}

	if ((accept_earlier) && (lastfound))
		return lastfound;

	if (autocreate) {
		found = new SeasonalAttribute();
		found->m_localStartDate = wtime2;

		SeasonalAttribute *_where = m_conditions.LH_Head();
		while (_where->LN_Succ()) {
			if (_where->m_localStartDate > found->m_localStartDate)
				break;
			_where = _where->LN_Succ();
		}
		m_conditions.Insert(found, _where->LN_Pred());
	}

	return found;
}


DailyAttribute *CCWFGM_TemporalAttributeFilter::findOption(const WTime &_time, bool autocreate) {
	DailyAttribute *found = nullptr;

	if ((m_timeManager->m_worldLocation.m_latitude() == 1000.0) && (m_rootEngine)) {
		weak_assert(false);
		SetWorldLocation(m_rootEngine.get(), nullptr);
	}

	WTime time(0ULL, m_timeManager);
	WTime _default((std::uint64_t)(-1), m_timeManager, false);
	if (_time == _default) {
		weak_assert(false);
		time.SetTime(_time);
	} else {
		WTime stime(_time);
		WTime lstime(stime, WTIME_FORMAT_AS_LOCAL | WTIME_FORMAT_WITHDST, 1);
		lstime.PurgeToDay(0);

#ifdef _DEBUG
		std::string stimet = stime.ToString(WTIME_FORMAT_STRING_ISO8601);
		std::string lstimet = lstime.ToString(WTIME_FORMAT_STRING_ISO8601);
#endif

		time.SetTime(lstime);
	}

	DailyAttribute *ao = m_attributes.LH_Head();
	while (ao->LN_Succ()) {
		if (ao->m_localStartTime == time) {
			found = ao;
			break;
		}
		if (ao->m_localStartTime > time) {

#ifdef _DEBUG
			while (ao->LN_Succ()) {
				weak_assert(ao->m_localStartTime != time);
				ao = ao->LN_Succ();
			}
#endif

			break;
		}
		ao = ao->LN_Succ();
	}

	if ((!found) && (autocreate)) {
		found = new DailyAttribute(m_timeManager);
		found->m_localStartTime.SetTime(time);

		DailyAttribute *_where = m_attributes.LH_Head();
		while (_where->LN_Succ()) {
			if (_where->m_localStartTime > found->m_localStartTime)
				break;
			_where = _where->LN_Succ();
		}
		m_attributes.Insert(found, _where->LN_Pred());
	}

	return found;
}


HRESULT CCWFGM_TemporalAttributeFilter::Count(unsigned short option, /*bool include_default,*/ unsigned short *count) {
	if (!count)
		return E_POINTER;

	if ((option == CWFGM_SCENARIO_OPTION_GRASSPHENOLOGY) ||
		(option == CWFGM_SCENARIO_OPTION_GREENUP) ||
		(option == FUELCOM_ATTRIBUTE_CURINGDEGREE)) {
		*count = m_conditions.GetCount();
	} else if ((option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_RH) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_FWI) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_ISI) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MAX_WS) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START_INTERPRET) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END_INTERPRET) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START_COMPUTED) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END_COMPUTED)) {
		*count = m_attributes.GetCount();
	} else
		*count = 0;
	return S_OK;
}


HRESULT CCWFGM_TemporalAttributeFilter::TimeAtIndex(unsigned short option, /*[int]*/unsigned short index, WTimeVariant *start_time) {
	if (!start_time)
		return E_POINTER;

	if ((option == CWFGM_SCENARIO_OPTION_GRASSPHENOLOGY) ||
		(option == CWFGM_SCENARIO_OPTION_GREENUP) ||
		(option == FUELCOM_ATTRIBUTE_CURINGDEGREE)) {
		SeasonalAttribute *sa = m_conditions.IndexNode(index);

		if (!sa)
			return E_INVALIDARG;

		*start_time = sa->m_localStartDate;
	} else if ((option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_RH) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_FWI) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_ISI) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MAX_WS) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START_INTERPRET) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END_INTERPRET) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START_COMPUTED) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END_COMPUTED)) {
		DailyAttribute *oa = m_attributes.IndexNode(index);

		if (!oa)
			return E_INVALIDARG;
		if (oa->m_localStartTime.IsValid()) {
			WTime st(oa->m_localStartTime, m_timeManager);
			WTime stime(st, WTIME_FORMAT_AS_LOCAL | WTIME_FORMAT_WITHDST, -1);
			*start_time = stime;

#ifdef _DEBUG
			std::string thetime = stime.ToString(WTIME_FORMAT_STRING_ISO8601);
#endif

		} else {
			weak_assert(false);
			WTime _default((std::uint64_t)(-1), m_timeManager, false);
			*start_time = _default;
		}
	} else
		return E_INVALIDARG;
	return S_OK;
}


HRESULT CCWFGM_TemporalAttributeFilter::DeleteIndex(unsigned short option, /*[int]*/unsigned short index) {
	if ((option == CWFGM_SCENARIO_OPTION_GRASSPHENOLOGY) ||
		(option == CWFGM_SCENARIO_OPTION_GREENUP) ||
		(option == FUELCOM_ATTRIBUTE_CURINGDEGREE)) {
		SeasonalAttribute *sa = m_conditions.IndexNode(index);

		if (!sa)
			return E_INVALIDARG;

		m_conditions.Remove(sa);
		delete sa;
	} else if ((option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_RH) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_FWI) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_ISI) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MAX_WS) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START_INTERPRET) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END_INTERPRET) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START_COMPUTED) ||
		(option == CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END_COMPUTED)) {
		if (index >= m_attributes.GetCount())
			return E_INVALIDARG;

		DailyAttribute *oa = m_attributes.IndexNode(index);
		if (!oa)
			return E_INVALIDARG;
		m_attributes.Remove(oa);
		delete oa;
	} else
		return E_INVALIDARG;
	m_bRequiresSave = TRUE;
	return S_OK;
}
