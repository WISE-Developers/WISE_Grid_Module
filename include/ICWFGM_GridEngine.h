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

#pragma once

#include "GridCOM.h"
#include "ICWFGM_Fuel.h"
#include "CWFGM_LayerManager.h"
#include "WTime.h"
#include "rectangles.h"
#include "propsysreplacement.h"
#include "ICWFGM_Weather.h"
#include "GridCom_ext.h"
#include <boost/multi_array.hpp>

class ICWFGM_CommonData {
public:
	HSS_Time::WTimeManager* m_timeManager;
};


class CalculationEventParms {
public:
	XY_Point SimulationMin;
	XY_Point SimulationMax;
	XY_Point CurrentGridMin;
	XY_Point CurrentGridMax;
	XY_Point TargetGridMin;
	XY_Point TargetGridMax;
};


typedef boost::multi_array<ICWFGM_Fuel *, 2> ICWFGM_Fuel_2d;
typedef boost::multi_array_ref<ICWFGM_Fuel *, 2> ICWFGM_Fuel_2d_ref;
typedef boost::const_multi_array_ref<ICWFGM_Fuel *, 2> const_ICWFGM_Fuel_2d_ref;

typedef boost::multi_array<bool, 2> bool_2d;
typedef boost::multi_array_ref<bool, 2> bool_2d_ref;
typedef boost::const_multi_array_ref<bool, 2> const_bool_2d_ref;

typedef boost::multi_array<std::uint8_t, 2> uint8_t_2d;
typedef boost::multi_array_ref<std::uint8_t, 2> uint8_t_2d_ref;
typedef boost::const_multi_array_ref<std::uint8_t, 2> const_uint8_t_2d_ref;

typedef boost::multi_array<grid::TerrainValue, 2> terrain_t_2d;
typedef boost::multi_array_ref<grid::TerrainValue, 2> terrain_t_2d_ref;
typedef boost::const_multi_array_ref<grid::TerrainValue, 2> const_terrain_t_2d_ref;

typedef boost::multi_array<grid::AttributeValue, 2> attribute_t_2d;
typedef boost::multi_array_ref<grid::AttributeValue, 2> attribute_t_2d_ref;
typedef boost::const_multi_array_ref<grid::AttributeValue, 2> const_attribute_t_2d_ref;

typedef boost::multi_array<std::uint16_t, 2> uint16_t_2d;
typedef boost::multi_array_ref<std::uint16_t, 2> uint16_t_2d_ref;
typedef boost::const_multi_array_ref<std::uint16_t, 2> const_uint16_t_2d_ref;

typedef boost::multi_array<double, 2> double_2d;
typedef boost::multi_array_ref<double, 2> double_2d_ref;
typedef boost::const_multi_array_ref<double, 2> const_double_2d_ref;

typedef boost::multi_array<IWXData, 2> IWXData_2d;
typedef boost::multi_array_ref<IWXData, 2> IWXData_2d_ref;
typedef boost::const_multi_array_ref<IWXData, 2> const_IWXData_2d_ref;

typedef boost::multi_array<IFWIData, 2> IFWIData_2d;
typedef boost::multi_array_ref<IFWIData, 2> IFWIData_2d_ref;
typedef boost::const_multi_array_ref<IFWIData, 2> const_IFWIData_2d_ref;

typedef boost::multi_array<DFWIData, 2> DFWIData_2d;
typedef boost::multi_array_ref<DFWIData, 2> DFWIData_2d_ref;
typedef boost::const_multi_array_ref<DFWIData, 2> const_DFWIData_2d_ref;

typedef boost::multi_array<NumericVariant, 2> NumericVariant_2d;
typedef boost::multi_array_ref<NumericVariant, 2> NumericVariant_2d_ref;
typedef boost::const_multi_array_ref<NumericVariant, 2> const_NumericVariant_2d_ref;


