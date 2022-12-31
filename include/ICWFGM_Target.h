/**
 * WISE_Grid_Module: ICWFGM_Target.h
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
#include "XY_PolyType.h"

/**
 * Vector data is provided to the simulation engine by the client application.  The simulation engine defines the interface it will use to obtain the vector fuel break data.  The simulation engine is not concerned with the kind of object that implements this interface, or the implementation of the object.  It may represent the GIS vector-based data, a user-defined static vector fuelbreak, or a user-controlled break that changes over time, or some other object a programmer has built to access a specialized data set.
 * The simulation engine does not define any object that implements this interface.  The simulation engine only defines the interface.  Objects that implement this interface should import this interface definition.  The object implementing this interface is to be handled entirely by the client application (such as CWFGM).  Methods to import vector data or manage vector filters are not found on this COM interface, as the simulation engine has no need for these methods.
 * This design takes advantage of object-oriented methodologies to simplify both the design of the simulation engine, and the integration of the simulation engine into another application.  For example, to provide data from a G.I.S. directly to the simulation engine, the programmer only needs to define an object that implements this interface and communicates with the G.I.S.
 * Unlike ICWFGM_GridEngine interface objects that must be chained (layered) together to attain the appropriate grid layer for the simulation, objects implementing this interface don't replace or override other vector object data, they are only combined.  As such, the order is unimportant so there are no methods to chain ICWFGM_VectorEngine objects together here.  The simulation's scenario object can use many ICWFGM_VectorEngine objects simultaneously.
 * At this time, the simulation engine assumes that fuel breaks defined by this interface have 100% effect on the fire; the fire will not escape the confines of a vector fuel break.
 * 
 * The IDL of the vector object required by the simulation engine is given below.  Program developers are encouraged to obtain the latest electronic copy of the programming tools from the resources listed at the end of this document.
 */
class ICWFGM_Target : public ICWFGM_CommonBase {
public:
	ICWFGM_Target();
	virtual ~ICWFGM_Target();

	/**
		Changes the state of the object with respect to access rights.  When the object is used by an active simulation, it must not be modified.
		When the object is somehow modified, it must be done so in an atomic action to prevent concerns with arising from multithreading.
		Note that these locks are primarily needed to ensure that data contributing during a simulation is not modified while the simulation is executing.\n\n All routines in the
		ICWFGM_VectorEngine interface are necessarily NOT multithreading safe (for performance) but other interfaces for a given COM object implementing this interface must be by specification.
		\param	exclusive	TRUE if the requester wants a write lock, false for read/shared access
		\param	obtain	TRUE to obtain the lock, FALSE to release the lock.  If this is FALSE, then the 'exclusive' parameter must match the initial call used to obtain the lock.
		\retval	SUCCESS_STATE_OBJECT_UNLOCKED	Lock was released.
		\retval	SUCCESS_STATE_OBJECT_LOCKED_WRITE	Exclusive/write lock obtained.
		\retval	SUCCESS_STATE_OBJECT_LOCKED_SCENARIO	A scenario successfully required a lock for purposes of simulating.
		\retval	SUCCESS_STATE_OBJECT_LOCKED_READ	Shared/read lock obtained.
		\retval	S_OK	Successful.
	*/
	virtual NO_THROW HRESULT MT_Lock(bool exclusive, unsigned short obtain) = 0;

	/**
		Creates a new VectorFilter object with all the same properties of the object being called, returns a handle to the new object in 'newFilter'.
		\param	newFilter Pointer to the new copy of the filter.
		\retval S_OK	Succesfull.
		\retval	E_POINTER	Invalid pointer.
		\retval E_OUTOFMEMORY	Out of memory.
		\retval E_NOINTERFACE	No interface
		\retval ERROR_SEVERITY_WARNING	Severity error warning
	*/
	virtual NO_THROW HRESULT Clone(boost::intrusive_ptr<ICWFGM_CommonBase>* newObject) const = 0;

	virtual NO_THROW HRESULT GetTargetRange(std::uint32_t index, XY_Point* min_pt, XY_Point* max_pt) = 0;

