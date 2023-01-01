/**
 * WISE_Grid_Module: ICWFGM_Asset.h
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

#pragma once

#include "ICWFGM_GridEngine.h"
#include "poly.h"


/**
 * Vector data is provided to the simulation engine by the client application.  The simulation engine defines the interface it will use to obtain the vector fuel break data.  The simulation engine is not concerned with the kind of object that implements this interface, or the implementation of the object.  It may represent the GIS vector-based data, a user-defined static vector fuelbreak, or a user-controlled break that changes over time, or some other object a programmer has built to access a specialized data set.
 * The simulation engine does not define any object that implements this interface.  The simulation engine only defines the interface.  Objects that implement this interface should import this interface definition.  The object implementing this interface is to be handled entirely by the client application (such as CWFGM).  Methods to import vector data or manage vector filters are not found on this COM interface, as the simulation engine has no need for these methods.
 * This design takes advantage of object-oriented methodologies to simplify both the design of the simulation engine, and the integration of the simulation engine into another application.  For example, to provide data from a G.I.S. directly to the simulation engine, the programmer only needs to define an object that implements this interface and communicates with the G.I.S.
 * Unlike ICWFGM_GridEngine interface objects that must be chained (layered) together to attain the appropriate grid layer for the simulation, objects implementing this interface don't replace or override other vector object data, they are only combined.  As such, the order is unimportant so there are no methods to chain ICWFGM_VectorEngine objects together here.  The simulation's scenario object can use many ICWFGM_VectorEngine objects simultaneously.
 * At this time, the simulation engine assumes that fuel breaks defined by this interface have 100% effect on the fire; the fire will not escape the confines of a vector fuel break.
 * 
 * The IDL of the vector object required by the simulation engine is given below.  Program developers are encouraged to obtain the latest electronic copy of the programming tools from the resources listed at the end of this document.
 */
class ICWFGM_Asset : public ICWFGM_CommonBase
{
public:
	ICWFGM_Asset();
	virtual ~ICWFGM_Asset();

	/**
		Changes the state of the object with respect to access rights.  When the object is used by an active simulation, it must not be modified.
		When the object is somehow modified, it must be done so in an atomic action to prevent concerns with arising from multithreading.
		Note that these locks are primarily needed to ensure that data contributing during a simulation is not modified while the simulation is executing.\n\n All routines in the
		ICWFGM_VectorEngine interface are necessarily NOT multithreading safe (for performance) but other interfaces for a given COM object implementing this interface must be by specification.
		\param	exclusive	TRUE if the requester wants a write lock, false for read/shared access
		\param	obtain	TRUE to obtain the lock, FALSE to release the lock.  If this is FALSE, then the 'exclusive' parameter must match the initial call used to obtain the lock.
	 */
	virtual NO_THROW HRESULT MT_Lock(bool exclusive, unsigned short obtain) = 0;

	/**
		This method returns whether this vector object has been initialized,and whether it contains all data necessary for a simulation for the simulation time identified by start_time and duration.
		If start_time and duration are both 0, then this method only returns whether the object is initialized.
		The invoked object is allowed to perform a variety of other operations and method calls to initialize and validate, and possibly cache data.
		\param	start_time	Start time of the simulation.  A GMT time provided as seconds since January 1st, 1600.
		\param	duration	Duration of the simulation, in seconds.
	 */
	virtual NO_THROW HRESULT Valid(const HSS_Time::WTime& start_time, const HSS_Time::WTimeSpan& duration) = 0;

	virtual NO_THROW HRESULT GetAssetRange(std::uint32_t index, const HSS_Time::WTime& time, XY_Point* min_pt, XY_Point* max_pt) = 0;

	virtual NO_THROW HRESULT GetAssetCount(const HSS_Time::WTime& time, std::uint32_t* count) = 0;

	virtual NO_THROW HRESULT GetAssetSetCount(const HSS_Time::WTime& time, std::uint32_t index, std::uint32_t* count) = 0;

	virtual NO_THROW HRESULT GetAssetSize(std::uint32_t index, std::uint32_t sub_index, const HSS_Time::WTime& time, std::uint32_t* size) = 0;