/**
 * Grid data is provided to the simulation engine by the client application.  The simulation engine defines the interface it will use to obtain the grid data (via this interface).  The simulation engine is not concerned with the kind of object that implements this interface, or the implementation of the object.  It may represent the original CWFGM grid data, a CWFGM grid filter object or some other object a programmer has built to access a specialized data set.
 * The simulation engine does not define any object that implements this interface.  The simulation engine only defines the interface.  Objects that implement this interface should import this interface definition.  The object implementing this interface is to be handled entirely by the client application (such as CWFGM).  Methods to import grid data or manage grid filters are not found on this COM interface, as the simulation engine has no need for these methods.
 * This design takes advantage of object-oriented methodologies to simplify both the design of the simulation engine, and the integration of the simulation engine into another application.  For example, to provide data from a G.I.S. directly to the simulation engine, the programmer only needs to define an object that implements this interface and communicates with the G.I.S.
 * A variety of objects that implement this interface already exist.
 * 
 * There are no methods on the simulation engine interface to add or remove FBP fuel types.  This is because the simulation engine is not concerned with managing fuel types, or the look-up tables used to associate a fuel type with a grid map entry.  These tasks are left entirely to the client application.  This works because the interface method GetFuelData()only returns handles to FBP fuel types after the client has finished resolving its own look-up tables and grid filters.
 * Similarly, there are no methods on the simulation engine interface to add or remove firebreaks or firelines.  Firebreaks and firelines are represented by objects supporting the FBP fuel type interface.  The object implementing the Grid COM interface locates firebreaks, etc. on the fuel array and returns the correct handles via the GetFuelData() method.  Any temporal aspect of these objects is the responsibility of the client application.
 * The IDL of the grid object required by the simulation engine is given below.
 */
class GRIDCOM_API ICWFGM_GridEngine : public ICWFGM_CommonBase {
public:
	ICWFGM_GridEngine();
	virtual ~ICWFGM_GridEngine();

	/**
		Changes the state of the object with respect to access rights.  When the object is used by an active simulation, it must not be modified.
		When the object is somehow modified, it must be done so in an atomic action to prevent concerns with arising from multithreading.
		Note that these locks are primarily needed to ensure that data contributing during a simulation is not modified while the simulation is executing.\n\n All routines in the
		ICWFGM_GridEngine interface are necessarily NOT multithreading safe (for performance) but other interfaces for a given COM object implementing this interface must be by specification.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	exclusive	TRUE if the requester wants a write lock, false for read/shared access
		\param	obtain	TRUE to obtain the lock, FALSE to release the lock.  If this is FALSE, then the 'exclusive' parameter must match the initial call used to obtain the lock.
	*/
	virtual HRESULT MT_Lock(class Layer *layerThread, bool exclusive, std::uint16_t obtain) = 0;

	/**
		This method returns whether this grid object has been initialized,and whether it contains all data necessary for a simulation (including weather data) for the simulation time identified by start_time and duration.
		If start_time and duration are both 0, then this method only returns whether the object is initialized.
		The invoked object is allowed to perform a variety of other operations and method calls to initialize and validate, and possibly cache data.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	start_time	Start time of the simulation.  A GMT time provided as seconds since January 1st, 1600.
		\param	duration	Duration of the simulation, in seconds.
		\param option	Determines type of Valid request.
		\param application_count	Optional (dependent on option).  Array of counts for how often a particular type of grid occurs in the set of ICWFGM_GridEngine objects associated with a scenario.
	*/
	virtual HRESULT Valid(Layer *layerThread, const HSS_Time::WTime &start_time, const HSS_Time::WTimeSpan &duration, std::uint32_t option, std::vector<uint16_t> *application_count);

	/**
		Polymorphic.  This method returns a specified value obtained from the projection file or grid data.
		These values are useful for importing vector data, locating the plot on the planet, etc. and are not specific to any particular location within the grid.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	option	The attribute of interest.
		\param	value	Location for the retrieved value to be placed.
	*/
	virtual HRESULT GetAttribute(Layer *layerThread, std::uint16_t option, PolymorphicAttribute *value) = 0;

	/**
		This method assigns 'x_dim' and 'y_dim' to the size of the grid, in grid units.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	x_dim	x dimension of the grid.
		\param	y_dim	y dimension of the grid.
	*/
	virtual HRESULT GetDimensions(Layer *layerThread, std::uint16_t *x_dim, std::uint16_t *y_dim);

	/**
		Given a (X,Y) location in the grid and a GMT time provided as seconds since January 1st, 1600), returns a handle to a CWFGM Fuel.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	pt			Location on the grid.
		\param	time	A GMT time.
		\param	fuel	A CWGFM fuel.
		\param fuel_valid Indicates if the return value in �fuel� is valid.
		\param cache_bbox: May be NULL. If provided, then the rectangle is filled or adjusted to indicate the maximum size of the cell containing the requested point which is constant and uniform in content and value.
	*/
	virtual HRESULT GetFuelData(Layer *layerThread, const XY_Point &pt, const HSS_Time::WTime &time, ICWFGM_Fuel **fuel, bool *fuel_valid, XY_Rectangle *cache_bbox);

