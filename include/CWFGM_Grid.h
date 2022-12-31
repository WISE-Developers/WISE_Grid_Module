/**
 * WISE_Grid_Module: CWFGM_Grid.h
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
#include "results.h"
#include "semaphore.h"
#include "ICWFGM_GridEngine.h"
#include "CWFGM_FuelMap.h"
#include "CWFGM_internal.h"
#include "linklist.h"
#include "ISerializeProto.h"
#include <map>
#include <ogr_api.h>
#include "cwfgmGrid.pb.h"

using namespace HSS_Time;

#ifdef HSS_SHOULD_PRAGMA_PACK
#pragma pack(push, 8)
#endif


#ifndef DOXYGEN_IGNORE_CODE

class GridData {
public:
	double				m_xllcorner,
						m_yllcorner;			// from grid file, for verification
	double				m_resolution,
						m_iresolution;			// resolution of the plot grid (metres)
	std::uint8_t		*m_fuelArray;			// array of fuel types
	bool				*m_fuelValidArray;
	std::int16_t		*m_elevationArray;		// array of elevations
	bool				*m_elevationValidArray;
	bool				*m_terrainValidArray;
	std::uint16_t		*m_slopeFactor;			// slope aspect (%-age up, horizontal plane)
	std::uint16_t		*m_slopeAzimuth;		// slope orientation (degrees on horizontal plane)
	std::uint16_t		m_xsize,
						m_ysize;				// size of our plots
	std::int16_t		m_minElev, m_maxElev, m_medianElev, m_meanElev;
	std::uint16_t		m_minSlopeFactor, m_maxSlopeFactor;
	std::uint16_t		m_minAzimuth, m_maxAzimuth;
	std::uint32_t		m_elevationFrequency[65536];

	GridData();
	GridData(const GridData &toCopy);
	~GridData();

	__INLINE std::uint16_t convertX(double x, XY_Rectangle *bbox);
	__INLINE std::uint16_t convertY(double y, XY_Rectangle *bbox);
	__INLINE double invertX(double x)					{ return x * m_resolution + m_xllcorner; }
	__INLINE double invertY(double y)					{ return y * m_resolution + m_yllcorner; }
	__INLINE std::uint32_t arrayIndex(const std::uint16_t x, const std::uint16_t y) const {
		weak_assert(x < m_xsize);
		weak_assert(y < m_ysize);
		return (m_ysize - (y + 1)) * m_xsize + x;
	};
	XY_Rectangle Bounds() const;
};

#endif


namespace Project {
	class CWFGMProject;
};


/**
 * A CWFGM Grid is a representation of an area and supports two interfaces: one interface to perform basic assignment, modification, and import operations (ICWFGM_Grid), and a second interface for retrieval of data for purposes of simulations, etc. (ICWFGM_GridEngine).  The second interface is a requirement for the simulation engine but is also used elsewhere to extract grid values for the display.  These methods are separated into two interfaces to simplify building another grid object for use with the simulation engine.
 */
