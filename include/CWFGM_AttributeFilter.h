/**
 * WISE_Grid_Module: CWFGM_AttributeFilter.h
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
#include "CWFGM_FuelMap.h"
#include "CWFGM_internal.h"

#include <string>
#include <boost/intrusive_ptr.hpp>
#include <boost/atomic.hpp>
#include "ISerializeProto.h"
#include "cwfgmFilter.pb.h"

#ifdef HSS_SHOULD_PRAGMA_PACK
#pragma pack(push, 8)
#endif



/**
 * Attribute Filter is a COM Class, part of GridCom.
 */
class GRIDCOM_API CCWFGM_AttributeFilter : public ICWFGM_GridEngine, public ISerializeProto {
    friend class CWFGM_AttributeFilterHelper;
public:

	CCWFGM_AttributeFilter();
	CCWFGM_AttributeFilter(const CCWFGM_AttributeFilter &toCopy);
	~CCWFGM_AttributeFilter();

public:
	/**
		Gets Fuel Map.  This property is only important if this object is configured to store a fuel grid.
		\param	pVal	Value of fuelMap
		\sa ICWFGM_AttributeFilter::FuelMap
		\sa ICWFGM_AttributeFilter::OptionKey
		\retval	E_POINTER	The address provided for pVal is invalid.
		\retval	S_OK	Successful.
		\retval	ERROR_GRID_UNINITIALIZED	The grid object has not been initialized.
		\retval	E_NOINTERFACE	Interface not supported, or non-existent.
	*/
	NO_THROW HRESULT get_FuelMap(CCWFGM_FuelMap * *pVal);
	/**
		Sets Fuel Map.  This property is only important if this object is configured to store a fuel grid.
		\param	newVal	The new value for fuelMap.
		\sa ICWFGM_AttributeFilter::FuelMap
		\sa ICWFGM_AttributeFilter::OptionKey
		\retval	E_POINTER	The address provided for pVal is invalid.
		\retval	S_OK	Successful.
		\retval ERROR_GRID_INITIALIZED		This object already has a fuel map assigned to it
		\retval	ERROR_SCENARIO_SIMULATION_RUNNING	Cannot be done while a scenario is running.
		\retval	E_NOINTERFACE	Interface not supported, or non-existent.
	*/
	NO_THROW HRESULT put_FuelMap(CCWFGM_FuelMap * newVal);
	/**
		Creates a new AttributeFilter object with all the same properties of the object being called, returns a handle to the new object in �newFilter�.
		No data is shared between these two objects, an exact copy is created.  However, handles to ICWFGM_Fuel, etc. objects are duplicated, not the ICWFGM_Fuel objects themselves.
		\param	newFilter	Handle to a new filter.
		\sa ICWFGM_AttributeFilter::Clone
		\retval	E_POINTER	The address provided for newFilter is invalid.
		\retval	E_OUTOFMEMORY	Insufficient memory.
		\retval	E_NOINTERFACE	Interface not supported, or non-existent.
		\retval	ERROR_SEVERITY_WARNING	A file exception or other miscellaneous error.
	*/
	NO_THROW HRESULT Clone(boost::intrusive_ptr<ICWFGM_CommonBase> *newObject) const;
	/**
		Imports a grid file.  Rules regarding expected contents/format of the imported file are partially determined by the OptionType property.
		If the import projection file is unspecified, then it is assumed to match the projection for the associated main ICWFGM_GridEngine object.
		\param	prj_file_name	Projection file name.
		\param	grid_file_name	Name of the file to import.
		\sa ICWFGM_AttributeFilter::ImportAttributeGrid
		\sa ICWFGM_AttributeFilter::OptionType
		\retval	ERROR_SCENARIO_SIMULATION_RUNNING	Action cannot be performed while running the scenario.
		\retval	ERROR_GRID_UNINITIALIZED	Grid is not initialized.
		\retval	E_UNEXPECTED	An unexpected error occurred.
		\retval	E_POINTER	Address provided for grid_file_name is invalid.
		\retval	E_OUTOFMEMORY	Insufficient memory.
		\retval	SUCCESS_GRID_DATA_UPDATED	Operation was successful and old data has been replaced.
		\retval	ERROR_READ_FAULT|ERROR_SEVERITY_WARNING Generic read error, including location and/or size of the grid does not match the main ICWFGM_GridEngine object.
		\retval	ERROR_ACCESS_DENIED	Access is denied.
		\retval	ERROR_INVALID_HANDLE	Generic file I/O error.
		\retval	ERROR_FILE_EXISTS	The file already exists.
		\retval	ERROR_INVALID_PARAMETER	An invalid parameter has been passed.
		\retval	ERROR_TOO_MANY_OPEN_FILES	The program has too many open files.
		\retval	ERROR_FILE_NOT_FOUND	The file cannot be found.
		\retval	ERROR_FUELS_FUEL_UNKNOWN	A fuel index is unknown.
		\retval	S_OK	Successful.
	*/
	NO_THROW HRESULT ImportAttributeGrid(const std::string &prj_file_name, const std::string &grid_file_name);
	/**
		Exports a grid file.  Rules regarding the format of the exported file are partially determined by the OptionType property.  Specification of the output projection file name is optional.
		\param	prj_file_name	Projection file name.
		\param	grid_file_name	File name to be exported to.
		\sa ICWFGM_AttributeFilter::ExportAttributeGrid
		\sa ICWFGM_AttributeFilter::OptionType
		\retval	E_POINTER	Address provided is invalid.
		\retval	E_INVALIDARG	Invalid arguments.
		\retval	S_OK	Successful.
		\retval	ERROR_READ_FAULT|ERROR_SEVERITY_WARNING	Generic write error.
		\retval	ERROR_ACCESS_DENIED	Access is denied.
		\retval	ERROR_INVALID_HANDLE	Generic file I/O error.
		\retval	ERROR_FILE_EXISTS	The file already exists.
		\retval	ERROR_INVALID_PARAMETER	An invalid parameter has been passed.
		\retval	ERROR_TOO_MANY_OPEN_FILES	The program has too many open files.
		\retval	ERROR_FILE_NOT_FOUND	The file cannot be found.
		\retval	ERROR_HANDLE_DISK_FULL	The disk that the file is being written to cannot store the file.
	*/
	NO_THROW HRESULT ExportAttributeGrid(const std::string &prj_file_name, const std::string &grid_file_name, const std::string& band_name);
	NO_THROW HRESULT ImportAttributeGridWCS(const std::string & url, const std::string & layer, const std::string & username, const std::string & password);
	NO_THROW HRESULT ExportAttributeGridWCS(const std::string & url, const std::string & layer, const std::string & username, const std::string & password);
	/**
		Gets Option Key.  If the option key is set to (std::uint16_t)-1, then, via its ICWFGM_GridEngine interface, this object acts like a replacement fuel grid.  If the option key is set to any
		other value then, via its ICWFGM_GridEngine interface, it will respond to gridded GetAttributeData and GetAttributeDataArray method invocations when this key matches the optionKey
		parameter to these calls.
		\param	pVal	Value of optionKey
		\sa ICWFGM_AttributeFilter::OptionKey
		\sa ICWFGM_GridEngine::GetAttributeData(__int64, unsigned short, unsigned short, unsigned __int64, unsigned short, NumericVariant *)
		\sa ICWFGM_GridEngine::GetAttributeDataArray
		\retval	E_POINTER	The address provided for pVal is invalid.
		\retval	S_OK	Successful.
	*/
	NO_THROW HRESULT get_OptionKey(std::uint16_t *pVal);
	/**
		Sets Option Key.  If the option key is set to (std::uint16_t)-1, then, via its ICWFGM_GridEngine interface, this object acts like a replacement fuel grid.  If the option key is set to any
		other value then, via its ICWFGM_GridEngine interface, it will respond to gridded GetAttributeData and GetAttributeDataArray method invocations when this key matches the optionKey
		parameter to these calls.
		\param	newVal	The new value for optionKey.
		\sa ICWFGM_AttributeFilter::OptionKey
		\sa ICWFGM_GridEngine::GetAttributeData(__int64, unsigned short, unsigned short, unsigned __int64, unsigned short, PolymorphicAttribute *)
		\sa ICWFGM_GridEngine::GetAttributeDataArray
		\retval	E_POINTER	The address provided for pVal is invalid.
		\retval	S_OK	Successful.
	*/
	NO_THROW HRESULT put_OptionKey(std::uint16_t newVal);
	/**
		Retrieves the datatype for storage of gridded data in this attribute object.
		\param	pVal	The value for optionType.
		\sa ICWFGM_AttributeFilter::OptionType
		\retval	E_POINTER	The address provided for pVal is invalid.
		\retval	S_OK	Successful.
	*/
	NO_THROW HRESULT get_OptionType(std::uint16_t *pVal);
	/**
		Sets datatype for storage of gridded data in this attribute object.  Any existed grid data is cleared, regardless of whether there is any change to the datetype.
		\param	newVal	New value of optionType.  Valid types are:
		<ul>
		<li>VT_I1
		<li>VT_I2
		<li>VT_I4
		<li>VT_I8
		<li>VT_UI1
		<li>VT_UI2
		<li>VT_UI4
		<li>VT_UI8
		<li>VT_R4
		<li>VT_R8
		<li>VT_BOOL
		</ul>
		\sa ICWFGM_AttributeFilter::OptionType
		\retval	S_OK	Successful.
		\retval	E_INVALIDARG	If newVal is passed as invalid.
	*/
	NO_THROW HRESULT put_OptionType(std::uint16_t newVal);
	/**
		Polymorphic.  Sets the attribute at (x, y) to the 'value'.
		\param	pt	Coordinate.
		\param	value	Polymorphic.  Value for (x, y).
		\sa ICWFGM_AttributeFilter::SetAttributePoint
		\sa ICWFGM_AttributeFilter::OptionType
		\retval	ERROR_SCENARIO_SIMULATION_RUNNING	Action cannot be performed while running the scenario.
		\retval	S_OK	Success
		\retval	ERROR_GRID_UNINITIALIZED	Grid object is not initialized.
		\retval	ERROR_GRID_LOCATION_OUT_OF_RANGE	Point x, or y is out of grid range.
		\retval	E_FAIL	The provided variant value cannot be converted to the grid storage datatype.
		\retval	ERROR_SEVERITY_WARNING	This attribute object has not been completely initialized.
		\retval	E_UNEXPECTED	An unexpected error occurred.
	*/
	NO_THROW HRESULT SetAttributePoint(const XY_Point &pt,const NumericVariant &value);
	/**
		Polymorphic.  Sets the attributes along a line to 'value'.
		\param	pt1	Starting Coordinate.
		\param	pt2	Starting Coordinate.
		\param	value	Polymorphic.  Value for each location along the line.
		\sa ICWFGM_AttributeFilter::SetAttributeLine
		\sa ICWFGM_AttributeFilter::OptionType
		\retval	ERROR_SCENARIO_SIMULATION_RUNNING	Action cannot be performed while running the scenario.
		\retval	S_OK	Success
		\retval	ERROR_GRID_UNINITIALIZED	Grid object not initialized.
		\retval	ERROR_GRID_LOCATION_OUT_OF_RANGE	Line outside of grid range.
		\retval	E_FAIL	The provided variant value cannot be converted to the grid storage datatype.
	*/
	NO_THROW HRESULT SetAttributeLine(XY_Point pt1,XY_Point pt2, const NumericVariant &value);
	/**
		Polymorphic.  Retrieves the attribute at (x, y) to the 'value'.  If there is NODATA at (x, y), then the return datatype is set to VT_EMPTY.
		\param	pt	Coordinate.
		\param	value	Returned value of the point.
		\sa ICWFGM_AttributeFilter::GetAttributeLine
		\sa ICWFGM_AttributeFilter::OptionType
		\retval	S_OK	Success
		\retval	ERROR_GRID_UNINITIALIZED	Grid object not initialized.
		\retval	ERROR_GRID_LOCATION_OUT_OF_RANGE	Point specified is out of grid range.
		\retval	E_POINTER	Address provided for value is invalid.
		\retval	ERROR_GRID_NO_DATA	No data at point specified.
		\retval	E_UNEXPECTED	Unknown or invalid OptionType..
		\retval	ERROR_SEVERITY_WARNING	Unspecified failure.
	*/
	NO_THROW HRESULT GetAttributePoint(const XY_Point &pt, NumericVariant *value, grid::AttributeValue *value_valid);
	/**
		Polymorphic.  If layerThread is non-zero, then this filter object simply forwards the call to the next lower GIS
		layer determined by layerThread.  If layerthread is zero, then this object will interpret the call.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param option	The attribute of interest.  Valid attributes are:
		<ul>
		<li><code>CWFGM_ATTRIBUTE_LOAD_WARNING</code>	BSTR.  Any warnings generated by the COM object when deserializating.
		</ul>
		\param value	Location for the retrieved value to be placed.
		\sa ICWFGM_GridEngine::GetAttribute
		\retval E_POINTER	value is NULL
		\retval	ERROR_GRID_UNINITIALIZED	No object in the grid layering to forward the request to.
		\retval S_OK Success
	*/
	NO_THROW HRESULT GetAttribute(std::uint16_t option, PolymorphicAttribute *value);
	NO_THROW HRESULT SetAttribute(std::uint16_t option, const PolymorphicAttribute &value);
	/**
		Polymorphic.  Allocates memory for the attribute object and assigns all attributes to the provided value.
		\param	value	Polymorphic.  Value for each location in the grid.
		\sa ICWFGM_AttributeFilter::ResetAttribute
		\sa ICWFGM_AttributeFilter::OptionType
		\retval	ERROR_SCENARIO_SIMULATION_RUNNING	Action cannot be performed while running the scenario.
		\retval	S_OK	Success
		\retval	ERROR_GRID_UNINITIALIZED	Grid object not initialized.
		\retval	E_OUTOFMEMORY	Insufficient memory.
		\retval	E_POINTER	Address provided for value is invalid.
		\retval	E_FAIL	The provided variant value cannot be converted to the grid storage datatype.
		\retval	E_UNEXPECTED	An unexpected error occurred.
	*/
	NO_THROW HRESULT ResetAttribute(const NumericVariant &value);
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
		\sa ICWFGM_GridEngine::MT_Lock
		\retval	SUCCESS_STATE_OBJECT_UNLOCKED	Lock was released.
		\retval	SUCCESS_STATE_OBJECT_LOCKED_WRITE	Exclusive/write lock obtained.
		\retval	SUCCESS_STATE_OBJECT_LOCKED_SCENARIO	A scenario successfully required a lock for purposes of simulating.
		\retval	SUCCESS_STATE_OBJECT_LOCKED_READ	Shared/read lock obtained.
		\retval	S_OK	Successful
		\retval	ERROR_GRID_UNINITIALIZED	No path via layerThread can be determined to further determine successful locks.
	*/
	virtual NO_THROW HRESULT MT_Lock(Layer *layerThread, bool exclusive, std::uint16_t obtain) override;
	/**
	  Polymorphic.  This routine retrieves an attribute/option value given the attribute/option index.
		\param option Supported / valid attribute/option index supported are:
	  <ul>
	  <li><code>CWFGM_ATTRIBUTE_LOAD_WARNING</code>	BSTR.  Any warnings generated by the COM object when deserializating.
	  </ul>
		\param value Return value for the attribute/option index
		\sa ICWFM_AttributeFilter::GetAttribute
		\retval	S_OK	Success
		\retval	E_POINTER	Address provided for value is invalid.
		\retval	E_INVALIDARG	No valid parameters.
	*/
	virtual NO_THROW HRESULT GetAttribute(Layer *layerThread, std::uint16_t option, PolymorphicAttribute *value) override;
	/**
		This filter will conditionally (based on configuration) return a pointer to a fuel.  If this object does not contain a grid fuel map,
		then it forwards the call to the next lower GIS layer determined by layerThread.
		\param layerThread Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param pt The x and y coordinate for the point
		\param time Specified GMT time since January 1, 1600
		\param fuel Returned fuel type.  May return NULL.
		\param fuel_valid Indicates if the return value in ‘fuel’ is valid.
		\param cache_bbox: May be NULL. If provided, then the rectangle is filled or adjusted to indicate the maximum size of the cell containing the requested point which is constant and uniform in content and value.
		\sa ICWFGM_Scenario::GetFuelData
		\retval	ERROR_GRID_UNINITIALIZED	No object in the grid layering to forward the request to.
		\retval	ERROR_FUELS_FUEL_UNKNOWN	NODATA at requested location
	*/
	virtual NO_THROW HRESULT GetFuelData(Layer *layerThread, const XY_Point &pt,const HSS_Time::WTime &time, ICWFGM_Fuel **fuel, bool *fuel_valid, XY_Rectangle *cache_bbox) override;
	/**
		This filter will conditionally (based on configuration) return the index to a fuel.
		If this object does not contain a fuel map, then it forwards the call to the next lower GIS layer determined by layerThread.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param pt The x and y coordinate for the point
		\param time Specified GMT time since January 1, 1600
		\param fuel Returned fuel type.  May return NULL.
		\param fuel_valid Indicates if the return value in ‘fuel’ is valid.
		\param cache_bbox: May be NULL. If provided, then the rectangle is filled or adjusted to indicate the maximum size of the cell containing the requested point which is constant and uniform in content and value.
		\param fuel_index Returned fuel index.
		\sa ICWFGM_GridEngine::GetFuelIndexData
		\retval	ERROR_GRID_UNINITIALIZED	No object in the grid layering to forward the request to.
		\retval	ERROR_FUELS_FUEL_UNKNOWN	NODATA at requested location
	*/
	virtual NO_THROW HRESULT GetFuelIndexData(Layer *layerThread, const XY_Point &pt,const HSS_Time::WTime &time,  std::uint8_t *fuel_index, bool *fuel_valid, XY_Rectangle *cache_bbox) override;
	/**
		Polymorphic.  This filter object will (conditionally) return an attribute value located at (x, y).  The condition is based on the requested option matching this object's OptionKey property.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	pt			Value.
		\param	time	A GMT time.
		\param	option			Data array option.
		\param	attribute			Return value for attribute.
		\sa ICWFGM_GridEngine::GetAttributeData
		\sa ICWFGM_AttributeFilter::OptionKey
		\retval S_OK				Returned attribute is valid.
		\retval ERROR_GRID_NO_DATA		Requested (x, y) has no data.
		\retval	ERROR_GRID_UNINITIALIZED	No object in the grid layering to forward the request to.
	*/
	virtual NO_THROW HRESULT GetAttributeData(Layer *layerThread, const XY_Point &pt,const HSS_Time::WTime &time, const HSS_Time::WTimeSpan& timeSpan, std::uint16_t option, std::uint64_t optionFlags, NumericVariant *attribute, grid::AttributeValue *attribute_valid, XY_Rectangle *cache_bbox) override;
	/**
		This filter will conditionally (based on configuration) return an array of pointers to fuels.  If this object does not contain a grid fuel map,
		then it forwards the call to the next lower GIS layer determined by layerThread.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	min_pt		Minimum value (inclusive).
		\param	max_pt		Maximum value (inclusive).
		\param	scale		Scale (meters) that the array is defined for
		\param	time	A GMT time.
		\param	fuel		Data array attribute.
		\sa ICWFGM_GridEngine::GetFuelDataArray
		\retval	ERROR_GRID_UNINITIALIZED	No object in the grid layering to forward the request to.
		\retval	ERROR_FUELS_FUEL_UNKNOWN	NODATA at requested location
		\retval E_INVALIDARG	The array is not 2D, or is insufficient in size to contain the requested data
	*/
	virtual NO_THROW HRESULT GetFuelDataArray(Layer *layerThread, const XY_Point &min_pt,const XY_Point &max_pt, double scale,const HSS_Time::WTime &time, ICWFGM_Fuel_2d *fuel, bool_2d *fuel_valid) override;
	/**
		This filter will conditionally (based on configuration) return indices to fuels.  If this object does not contain a grid fuel map,
		then it forwards the call to the next lower GIS layer determined by layerThread.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	min_pt		Minimum value (inclusive).
		\param	max_pt		Maximum value (inclusive).
		\param	scale		Scale (meters) that the array is defined for
		\param	time	A GMT time.
		\param	fuel		Data array attribute.
		\sa ICWFGM_GridEngine::GetFuelIndexDataArray
		\retval	ERROR_GRID_UNINITIALIZED	No object in the grid layering to forward the request to.
		\retval	ERROR_FUELS_FUEL_UNKNOWN	NODATA at requested location
		\retval E_INVALIDARG	The array is not 2D, or is insufficient in size to contain the requested data
	*/
	virtual NO_THROW HRESULT GetFuelIndexDataArray(Layer *layerThread, const XY_Point &min_pt,const XY_Point &max_pt, double scale,const HSS_Time::WTime &time, uint8_t_2d *fuel, bool_2d *fuel_valid) override;
	/**
		Polymorphic.  This filter object will (conditionally) return an array of attribute values.  The condition is based on the requested option matching this object's OptionKey property.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	min_pt		Minimum value (inclusive).
		\param	max_pt		Maximum value (inclusive).
		\param	scale		Scale (meters) that the array is defined for
		\param	time	A GMT time.
		\param	option		Data array option.
		\param	attribute	Data array attribute.
		\sa ICWFGM_GridEngine::GetAttributeDataArray
		\sa ICWFGM_AttributeFilter::OptionKey
		\retval S_OK				Returned attribute is valid.
		\retval E_POINTER	attribute is invalid
		\retval E_INVALIDARG	The array is not 2D, or is insufficient in size to contain the requested data
		\retval	ERROR_GRID_UNINITIALIZED	No object in the grid layering to forward the request to.
	*/
	virtual NO_THROW HRESULT GetAttributeDataArray(Layer *layerThread, const XY_Point &min_pt,const XY_Point &max_pt, double scale, const HSS_Time::WTime &time, const HSS_Time::WTimeSpan& timeSpan, std::uint16_t option, std::uint64_t optionFlags, NumericVariant_2d *attribute, attribute_t_2d *attribute_valid) override;

#ifndef DOXYGEN_IGNORE_CODE
public:
	static constexpr std::uint16_t VT_EMPTY = 0;
	static constexpr std::uint16_t VT_I1 = 16;
	static constexpr std::uint16_t VT_I2 = 2;
	static constexpr std::uint16_t VT_I4 = 3;
	static constexpr std::uint16_t VT_I8 = 20;
	static constexpr std::uint16_t VT_UI1 = 17;
	static constexpr std::uint16_t VT_UI2 = 18;
	static constexpr std::uint16_t VT_UI4 = 19;
	static constexpr std::uint16_t VT_UI8 = 21;
	static constexpr std::uint16_t VT_R4 = 4;
	static constexpr std::uint16_t VT_R8 = 5;
	static constexpr std::uint16_t VT_BOOL = 11;
	static constexpr std::uint16_t VT_ILLEGAL = 0xffff;

public:
	virtual std::int32_t serialVersionUid(const SerializeProtoOptions& options) const noexcept override;
	virtual WISE::GridProto::CwfgmAttributeFilter* serialize(const SerializeProtoOptions& options) override;
	virtual CCWFGM_AttributeFilter *deserialize(const google::protobuf::Message& proto, std::shared_ptr<validation::validation_object> valid, const std::string& name) override;
	virtual std::optional<bool> isdirty(void) const noexcept override { return m_bRequiresSave; }

public:
	virtual HRESULT PutGridEngine(Layer *layerThread, ICWFGM_GridEngine *newVal) override;

protected:
	boost::intrusive_ptr<CCWFGM_FuelMap>		m_fuelMap;
	CRWThreadSemaphore		m_lock;

