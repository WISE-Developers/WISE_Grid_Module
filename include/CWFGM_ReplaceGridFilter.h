/**
 * WISE_Grid_Module: CWFGM_ReplaceGridFilter.h
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

#include "WTime.h"
#include "semaphore.h"
#include "results.h"
#include <map>
#include "ICWFGM_GridEngine.h"
#include "ISerializeProto.h"

#include <string>
#include <boost/intrusive_ptr.hpp>
#include <boost/atomic.hpp>
#include "cwfgmFilter.pb.h"

#ifdef HSS_SHOULD_PRAGMA_PACK
#pragma pack(push, 8)
#endif



/**
 * The purpose of this object is to transparently override or change the original grid data from the perspective of the simulation.  It supports two interfaces: one interface to perform basic assignment and retrieval operations (ICWFGM_ReplaceGridFilter), and a second interface for the simulation engine (ICWFGM_GridEngine).  The second interface is a requirement for the simulation engine but is also used elsewhere to extract grid values for the display.
 * 
 * This object can be used to experiment with different fuels in different regions, or to correct invalid data.  It can be used to exchange a fueltype, or an index of a fueltype for a different fueltype either for the entire map or a (rectangular) portion of the grid.
 * 
 * Only one replacement 'rule' is allowed for a replacement grid filter object.  If a second rule is needed, a second replacement grid filter object will be needed.
 * 
 * This object also implements the standard COM IPersistStream, IPersistStreamInit, and IPersistStorage interfaces, for use in loading and saving.  Serialization methods declared in these interfaces only save the location of the relationship, not the fueltypes (i.e. the rule) themselves.  This rule is imposed because it is expected that the client application is already saving the fueltypes.
 */