class GRIDCOM_API CCWFGM_Grid : public ICWFGM_GridEngine, public ISerializeProto {
	friend class CWFGM_GridHelper;
	friend class Project::CWFGMProject;
public:
	CCWFGM_Grid();
	CCWFGM_Grid(const CCWFGM_Grid &toCopy);
	~CCWFGM_Grid();

public:
	/**
		Polymorphic.  This routine retrieves an attribute/option value given the attribute/option index.
			\param option	Returns the attribute of interest.  Valid attributes are:
		<ul>
		<li><code>CWFGM_GRID_ATTRIBUTE_XLLCORNER</code> 64-bit floating point.  UTM value for the X coordinate of the lower-left corner of the grid extent.
		<li><code>CWFGM_GRID_ATTRIBUTE_YLLCORNER</code> 64-bit floating point.  UTM value for the Y coordinate of the lower-left corner of the grid extent.
		<li><code>CWFGM_GRID_ATTRIBUTE_PLOTRESOLUTION</code> 64-bit floating point, metres.  Resolution of the underlying grid data.
		<li><code>CWFGM_GRID_ATTRIBUTE_LATITUDE</code> 64-bit floating point, radians.
		<li><code>CWFGM_GRID_ATTRIBUTE_LONGITUDE</code> 64-bit floating point, radians.
		<li><code>CWFGM_GRID_ATTRIBUTE_DEFAULT_ELEVATION</code> 64-bit floating point, metres.  User specified elevation to use when there is no grid elevation available for at a requested grid location.
		<li><code>CWFGM_GRID_ATTRIBUTE_MIN_ELEVATION</code> 64-bit floating point, metres.
		<li><code>CWFGM_GRID_ATTRIBUTE_MAX_ELEVATION</code> 64-bit floating point, metres.
		<li><code>CWFGM_GRID_ATTRIBUTE_MEDIAN_ELEVATION</code> 64-bit floating point, metres.
		<li><code>CWFGM_GRID_ATTRIBUTE_MIN_SLOPE</code> 64-bit floating point.  Percentage ground slope specified as a decimal value (0 - 1)
		<li><code>CWFGM_GRID_ATTRIBUTE_MAX_SLOPE</code> 64-bit floating point.  Percentage ground slope specified as a decimal value (0 - 1)
		<li><code>CWFGM_GRID_ATTRIBUTE_MIN_AZIMUTH</code> 64-bit floating point.  Direction of up-slope, Cartesian radians.
		<li><code>CWFGM_GRID_ATTRIBUTE_MAX_AZIMUTH</code> 64-bit floating point.  Direction of up-slope, Cartesian radians.
		<li><code>CWFGM_GRID_ATTRIBUTE_TIMEZONE_ID</code>	32-bit unsigned integer.  A unique ID for a pre-defined set of timezone settings. The timezone information can be retrieved using <code>WorldLocation::TimeZoneFromId</code>.
		<li><code>CWFGM_GRID_ATTRIBUTE_TIMEZONE</code>		64-bit signed integer.  Units are in seconds, relative to GMT.  For example, MST (Mountain Standard Time) would be -6 * 60 * 60 seconds.  Valid values are from -12 hours to +12 hours.
		<li><code>CWFGM_GRID_ATTRIBUTE_DAYLIGHT_SAVINGS</code>	64-bit signed integer.  Units are in seconds.  Amount of correction to apply for daylight savings time.
		<li><code>CWFGM_GRID_ATTRIBUTE_DST_START</code>	64-bit unsigned integer.  Units are in seconds.  Julian date determining when daylight savings starts within the calendar year.
		<li><code>CWFGM_GRID_ATTRIBUTE_DST_END</code>	64-bit unsigned integer.  Units are in seconds.  Julian date determining when daylight savings ends within the calendar year.
		<li><code>CWFGM_GRID_ATTRIBUTE_FUELS_PRESENT</code>  Boolean.  TRUE if a valid fuel grid has been successfully loaded.
		<li><code>CWFGM_GRID_ATTRIBUTE_DEM_PRESENT</code>  Boolean.  TRUE if an elevation grid has been successfully loaded.
		<li><code>CWFGM_ATTRIBUTE_LOAD_WARNING</code>	BSTR.  Any warnings generated by the COM object when deserializating.
		<li><code>CWFGM_GRID_ATTRIBUTE_SPATIALREFERENCE</code> BSTR.  GDAL WKT format string defining the spatial reference of the grid.
		<li><code>CWFGM_GRID_ATTRIBUTE_PROJECTION_UNITS</code> BSTR.  Units of the projection file for the fuel grid.
		</ul>
		\param value	Location for the retrieved value to be placed.
		\sa ICWFGM_Grid::GetAttribute
		\retval	E_POINTER	The address provided for value is invalid.
		\retval	S_OK	Successful.
		\retval	ERROR_SEVERITY_WARNING	If asking for default elevation where the default elevation is not set.
		\retval	S_FALSE	If asking for values where the appropriate array has not been initialized.
		\retval	ERROR_GRID_UNINITIALIZED	No FBP grid data has been loaded.
		\retval	E_OUTOFMEMORY	Insufficient memory (can occur when retrieving slope data which hasn't been loaded so must be calculated).
		\retval	E_INVALIDARG	The attribute/option index was invalid/unknown for this object.
	*/
	NO_THROW HRESULT GetAttribute(std::uint16_t option, PolymorphicAttribute *value);
	/**
		Sets the value for the attribute denoted by "option".
		\param option	The attribute to change.  Valid option values are:
		<ul>
		<li><code>CWFGM_GRID_ATTRIBUTE_LATITUDE</code> 64-bit floating point, radians.
		<li><code>CWFGM_GRID_ATTRIBUTE_LONGITUDE</code> 64-bit floating point, radians.
		<li><code>CWFGM_GRID_ATTRIBUTE_DEFAULT_ELEVATION</code> 64-bit floating point, metres.  User specified elevation to use when there is no grid elevation available for at a requested grid location.
		<li><code>CWFGM_GRID_ATTRIBUTE_TIMEZONE_ID</code>	32-bit unsigned integer.  A unique ID for a pre-defined set of timezone settings. The timezone information can be retrieved using <code>WorldLocation::TimeZoneFromId</code>.
		<li><code>CWFGM_GRID_ATTRIBUTE_TIMEZONE</code>		64-bit signed integer.  Units are in seconds, relative to GMT.  For example, MST (Mountain Standard Time) would be -6 * 60 * 60 seconds.  Valid values are from -12 hours to +12 hours.
		<li><code>CWFGM_GRID_ATTRIBUTE_DAYLIGHT_SAVINGS</code>	64-bit signed integer.  Units are in seconds.  Amount of correction to apply for daylight savings time.
		<li><code>CWFGM_GRID_ATTRIBUTE_DST_START</code>	64-bit unsigned integer.  Units are in seconds.  Julian date determining when daylight savings starts within the calendar year.
		<li><code>CWFGM_GRID_ATTRIBUTE_DST_END</code>	64-bit unsigned integer.  Units are in seconds.  Julian date determining when daylight savings ends within the calendar year.
		<li><code>CWFGM_GRID_ATTRIBUTE_SPATIALREFERENCE</code> BSTR.  GDAL WKT format string defining the spatial reference of the grid.
		</ul>bit flags, defined in "GridCom_Ext.h".
		\param value	The value to set the attribute to.
		\sa ICWFGM_Grid::SetAttribute
		\retval	S_OK	Successful.
		\retval	E_INVALIDARG	On assignment if the value is out of range.
		\retval	ERROR_SCENARIO_SIMULATION_RUNNING	Value cannot be changed as it is being used in a currently running scenario.
		\retval	E_FAIL	The provided value (variant) was of the wrong type that could not be converted to the correct type.
	*/
	NO_THROW HRESULT SetAttribute(std::uint16_t option, const PolymorphicAttribute &value);
	/**
		This method exports grid FBP data to files.
		\param grid_file_name	Grid file name.
		\retval	E_POINTER	Invalid pointer.
		\retval	E_INVALIDARG	Invalid arguments.
		\retval	ERROR_ACCESS_DENIED	Access denied.
		\retval	ERROR_INVALID_HANDLE	Generic file I/O error.
		\retval	ERROR_FILE_EXISTS	The file already exists.
		\retval	ERROR_INVALID_PARAMETER	The parameters are invalid.
		\retval	ERROR_TOO_MANY_OPEN_FILES	Too many files are open.
		\retval	ERROR_FILE_NOT_FOUND	The file cannot be found.
		\retval	E_OUTOFMEMORY	Insufficient memory.
		\retval	ERROR_HANDLE_DISK_FULL Disk full
		\retval	ERROR_SEVERITY_WARNING	Unsure of reason for failure.
		\retval	ERROR_FUELS_FUEL_UNKNOWN	Fueltype is unknown.
		\retval	S_OK	Successful.
	*/
	NO_THROW HRESULT ExportGrid(const std::string & grid_file_name, std::uint32_t compression = 0);
	NO_THROW HRESULT ExportGridWCS(const std::string & url, const std::string & layer, const std::string & username, const std::string & password);
	NO_THROW HRESULT ExportElevationWCS(const std::string & url, const std::string & layer, const std::string & username, const std::string & password);
	NO_THROW HRESULT ExportAspectWCS(const std::string & url, const std::string & layer, const std::string & username, const std::string & password);
	NO_THROW HRESULT ExportSlopeWCS(const std::string & url, const std::string & layer, const std::string & username, const std::string & password);
	/**
		This method will import the elevation data.  The elevation's grid file name is necessary.  The projection file name is optional and if provided,
		must have matching values to the fuel map already imported.
		\param prj	A default projection to use if the grid does not have one associated with it.
		\param grid_file_name	Grid file name.
		\param forcePrj	Force the importer to use the prj instead of any other projections found for the import file.
		\sa ICWFGM_Grid::ImportElevation
		\retval	E_POINTER	If any of the parameters cannot be read.
		\retval	ERROR_SCENARIO_SIMULATION_RUNNING	Value cannot be changed as it is being used in a currently running scenario.
		\retval	S_OK	Successful.
		\retval	ERROR_FILE_NOT_FOUND	If either file cannot be found in the file system.
		\retval	ERROR_ACCESS_DENIED	If either file cannot be opened.
		\retval	ERROR_READ_FAULT | ERROR_SEVERITY_WARNING	The file format is unrecognized, or if there is an error in the file format.
		\retval	ERROR_GRID_LOCATION_OUT_OF_RANGE	The import files do not appear to match data that has already been imported.
		\retval ERROR_GRID_SIZE_INCORRECT		The dimensions and/or resolution of the file do not match data that has already been imported.
		\retval ERROR_GRID_UNSUPPORTED_RESOLUTION	The grid resolution of the file do not match data that has already been imported.
		\retval	E_OUTOFMEMORY	Insufficient memory.
		\retval	SUCCESS_GRID_IMPORT_CONTAINED_NODATA	Successful operation, but the elevation grid contained NODATA entries.
		\retval	SUCCESS_GRID_DATA_UPDATED	The original elevation grid map has been successfully replaced.
		\retval	ERROR_TOO_MANY_OPEN_FILES	System error; current application has too many files open.
		\retval	E_FAIL	A valid geographic projection transformation could not be created.
		\retval	S_OK	Successful.
		\retval	ERROR_INVALID_HANDLE	Generic file I/O error.
		\retval	ERROR_FILE_EXISTS	File already exists.
		\retval	ERROR_INVALID_PARAMETER	Parameter is not valid.
		\retval	ERROR_HANDLE_DISK_FULL Disk full
	*/
	NO_THROW HRESULT ImportElevation(const std::string & prj, const std::string & grid_file_name, bool forcePrj, std::uint8_t *calc_bits);
	/**
		This method will (re)import the FBP grid layer.  Both the PRJ (projection) and the Grid files are needed to complete this operation.  'fail_index' may be NULL.
		\param prj	A default projection to use if the grid does not have one associated with it.
		\param grid_file_name	Grid file name.
		\param forcePrj	Force the importer to use the prj instead of any other projections found for the import file.
		\param fail_index	Fuel import index that was unrecognized, which caused the operation to fail (if it did).
		\sa ICWFGM_Grid::ImportGrid
		\retval	E_POINTER	'prj_file_name', 'grid_file_name', or 'fail_index' cannot be read or written to.
		\retval	S_OK	Successful.
		\retval	ERROR_SCENARIO_SIMULATION_RUNNING	Value cannot be changed as it is being used in a currently running scenario.
		\retval	E_FAIL	A valid geographic projection transformation could not be created.
		\retval	ERROR_READ_FAULT | ERROR_SEVERITY_WARNING	The file format is unrecognized, or if there is an error in the file format.
		\retval	ERROR_GRID_UNINITIALIZED	The FuelMap property has not be set.
		\retval	ERROR_FILE_NOT_FOUND	If either file cannot be found.
		\retval	ERROR_ACCESS_DENIED	If either file cannot be opened.
		\retval	ERROR_FUELS_FUEL_UNKNOWN	The grid file contains a fuel index that is not defined in the FuelMap object.  If 'fail_index' is not NULL, then the index found in the file that is not defined in FuelMap is placed here.
		\retval	ERROR_GRID_LOCATION_OUT_OF_RANGE	The import files do not appear to match data that has already been imported.
		\retval	E_OUTOFMEMORY	Insufficient memory.
		\retval	ERROR_TOO_MANY_OPEN_FILES	System error; current application has too many files open.
		\retval	SUCCESS_GRID_DATA_UPDATED	The original fuel grid map has been successfully replaced.
		\retval	S_OK	Successful.
		\retval	ERROR_INVALID_HANDLE	Generic file I/O error.
		\retval	ERROR_FILE_EXISTS	File already exists.
		\retval	ERROR_INVALID_PARAMETER	Parameter is not valid.
		\retval	ERROR_HANDLE_DISK_FULL Disk full
	*/
	NO_THROW HRESULT ImportGrid(const std::string & prj, const std::string & grid_file_name, bool forcePrj, long *fail_index);
	NO_THROW HRESULT ImportGridWCS(const std::string & url, const std::string & layer, const std::string & username, const std::string & password, XY_Point lowerleft, XY_Point upperright);
	NO_THROW HRESULT ImportElevationWCS(const std::string & url, const std::string & layer, const std::string & username, const std::string & password);
	/**
		Creates a new grid object with all the same properties of the object being called, returns a handle to the new object in 'newGrid'.
		No data is shared between these two objects, an exact copy is created.  The FuelMap property should be set on the copied object immediately after successful completion of this call.
		\param newGrid	The grid object.
		\sa ICWFGM_Grid::Clone
		\retval	E_POINTER	The address provided for newGrid is invalid.
		\retval	S_OK	Successful.
		\retval	E_NOINTERFACE	'newGrid' is not a successfully created CWFGM Grid object.
		\retval	E_OUTOFMEMORY	Insufficient memory.
		\retval	ERROR_SEVERITY_WARNING	Unspecified failure.
	*/
	NO_THROW HRESULT Clone(boost::intrusive_ptr<ICWFGM_CommonBase> *newObject) const;
	/**
		The CWFGM Grid needs an object supporting the ICWFGM_FuelMap interface; the creator of the Grid should assign this property to a valid value before using the grid object.  This property is write-once-read-many because of its high dependency on internal indexes assigned by the fuel  map.  The COM reference count for the fuel map is incremented on assignment and decremented when this object is destroyed.
		\param pVal	The fuel map.
		\sa ICWFGM_Grid::FuelMap
		\retval	E_POINTER	The address provided for index is invalid.
		\retval	S_OK	Successful.
		\retval	E_NOINTERFACE	On assignment if not passed CWFGM FuelMap object.
		\retval	ERROR_GRID_INITIALIZED	If the assignment is tried more than once.
		\retval	ERROR_GRID_UNINITIALIZED	When reading the value before it has been set.
		\retval	ERROR_SCENARIO_SIMULATION_RUNNING	Value cannot be changed as it is being used in a currently running scenario.
	*/
	NO_THROW HRESULT get_FuelMap(CCWFGM_FuelMap * *pVal);
	/**
		The CWFGM Grid needs an object supporting the ICWFGM_FuelMap interface; the creator of the Grid should assign this property to a valid value before using the grid object.  This property is write-once-read-many because of its high dependency on internal indexes assigned by the fuel  map.  The COM reference count for the fuel map is incremented on assignment and decremented when this object is destroyed.
		\param newVal	The fuel map.
		\sa ICWFGM_Grid::FuelMap
		\retval	E_POINTER	The address provided for index is invalid.
		\retval	S_OK	Successful.
		\retval	E_NOINTERFACE	On assignment if not passed CWFGM FuelMap object.
		\retval	ERROR_GRID_INITIALIZED	If the assignment is tried more than once.
		\retval	ERROR_GRID_UNINITIALIZED	When reading the value before it has been set.
		\retval	ERROR_SCENARIO_SIMULATION_RUNNING	Value cannot be changed as it is being used in a currently running scenario.
	*/
	NO_THROW HRESULT put_FuelMap(CCWFGM_FuelMap * newVal);
	/**
		This method reports whether a fuel is assigned to any point in the grid.
		\param fuel	Fuel in question.
		\sa ICWFGM_Grid::IsFuelUsed
		\retval	E_POINTER	Invalid pointer to a fuel.
		\retval	S_OK	If the fuel is used.
		\retval	ERROR_SEVERITY_WARNING	If the fuel is not used.
		\retval	ERROR_FUELS_FUEL_UNKNOWN	'fuel' is not in any associations.
		\retval	ERROR_GRID_UNINITIALIZED	Grid not initialized yet.
	*/
	NO_THROW HRESULT IsFuelUsed(ICWFGM_Fuel *fuel);