	virtual NO_THROW HRESULT GetTargetCount(std::uint32_t* count) = 0;

	virtual NO_THROW HRESULT GetTargetSetCount(std::uint32_t index, std::uint32_t* count) = 0;

	virtual NO_THROW HRESULT GetTarget(std::uint32_t index, std::uint32_t sub_index, XY_Point* target) = 0;

	virtual NO_THROW HRESULT GetTargetSet(std::uint32_t index, std::uint32_t* size, XY_Poly* xy_pairs) = 0;

	/**
		Polymorphic.  This routine retrieves an attribute/option value given the attribute/option index.
		\param option Supported / valid attribute/option index supported are:
		<ul>
		<li><code>CWFGM_ATTRIBUTE_LOAD_WARNING</code>	BSTR.  Any warnings generated by the COM object when deserializating.
		</ul>
		\param value Return value for the attribute/option index
		\sa ICWFGM_Asset::GridEngine
		\retval	S_OK	Success
		\retval	E_POINTER	Address provided for value is invalid.
		\retval	E_INVALIDARG	No valid parameters.
	*/
	virtual NO_THROW HRESULT GetAttribute(std::uint16_t option, PolymorphicAttribute* value) = 0;

	virtual NO_THROW HRESULT SetAttribute(std::uint16_t option, const PolymorphicAttribute& value) = 0;

	/**
		Retrieves the object exposing the GridEngine interface that this VectorEngine object may refer to, to use for tasks such as bounds clipping, etc.
		\param	pVal	Value of GridEngine.
		\sa ICWFGM_VectorEngine::GridEngine
		\retval	E_POINTER	The address provided for pVal is invalid, or upon setting pVal the pointer doesn't appear to belong to an object exposing the ICWFGM_GridEngine interface.
		\retval	S_OK	Successful.
		\retval	ERROR_VECTOR_UNINITIALIZED	The Grid Engine property has not be set.
	*/
	virtual NO_THROW HRESULT get_GridEngine(boost::intrusive_ptr<ICWFGM_GridEngine>* pVal) = 0;

	/**
		Sets the object exposing the GridEngine interface that this VectorEngine object may refer to, to use for tasks such as bounds clipping, etc.
		\param	newVal	Replacement value for GridEngine.
		\sa ICWFGM_VectorEngine::GridEngine
		\retval	E_POINTER	The address provided for pVal is invalid, or upon setting pVal the pointer doesn't appear to belong to an object exposing the ICWFGM_GridEngine interface.
		\retval	S_OK	Successful.
		\retval	ERROR_VECTOR_UNINITIALIZED	The Grid Engine property has not be set.
		\retval	E_NOINTERFACE	The object provided does not implement the ICWFGM_GridEngine interface.
	*/
	virtual NO_THROW HRESULT put_GridEngine(ICWFGM_GridEngine* newVal) = 0;

	virtual NO_THROW HRESULT GetPointSetAttributeCount(std::uint32_t* count) = 0;
	virtual NO_THROW HRESULT GetPointSetAttributeName(std::uint32_t count, std::string* attribute_name) = 0;
	virtual NO_THROW HRESULT GetPointSetAttributeValue(std::uint32_t index, const std::string& attribute_name, PolymorphicAttribute* ignition_type) = 0;

	/**
		This object requires no work to be performed in this call.
		\param	time	A GMT time provided as seconds since January 1st, 1600.
		\param	mode	Calculation mode.
		\retval	S_OK	Always successful.
	*/
	virtual NO_THROW HRESULT PreCalculationEvent(const HSS_Time::WTime& time, std::uint32_t mode, CalculationEventParms* parms) = 0;

	/**
		This object requires no work to be performed in this call.
		\param	time	A GMT time provided as seconds since January 1st, 1600.
		\param	mode	Calculation mode.
		\retval	S_OK	Always successful
	*/
	virtual NO_THROW HRESULT PostCalculationEvent(const HSS_Time::WTime& time, std::uint32_t mode, CalculationEventParms* parms) = 0;
};
