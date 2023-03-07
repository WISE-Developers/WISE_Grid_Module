/**
 * WISE_Grid_Module: CWFGM_TemporalAttributeFilter.Serialize.cpp
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

#include <algorithm>
#include "google/protobuf/message.h"
#include "GridCOM.h"
#include "CWFGM_TemporalAttributeFilter.h"
#include "lines.h"
#include "GridCom_ext.h"
#include "FireEngine_ext.h"
#include <errno.h>
#include <stdio.h>
#include "CoordinateConverter.h"
#include "GDALExporter.h"
#include "GDALImporter.h"
#include "doubleBuilder.h"
#include "str_printf.h"
#include <ctime>


std::int32_t CCWFGM_TemporalAttributeFilter::serialVersionUid(const SerializeProtoOptions& options) const noexcept {
	return options.fileVersion() + 1;
}


WISE::GridProto::TemporalCondition* CCWFGM_TemporalAttributeFilter::serialize(const SerializeProtoOptions& options) {
	auto tempo = new WISE::GridProto::TemporalCondition();
	tempo->set_version(serialVersionUid(options));

	DailyAttribute* da = m_attributes.LH_Head();
	while (da->LN_Succ())
	{
		auto daily = tempo->add_daily();
		daily->set_version(1);
		HSS_Time::WTime localStartTime(da->m_localStartTime, nullptr);
		daily->set_allocated_localstarttime(HSS_Time::Serialization::TimeSerializer().serializeTime(localStartTime, options.fileVersion()));
		daily->set_allocated_starttime(HSS_Time::Serialization::TimeSerializer().serializeTimeSpan(da->m_startTime));
		if (da->m_startTimeRelative)
		daily->set_localstarttimerelative(da->m_startTimeRelative);
		daily->set_allocated_endtime(HSS_Time::Serialization::TimeSerializer().serializeTimeSpan(da->m_endTime));
		if (da->m_endTimeRelative)
		daily->set_localendtimerelative(da->m_endTimeRelative);
		daily->set_allocated_minrh(DoubleBuilder().withValue(da->m_MinRH * 100.0).forProtobuf(options.useVerboseFloats()));
		daily->set_allocated_maxws(DoubleBuilder().withValue(da->m_MaxWS).forProtobuf(options.useVerboseFloats()));
		daily->set_allocated_minfwi(DoubleBuilder().withValue(da->m_MinFWI).forProtobuf(options.useVerboseFloats()));
		daily->set_allocated_minisi(DoubleBuilder().withValue(da->m_MinISI).forProtobuf(options.useVerboseFloats()));

		da = da->LN_Succ();
	}

	SeasonalAttribute* sa = m_conditions.LH_Head();
	while (sa->LN_Succ())
	{
		auto seasonal = tempo->add_seasonal();
		seasonal->set_version(1);
		seasonal->set_allocated_localstarttime(HSS_Time::Serialization::TimeSerializer().serializeTimeSpan(sa->m_localStartDate));

		if (sa->m_optionFlagsSet & (1 << CWFGM_SCENARIO_OPTION_GRASSPHENOLOGY)) {
			auto att = seasonal->add_attributes();
			att->set_active((sa->m_optionFlags & (1 << CWFGM_SCENARIO_OPTION_GRASSPHENOLOGY)) ? true : false);
			att->set_type(WISE::GridProto::TemporalCondition_SeasonalAttribute_EffectiveAttribute_Type::TemporalCondition_SeasonalAttribute_EffectiveAttribute_Type_GRASS_PHENOLOGY);
		}

		if (sa->m_optionFlagsSet & (1 << CWFGM_SCENARIO_OPTION_GREENUP)) {
			auto att = seasonal->add_attributes();
			att->set_active((sa->m_optionFlags & (1 << CWFGM_SCENARIO_OPTION_GREENUP)) ? true : false);
			att->set_type(WISE::GridProto::TemporalCondition_SeasonalAttribute_EffectiveAttribute_Type::TemporalCondition_SeasonalAttribute_EffectiveAttribute_Type_GREENUP);
		}

		if (sa->m_bits & TEMPORAL_BITS_CURING_DEGREE_EFFECTIVE) {
			auto att = seasonal->add_attributes();
			att->set_active(true);
			att->set_allocated_value(DoubleBuilder().withValue(sa->m_curingDegree).forProtobuf(options.useVerboseFloats()));
			att->set_type(WISE::GridProto::TemporalCondition_SeasonalAttribute_EffectiveAttribute_Type::TemporalCondition_SeasonalAttribute_EffectiveAttribute_Type_CURING_DEGREE);
		}

		sa = sa->LN_Succ();
	}

	return tempo;
}


CCWFGM_TemporalAttributeFilter *CCWFGM_TemporalAttributeFilter::deserialize(const google::protobuf::Message& proto, std::shared_ptr<validation::validation_object> valid, const std::string& name) {
	auto tempo = dynamic_cast_assert<const WISE::GridProto::TemporalCondition*>(&proto);
	WTimeSpan w;
	double dVal;

	if (!tempo) {
		if (valid)
			/// <summary>
			/// The object passed as a grid filter is invalid. An incorrect object type was passed to the parser.
			/// </summary>
			/// <type>internal</type>
			valid->add_child_validation("WISE.GridProto.TemporalCondition", name, validation::error_level::SEVERE, validation::id::object_invalid, proto.GetDescriptor()->name());
		weak_assert(false);
		m_loadWarning = "Error: WISE.GridProto.CwfgmTemporalAttributeFilter: Protobuf object invalid";
		throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmTemporalAttributeFilter: Protobuf object invalid", ERROR_PROTOBUF_OBJECT_INVALID);
	}
	if ((tempo->version() != 1) &&
		(tempo->version() != 2) &&
		(tempo->version() != 3)) {
		if (valid)
			/// <summary>
			/// The object version is not supported. The asset is not supported by this version of Prometheus.
			/// </summary>
			/// <type>user</type>
			valid->add_child_validation("WISE.GridProto.TemporalCondition", name, validation::error_level::SEVERE, validation::id::version_mismatch, std::to_string(tempo->version()));
		weak_assert(false);
		m_loadWarning = "Error: WISE.GridProto.CwfgmTemporalAttributeFilter: Version is invalid";
		throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmTemporalAttributeFilter: Version is invalid", ERROR_PROTOBUF_OBJECT_VERSION_INVALID);
	}
	if (tempo->version() != 3) {
		if (valid)
			/// <summary>
			/// The object version is out of date, but still supported by this version of Prometheus.
			/// </summary>
			/// <type>user</type>
			valid->add_child_validation("WISE.GridProto.TemporalCondition", name, validation::error_level::INFORMATION, validation::id::version_not_current, std::to_string(tempo->version()));
	}

	auto vt = validation::conditional_make_object(valid, "WISE.GridProto.TemporalCondition", name);
	auto v = vt.lock();

	int i;
	for (i = 0; i < tempo->daily_size(); i++)
	{
		auto vt2 = validation::conditional_make_object(v, "WISE.GridProto.TemporalCondition.DailyAttribute", strprintf("daily[%d]", i));
		auto v2 = vt2.lock();
		auto daily = tempo->daily(i);
		DailyAttribute* da = new DailyAttribute(m_timeManager);
		auto time = HSS_Time::Serialization::TimeSerializer().deserializeTime(daily.localstarttime(), nullptr, v, "localStartTime");
		WTime daystart(*time, m_timeManager);

#ifdef _DEBUG
		std::string atime = daystart.ToString(WTIME_FORMAT_STRING_ISO8601);
#endif

		delete time;

		if ((daystart < WTime::GlobalMin(m_timeManager)) || (daystart > WTime::GlobalMax(m_timeManager)))
		{
			m_loadWarning = "Error: WISE.GridProto.TemporalCondition.DailyAttribute.localStartTime: Invalid start date";
			if (v2)
				/// <summary>
				/// The start date is out of range or invalid.
				/// </summary>
				/// <type>user</type>
				v2->add_child_validation("HSS.Times.WTime", "dayStart", validation::error_level::WARNING, validation::id::time_invalid, daystart.ToString(WTIME_FORMAT_STRING_ISO8601), { true, WTime::GlobalMin().ToString(WTIME_FORMAT_STRING_ISO8601) }, { true, WTime::GlobalMax().ToString(WTIME_FORMAT_STRING_ISO8601) });
			else {
				delete da;
				throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmTemporalAttributeFilter: Invalid start date");
			}
		}
		if (daystart.GetMicroSeconds(WTIME_FORMAT_AS_LOCAL | WTIME_FORMAT_WITHDST)) {
			m_loadWarning += "Warning: fractions of seconds on the start date will be purged to the start of the second.";
			if (v)
				/// <summary>
				/// The start date contains microseconds.
				/// </summary>
				/// <type>user</type>
				v->add_child_validation("HSS.Times.WTime", "dayStart", validation::error_level::INFORMATION, validation::id::time_invalid, daystart.ToString(WTIME_FORMAT_STRING_ISO8601), "Fractions of seconds will be purged.");
			daystart.PurgeToSecond(WTIME_FORMAT_AS_LOCAL | WTIME_FORMAT_WITHDST);
		}

		da->m_localStartTime = daystart;

		auto timespan = HSS_Time::Serialization::TimeSerializer().deserializeTimeSpan(daily.starttime(), v2, "startTime");
		if (timespan) {
			w = WTimeSpan(*timespan);
			if (w.GetTotalMicroSeconds() == (INTNM::int64_t)-1) {
				m_loadWarning = "Error: WISE.GridProto.TemporalCondition.DailyAttribute.startTime: Invalid start time";
				if (v2)
					v2->add_child_validation("HSS_Times.WTimeSpan", "startTime", validation::error_level::WARNING, validation::id::parse_invalid, daily.starttime().time());
				else {
					delete da;
					throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmTemporalAttributeFilter: Invalid start time");
				}
			}
			else if (w.GetMicroSeconds()) {
				m_loadWarning = "Error: WISE.GridProto.TemporalCondition.DailyAttribute.startTime: Invalid start time";
				if (v2)
					v2->add_child_validation("HSS_Times.WTimeSpan", "startTime", validation::error_level::INFORMATION, validation::id::parse_invalid, daily.starttime().time(), "Fractions of seconds will be purged.");
				w.PurgeToSecond();
			}
			if (w < WTimeSpan(-1, 0, 0, 0)) {
				m_loadWarning = "Error: WISE.GridProto.TemporalCondition.DailyAttribute.startTime: Invalid start time";
				if (v2)
					v2->add_child_validation("HSS_Times.WTimeSpan", "startTime", validation::error_level::INFORMATION, validation::id::parse_invalid, daily.endtime().time(), "Start time prior to -24 hours will be purged.");
				w = WTimeSpan(-1, 0, 0, 0);
			}
			else if (w > WTimeSpan(2, 0, 0, 0)) {
				m_loadWarning = "Error: WISE.GridProto.TemporalCondition.DailyAttribute.startTime: Invalid start time";
				if (v2)
					v2->add_child_validation("HSS_Times.WTimeSpan", "startTime", validation::error_level::INFORMATION, validation::id::parse_invalid, daily.endtime().time(), "Start time past 24 hours will be purged.");
				w = WTimeSpan(2, 0, 0, 0);
			}
			da->m_startTime = w;
			da->m_bits |= TEMPORAL_BITS_STIME_EFFECTIVE;
			delete timespan;
		}

		if (daily.has_localstarttimerelative())
			da->m_startTimeRelative = daily.localstarttimerelative();

		timespan = HSS_Time::Serialization::TimeSerializer().deserializeTimeSpan(daily.endtime(), v2, "endTime");
		if (timespan) {
			w = WTimeSpan(*timespan);
			if (w.GetTotalMicroSeconds() == (INTNM::int64_t)-1) {
				m_loadWarning = "Error: WISE.GridProto.TemporalCondition.DailyAttribute.endTime: Invalid end time";
				if (v2)
					v2->add_child_validation("HSS_Times.WTimeSpan", "endTime", validation::error_level::WARNING, validation::id::parse_invalid, daily.endtime().time());
				else {
					delete da;
					throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmTemporalAttributeFilter: Invalid end time");
				}
			}
			else if (w.GetMicroSeconds()) {
				m_loadWarning = "Error: WISE.GridProto.TemporalCondition.DailyAttribute.endTime: Invalid end time";
				if (v2)
					v2->add_child_validation("HSS_Times.WTimeSpan", "endTime", validation::error_level::INFORMATION, validation::id::parse_invalid, daily.endtime().time(), "Fractions of seconds will be purged.");
				w.PurgeToSecond();
			}
			if (w < WTimeSpan(-1, 0, 0, 0)) {
				m_loadWarning = "Error: WISE.GridProto.TemporalCondition.DailyAttribute.endTime: Invalid end time";
				if (v2)
					v2->add_child_validation("HSS_Times.WTimeSpan", "endTime", validation::error_level::INFORMATION, validation::id::parse_invalid, daily.endtime().time(), "End time prior to -24 hours will be purged.");
				w = WTimeSpan(-1, 0, 0, 0);
			}
			else if (w > WTimeSpan(2, 0, 0, 0)) {
				m_loadWarning = "Error: WISE.GridProto.TemporalCondition.DailyAttribute.endTime: Invalid end time";
				if (v2)
					v2->add_child_validation("HSS_Times.WTimeSpan", "endTime", validation::error_level::INFORMATION, validation::id::parse_invalid, daily.endtime().time(), "End time past 48 hours will be purged.");
				w = WTimeSpan(2, 0, 0, 0);
			}
			da->m_endTime = w;
			da->m_bits |= TEMPORAL_BITS_ETIME_EFFECTIVE;
			delete timespan;
		}

		if (daily.has_localendtimerelative())
			da->m_endTimeRelative = daily.localendtimerelative();

		if (daily.has_minrh()) 
		{
			dVal = DoubleBuilder().withProtobuf(daily.minrh(), v2, "minRh").getValue();

			if (tempo->version() < 2) {
				if ((dVal < 0.0) || (dVal > 1.0))
				{
					m_loadWarning = "Error: WISE.GridProto.CwfgmTemporalAttributeFilter.DailyAttribute: Invalid min RH value";
					if (v2)
						v2->add_child_validation("Math.Double", "minRh",
							validation::error_level::WARNING, validation::id::parse_invalid, std::to_string(daily.minrh().value()),
							{ true, 0.0 }, { true, 1.0 });
					else {
						delete da;
						throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmTemporalAttributeFilter: Invalid min RH value");
					}
				}
			}
			else {
				if ((dVal < 0.0) || (dVal > 100.0))
				{
					m_loadWarning = "Error: WISE.GridProto.CwfgmTemporalAttributeFilter.DailyAttribute: Invalid min RH value";
					if (v2)
						v2->add_child_validation("Math.Double", "minRh",
							validation::error_level::WARNING, validation::id::parse_invalid, std::to_string(daily.minrh().value()),
							{ true, 0.0 }, { true, 100.0 });
					else {
						delete da;
						throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmTemporalAttributeFilter: Invalid min RH value");
					}
				}
				dVal *= 0.01;			// making the range from 0..100 to be consistent with other places we are storing RH in this FGMJ
			}
			da->m_MinRH = dVal;
			da->m_bits |= TEMPORAL_BITS_RH_EFFECTIVE;
		}
		else
			da->m_MinRH = 1.0;

		if (daily.has_maxws())
		{
			dVal = DoubleBuilder().withProtobuf(daily.maxws(), v2, "maxWs").getValue();

			if ((dVal < 0.0) || (dVal > 200.0)) {
				m_loadWarning = "Error: WISE.GridProto.CwfgmTemporalAttributeFilter.DailyAttribute: Invalid max WS value";
				if (v2)
					v2->add_child_validation("Math.Double", "maxWs",
						validation::error_level::WARNING, validation::id::parse_invalid, std::to_string(daily.maxws().value()),
						{ true, 0.0 }, { true, 200.0 });
				else {
					delete da;
					throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmTemporalAttributeFilter: Invalid max WS value");
				}
			}
			da->m_MaxWS = dVal;
			da->m_bits |= TEMPORAL_BITS_WIND_EFFECTIVE;
		}
		else
			da->m_MaxWS = 0.0;

		if (daily.has_minfwi()) 
		{
			dVal = DoubleBuilder().withProtobuf(daily.minfwi(), v2, "minFwi").getValue();

			if (dVal < 0.0) {
				m_loadWarning = "Error: WISE.GridProto.CwfgmTemporalAttributeFilter.DailyAttribute: Invalid min FWI value";
				if (v2)
					v2->add_child_validation("Math.Double", "minFwi",
						validation::error_level::WARNING, validation::id::parse_invalid, std::to_string(daily.minfwi().value()),
						{ true, 0.0 }, { false, std::numeric_limits<double>::infinity() });
				else {
					delete da;
					throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmTemporalAttributeFilter: Invalid min FWI value");
				}
			}
			da->m_MinFWI = dVal;
			da->m_bits |= TEMPORAL_BITS_FWI_EFFECTIVE;
		}
		else
			da->m_MinFWI = 0.0;

		if (daily.has_minisi()) 
		{
			dVal = DoubleBuilder().withProtobuf(daily.minisi(), v2, "minIsi").getValue();

			if (dVal < 0.0) {
				m_loadWarning = "Error: WISE.GridProto.CwfgmTemporalAttributeFilter.DailyAttribute: Invalid min ISI value";
				if (v2)
					v2->add_child_validation("Math.Double", "minIsi",
						validation::error_level::WARNING, validation::id::parse_invalid, std::to_string(daily.minisi().value()),
						{ true, 0.0 }, { false, std::numeric_limits<double>::infinity() });
				else {
					delete da;
					throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmTemporalAttributeFilter: Invalid min ISI value");
				}
			}
			da->m_MinISI = dVal;
			da->m_bits |= TEMPORAL_BITS_ISI_EFFECTIVE;
		}
		else
			da->m_MinISI = 0.0;

		if (da->m_localStartTime.IsValid()) {
			DailyAttribute* _where = m_attributes.LH_Head();
			while (_where->LN_Succ())
			{
				if (_where->m_localStartTime > da->m_localStartTime)
					break;
				_where = _where->LN_Succ();
			}
			m_attributes.Insert(da, _where->LN_Pred());
		}
		else {
			weak_assert(false);
			delete da;
		}
	}

	SeasonalAttribute* sa;
	while ((sa = m_conditions.RemHead()))
		delete sa;

	for (i = 0; i < tempo->seasonal_size(); i++)
	{
		auto vt2 = validation::conditional_make_object(v, "WISE.GridProto.TemporalCondition.SeasonalAttribute", strprintf("seasonal[%d]", i));
		auto v2 = vt2.lock();
		auto seasonal = tempo->seasonal(i);
		sa = new SeasonalAttribute();
		
		auto timespan = HSS_Time::Serialization::TimeSerializer().deserializeTimeSpan(seasonal.localstarttime(), nullptr, "");
		sa->m_localStartDate = WTimeSpan(*timespan);
		delete timespan;

		if (sa->m_localStartDate.GetTotalSeconds() < 0)
		{
			m_loadWarning = "Error: WISE.GridProto.TemporalCondition.SeasonalAttribute: Invalid start date";
			if (v2)
				v2->add_child_validation("HSS_Times.WTimeSpan", "localStartTime", validation::error_level::WARNING, validation::id::parse_invalid, seasonal.localstarttime().time());
			else {
				delete sa;
				throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmTemporalAttributeFilter: Invalid start date");
			}
		}
		else if (sa->m_localStartDate.GetMicroSeconds()) {
			m_loadWarning = "Error: WISE.GridProto.CwfgmTemporalAttributeFilter.SeasonalAttribute: Invalid start date";
			if (v2)
				v2->add_child_validation("HSS_Times.WTimeSpan", "localStartTime", validation::error_level::INFORMATION, validation::id::parse_invalid, seasonal.localstarttime().time(), "Fractions of seconds will be purged.");
			sa->m_localStartDate.PurgeToSecond();
		}

		for (int ii = 0; ii < seasonal.attributes_size(); ii++) {
			auto att = seasonal.attributes(ii);
			
			if (att.type() == WISE::GridProto::TemporalCondition_SeasonalAttribute_EffectiveAttribute_Type::TemporalCondition_SeasonalAttribute_EffectiveAttribute_Type_GRASS_PHENOLOGY) {
				sa->m_optionFlagsSet |= (1 << CWFGM_SCENARIO_OPTION_GRASSPHENOLOGY);
				if (att.active())
					sa->m_optionFlags |= (1 << CWFGM_SCENARIO_OPTION_GRASSPHENOLOGY);
			}
			else if (att.type() == WISE::GridProto::TemporalCondition_SeasonalAttribute_EffectiveAttribute_Type::TemporalCondition_SeasonalAttribute_EffectiveAttribute_Type_GREENUP) {
				sa->m_optionFlagsSet |= (1 << CWFGM_SCENARIO_OPTION_GREENUP);
				if (att.active())
					sa->m_optionFlags |= (1 << CWFGM_SCENARIO_OPTION_GREENUP);
			} else if (att.type() == WISE::GridProto::TemporalCondition_SeasonalAttribute_EffectiveAttribute_Type::TemporalCondition_SeasonalAttribute_EffectiveAttribute_Type_CURING_DEGREE) {
				sa->m_bits |= TEMPORAL_BITS_CURING_DEGREE_EFFECTIVE;

				dVal = DoubleBuilder().withProtobuf(att.value()).getValue();

				if ((dVal < 0.0) || (dVal > 100.0)) {
					m_loadWarning = "Error: WISE.GridProto.CwfgmTemporalAttributeFilter.SeasonalAttribute: Invalid curing degree value";
					if (valid)
						valid->add_child_validation("Math.Double", "value",
							validation::error_level::WARNING, validation::id::parse_invalid, std::to_string(att.value().value()),
							{ true, 0.0 }, { true, 100.0 }, "%%", "Value, interpretted as curing degree, is out of range.");
					else {
						delete sa;
						throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmTemporalAttributeFilter: Invalid curing degree value");
					}
				}
				sa->m_curingDegree = dVal;
			}
		}
		
		if (sa->m_localStartDate.GetTotalMicroSeconds() != (INTNM::int64_t)-1) {
			SeasonalAttribute* _where = m_conditions.LH_Head();
			bool add = true;
			while (_where->LN_Succ())
			{
				if (_where->m_localStartDate > sa->m_localStartDate)
					break;
				else if (_where->m_localStartDate == sa->m_localStartDate) {
					_where->m_optionFlags |= sa->m_optionFlags;
					_where->m_optionFlagsSet |= sa->m_optionFlagsSet;
					_where->m_bits |= sa->m_bits;
					if (sa->m_bits & TEMPORAL_BITS_CURING_DEGREE_EFFECTIVE)
						_where->m_curingDegree = sa->m_curingDegree;
					delete sa;
					add = false;
				}
				_where = _where->LN_Succ();
			}
			if (add)
				m_conditions.Insert(sa, _where->LN_Pred());
		}
		else {
			weak_assert(false);
			delete sa;
		}
	}

	return this;
}