	/**
		Changes the state of the object with respect to access rights.  When the object is used by an active simulation, it must not be modified.
		When the object is somehow modified, it must be done so in an atomic action to prevent concerns with arising from multithreading.
		Note that these locks are primarily needed to ensure that data contributing during a simulation is not modified while the simulation is executing.\n\n All routines in the
		ICWFGM_GridEngine interface are necessarily NOT multithreading safe (for performance) but other interfaces for a given COM object
		implementing this interface must be by specification.  Locking request is forwarded to the attached ICWFGM_FuelMap object.\n\n
		In the event of an error, then locking is undone to reflect an error state.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	exclusive	TRUE if the requester wants a write lock, false for read/shared access
		\param	obtain	TRUE to obtain the lock, FALSE to release the lock.  If this is FALSE, then the 'exclusive' parameter must match the initial call used to obtain the lock.
		\sa ICWFGM_GridEngine::MT_Lock
		\sa ICWFGM_FuelMap::MT_Lock
		\retval	SUCCESS_STATE_OBJECT_UNLOCKED	Lock was released.
		\retval	SUCCESS_STATE_OBJECT_LOCKED_WRITE	Exclusive/write lock obtained.
		\retval	SUCCESS_STATE_OBJECT_LOCKED_SCENARIO	A scenario successfully required a lock for purposes of simulating.
		\retval	SUCCESS_STATE_OBJECT_LOCKED_READ	Shared/read lock obtained.
		\retval	S_OK	Successful
		\retval	ERROR_GRID_UNINITIALIZED	No path via layerThread can be determined to further determine successful locks.
	*/
	virtual NO_THROW HRESULT MT_Lock(Layer *layerThread, bool exclusive, std::uint16_t obtain) override;
	/**
		This method determines whether this object is valid for any simulating.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	start_time	Ignored.
		\param	duration	Ignored.
		\param option	Determines type of Valid request.
		\param diurnal_application_count	Optional (dependent on option).  Array of counts for how often a particular type of grid occurs in the set of ICWFGM_GridEngine objects associated with a scenario.
		\sa ICWFGM_GridEngine::Valid
		\retval S_OK		Successful.
		\retval	ERROR_SEVERITY_WARNING	Grid is not initialized.
		\retval ERROR_GRID_WEATHER_NOT_IMPLEMENTED	Grid is valid but does not provide any weather data.
	*/
	virtual NO_THROW HRESULT Valid(Layer *layerThread, const HSS_Time::WTime &start_time, const HSS_Time::WTimeSpan &duration, std::uint32_t option, std::vector<uint16_t> *application_count) override;
	/*! \fn  GetAttribute([in] Layer *layerThread, [in] std::uint16_t option, [out, retval] PolymorphicAttribute *value);
		Polymorphic.  This method returns a specified attribute/option value.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param option	The attribute of interest.  Valid option values are as follows, defined in "GridCom_Ext.h".
		<ul>
		<li><code>CWFGM_GRID_ATTRIBUTE_XLLCORNER</code> 64-bit floating point.  UTM value for the X coordinate of the lower-left corner of the grid extent.
		<li><code>CWFGM_GRID_ATTRIBUTE_YLLCORNER</code> 64-bit floating point.  UTM value for the Y coordinate of the lower-left corner of the grid extent.
		<li><code>CWFGM_GRID_ATTRIBUTE_PLOTRESOLUTION</code> 64-bit floating point, metres.  Resolution of the underlying grid data.
		<li><code>CWFGM_GRID_ATTRIBUTE_LATITUDE</code> 64-bit floating point, radians.
		<li><code>CWFGM_GRID_ATTRIBUTE_LONGITUDE</code> 64-bit floating point, radians.
		<li><code>CWFGM_GRID_ATTRIBUTE_DEFAULT_ELEVATION</code> 64-bit floating point, metres.  User specified elevation to use when there is no grid elevation available for at a requested grid location.
		<li><code>CWFGM_GRID_ATTRIBUTE_MIN_ELEVATION</code> 64-bit floating point, metres.
		<li><code>CWFGM_GRID_ATTRIBUTE_MAX_ELEVATION</code> 64-bit floating point, metres.
		<li><code>CWFGM_GRID_ATTRIBUTE_MEDIAN_ELEVATION</code> 64-bit floating point, metres.
		<li><code>CWFGM_GRID_ATTRIBUTE_MIN_SLOPE</code> 64-bit floating point.  Percentage ground slope specified as a decimal value (0 - 1)
		<li><code>CWFGM_GRID_ATTRIBUTE_MAX_SLOPE</code> 64-bit floating point.  Percentage ground slope specified as a decimal value (0 - 1)
		<li><code>CWFGM_GRID_ATTRIBUTE_MIN_AZIMUTH</code> 64-bit floating point.  Direction of up-slope, Cartesian radians.
		<li><code>CWFGM_GRID_ATTRIBUTE_MAX_AZIMUTH</code> 64-bit floating point.  Direction of up-slope, Cartesian radians.
		<li><code>CWFGM_GRID_ATTRIBUTE_TIMEZONE_ID</code>	32-bit unsigned integer.  A unique ID for a pre-defined set of timezone settings. The timezone information can be retrieved using <code>WorldLocation::TimeZoneFromId</code>.
		<li><code>CWFGM_GRID_ATTRIBUTE_TIMEZONE</code>		64-bit signed integer.  Units are in seconds, relative to GMT.  For example, MST (Mountain Standard Time) would be -6 * 60 * 60 seconds.  Valid values are from -12 hours to +12 hours.
		<li><code>CWFGM_GRID_ATTRIBUTE_DAYLIGHT_SAVINGS</code>	64-bit signed integer.  Units are in seconds.  Amount of correction to apply for daylight savings time.
		<li><code>CWFGM_GRID_ATTRIBUTE_DST_START</code>	64-bit unsigned integer.  Units are in seconds.  Julian date determining when daylight savings starts within the calendar year.
		<li><code>CWFGM_GRID_ATTRIBUTE_DST_END</code>	64-bit unsigned integer.  Units are in seconds.  Julian date determining when daylight savings ends within the calendar year.
		<li><code>CWFGM_GRID_ATTRIBUTE_FUELS_PRESENT</code>  Boolean.  TRUE if a valid fuel grid has been successfully loaded.
		<li><code>CWFGM_GRID_ATTRIBUTE_DEM_PRESENT</code>  Boolean.  TRUE if an elevation grid has been successfully loaded.
		<li><code>CWFGM_ATTRIBUTE_LOAD_WARNING</code>	BSTR.  Any warnings generated by the COM object when deserializating.
		<li><code>CWFGM_GRID_ATTRIBUTE_SPATIALREFERENCE</code> BSTR.  GDAL WKT format string defining the spatial reference of the grid.
		</ul>
		\param value	Location for the retrieved value to be placed.
		\sa ICWFGM_GridEngine::GetAttribute
		\retval	E_POINTER	The address provided for value is invalid.
		\retval	S_OK	Successful.
		\retval	ERROR_SEVERITY_WARNING	If asking for default elevation where the default elevation is not set.
		\retval	S_FALSE	If asking for values where the appropriate array has not been initialized.
		\retval	ERROR_GRID_UNINITIALIZED	No FBP grid data has been loaded.
		\retval	E_OUTOFMEMORY	Insufficient memory (can occur when retrieving slope data which hasn't been loaded so must be calculated).
		\retval	E_INVALIDARG	The attribute/option index was invalid/unknown for this object.
	*/
	virtual NO_THROW HRESULT GetAttribute(Layer *layerThread, std::uint16_t option, PolymorphicAttribute *value) override;
	virtual NO_THROW HRESULT GetCommonData(Layer* layerThread, ICWFGM_CommonData** pVal);
	/**
		This method provides information regarding grid dimenions
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	x_dim	X dimension of the grid
		\param	y_dim	Y dimension of the grid
		\sa ICWFGM_GridEngine::GetDimensions
		\retval	E_POINTER	The address provided for value is invalid.
		\retval	S_OK	Successful.
		\retval	ERROR_SEVERITY_WARNING	If asking for default elevation where the default elevation is not set.
		\retval	S_FALSE	If asking for values where the appropriate array has not been initialized.
		\retval	ERROR_GRID_UNINITIALIZED	No FBP grid data has been loaded.
		\retval	E_OUTOFMEMORY	Insufficient memory (can occur when retrieving slope data which hasn't been loaded so must be calculated).
		\retval	E_INVALIDARG	The attribute/option index was invalid/unknown for this object.
	*/
	virtual NO_THROW HRESULT GetDimensions(Layer *layerThread, std::uint16_t *x_dim, std::uint16_t *y_dim) override;
	/**
		Returns fuel data from the imported fuel grid map.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	pt			Location.
		\param	time	A GMT time.
		\param	fuel		Fuel Information.
		\param fuel_valid Indicates if the return value in 'fuel' is valid.
		\param cache_bbox: May be NULL. If provided, then the rectangle is filled or adjusted to indicate the maximum size of the cell containing the requested point which is constant and uniform in content and value.
		\sa ICWFGM_GridEngine::GetFuelData
		\retval	ERROR_GRID_UNINITIALIZED	No grid data has been loaded
		\retval ERROR_GRID_LOCATION_OUT_OF_RANGE Requested location (in grid units) is outside the grid's bounds.
		\retval E_POINTER fuel is invalid
		\retval ERROR_FUELS_FUEL_UNKNOWN	The location requested contains NODATA
	*/
	virtual NO_THROW HRESULT GetFuelData(Layer *layerThread, const XY_Point &pt,const HSS_Time::WTime &time, ICWFGM_Fuel **fuel, bool *fuel_valid, XY_Rectangle *cache_bbox) override;
	/**
		Returns fuel index data from the imported fuel grid map.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	pt			Location.
		\param	time	A GMT time.
		\param	fuel_index	Fuel Index Information.
		\param fuel_valid Indicates if the return value in 'fuel' is valid.
		\param cache_bbox: May be NULL. If provided, then the rectangle is filled or adjusted to indicate the maximum size of the cell containing the requested point which is constant and uniform in content and value.
		\sa ICWFGM_GridEngine::GetFuelIndexData
		\retval	ERROR_GRID_UNINITIALIZED	No grid data has been loaded
		\retval ERROR_GRID_LOCATION_OUT_OF_RANGE Requested location (in grid units) is outside the grid's bounds.
		\retval E_POINTER fuel is invalid
		\retval ERROR_FUELS_FUEL_UNKNOWN	The location requested contains NODATA
	*/
	virtual NO_THROW HRESULT GetFuelIndexData(Layer *layerThread, const XY_Point &pt,const HSS_Time::WTime &time, std::uint8_t *fuel_index, bool *fuel_valid, XY_Rectangle *cache_bbox) override;
	virtual NO_THROW HRESULT GetElevationData(Layer *layerThread, const XY_Point &pt, bool allow_defaults_returned, double *elevation, double
	    *slope_factor, double *slope_azimuth, grid::TerrainValue*elev_valid, grid::TerrainValue *terrain_valid, XY_Rectangle *cache_bbox) override;
	/**
		This object does not implement any functionality regarding weather.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	pt			Location.
		\param	time	A GMT time.
		\param	interpolate_method		Interpolation method identifier.
		\param	wx			Weather information.
		\param	ifwi		IFWI Information.
		\param	dfwi		DFWI Information.
		\sa ICWFGM_GridEngine::GetWeatherData
		\retval	E_NOTIMPL	Method not implemented
	*/
	virtual NO_THROW HRESULT GetWeatherData(Layer *layerThread, const XY_Point &pt, const HSS_Time::WTime &time, std::uint64_t interpolate_method,
	     IWXData *wx, IFWIData *ifwi, DFWIData *dfwi, bool *wx_valid, XY_Rectangle *cache_bbox) override;
	/**
		Polymorphic.  	This object does not implement any functionality regarding specific polymorphic gridded attributes.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	pt			Location.
		\param	time	A GMT time.
		\param	option		Data array option.
		\param	attribute	Data array attribute.
		\param cache_bbox: May be NULL. If provided, then the rectangle is filled or adjusted to indicate the maximum size of the cell containing the requested point which is constant and uniform in content and value.
		\sa ICWFGM_GridEngine::GetAttributeData
		\retval	E_NOTIMPL	Method not implemented
	*/
	virtual NO_THROW HRESULT GetAttributeData(Layer *layerThread, const XY_Point &pt,const HSS_Time::WTime &time, const HSS_Time::WTimeSpan& timeSpan, std::uint16_t option, std::uint64_t optionFlags,   NumericVariant *attribute, grid::AttributeValue *attribute_valid, XY_Rectangle *cache_bbox) override;
	/**
		Fills in the provided array of fuel data from the imported fuel grid map.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	min_pt		Minimum value (inclusive).
		\param	max_pt		Maximum value (inclusive).
		\param	scale		Scale (meters) that the array is defined for
		\param	time	A GMT time.
		\param	fuel		Data array attribute.
		\param fuel_valid Indicates if the return value in 'fuel' is valid.
		\sa ICWFGM_GridEngine::GetFuelDataArray
		\retval E_INVALIDARG	The array is not 2D, or is insufficient in size to contain the requested data
		\retval	ERROR_GRID_UNINITIALIZED	No grid data has been loaded
		\retval ERROR_GRID_LOCATION_OUT_OF_RANGE Requested location (in grid units) is outside the grid's bounds.
		\retval E_POINTER fuel is invalid
		\retval ERROR_FUELS_FUEL_UNKNOWN	The location requested contains NODATA
	*/
	virtual NO_THROW HRESULT GetFuelDataArray(Layer *layerThread, const XY_Point &min_pt,const XY_Point &max_pt, double scale, const HSS_Time::WTime &time, ICWFGM_Fuel_2d *fuel, bool_2d *fuel_valid) override;
	/**
		Fills in the provided array of fuel index data from the imported fuel grid map.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	min_pt		Minimum value (inclusive).
		\param	max_pt		Maximum value (inclusive).
		\param	scale		Scale (meters) that the array is defined for
		\param	time	A GMT time.
		\param	fuel		Data array attribute.
		\param fuel_valid Indicates if the return value in 'fuel' is valid.
		\sa ICWFGM_GridEngine::GetFuelIndexDataArray
		\retval E_INVALIDARG	The array is not 2D, or is insufficient in size to contain the requested data
		\retval	ERROR_GRID_UNINITIALIZED	No grid data has been loaded
		\retval ERROR_GRID_LOCATION_OUT_OF_RANGE Requested location (in grid units) is outside the grid's bounds.
		\retval E_POINTER fuel is invalid
		\retval ERROR_FUELS_FUEL_UNKNOWN	The location requested contains NODATA
	*/
	virtual NO_THROW HRESULT GetFuelIndexDataArray(Layer *layerThread, const XY_Point &min_pt,const XY_Point &max_pt, double scale,const HSS_Time::WTime &time, uint8_t_2d *fuel, bool_2d *fuel_valid) override;
	virtual NO_THROW HRESULT GetElevationDataArray(Layer *layerThread, const XY_Point &min_pt,const XY_Point &max_pt, double scale, bool allow_defaults_returned,
	    double_2d *elevation, double_2d *slope_factor, double_2d *slope_azimuth, terrain_t_2d* elev_valid, terrain_t_2d* terrain_valid) override;
	/**
		This object does not implement any functionality regarding weather.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	min_pt		Minimum value (inclusive).
		\param	max_pt		Maximum value (inclusive).
		\param	scale		Scale (meters) that the array is defined for
		\param	time	A GMT time.
		\param	interpolate_method		Interpolation method identifier.
		\param	wx		Array of Weather information.
		\param	ifwi		Array of Instantaneous FWI codes.
		\param	dfwi		Array of Daily FWI codes.
		\sa ICWFGM_GridEngine::GetWeatherDataArray
		\retval	ERROR_GRID_UNINITIALIZED	No object in the grid layering to forward the request to.
	*/
	virtual NO_THROW HRESULT GetWeatherDataArray(Layer *layerThread, const XY_Point &min_pt,const XY_Point &max_pt, double scale,const HSS_Time::WTime &time, std::uint64_t interpolate_method,
	    IWXData_2d *wx, IFWIData_2d *ifwi, DFWIData_2d *dfwi, bool_2d *wx_valid) override;
	/**
		Polymorphic.  	This object does not implement any functionality regarding specific polymorphic gridded attributes.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	min_pt		Minimum value (inclusive).
		\param	max_pt		Maximum value (inclusive).
		\param	scale		Scale (meters) that the array is defined for
		\param	time	A GMT time.
		\param	option		Data array option.
		\param	attribute	Data array attribute.
		\sa ICWFGM_GridEngine::GetAttributeDataArray
		\retval	E_NOTIMPL	Method not implemented
	*/
	virtual NO_THROW HRESULT GetAttributeDataArray(Layer *layerThread, const XY_Point &min_pt,const XY_Point &max_pt, double scale, const HSS_Time::WTime &time, const HSS_Time::WTimeSpan& timeSpan, std::uint16_t option, std::uint64_t optionFlags,   NumericVariant_2d *attribute, attribute_t_2d *attribute_valid) override;
	/**
		This filter object simply returns since it does not introduce any events to a simulation.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	flags	Calculation flags.
		\param	from_time	A GMT time provided as seconds since January 1st, 1600.
		\param	next_event	A GMT time provided as seconds since January 1st, 1600, representing the time for the next event, based on 'flags'.
		\sa ICWFGM_GridEngine::GetEventTime
		\retval ERROR_SEVERITY_WARNING	Grid is not initialized.
		\retval	S_OK		This object does not introduce any simulation events.
	*/
	virtual NO_THROW HRESULT GetEventTime(Layer *layerThread, const XY_Point& pt, std::uint32_t flags, const HSS_Time::WTime &from_time, HSS_Time::WTime *next_event, bool* event_valid) override;
	/**
		This object requires no work to be performed in this call.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	time	A GMT time.
		\param	mode	Calculation mode.
		\param	parms	parameters to send and receive values from/to the simulation and grid objects, may be NULL
		\sa ICWFGM_GridEngine::PreCalculationEvent
		\retval	S_OK	Always successful.
	*/
	virtual NO_THROW HRESULT PreCalculationEvent(Layer *layerThread, const HSS_Time::WTime &time, std::uint32_t mode, CalculationEventParms *parms) override;
	/**
		This object requires no work to be performed in this call.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	time	A GMT time.
		\param	mode	Calculation mode.
		\param	parms	parameters to send and receive values from/to the simulation and grid objects, may be NULL
		\sa ICWFGM_GridEngine::PostCalculationEvent
		\retval	S_OK	Always successful
	*/
	virtual NO_THROW HRESULT PostCalculationEvent(Layer *layerThread, const HSS_Time::WTime &time, std::uint32_t mode, CalculationEventParms *parms) override;
	virtual NO_THROW HRESULT PutGridEngine(Layer *layerThread, ICWFGM_GridEngine * newVal) override;

public:
	/**
		Rather than importing an FBP fuel grid, this function will allow the client application to manually create the grid.  This grid has a location and is set to a single fuel type.
		\param xsize	East-west dimension size.
		\param ysize	North-south dimension size.
		\param xllcorner	Location.
		\param yllcorner	Location.
		\param resolution	Resolution of each grid cell.
		\param BasicFuel	The initial fuel to assign the entire grid to.
		\sa ICWFGM_Grid::CreateGrid
		\retval	S_OK	Successful.
		\retval	E_INVALIDARG	If any of the parameters are invalid.
		\retval	ERROR_GRID_UNINITIALIZED	Grid objects are not correctly initialized.
		\retval	ERROR_FUELS_FUEL_UNKNOWN	The index provide is unknown by the fuel map.
		\retval	E_OUTOFMEMORY	Insufficient memory to create the grid.
		\retval	ERROR_SCENARIO_SIMULATION_RUNNING	Value cannot be changed as it is being used by a currently running scenario.
	*/
	virtual NO_THROW HRESULT CreateGrid(std::uint16_t xsize, std::uint16_t ysize, double xllcorner, double yllcorner, double resolution, std::uint8_t BasicFuel);
	/**
		Used primarily for testing purposes.  Sets elevation, slope, aspect to constant values.  Note that elevations are NOT calculated to match slope, aspect data.
		\param elevation	Elevation for the entire grid.
		\param slope	Slope for the entire grid.
		\param aspect	Aspect of the slope for the entire grid.
		\sa ICWFGM_Grid::CreateSlopeElevationGrid
		\retval	S_OK	Successful.
		\retval	ERROR_GRID_INITIALIZED	Data had already been loaded/initialized and so cannot be reset.
		\retval	E_OUTOFMEMORY	Insufficient memory to create the grid.
		\retval	ERROR_SCENARIO_SIMULATION_RUNNING	Value cannot be changed as it is being used by a currently running scenario.
	*/
	virtual NO_THROW HRESULT CreateSlopeElevationGrid(std::int16_t elevation, std::uint16_t slope, std::uint16_t aspect);
	/**
		Writes a projection file.  All imported grids share the same dimensions and projection information.
		\param szPath	The path/file to write the projection to.
		\sa ICWFGM_Grid::WriteProjection
		\retval	E_POINTER	Path is invalid.
		\retval E_INVALIDARG	Path is invalid.
		\retval	S_OK	Successful.
		\retval	S_INVALIDARG	Invalid arguments.
		\retval	ERROR_ACCESS_DENIED	Access denied.
		\retval	ERROR_INVALID_HANDLE	Generic file I/O error.
		\retval	ERROR_FILE_EXISTS	The file already exists.
		\retval	ERROR_INVALID_PARAMETER	The parameters are invalid.
		\retval	ERROR_TOO_MANY_OPEN_FILES	Too many files are open.
		\retval	ERROR_FILE_NOT_FOUND	The file cannot be found.
		\retval	E_OUTOFMEMORY	Insufficient memory.
		\retval	ERROR_HANDLE_DISK_FULL Disk full
		\retval	ERROR_SEVERITY_WARNING	Unsure of reason for failure.
	*/
	virtual NO_THROW HRESULT WriteProjection(const std::string & szPath);
	/**
		This method exports elevation data from the grid to a file
		\param	grid_file_name	File name where the data should be exported.
		\sa ICWFGM_Grid::ExportElevation
		\retval	S_OK	Successful.
		\retval E_POINTER	Bad pointer.
		\retval	ERROR_GRID_UNINITIALIZED	Grid not initialized.
		\retval E_INVALIDARG	Invalid argument(s).
	*/
	virtual NO_THROW HRESULT ExportElevation(const std::string & grid_file_name);
	/**
		This method exports Slope data from the grid to a file
		\param	grid_file_name	File name where the data should be exported.
		\sa ICWFGM_Grid::ExportSlope
		\retval	S_OK	Successful.
		\retval E_POINTER	Bad pointer.
		\retval	ERROR_GRID_UNINITIALIZED	Grid not initialized.
		\retval E_INVALIDARG	Invalid argument(s).
	*/
	virtual NO_THROW HRESULT ExportSlope(const std::string & grid_file_name);
	/**
		This method export Aspect data from the grid to a file
		\param	grid_file_name	File name where the data should be exported.
		\sa ICWFGM_Grid::ExportAspect
		\retval	S_OK	Successful.
		\retval E_POINTER	Bad pointer.
		\retval	ERROR_GRID_UNINITIALIZED	Grid not initialized.
		\retval E_INVALIDARG	Invalid argument(s).
	*/
	virtual NO_THROW HRESULT ExportAspect(const std::string & grid_file_name);

protected:
	NO_THROW HRESULT calculateSlopeFactorAndAzimuth(Layer *layerThread, std::uint8_t *calc_bits);
	NO_THROW bool interpolateElevation(GridData *gd, std::uint16_t i, std::uint16_t j, std::uint16_t *elev);
	void determineElevationAreas(std::uint8_t *outside);

#ifndef DOXYGEN_IGNORE_CODE
public:
	virtual std::int32_t serialVersionUid(const SerializeProtoOptions& options) const noexcept override;
	virtual WISE::GridProto::CwfgmGrid* serialize(const SerializeProtoOptions& options) override;
	virtual CCWFGM_Grid *deserialize(const google::protobuf::Message& proto, std::shared_ptr<validation::validation_object> valid, const std::string& name) override;
	virtual std::optional<bool> isdirty(void) const noexcept override { return m_bRequiresSave; }

protected:
	GridData			*m_gridData(Layer *layerThread);
	void				assignGridData(Layer *layerThread, GridData *gd);
	boost::intrusive_ptr<CCWFGM_FuelMap>	m_fuelMap;
	CRWThreadSemaphore	m_lock;
	CThreadSemaphore	m_tokenlock;