class GRIDCOM_API CCWFGM_ReplaceGridFilter : public ICWFGM_GridEngine, public ISerializeProto {
    friend class CWFGM_ReplaceGridFilterHelper;

public:
	CCWFGM_ReplaceGridFilter();
	CCWFGM_ReplaceGridFilter(const CCWFGM_ReplaceGridFilter &toCopy);
	~CCWFGM_ReplaceGridFilter();

public:
	/**
		Creates a new ReplaceGridFilter object with all the same properties of the object being called, returns a handle to the new object in 'newFilter'.
		No data is shared between these two objects, an exact copy is created.  However, handles to ICWFGM_Fuel objects are duplicated, not the ICWFGM_Fuel objects themselves.
		\param	newFilter	New ReplaceGridFilter object.
		\retval	E_POINTER	The address provided for ReplaceGridFilter is invalid.
		\retval	S_OK	Successful.
		\retval	E_OUTOFMEMORY	Insufficient memory.
		\retval	E_NOINTERFACE	'newFilterManager' is not a successfully created CWFGM ReplaceGridFilter object.
		\retval	ERROR_SEVERITY_WARNING	Unspecified failure.
	*/
	virtual NO_THROW HRESULT Clone(boost::intrusive_ptr<ICWFGM_CommonBase> *newObject) const;
	/**
		Sets the replacement 'rule' to be used by this object.
		If 'from_index' is valid (not (std::uint8_t)-1), then it is used to represent the internal index of the fuel / import index relationship (this allows detailed classification rules to be created), to identify when to change to 'to_fuel'.
		Otherwise, when 'from_index' is (std::uint8_t)-1, then:
		If 'from_fuel' is provided and valid, then 'from_fuel' is always converted to 'to_fuel'.
		If 'from_fuel' is NULL, then all fuel types are converted to 'to_fuel'.
		If 'from_fuel' is ~NULL, then all combustible fuel types are converted to 'to_fuel'.
		\param	from_fuel	Identifies the fuel which should be replaced with to_fuel.
		\param	from_index	Fuels can be mapped to multiple indicies.  If a specific index for a fuel should be overridden, then this parameter should be used to identify what should be changed, and 'from_fuel' should be set to NULL.
		\param	to_fuel	Identifies the fuel used to replace the original fuel.
		\retval	E_NOINTERFACE	The address provided for from_fuel or to_fuel is invalid.
		\retval	S_OK	Successful.
		\retval	E_INVALIDARG	One or more of the parameters are invalid.
		\retval	SUCCESS_GRID_TOTAL_AREA	If replacing the entire grid.
		\retval	ERROR_SCENARIO_SIMULATION_RUNNING	Value cannot be changed as it is being used by a currently running scenario.
	*/
	virtual NO_THROW HRESULT SetRelationship(ICWFGM_Fuel *from_fuel, std::uint8_t from_index, ICWFGM_Fuel *to_fuel);
	/**
		Retrieves the replacement 'rule' to be used by this object.
		If 'from_index' is valid (not (std::uint8_t)-1), then it is used to represent the internal index of the fuel / import index relationship (this allows detailed classification rules to be created), to identify when to change to 'to_fuel'.
		Otherwise, when 'from_index' is (std::uint8_t)-1, then:
		If 'from_fuel' is provided and valid, then 'from_fuel' is always converted to 'to_fuel'.
		If 'from_fuel' is NULL, then all fuel types are converted to 'to_fuel'.
		If 'from_fuel' is ~NULL, then all combustible fuel types are converted to 'to_fuel'.
		\param	from_fuel	Identifies the fuel which should be changed from.
		\param	from_index	If 'from_fuel' is NULL, then a specific index identifying the fuel is being changed and this will have a valid value.
		\param	to_fuel	Identifies the fuel which should be replacing 'from_fuel' or 'from_index'.
		\retval	E_POINTER	the address provided for from_index is invalid
		\retval	E_NOINTERFACE	from_fuel or to_fuel is an invalid pointer
		\retval	S_OK	successful
		\retval	SUCCESS_GRID_TOTAL_AREA	if replacing the entire grid
	*/
	virtual NO_THROW HRESULT GetRelationship(ICWFGM_Fuel **from_fuel, std::uint8_t *from_index, ICWFGM_Fuel **to_fuel);
	/**
		Sets the ReplaceGridFilter objects co-ordinates.  If all parameters are -1, then the replacement rule will be applied to the entire grid area.  Otherwise, the parameters will be used to identify a rectangular portion of the grid (in grid units) that the replacement rule will apply to.	
		\param	pt1		Minimum value (inclusive).
		\param	pt2		Maximum value (inclusive).
		\retval	E_POINTER	One or more of the addresses provided is invalid.
		\retval	S_OK	Successful.
		\retval	E_INVALIDARG	If x1 greater than x2, or y1 greater than y2.
		\retval	ERROR_GRID_UNINITIALIZED	The Grid Engine property has not be set.
		\retval	ERROR_GRID_LOCATION_OUT_OF_RANGE	If the coordinates are out of range of the grid bounds.
		\retval	SUCCESS_GRID_TOTAL_AREA	If replacing the entire grid (all of the coordinate values are -1).
		\retval	ERROR_SCENARIO_SIMLUATION_RUNNING	Value cannot be changed as it is being used in a currently running scenario.
	*/
	virtual NO_THROW HRESULT SetArea(const XY_Point &pt1, const XY_Point &pt2);
	/**
		Retrieves the ReplaceGridFilter objects co-ordinates.  If all values are (-1), then the replacement rule applies to the entire grid.  Otherwise, the coordinates identify the rectangular portion of the grid (in grid units) to which the replacement rule applies.
		\param	pt1		Minimum value (inclusive).
		\param	pt2		Maximum value (inclusive).
		\retval	E_POINTER	The address provided for x1, y1, x2 or y2 is invalid.
		\retval	S_OK	Successful.
		\retval	SUCCESS_GRID_TOTAL_AREA	If replacing the entire grid.
	*/
	virtual NO_THROW HRESULT GetArea(XY_Point* pt1, XY_Point* pt2);
	/**
		Polymorphic.  This routine retrieves an attribute/option value given the attribute/option index.
		\param option Supported / valid attribute/option index supported are:
		<ul>
		<li><code>CWFGM_ATTRIBUTE_LOAD_WARNING</code>	BSTR.  Any warnings generated by the COM object when deserializating.
		</ul>
		\param value Return value for the attribute/option index
		\sa ICWFGM_ReplaceGridFilter::GetAttribute
		\retval	S_OK	Success
		\retval	E_POINTER	Address provided for value is invalid.
		\retval	E_INVALIDARG	No valid parameters.
	*/
	virtual NO_THROW HRESULT GetAttribute(std::uint16_t option, PolymorphicAttribute *value);
	virtual NO_THROW HRESULT SetAttribute(std::uint16_t option, const PolymorphicAttribute &value);