	/**
		Given a (X,Y) location in the grid and a GMT time since January 1st, 1600, returns an internal index of a CWFGM Fuel.  The fuel can then be determined by a call to the attached FuelMap object.
		This method is not used by the simulation engine, but instead is intended for filters that need core data from the grid.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	pt		The grid location.
		\param	time	A GMT time.
		\param	fuel_index	Index of a CWFGM fuel.
	*/
	virtual HRESULT GetFuelIndexData(Layer *layerThread, const XY_Point &pt, const HSS_Time::WTime &time, std::uint8_t *fuel_index, bool *fuel_valid, XY_Rectangle *cache_bbox);

	/**
		Given a (X,Y) location in the grid, returns elevation information.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	pt	The grid location.
		\param	allow_defaults_returned	Flag for allowing defaults to be returned
		\param	elevation	Elevation (m) of the requested location.
		\param	slope_factor	Percentage ground slope specified as a decimal value (0 - 1)
		\param	slope_azimuth	Direction of up-slope, Cartesian radians.
	*/
	virtual HRESULT GetElevationData(Layer *layerThread, const XY_Point &pt, bool allow_defaults_returned, double *elevation,
			double *slope_factor, double *slope_azimuth, grid::TerrainValue *elev_valid, grid::TerrainValue *terrain_valid, XY_Rectangle *cache_bbox);

	/**
		Polymorphic.  Given a (X,Y) location in the the grid and a GMT time since January 1st, 1600, returns an attribute value keyed on 'option'.  Note that only specific objects implementing this interface can be expected
		to return valid values from this function call.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	pt		Grid location.
		\param	time	A GMT time.
		\param	option	Option information.  Currently expected values are:
		<ul>
		<li><code>FUELCOM_ATTRIBUTE_PC</code>  Percent conifer grid attribute (optional).
		<li><code>FUELCOM_ATTRIBUTE_PDF</code>  Percent dead fir grid attribute (optional).
		<li><code>FUELCOM_ATTRIBUTE_CURINGDEGREE</code>  Grass curing degree grid attribute (optional).
		<li><code>FUELCOM_ATTRIBUTE_CBH</code>  Crown base height grid attribute (optional).
		<li><code>FUELCOM_ATTRIBUTE_TREE_HEIGHT</code> Tree height grid attribute (optional).
		</ul>
		All other values are reserved for future use.
		\param	attribute	Attirbute data
	*/
	virtual HRESULT GetAttributeData(Layer *layerThread, const XY_Point &pt, const HSS_Time::WTime &time, const HSS_Time::WTimeSpan& timeSpan, std::uint16_t option,
			std::uint64_t optionFlags, NumericVariant *attribute, grid::AttributeValue *attribute_valid, XY_Rectangle *cache_bbox);

	/**
		Given a (X,Y) location in the grid, returns weather information.  Note that only specific objects implementing this interface can be expected to return valid values from this function call.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	pt	Grid location.
		\param	time	A GMT time.
		\param	interpolate_method	Method of interpolation.
		\param	wx			Weather information.
		\param	ifwi		IFWI Information.
		\param	dfwi		DFWI Information.
	*/
	virtual HRESULT GetWeatherData(Layer *layerThread, const XY_Point &pt, const HSS_Time::WTime &time,
			std::uint64_t interpolate_method, IWXData *wx, IFWIData *ifwi, DFWIData *dfwi, bool *wx_valid, XY_Rectangle *cache_bbox);

	/**
		Given a ('X_min','Y_min')->('X_max','Y_max') range of locations (inclusive) in the grid, and a GMT time (seconds since January 1st, 1600), this method fills the provided array with handles to a CWFGM Fuels.
		This routine uses the array provided by the client rather than returning allocated memory in case the client wants to make successive requests - this will avoid repeated memory allocations and deallocations.
		The 2D array is expected to begin at (0,0) and contain enough room for both dimensions of requested data.
		When using this routine to retrieve 2000 points at a time, it is approximately 75% faster than successive GetFuelData() calls.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	min_pt		Minimum value (inclusive).
		\param	max_pt		Maximum value (inclusive).
		\param	scale		Scale (meters) that the array is defined for
		\param	time	A GMT time.
		\param	fuel	Array of CWFGM fuels (to fill in).
		\param fuel_valid Indicates if the return value in �fuel� is valid.
	*/
	virtual HRESULT GetFuelDataArray(Layer *layerThread, const XY_Point &min_pt,
			const XY_Point &max_pt, double scale, const HSS_Time::WTime &time, ICWFGM_Fuel_2d *fuel, bool_2d *fuel_valid);