	std::uint16_t			m_xsize,
							m_ysize;			// size of our plots

	std::uint16_t			m_optionKey;
	std::uint16_t			m_optionType;

	union {
		std::int8_t		*m_array_i1;
		std::int16_t	*m_array_i2;
		std::int32_t	*m_array_i4;
		std::int64_t	*m_array_i8;
		std::uint8_t	*m_array_ui1;
		std::uint16_t	*m_array_ui2;
		std::uint32_t	*m_array_ui4;
		std::uint64_t	*m_array_ui8;
		float			*m_array_r4;
		double			*m_array_r8;
	};

	bool				*m_array_nodata;
	std::string			m_loadWarning;
	double				m_xllcorner, m_yllcorner, m_resolution, m_iresolution;
	std::string			m_gisURL, m_gisLayer, m_gisUID, m_gisPWD;
	unsigned long		m_flags;					// see CWFGM_internal.h for valid options

	std::uint16_t convertX(double x, XY_Rectangle *bbox);
	std::uint16_t convertY(double y, XY_Rectangle *bbox);
	double invertX(double x)					{ return x * m_resolution + m_xllcorner; }
	double invertY(double y)					{ return y * m_resolution + m_yllcorner; }
	std::uint32_t arrayIndex(const std::uint16_t x, const std::uint16_t y) const {
															weak_assert(x < m_xsize);
															weak_assert(y < m_ysize);
															return (m_ysize - (y + 1)) * m_xsize + x;
														}
	HRESULT setPoint(const XY_Point &pt, const NumericVariant &value);
	HRESULT setPoint(const std::uint16_t x, const std::uint16_t y, const NumericVariant &value);
	HRESULT setPoint(const std::uint32_t index, const NumericVariant &value);
	HRESULT getPoint(const XY_Point &pt, NumericVariant *value, grid::AttributeValue *value_valid);
	HRESULT getPoint(const std::uint16_t x, const std::uint16_t y, NumericVariant *value, grid::AttributeValue *value_valid);
	HRESULT getPoint(const std::uint32_t index, NumericVariant*value, grid::AttributeValue *value_valid);
	HRESULT fixResolution(std::shared_ptr<validation::validation_object> valid, const std::string& name);

	friend bool __cdecl break_fcn(APTR parameter, const XY_Point *loc);

protected:
	bool m_bRequiresSave;

#endif
};

#ifdef HSS_SHOULD_PRAGMA_PACK
#pragma pack(pop)
#endif
