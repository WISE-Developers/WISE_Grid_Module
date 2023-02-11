/**
 * WISE_Grid_Module: CWFGM_PolyReplaceGridFilter.h
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
#include <boost/multi_array.hpp>
#include "poly.h"
#include "ICWFGM_GridEngine.h"
#include "CWFGM_internal.h"
#include "ISerializeProto.h"
#include <map>

#include <string>
#include <boost/intrusive_ptr.hpp>
#include <boost/atomic.hpp>
#include "cwfgmFilter.pb.h"

#ifdef HSS_SHOULD_PRAGMA_PACK
#pragma pack(push, 8)
#endif

#ifndef DOXYGEN_IGNORE_CODE

typedef boost::multi_array<bool, 2> bool_2d;
typedef boost::multi_array_ref<bool, 2> bool_2d_ref;
typedef boost::const_multi_array_ref<bool, 2> const_bool_2d_ref;

#endif


/**
 * This class is part of the GridCom.
 */
class GRIDCOM_API CCWFGM_PolyReplaceGridFilter : public ICWFGM_GridEngine, public ISerializeProto {
    friend class CWFGM_PolyReplaceGridFilterHelper;

public:
	CCWFGM_PolyReplaceGridFilter();
	CCWFGM_PolyReplaceGridFilter(const CCWFGM_PolyReplaceGridFilter &toCopy);
	~CCWFGM_PolyReplaceGridFilter();

public:
	/**
		Creates a new PolyReplaceGridFilter object with all the same properties of the object being called, returns a handle to the new object in 'newFilter'.
		No data is shared between these two objects, an exact copy is created.  However, handles to ICWFGM_Fuel objects are duplicated, not the ICWFGM_Fuel objects themselves.
		\param	newFilter	Memory for the newly created PolyReplaceGridFilter object (copy).
		\retval	E_POINTER	The address provided for newFilter is invalid.
		\retval	S_OK	Successful.
		\retval	E_NOINTERFACE	The interface does not exist.
		\retval	E_OUTOFMEMORY	Insufficient memory.
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
		\param	from_fuel	From fuel information
		\param	from_index	From fuel index information
		\param	to_fuel	To fuel information
		\retval	S_OK	Successful.
		\retval	E_NOINTERFACE	Interface not supported, or non-existent.
		\retval	E_INVALIDARG	Invalid arguments.
		\retval	ERROR_SCENARIO_SIMULATION_RUNNING	Value cannot be changed while running a scenario.
	*/
	virtual NO_THROW HRESULT SetRelationship(ICWFGM_Fuel *from_fuel, std::uint8_t from_index, ICWFGM_Fuel *to_fuel);
	/**
		Retrieves the replacement 'rule' to be used by this object.
		If 'from_index' is valid (not (std::uint8_t)-1), then it is used to represent the internal index of the fuel / import index relationship (this allows detailed classification rules to be created), to identify when to change to 'to_fuel'.
		Otherwise, when 'from_index' is (std::uint8_t)-1, then:
		If 'from_fuel' is provided and valid, then 'from_fuel' is always converted to 'to_fuel'.
		If 'from_fuel' is NULL, then all fuel types are converted to 'to_fuel'.
		If 'from_fuel' is ~NULL, then all combustible fuel types are converted to 'to_fuel'.
		\param	from_fuel	From fuel information
		\param	from_index	From fuel index information
		\param	to_fuel	To fuel information
		\retval	S_OK	Successful.
		\retval	E_NOINTERFACE	Interface not supported, or non-existent.
		\retval	E_POINTER	Address provided for from_index is invalid.
	*/
	virtual NO_THROW HRESULT GetRelationship(ICWFGM_Fuel **from_fuel, std::uint8_t *from_index, ICWFGM_Fuel **to_fuel);
	/**
		This method imports polygons from the specified file.  If the file is an ArcInfo Generate file, then it will use a projection file (same name, but ".prj" for an extension).  This projection file is optional.
		Other file formats will use GDAL/OGR conventions regarding projection files.  Imported data is automatically reprojected into the grid projection during import.
		\param	file_path	File path of the polygon to import.
		\param	permissible_drivers	Array of drivers (known by GDAL) identifying the file types that are allowed to be imported.
		\retval	S_OK	Successful.
		\retval	E_POINTER	Address provided for file_path is invalid.
		\retval	ERROR_FIREBREAK_NOT_FOUND	Polygon does not exist.
		\retval	E_OUTOFMEMORY	Insufficient memory.
		\retval	ERROR_GRID_UNINITIALIZED	Grid object not initialized.
		\retval	ERROR_SEVERITY_WARNING	Unspecified failure.
		\retval	S_FALSE	If asking for values where the appropriate array has not been initialized.
		\retval	E_INVALIDARG	Invalid arguments.
		\retval	ERROR_FILE_NOT_FOUND	The file cannot be found.
		\retval	ERROR_TOO_MANY_OPEN_FILES	The program has too many open files.
		\retval	ERROR_ACCESS_DENIED	Access is denied.
		\retval	ERROR_INVALID_HANDLE	Generic file I/O error.
		\retval	ERROR_HANDLE_DISK_FULL Disk full
		\retval	ERROR_FILE_EXISTS	File already exists.
	*/
	virtual NO_THROW HRESULT ImportPolygons(const std::filesystem::path & file_path, const std::vector<std::string_view> &permissible_drivers);
	/**
		This method imports polygons from the specified WFS, looking in the identified layer.  Imported data is automatically reprojected into the grid projection during import.
		\param	url		Identifies the WFS provider.
		\param	layer	Identifies the layer in the WFS provider.
		\param	username Username for access, if needed.
		\param	password Password for the user, if needed.
		\retval	S_OK	Successful.
		\retval	E_POINTER	Address provided for any of the parameters is invalid.
		\retval	ERROR_FIREBREAK_NOT_FOUND	Polygon does not exist.
		\retval	E_OUTOFMEMORY	Insufficient memory.
		\retval	ERROR_GRID_UNINITIALIZED	Grid object not initialized.
		\retval	ERROR_SEVERITY_WARNING	Unspecified failure.
		\retval	S_FALSE	If asking for values where the appropriate array has not been initialized.
		\retval	E_INVALIDARG	Invalid arguments.
	*/
	virtual NO_THROW HRESULT ImportPolygonsWFS(const std::string & url, const std::string & layer, const std::string & username, const std::string & password);
	/**
		This method exports polygons to the specified file.  The projection file is optionally created, and will be the grid's projection.  Driver names are defined by GDAL/OGR, plus "ARCInfo Generate".
		\param	driver_name	Driver name related information.  Refer to GDAL/OGR documentation.
		\param	projection	Projection file for the exported polygon(s).
		\param	file_path	File name and path for the exported polygon(s).
		\retval	S_OK	Successful.
		\retval	E_POINTER	Address provided for file_path, driver_name, or projection is invalid.
		\retval	E_FAIL	Generic failure code.
		\retval	E_OUTOFMEMORY	Insufficient memory.
		\retval	ERROR_GRID_UNINITIALIZED	Grid object not initialized.
		\retval	ERROR_SEVERITY_WARNING	Unspecified failure.
		\retval	S_FALSE	If asking for values where the appropriate array has not been initialized.
		\retval	E_INVALIDARG	Invalid arguments.
	*/
	virtual NO_THROW HRESULT ExportPolygons(std::string_view driver_name, const std::string & projection, const std::filesystem::path & file_path);
	/**
		This method exports polygons to the specified WFS, to the identified layer.  Exported data is in the grid's projection.
		\param	url		Identifies the WFS provider.
		\param	layer	Identifies the layer in the WFS provider.
		\param	username Username for access, if needed.
		\param	password Password for the user, if needed.
		\retval	S_OK	Successful.
		\retval	E_POINTER	Address provided for file_path, driver_name, or projection is invalid.
		\retval	E_FAIL	Generic failure code.
		\retval	E_OUTOFMEMORY	Insufficient memory.
		\retval	ERROR_GRID_UNINITIALIZED	Grid object not initialized.
		\retval	ERROR_SEVERITY_WARNING	Unspecified failure.
		\retval	S_FALSE	If asking for values where the appropriate array has not been initialized.
		\retval	E_INVALIDARG	Invalid arguments.
	*/
	virtual NO_THROW HRESULT ExportPolygonsWFS(const std::string & url, const std::string & layer,   const std::string & username, const std::string & password);
	/**
		This method adds a polygon defining an area to apply the replacement filter, to the grid filter.  A grid filter can contain many polygons, and polygons can be overlapping.
		However, only one rule applies to all polygons.  Polygons are provided in grid units.
		\param xy_pairs 2D Array of coordinates
		\param	index	Index of the newly added polygon.
		\retval	S_OK	Successful.
		\retval	E_POINTER	Address provided for index, or xy_pairs is invalid.
		\retval	E_OUTOFMEMORY	Insufficient memory.
		\retval	ERROR_SCENARIO_SIMULATION_RUNNING	Value cannot be changed as it is being used in a currently running scenario.
	*/
	virtual NO_THROW HRESULT AddPolygon(const XY_PolyConst &xy_pairs, std::uint32_t *index);
	/**
		This method clears the specified polygon.  If index = (unsigned long) -1, then all polygons are cleared.
		\param	index	Index of polygon.
		\retval	S_OK	Successful.
		\retval	ERROR_FIREBREAK_NOT_FOUND	Polygon does not exist.
		\retval	ERROR_SCENARIO_SIMULATION_RUNNING	Value cannot be changed as it is being used in a currently running scenario.
	*/
	virtual NO_THROW HRESULT ClearPolygon(std::uint32_t index);
	/**
		This method returns the bounding box of a specific polygon, or of all polygons in the grid filter.
		\param	index	Index of polygon.  If the bounding box for all polygons is desired, then pass (unsigned long)-1.
		\param	min_pt	Minimum coordinate.
		\param	max_pt	Maximum coordinate.
		\retval	S_OK	Successful.
		\retval	E_POINTER	Address provided for x_min, x_max, y_min, or y_max is invalid.
		\retval	ERROR_FIREBREAK_NOT_FOUND	Polygon does not exist.
	*/
	virtual NO_THROW HRESULT GetPolygonRange(std::uint32_t index, XY_Point* min_pt, XY_Point* max_pt);
	/**
		This method retrieves a polygon from the grid.  Polygons are retrieved in grid units.
		\param	index	Index of polygon.
		\param	size	Size of polygon.
		\param xy_pairs 2D Array of coordinates
		\retval	S_OK	Successful.
		\retval	E_POINTER	Address provided for size, or xy_pairs is invalid.
		\retval	ERROR_FIREBREAK_NOT_FOUND	Polygon does not exist.
		\retval	E_OUTOFMEMORY	Insufficient memory.
	*/
	virtual NO_THROW HRESULT GetPolygon(std::uint32_t index, XY_Poly *xy_pairs);
	/**
		This method retrieves the area of the grid filter.  Note that this function does not consider overlapping polygons.
		\param	area	Pointer to the retrieves data.
		\sa ICWFGM_PolyReplaceGridFilter::GetArea
		\retval	S_OK	Successful.
		\retval	E_POINTER	Address provided for area is invalid.
	*/
	virtual NO_THROW HRESULT GetArea(double *area);
	/**
		This method gets the number of polygons in the grid.
		\param	count	Number of polygons
		\sa ICWFGM_PolyReplaceGridFilter::GetPolygonCount
		\retval	S_OK	Successful.
		\retval	E_POINTER	Address provided for count is invalid.
	*/
	virtual NO_THROW HRESULT GetPolygonCount(std::uint32_t *count);
	/**
		This method gets polygon size (number of vertices).  Or, if index = -1, then this method gets the maximum size (number of vertices) of the polygons in the grid filter.
		\param	index	Index of polygon.
		\param	size	Size of polygon.
		\retval	S_OK	Successful.
		\retval	E_POINTER	Address provided for size is invalid.
		\retval	ERROR_FIREBREAK_NOT_FOUND	Polygon does not exist.
	*/
	virtual NO_THROW HRESULT GetPolygonSize(std::uint32_t index, std::uint32_t *size);
	/**
		Polymorphic.  This routine retrieves an attribute/option value given the attribute/option index.
		\param option Supported / valid attribute/option index supported are:
		<ul>
		<li><code>CWFGM_ATTRIBUTE_LOAD_WARNING</code>	BSTR.  Any warnings generated by the COM object when deserializating.
		</ul>
		\param value Return value for the attribute/option index
		\sa ICWFGM_PolyReplaceGridFilter::GetAttribute
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
		\param	time	A GMT time.
		\param	fuel		Returned fuel Information.
		\param fuel_valid Indicates if the return value in "fuel" is valid.
		\param cache_bbox: May be NULL. If provided, then the rectangle is filled or adjusted to indicate the maximum size of the cell containing the requested point which is constant and uniform in content and value.
		\sa ICWFGM_GridEngine::GetFuelData
		\sa ICWFGM_PolyReplaceGridFilter::SetRelationship
		\sa ICWFGM_FuelMap
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
		\param fuel_valid Indicates if the return value in "fuel" is valid.
		\sa ICWFGM_PolyReplaceGridFilter::SetRelationship
		\sa ICWFGM_FuelMap
		\retval	E_POINTER	Address provided for value is invalid.
		\retval E_INVALIDARG	The array is not 2D, or is insufficient in size to contain the requested data
		\retval	ERROR_GRID_UNINITIALIZED	No object in the grid layering to forward the request to.
	*/
	virtual NO_THROW HRESULT GetFuelDataArray(Layer *layerThread, const XY_Point &min_pt,  const XY_Point &max_pt, double scale, const HSS_Time::WTime &time, ICWFGM_Fuel_2d *fuel, bool_2d *fuel_valid);
	virtual NO_THROW HRESULT PutGridEngine(Layer *layerThread, ICWFGM_GridEngine *newVal);

#ifndef DOXYGEN_IGNORE_CODE
public:
	virtual std::int32_t serialVersionUid(const SerializeProtoOptions& options) const noexcept override;
	virtual WISE::GridProto::CwfgmPolyReplaceGridFilter* serialize(const SerializeProtoOptions& options) override;
	virtual CCWFGM_PolyReplaceGridFilter *deserialize(const google::protobuf::Message& proto, std::shared_ptr<validation::validation_object> valid, const std::string& name) override;
	virtual std::optional<bool> isdirty(void) const noexcept override { return m_bRequiresSave; }

protected:
	CRWThreadSemaphore		m_lock, m_calcLock;

	std::uint8_t			m_fromIndex;
	boost::intrusive_ptr<ICWFGM_Fuel>		m_fromFuel,
							m_toFuel;
	XY_PolyLLSet			m_polySet;
	bool_2d					*m_replaceArray;
	std::string				m_loadWarning;
	double					m_xllcorner, m_yllcorner, m_resolution, m_iresolution;
	bool					m_allCombustible, m_allNoData;
	std::string				m_gisURL, m_gisLayer, m_gisUID, m_gisPWD;
	unsigned long			m_flags;										// see CWFGM_internal.h for valid options

	std::uint16_t convertX(double x, XY_Rectangle *bbox);
	std::uint16_t convertY(double y, XY_Rectangle *bbox);
	double invertX(double x)			{ return x * m_resolution + m_xllcorner; }
	double invertY(double y)			{ return y * m_resolution + m_yllcorner; }

	void calculateReplaceArray();
	void clearReplaceArray();
	HRESULT fixResolution();

protected:
	bool m_bRequiresSave;

#endif
};

#ifdef HSS_SHOULD_PRAGMA_PACK
#pragma pack(pop)
#endif
