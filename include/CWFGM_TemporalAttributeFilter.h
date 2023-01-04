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

#ifndef __CWFGM_TEMPORALATTRIBUTEFILTER_H_
#define __CWFGM_TEMPORALATTRIBUTEFILTER_H_

#include "WTime.h"
#include "semaphore.h"
#include "results.h"
#include <map>
#include "ICWFGM_GridEngine.h"
#include "ISerializeProto.h"
#include "objectcache_mt.h"
#include "CoordinateConverter.h"

#include <string>
#include <boost/intrusive_ptr.hpp>
#include <boost/atomic.hpp>
#include <variant>
#include "cwfgmFilter.pb.h"

#ifdef HSS_SHOULD_PRAGMA_PACK
#pragma pack(push, 8)
#endif

using namespace HSS_Time;

class DailyAttribute : public MinNode {
public:
	DEVICE DailyAttribute *LN_Succ() const { return (DailyAttribute *)MinNode::LN_Succ(); };
	DEVICE DailyAttribute *LN_Pred() const { return (DailyAttribute *)MinNode::LN_Pred(); };

	DailyAttribute(WTimeManager *tm);

	WTime			m_localStartTime;
	double			m_MinRH;
	double			m_MaxWS;
	double			m_MinFWI;
	double			m_MinISI;
	WTimeSpan		m_startTime,
					m_endTime;
	::WISE::GridProto::TemporalCondition_DailyAttribute_TimeRelative m_startTimeRelative,
																	  m_endTimeRelative;

	std::uint32_t	m_bits;
			#define TEMPORAL_BITS_RH_EFFECTIVE				0x00000001
			#define TEMPORAL_BITS_FWI_EFFECTIVE				0x00000002
			#define TEMPORAL_BITS_ISI_EFFECTIVE				0x00000004
			#define TEMPORAL_BITS_WIND_EFFECTIVE			0x00000008
			#define TEMPORAL_BITS_STIME_EFFECTIVE			0x00000010
			#define TEMPORAL_BITS_ETIME_EFFECTIVE			0x00000020
			#define TEMPORAL_BITS_TIMES_EFFECTIVE			0x00000030

	DECLARE_OBJECT_CACHE_MT(DailyAttribute, DailyAttribute)
};


class SeasonalAttribute : public MinNode {
public:
	DEVICE SeasonalAttribute *LN_Succ() const { return (SeasonalAttribute *)MinNode::LN_Succ(); };
	DEVICE SeasonalAttribute *LN_Pred() const { return (SeasonalAttribute *)MinNode::LN_Pred(); };

	SeasonalAttribute();

	WTimeSpan		m_localStartDate;
	std::uint32_t	m_optionFlags, m_optionFlagsSet;

	std::uint32_t	m_bits;
			#define TEMPORAL_BITS_CURING_DEGREE_EFFECTIVE				0x00010000

	double			m_curingDegree;

	DECLARE_OBJECT_CACHE_MT(SeasonalAttribute, SeasonalAttribute)
};


typedef std::variant<WTimeSpan, WTime> WTimeVariant;


/**
 * Attribute Filter is a COM Class, part of GridCom.
 */
class GRIDCOM_API CCWFGM_TemporalAttributeFilter : public ICWFGM_GridEngine, public ISerializeProto {
    friend class CWFGM_TemporalAttributeFilterHelper;

public:
	CCWFGM_TemporalAttributeFilter();
	CCWFGM_TemporalAttributeFilter(const CCWFGM_TemporalAttributeFilter &toCopy);
	~CCWFGM_TemporalAttributeFilter();

	/**
		Creates a new AttributeFilter object with all the same properties of the object being called, returns a handle to the new object in �newFilter�.
		No data is shared between these two objects, an exact copy is created.  However, handles to ICWFGM_Fuel, etc. objects are duplicated, not the ICWFGM_Fuel objects themselves.
		\param	newFilter	Handle to a new filter.
		\retval	E_POINTER	The address provided for newFilter is invalid.
		\retval	E_OUTOFMEMORY	Insufficient memory.
		\retval	E_NOINTERFACE	Interface not supported, or non-existent.
		\retval	ERROR_SEVERITY_WARNING	A file exception or other miscellaneous error.
	*/
	NO_THROW HRESULT Clone(boost::intrusive_ptr<ICWFGM_CommonBase> *newObject) const;
	NO_THROW HRESULT GetOptionKey(unsigned short option, const WTimeVariant &start_time, PolymorphicAttribute *value, bool *value_valid);
	NO_THROW HRESULT SetOptionKey(unsigned short option, const WTimeVariant &start_time, const PolymorphicAttribute &value, bool value_valid);
	NO_THROW HRESULT ClearOptionKey(unsigned short option, const WTimeVariant &start_time);
	NO_THROW HRESULT Count(unsigned short option, unsigned short *count);
	NO_THROW HRESULT TimeAtIndex(unsigned short option, unsigned short index, WTimeVariant *start_time);
	NO_THROW HRESULT DeleteIndex(unsigned short option, unsigned short index);
	NO_THROW HRESULT GetAttribute(unsigned short option, PolymorphicAttribute *value);
	NO_THROW HRESULT SetAttribute(unsigned short option, const PolymorphicAttribute &value);