	/**
		Given a ('X_min','Y_min')->('X_max','Y_max') range of locations (inclusive) in the grid, and a GMT time (count of seconds since January 1st, 1600), this method fills the provided array with internal indices of CWFGM Fuels.
		The fuels can then be determined by calls to the attached FuelMap object.  This method is not used by the simulation engine, but instead is intended for filters that need core data from the grid.
		This routine uses the array provided by the client rather than returning allocated memory in case the client wants to make successive requests - this will avoid repeated memory allocations and deallocations.
		The 2D array is expected to begin at (0,0) and contain enough room for both dimensions of requested data.
		When using this routine to retrieve 2000 points at a time, it is approximately 75% faster than successive GetFuelData() calls.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	min_pt		Minimum position (inclusive).
		\param	max_pt		Maximum position (inclusive).
		\param	scale		Scale (meters) that the array is defined for
		\param	time	A GMT time.
		\param	fuel_index	Array of indices of CWFGM fuels (to fill in).
		\param fuel_valid Indicates if the return value in �fuel� is valid.
	*/
	virtual HRESULT GetFuelIndexDataArray(Layer *layerThread, const XY_Point &min_pt, const XY_Point &max_pt,
			double scale, const HSS_Time::WTime &time, uint8_t_2d *fuel_index, bool_2d *fuel_valid);

	/*!	\fn	GetElevationDataArray([in] Layer *layerThread, [in] const XY_Point &min_pt, [in] const XY_Point &max_pt, [in] double scale, [in]bool allow_defaults_returned, [in,out] SAFEARRAY(double) *elevation, [in,out] SAFEARRAY(double) *slope_factor, [in,out] SAFEARRAY(double) *slope_azimuth);
	Given a ('X_min','Y_min')->('X_max','Y_max') range of locations (inclusive) in the grid, this method fills the provided arrays with elevation data.
	This routine uses the array provided by the client rather than returning allocated memory in case the client wants to make successive requests - this will avoid repeated memory allocations and de-allocations.
	The 2D array is expected to begin at (0,0) and contain enough room for both dimensions of requested data.
	\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
	\param	min_pt		Minimum position (inclusive).
	\param	max_pt		Maximum position (inclusive).
	\param	scale		Scale (meters) that the array is defined for
	\param	allow_defaults_returned	Flag for allowing defaults to be returned
	\param	elevation	Array of elevations (to fill in).
	\param	slope_factor	Array of slope factors (to fill in).
	\param	slope_azimuth	Array of slope azimuths (to fill in).
	*/
	virtual HRESULT GetElevationDataArray(Layer *layerThread, const XY_Point &min_pt, const XY_Point &max_pt, double scale, bool allow_defaults_returned,
			double_2d *elevation, double_2d *slope_factor, double_2d *slope_azimuth, terrain_t_2d* elev_valid, terrain_t_2d* terrain_valid);

	/**
		Given a ('X_min','Y_min')->('X_max','Y_max') range of locations (inclusive) in the grid, this method fills the provided arrays with weather data.
		This routine uses the array provided by the client rather than returning allocated memory in case the client wants to make successive requests - this will avoid repeated memory allocations and de-allocations.
		The 2D arrays are expected to begin at (0,0) and contain enough room for both dimensions of requested data.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	min_pt		Minimum value (inclusive).
		\param	max_pt		Maximum value (inclusive).
		\param	scale		Scale (meters) that the array is defined for
		\param	time	A GMT time.
		\param	interpolate_method	Method of interpolation.
		\param	wx		Array of Weather information.
		\param	ifwi		Array of Instantaneous FWI codes.
		\param	dfwi		Array of Daily FWI codes.
	*/
	virtual HRESULT GetWeatherDataArray(Layer *layerThread, const XY_Point &min_pt, const XY_Point &max_pt, double scale,
			const HSS_Time::WTime &time, std::uint64_t interpolate_method, IWXData_2d *wx, IFWIData_2d *ifwi, DFWIData_2d *dfwi, bool_2d *wx_valid);