	virtual NO_THROW HRESULT GetAsset(std::uint32_t index, std::uint32_t sub_index, const HSS_Time::WTime& time, std::uint32_t* size, XY_Poly* xy_pairs, std::uint16_t* type) = 0;

	/**
		Polymorphic.  This routine retrieves an attribute/option value given the attribute/option index.
		\param	option	Attribute of interest.
		\param	value	Return value for the attribute/option index.
	 */
	virtual NO_THROW HRESULT GetAttribute(std::uint16_t option, PolymorphicAttribute* value) = 0;

	/**
		Sets the value for the attribute denoted by "option".
		\param option	The attribute to change.
		\param value	The value to set the attribute to.
	 */
	virtual NO_THROW HRESULT SetAttribute(std::uint16_t option, const PolymorphicAttribute& value) = 0;

	/**
		This method allows the simulation engine to scan forward or back (based on flags) from a given time for the next time-based event.  This is important for the event-driven nature of the simulation.
		\param	flags	Event related flags.  Currently valid values are:
		<ul>
		<li><code>CWFGM_GETEVENTTIME_FLAG_SEARCH_FORWARD</code> search forward in time
		<li><code>CWFGM_GETEVENTTIME_FLAG_SEARCH_BACKWARD</code> search backwards in time
		<li><code>CWFGM_GETEVENTTIME_QUERY_PRIMARY_WX_STREAM</code> only the primary weather stream may respond
		<li><code>CWFGM_GETEVENTTIME_QUERY_ANY_WX_STREAM</code> only weather streams may respond
		</ul>
		All other values are reserved for future use.
		\param	from_time	A GMT time provided as seconds since January 1st, 1600.
		\param	next_event	A GMT time provided as seconds since January 1st, 1600, representing the time for the next event, based on 'flags'.
	 */
	virtual NO_THROW HRESULT GetEventTime(std::uint32_t flags, const HSS_Time::WTime& from_time, HSS_Time::WTime* next_event) = 0;

	/**
		This method is invoked before any calculations are performed in the simulation, and before calculations are performed on a given simulation time step.  It's intended to perform pre-calculation
		caching events, etc.
		\param	time	A GMT time.
		\param	mode	Calculation mode: 0 represents start of simulation, 1 represents start of a specific time step
		\param	parms	parameters to send and receive values from/to the simulation and grid objects, may be NULL
	 */
	virtual NO_THROW HRESULT PreCalculationEvent(const HSS_Time::WTime& time, std::uint32_t mode, CalculationEventParms* parms) = 0;

	/**
		This method is invoked after all calculations are performed in the simulation, and after calculations are performed on a given simulation time step.  It's intended to perform post-calculation
		clean-up, etc.
		\param	time	A GMT time.
		\param	mode	Calculation mode: 0 represents start of simulation, 1 represents start of a specific time step
		\param	parms	parameters to send and receive values from/to the simulation and grid objects, may be NULL
	 */
	virtual NO_THROW HRESULT PostCalculationEvent(const HSS_Time::WTime& time, std::uint32_t mode, CalculationEventParms* parms) = 0;

	/**
		Sets or retrieves the object exposing the GridEngine interface that this VectorEngine object may refer to, to use for tasks such as bounds clipping, etc.
		\param	pVal	Value of GridEngine.
		\param	newVal	Replacement value for GridEngine.
	 */
	virtual NO_THROW HRESULT get_GridEngine(boost::intrusive_ptr<ICWFGM_GridEngine>* pVal) = 0;

	virtual NO_THROW HRESULT put_GridEngine(ICWFGM_GridEngine* newVal) = 0;

	virtual NO_THROW HRESULT GetPolyLineAttributeCount(std::uint32_t* count) = 0;
	virtual NO_THROW HRESULT GetPolyLineAttributeName(std::uint32_t count, std::string* attribute_name) = 0;
	virtual NO_THROW HRESULT GetPolyLineAttributeValue(std::uint32_t index, const std::string& attribute_name, PolymorphicAttribute* ignition_type) = 0;
};