	/**
		Changes the state of the object with respect to access rights.  When the object is used by an active simulation, it must not be modified.
		When the object is somehow modified, it must be done so in an atomic action to prevent concerns with arising from multithreading.
		Note that these locks are primarily needed to ensure that data contributing during a simulation is not modified while the simulation is executing.\n\n All routines in the
		ICWFGM_GridEngine interface are necessarily NOT multithreading safe (for performance) but other interfaces for a given COM object
		implementing this interface must be by specification.  Locking request is forwarded to the next lower object in the 'layerThread' layering.\n\n
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
	virtual NO_THROW HRESULT GetAttribute(Layer *layerThread, std::uint16_t option, PolymorphicAttribute *value);
	/**
		This method forwards the call to the next lower GIS layer determined by layerThread.  Then, the next lower's return value (fuel) may be changed based on the filter's rules.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	pt			Location on the grid.
		\param	time		A GMT time.
		\param	fuel		Returned fuel Information.
		\param fuel_valid Indicates if the return value in 'fuel' is valid.
		\param cache_bbox: May be NULL. If provided, then the rectangle is filled or adjusted to indicate the maximum size of the cell containing the requested point which is constant and uniform in content and value.
		\retval	E_POINTER	Address provided for value is invalid.
		\retval	ERROR_GRID_UNINITIALIZED	No object in the grid layering to forward the request to.
	*/
	virtual NO_THROW HRESULT GetFuelData(Layer *layerThread, const XY_Point &pt, const HSS_Time::WTime &time, ICWFGM_Fuel **fuel, bool *fuel_valid, XY_Rectangle *cache_bbox);
	/**
		This method forwards the call to the next lower GIS layer determined by layerThread.  Then, the next lower's return value (array of fuels) may be changed based on the filter's rules.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	min_pt		Minimum value (inclusive).
		\param	max_pt		Maximum value (inclusive).
		\param	scale		Scale (meters) that the array is defined for
		\param	time	A GMT time.
		\param	fuel		2D array of fuel pointers
		\param fuel_valid Indicates if the return value in 'fuel' is valid.
		\retval	E_POINTER	Address provided for value is invalid.
		\retval E_INVALIDARG	The array is not 2D, or is insufficient in size to contain the requested data
		\retval	ERROR_GRID_UNINITIALIZED	No object in the grid layering to forward the request to.
	*/
	virtual NO_THROW HRESULT GetFuelDataArray(Layer *layerThread, const XY_Point &min_pt, const XY_Point &max_pt, double scale, const HSS_Time::WTime &time, ICWFGM_Fuel_2d *fuel, bool_2d *fuel_valid);
	virtual NO_THROW HRESULT PutGridEngine(Layer *layerThread, ICWFGM_GridEngine * newVal);

#ifndef DOXYGEN_IGNORE_CODE
public:
	virtual std::int32_t serialVersionUid(const SerializeProtoOptions& options) const noexcept override;
	virtual WISE::GridProto::CwfgmReplaceGridFilter* serialize(const SerializeProtoOptions& options) override;
	virtual CCWFGM_ReplaceGridFilter *deserialize(const google::protobuf::Message& proto, std::shared_ptr<validation::validation_object> valid, const std::string& name) override;
	virtual std::optional<bool> isdirty(void) const noexcept override { return m_bRequiresSave; }

protected:
	CRWThreadSemaphore		m_lock;

	std::uint16_t				m_x1, m_y1, m_x2, m_y2;
	std::uint8_t				m_fromIndex;
	boost::intrusive_ptr<ICWFGM_Fuel>		m_fromFuel,
					m_toFuel;
	std::string				m_loadWarning;
	double				m_xllcorner, m_yllcorner, m_resolution, m_iresolution;
	bool				m_allCombustible, m_allNoData;

	std::uint16_t convertX(double x, XY_Rectangle *bbox);
	std::uint16_t convertY(double y, XY_Rectangle *bbox);
	double invertX(double x)			{ return x * m_resolution + m_xllcorner; }
	double invertY(double y)			{ return y * m_resolution + m_yllcorner; }

	HRESULT fixResolution();

protected:
	bool m_bRequiresSave;

#endif
};

#ifdef HSS_SHOULD_PRAGMA_PACK
#pragma pack(pop)
#endif