	/**
		Polymorphic.  Given a ('X_min','Y_min')->('X_max','Y_max') range of locations (inclusive) in the grid and a GMT time since January 1st, 1600, returns an attribute value keyed on 'option'.
		Note that only specific objects implementing this interface can be expected to return valid values from this function call.
		The 2D array is expected to begin at (0,0) and contain enough room for both dimensions of requested data.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	min_pt		Minimum value (inclusive).
		\param	max_pt		Maximum value (inclusive).
		\param	scale		Scale (meters) that the array is defined for
		\param	time	A GMT time.
		\param	option	Option information.  Currently expected values are:
		<ul>
		<li><code>FUELCOM_ATTRIBUTE_PC</code>  Percent conifer grid attribute (optional).
		<li><code>FUELCOM_ATTRIBUTE_PDF</code>  Percent dead fir grid attribute (optional).
		<li><code>FUELCOM_ATTRIBUTE_CURINGDEGREE</code>  Grass curing degree grid attribute (optional).
		<li><code>FUELCOM_ATTRIBUTE_CBH</code>  Crown base height grid attribute (optional).
		<li><code>FUELCOM_ATTRIBUTE_TREE_HEIGHT</code> Tree height grid attribute (optional).
		</ul>
		All other values are reserved for future use.
		\param	attribute	Data array
	*/
	virtual HRESULT GetAttributeDataArray(Layer *layerThread, const XY_Point &min_pt, const XY_Point &max_pt, double scale, const HSS_Time::WTime &time,
			const HSS_Time::WTimeSpan& timeSpan, std::uint16_t option, std::uint64_t optionFlags, NumericVariant_2d *attribute, attribute_t_2d *attribute_valid);

	/**
		This method allows the simulation engine to scan forward or back (based on flags) from a given time for the next time-based event.  This is important for the event-driven nature of the simulation.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
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
	virtual HRESULT GetEventTime(Layer *layerThread, const XY_Point& pt, std::uint32_t flags, const HSS_Time::WTime &from_time, HSS_Time::WTime *next_event, bool *event_valid);

	/**
		This method is invoked before any calculations are performed in the simulation, and before calculations are performed on a given simulation time step.  It's intended to perform pre-calculation
		caching events, etc.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	time	A GMT time.
		\param	mode	Calculation mode: 0 represents start of simulation, 1 represents start of a specific time step
		\param	parms	parameters to send and receive values from/to the simulation and grid objects, may be NULL
	*/
	virtual HRESULT PreCalculationEvent(Layer *layerThread, const HSS_Time::WTime &time, std::uint32_t mode, CalculationEventParms *parms);

	/**
		This method is invoked after all calculations are performed in the simulation, and after calculations are performed on a given simulation time step.  It's intended to perform post-calculation
		clean-up, etc.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	time	A GMT time.
		\param	mode	Calculation mode: 0 represents start of simulation, 1 represents start of a specific time step
		\param	parms	parameters to send and receive values from/to the simulation and grid objects, may be NULL
	*/
	virtual HRESULT PostCalculationEvent(Layer *layerThread, const HSS_Time::WTime &time, std::uint32_t mode, CalculationEventParms *parms);

	/**
		This method retrieves the next lower GIS layer for layerThread, used to determine the stacking and order of objects associated to a scenario.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	pVal	Pointer to Grid Engine
	*/
	virtual HRESULT GetGridEngine(Layer *layerThread, boost::intrusive_ptr<ICWFGM_GridEngine> *pVal) const;

	/**
		This method sets the next lower GIS layer for layerThread, used to determine the stacking and order of objects associated to a scenario.
		\param	layerThread		Handle for scenario layering/stack access, allocated from an ICWFGM_LayerManager COM object.  Needed.  It is designed to allow nested layering analogous to the GIS layers.
		\param	newVal	Pointer to Grid Engine
	*/
	virtual HRESULT PutGridEngine(Layer *layerThread, ICWFGM_GridEngine *newVal);

	virtual HRESULT GetCommonData(Layer* layerThread, ICWFGM_CommonData** pVal);

	virtual HRESULT PutCommonData(Layer* layerThread, ICWFGM_CommonData* pVal);

	/**
		This property is must be set before any calls to GetGridEngine or PutGridEngine.  Additionally, this property should be considered write-once.  This value should be set to
		the ICWFGM_LayerManager object which any layerThread assigned to this object will have been allocated from.
		\param	pVal	value of LayerManager
	*/
	virtual HRESULT get_LayerManager(class CCWFGM_LayerManager **pVal);

	/**
		This property is must be set before any calls to GetGridEngine or PutGridEngine.  Additionally, this property should be considered write-once.  This value should be set to
		the ICWFGM_LayerManager object which any layerThread assigned to this object will have been allocated from.
		\param	newVal	value for LayerManager
	*/
	virtual HRESULT put_LayerManager(class CCWFGM_LayerManager *newVal);

protected:
	boost::intrusive_ptr<ICWFGM_GridEngine>	m_rootEngine;
	boost::intrusive_ptr<CCWFGM_LayerManager> m_layerManager;
	boost::intrusive_ptr<ICWFGM_GridEngine> m_gridEngine(Layer *layerThread, std::uint32_t **cnt = nullptr) const;
};