	// stuff we get from the PRJ files
	std::string			m_projectionContents,
						m_tokens,
						m_header,
						m_units,
						m_loadWarning;
	OGRSpatialReferenceH	m_sourceSRS;
	WorldLocation		m_worldLocation;			// latitude, longitude, and time zone of the plot
	WTimeManager		m_timeManager;				// works hand-in-hand with WorldLocation
	ICWFGM_CommonData	m_commonData;
	GridData			m_baseGrid;

	double			m_defaultElevation;			// to be used when no DEM is provided
	double			m_defaultFMC;				// used for areas outside of Canada where the FMC calculations are no good

	std::string		m_gisGridURL, m_gisGridLayer, m_gisGridUID, m_gisGridPWD;
	std::string		m_gisElevURL, m_gisElevLayer, m_gisElevUID, m_gisElevPWD;
	double			m_initialSize, m_growSize, m_reactionSize;

	unsigned long	m_flags;					// see CWFGM_internal.h for valid options

	bool fixWorldLocation();
	void calcWarnings(const std::uint8_t calc_bits);

	HRESULT sizeGrid(Layer *layerThread, CalculationEventParms *parms);

protected:
	bool m_bRequiresSave;

#endif
};

#ifdef HSS_SHOULD_PRAGMA_PACK
#pragma pack(pop)
#endif