	/**
		Changes the state of the object with respect to access rights.  When the object is used by an active simulation, it must not be modified.
		When the object is somehow modified, it must be done so in an atomic action to prevent concerns with arising from multithreading.
		Note that these locks are primarily needed to ensure that data contributing during a simulation is not modified while the simulation is executing.\n\n All routines in the
		ICWFGM_GridEngine interface are necessarily NOT multithreading safe (for performance) but other interfaces for a given COM object
		implementing this interface must be by specification.  Locking request is forwarded to the next lower object in the 'layerThread' layering.
		If this object is configured to contain a fuel grid, then the ICWFGM_FuelMap object is also forwarded the locking request.\n\n
		In the event of an error, then locking is undone to reflect an error state.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	exclusive	TRUE if the requester wants a write lock, false for read/shared access
		\param	obtain	TRUE to obtain the lock, FALSE to release the lock.  If this is FALSE, then the 'exclusive' parameter must match the initial call used to obtain the lock.
		\retval	SUCCESS_STATE_OBJECT_UNLOCKED	Lock was released.
		\retval	SUCCESS_STATE_OBJECT_LOCKED_WRITE	Exclusive/write lock obtained.
		\retval	SUCCESS_STATE_OBJECT_LOCKED_SCENARIO	A scenario successfully required a lock for purposes of simulating.
		\retval	SUCCESS_STATE_OBJECT_LOCKED_READ	Shared/read lock obtained.
		\retval	S_OK	Successful
		\retval	ERROR_GRID_UNINITIALIZED	No path via layerThread can be determined to further determine successful locks.
	*/
	virtual NO_THROW HRESULT MT_Lock(Layer *layerThread, bool exclusive, std::uint16_t obtain) override;
	/**
		This filter object simply forwards the call to the next lower GIS layer determined by layerThread.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	start_time	Ignored.
		\param	duration	Ignored.
		\param option	Determines type of Valid request.
		\param application_count	Optional (dependent on option).  Array of counts for how often a particular type of grid occurs in the set of ICWFGM_GridEngine objects associated with a scenario.
		\retval	ERROR_GRID_UNINITIALIZED	No object in the grid layering to forward the request to.
	*/
	virtual NO_THROW HRESULT Valid(Layer *layerThread, const HSS_Time::WTime &start_time, const HSS_Time::WTimeSpan &duration, std::uint32_t option, std::vector<uint16_t> *application_count) override;
	/**
		Polymorphic.  If layerThread is non-zero, then this filter object simply forwards the call to the next lower GIS
		layer determined by layerThread.  If layerthread is zero, then this object will interpret the call.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param option	The attribute of interest.  Valid attributes are:
		<ul>
		<li><code>CWFGM_ATTRIBUTE_LOAD_WARNING</code>	BSTR.  Any warnings generated by the COM object when deserializating.
		</ul>
		\param value	Location for the retrieved value to be placed.
		\retval E_POINTER	value is NULL
		\retval	ERROR_GRID_UNINITIALIZED	No object in the grid layering to forward the request to.
		\retval S_OK Success
	*/
	virtual NO_THROW HRESULT GetAttribute(Layer *layerThread, std::uint16_t option, PolymorphicAttribute *value) override;
	/**
		Polymorphic.  This filter object will (conditionally) return an attribute value located at (x, y).  The condition is based on the requested option matching this object's OptionKey property.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	_x			Value on x-axis.
		\param	_y			Value on y-axis.
		\param	time	A GMT time.
		\param	option			Data array option.
		\param	attribute			Return value for attribute.
		\retval S_OK				Returned attribute is valid.
		\retval ERROR_GRID_NO_DATA		Requested (x, y) has no data.
		\retval	ERROR_GRID_UNINITIALIZED	No object in the grid layering to forward the request to.
	*/
	virtual NO_THROW HRESULT GetAttributeData(Layer *layerThread, const XY_Point &pt,const HSS_Time::WTime &time, const HSS_Time::WTimeSpan& timeSpan, std::uint16_t option,  std::uint64_t optionFlags, NumericVariant *attribute, grid::AttributeValue *attribute_valid, XY_Rectangle *cache_bbox) override;
	/**
		Polymorphic.  This filter object will (conditionally) return an array of attribute values.  The condition is based on the requested option matching this object's OptionKey property.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	_x_min		Minimum value (inclusive) on x-axis.
		\param	_y_min		Minimum value (inclusive) on y-axis.
		\param	_x_max		Maximum value (inclusive) on x-axis.
		\param	_y_max		Maximum value (inclusive) on y-axis.
		\param	time	A GMT time.
		\param	option		Data array option.
		\param	attribute	Data array attribute.
		\retval S_OK				Returned attribute is valid.
		\retval E_POINTER	attribute is invalid
		\retval E_INVALIDARG	The array is not 2D, or is insufficient in size to contain the requested data
		\retval	ERROR_GRID_UNINITIALIZED	No object in the grid layering to forward the request to.
	*/
	virtual NO_THROW HRESULT GetAttributeDataArray(Layer *layerThread, const XY_Point &min_pt, const XY_Point &max_pt,  double scale, const HSS_Time::WTime &time, const HSS_Time::WTimeSpan& timeSpan, std::uint16_t option,  std::uint64_t optionFlags, NumericVariant_2d *attribute, attribute_t_2d *attribute_valid) override;
	/**
		This filter object simply forwards the call to the next lower GIS layer determined by layerThread.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	flags	Calculation flags.
		\param	from_time	A GMT time provided as seconds since January 1st, 1600.
		\param	next_event	A GMT time provided as seconds since January 1st, 1600, representing the time for the next event, based on 'flags'.
		\retval	ERROR_GRID_UNINITIALIZED	No object in the grid layering to forward the request to.
	*/
	virtual NO_THROW HRESULT GetEventTime(Layer *layerThread, const XY_Point& pt, std::uint32_t flags, const HSS_Time::WTime &from_time, HSS_Time::WTime *next_event, bool *event_valid) override;
	virtual NO_THROW HRESULT PutGridEngine(Layer *layerThread, ICWFGM_GridEngine * newVal) override;
	virtual NO_THROW HRESULT PutCommonData(Layer* layerThread, ICWFGM_CommonData* pVal) override;

#ifndef DOXYGEN_IGNORE_CODE
public:
	virtual std::int32_t serialVersionUid(const SerializeProtoOptions& options) const noexcept override;
	virtual WISE::GridProto::TemporalCondition* serialize(const SerializeProtoOptions& options) override;
	virtual CCWFGM_TemporalAttributeFilter *deserialize(const google::protobuf::Message& proto, std::shared_ptr<validation::validation_object> valid, const std::string& name) override;
	virtual std::optional<bool> isdirty(void) const noexcept override { return m_bRequiresSave; }

protected:
	CRWThreadSemaphore		m_lock;

	MinListTempl<DailyAttribute> m_attributes;
	MinListTempl<SeasonalAttribute> m_conditions;
	std::string			m_loadWarning;

	HRESULT SetWorldLocation(ICWFGM_GridEngine *gridEngine, Layer *layerThread);
#endif

	WTimeManager			*m_timeManager;
	CCoordinateConverter	m_converter;
	bool					m_bRequiresSave;

	DailyAttribute *findOption(const WTime &_time, bool autocreate);
	SeasonalAttribute *findSeasonOption(const WTimeSpan &wtime, std::uint16_t option, bool autocreate, bool accept_earlier);

	public:
	std::uint16_t solarTimes(const XY_Point& pt, const WTime& from_time, WTime& rise, WTime& set, WTime& noon) const;
	void calculatedTimes(const DailyAttribute *day, const XY_Point &pt, WTime& startTime, bool& startEffective, WTime& endTime, bool& endEffective) const;
};

#ifdef HSS_SHOULD_PRAGMA_PACK
#pragma pack(pop)
#endif

#endif //__CWFGM_TEMPORALATTRIBUTEFILTER_H_
