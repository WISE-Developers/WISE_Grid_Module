/**
 * WISE_Grid_Module: results.h
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

#define ERROR_GRID_UNINITIALIZED			((HRESULT)(12000 | ERROR_SEVERITY_WARNING))	// requested data from a grid that hasn't been
													// initialized yet
#define ERROR_GRID_INITIALIZED				((HRESULT)(12001 | ERROR_SEVERITY_WARNING))	// tried to overwrite data that has already been
													// loaded
#define ERROR_GRID_LOCATION_OUT_OF_RANGE		((HRESULT)(12002 | ERROR_SEVERITY_WARNING))	// requested data that was out of range
#define ERROR_GRID_TIME_OUT_OF_RANGE			((HRESULT)(12003 | ERROR_SEVERITY_WARNING))	// requested data for an invalid time
#define SUCCESS_GRID_DATA_UPDATED			((HRESULT)(12004))				// imported data replaced old imported data (as
#define ERROR_GRID_WEATHER_NOT_IMPLEMENTED		((HRESULT)(12005 | ERROR_SEVERITY_WARNING))	// may be returned to scenario on a valid() call
#define ERROR_GRID_WEATHER_INVALID_DATES		((HRESULT)(12006 | ERROR_SEVERITY_WARNING))	// if the times for the weather don't work with the scenario
													// start and end times
#define SUCCESS_GRID_TOTAL_AREA				((HRESULT)(12007))				// if setting a grid filter for replacing a fuel type with
													// another fuel type will cover the entire area
#define SUCCESS_RETURNING_CALCED_SLOPE			((HRESULT)(12009))				// return value when DEM is provided but slope/aspect wasn't
													// and had to be calculated
#define ERROR_GRID_WEATHER_NO_DATA			((HRESULT)(12010 | ERROR_SEVERITY_WARNING))	// like ERROR_FUELS_FUEL_UNKNOWN but for weather data
#define ERROR_GRID_NO_DATA				((HRESULT)(12013 | ERROR_SEVERITY_WARNING))	// asking for a location that has NODATA
#define ERROR_PROJECTION_UNKNOWN			((HRESULT)(12020 | ERROR_SEVERITY_WARNING))	// can't parse the provided projection file

#define ERROR_GRID_PRIMARY_STREAM_UNSPECIFIED		((HRESULT)(12014 | ERROR_SEVERITY_WARNING))
#define ERROR_GRID_WEATHERSTATIONS_TOO_CLOSE		((HRESULT)(12016 | ERROR_SEVERITY_WARNING))
#define ERROR_GRID_WEATHERSTREAM_TIME_OVERLAPS		((HRESULT)(12017 | ERROR_SEVERITY_WARNING))
#define ERROR_GRID_WEATHER_STATION_ALREADY_PRESENT	((HRESULT)(12018 | ERROR_SEVERITY_WARNING))

#define ERROR_FIREBREAK_NOT_FOUND			((HRESULT)(12050 | ERROR_SEVERITY_WARNING))	// asked to perform on operation on a firebreak but it doesn't
													// see to exist (any more)
#define SUCCESS_FUELS_FUEL_ALREADY_ADDED		((HRESULT)(12500))				// added the same fuel twice - with a different file index
#define ERROR_FUELS_GRIDVALUE_ALREADY_ASSIGNED		((HRESULT)(12501 | ERROR_SEVERITY_WARNING))	// tried to add a fuel for a file index already used
#define ERROR_FUELS_FUEL_UNKNOWN			((HRESULT)(12502 | ERROR_SEVERITY_WARNING))	// queried about a fuel that isn't known

#define ERROR_FIRE_IGNITION_TYPE_UNKNOWN		((HRESULT)(12100 | ERROR_SEVERITY_WARNING))	// invalid ignition type specified
#define SUCCESS_FIRE_NOT_STARTED			((HRESULT)(12102))				// asked for the fire's size before it's started
#define SUCCESS_FIRE_BURNED_OUT				((HRESULT)(12103))				// fire has burned, and nothing left of it can burn
#define ERROR_FIRE_INVALID_TIME				((HRESULT)(12104 | ERROR_SEVERITY_WARNING))	// stats for the time requested is invalid (0)
#define ERROR_FIRE_STAT_UNKNOWN				((HRESULT)(12105 | ERROR_SEVERITY_WARNING))	// requested for an unknown statistic
#define SUCCESS_FIRE_NO_HISTORY				((HRESULT)(12106))				// asked for the history indexes of the fire front, but asked
#define ERROR_SCENARIO_FIRE_ALREADY_ADDED		((HRESULT)(12200 | ERROR_SEVERITY_WARNING))	// trying to add the same fire twice
#define ERROR_SCENARIO_FIRE_UNKNOWN			((HRESULT)(12201 | ERROR_SEVERITY_WARNING))	// trying to remove a fire that wasn't added
#define ERROR_SCENARIO_FIRE_KNOWN			((HRESULT)(12217 | ERROR_SEVERITY_WARNING))
#define ERROR_SCENARIO_NO_FIRES				((HRESULT)(12202 | ERROR_SEVERITY_WARNING))	// trying to reset a scenario with no fires attached
#define ERROR_SCENARIO_OPTION_INVALID			((HRESULT)(12203 | ERROR_SEVERITY_WARNING))	// trying to get/set an option that is invalid
#define ERROR_SCENARIO_BAD_TIMES			((HRESULT)(12204 | ERROR_SEVERITY_WARNING))	// bad start or end times
#define ERROR_SCENARIO_BAD_TIMESTEPS			((HRESULT)(12212 | ERROR_SEVERITY_WARNING))	// bad/invalid calc or output interval (can't be 0's, and
													// output interval must be an even multiple of the calc interval)
#define ERROR_SCENARIO_INVALID_BURNCONDITIONS		((HRESULT)(12218 | ERROR_SEVERITY_WARNING))	// invalid burn condition settings (burning periods between days overlap)
#define ERROR_SCENARIO_BAD_STATE			((HRESULT)(12205 | ERROR_SEVERITY_WARNING))	// called Step() when it shouldn't have been 
#define SUCCESS_SCENARIO_SIMULATION_RESET		((HRESULT)(12206))				// simulation is reset (ready to run)
#define SUCCESS_SCENARIO_SIMULATION_RUNNING		((HRESULT)(12207))				// simulation is running
#define ERROR_SCENARIO_SIMULATION_RUNNING		((HRESULT)(SUCCESS_SCENARIO_SIMULATION_RUNNING | ERROR_SEVERITY_WARNING))
#define SUCCESS_SCENARIO_SIMULATION_COMPLETE		((HRESULT)(12208))				// simulation is done (reached it's end time)
#define ERROR_SCENARIO_VECTORENGINE_ALREADY_ADDED	((HRESULT)(12209 | ERROR_SEVERITY_WARNING))	// trying to add the same fire twice
#define ERROR_SCENARIO_VECTORENGINE_UNKNOWN		((HRESULT)(12210 | ERROR_SEVERITY_WARNING))	// trying to remove a fire that wasn't added
#define ERROR_SCENARIO_VECTORENGINE_KNOWN		((HRESULT)(12211 | ERROR_SEVERITY_WARNING))	// trying to remove a fire that wasn't added
#define ERROR_SCENARIO_WEATHERGRID_UNKNOWN		((HRESULT)(12216 | ERROR_SEVERITY_WARNING))	// trying to remove a fire that wasn't added
#define ERROR_SCENARIO_UNKNOWN				((HRESULT)(12214 | ERROR_SEVERITY_WARNING))	// only for PrometheusCOM
#define ERROR_SCENARIO_KNOWN				((HRESULT)(12215 | ERROR_SEVERITY_WARNING))	// only for PrometheusCOMERR
#define ERROR_SCENARIO_ASSET_UNKNOWN				((HRESULT)(12219 | ERROR_SEVERITY_WARNING))	// trying to remove an asset that wasn't added
#define ERROR_SCENARIO_ASSET_ALREADY_ADDED			((HRESULT)(12220 | ERROR_SEVERITY_WARNING))	// trying to add the same asset twice
#define ERROR_SCENARIO_ASSET_GEOMETRY_UNKNOWN		((HRESULT)(12221 | ERROR_SEVERITY_WARNING))	// trying to remove an asset that wasn't added
#define ERROR_SCENARIO_ASSET_NOT_ARRIVED			((HRESULT)(12224 | ERROR_SEVERITY_WARNING))	// asking information on an asset that the scenario didn't reach
#define SUCCESS_SCENARIO_SIMULATION_COMPLETE_ASSET	((HRESULT)(12222))							// simulation is complete because the correct # assets have been reached
#define SUCCESS_SCENARIO_SIMULATION_COMPLETE_EXTENTS ((HRESULT)(12223))							// simulation is complete because the boundary of the grid data has been reached
#define SUCCESS_SCENARIO_SIMULATION_COMPLETE_STOPCONDITION_FI90 ((HRESULT)(12229))					// simulation is complete because the FI 90% stop condition was met
#define SUCCESS_SCENARIO_SIMULATION_COMPLETE_STOPCONDITION_FI95 ((HRESULT)(12225))					// simulation is complete because the FI 95% stop condition was met
#define SUCCESS_SCENARIO_SIMULATION_COMPLETE_STOPCONDITION_FI100 ((HRESULT)(12226))					// simulation is complete because the FI 100% stop condition was met
#define SUCCESS_SCENARIO_SIMULATION_COMPLETE_STOPCONDITION_RH ((HRESULT)(12227))					// simulation is complete because the RH stop condition was met
#define SUCCESS_SCENARIO_SIMULATION_COMPLETE_STOPCONDITION_PRECIP ((HRESULT)(12228))					// simulation is complete because the precipitation stop condition was met
#define SUCCESS_SCENARIO_SIMULATION_COMPLETE_STOPCONDITION_AREA ((HRESULT)(12230))					// simulation is complete because the fire area has reached its threshold
#define SUCCESS_SCENARIO_SIMULATION_COMPLETE_STOPCONDITION_BURNDISTANCE ((HRESULT)(12231))			// simulation is complete because the fire area has reached its threshold

#define ERROR_WEATHER_STREAM_ALREADY_ADDED		((HRESULT)(12300 | ERROR_SEVERITY_WARNING))	// stream has already been added to this station/weathergrid
#define ERROR_WEATHER_STREAM_UNKNOWN			((HRESULT)(12301 | ERROR_SEVERITY_WARNING))	// trying to remove a stream that wasn't added
#define ERROR_WEATHER_STREAM_NOT_ASSIGNED		((HRESULT)(12302 | ERROR_SEVERITY_WARNING))	// trying to do something to a stream that hasn't been assigned
													// to a weather station yet
#define ERROR_WEATHER_STREAM_ALREADY_ASSIGNED		((HRESULT)(12303 | ERROR_SEVERITY_WARNING))	// trying to assign a stream to more than one station
#define ERROR_WEATHER_STATION_ALREADY_PRESENT		((HRESULT)(12304 | ERROR_SEVERITY_WARNING))	// trying to assign a stream to a grid when we already have a
													// different stream from the stream-to-add's station, or trying
													// to assign a new station to a stream after it's attached to a
													// grid
#define ERROR_SCENARIO_MULTIPLE_WS_GRIDS		((HRESULT)(12307 | ERROR_SEVERITY_WARNING))	// ws grids overlap
#define ERROR_SCENARIO_MULTIPLE_WD_GRIDS		((HRESULT)(12308 | ERROR_SEVERITY_WARNING))	// wd grids overlap
#define ERROR_WEATHER_OPTION_INVALID			((HRESULT)(12305 | ERROR_SEVERITY_WARNING))	// trying to get/set a value for an unknown option
#define ERROR_WEATHER_STATION_UNKNOWN			((HRESULT)(12306 | ERROR_SEVERITY_WARNING))	// for PromtheusCOM - identifier for a weather station is unknown
#define ERROR_WEATHER_STATION_UNINITIALIZED		((HRESULT)(12309 | ERROR_SEVERITY_WARNING))
#define ERROR_IGNITION_UNINITIALIZED			((HRESULT)(12310 | ERROR_SEVERITY_WARNING))
#define ERROR_VECTOR_UNINITIALIZED				((HRESULT)(12311 | ERROR_SEVERITY_WARNING))

#define ERROR_POINT_NOT_IN_FIRE				((HRESULT)(12400 | ERROR_SEVERITY_WARNING))
#define ERROR_POINT_INSIDE_IGNITION			((HRESULT)(12401 | ERROR_SEVERITY_WARNING))

#define ERROR_FILE_FORMAT_INVALID			((HRESULT)(12999 | ERROR_SEVERITY_WARNING))	// when an input/export file format is unknown

#define SUCCESS_STATE_OBJECT_UNLOCKED			((HRESULT)12600)				// means that the object is just sitting there unused
#define SUCCESS_STATE_OBJECT_LOCKED_WRITE		((HRESULT)12601)				// means that someone has gained a write (unique) lock on this object
#define SUCCESS_STATE_OBJECT_LOCKED_READ		((HRESULT)12602)				// means that someone has gained a read (shared) lock on this object
#define SUCCESS_STATE_OBJECT_LOCKED_SCENARIO		((HRESULT)12603)				// means that a scenario has gained a read (shared) lock on this object
#define ERROR_STATE_OBJECT_LOCKED			((HRESULT)(12604 | ERROR_SEVERITY_WARNING))	// the object is currently locked so the operation is aborted

#define ERROR_GRIDFILTER_UNKNOWN			((HRESULT)(12700 | ERROR_SEVERITY_WARNING))
#define ERROR_GRIDFILTER_KNOWN				((HRESULT)(12701 | ERROR_SEVERITY_WARNING))

#define ERROR_WEATHER_STREAM_ATTEMPT_PREPEND		((HRESULT)(12800 | ERROR_SEVERITY_WARNING))	// failed to prepend data to an existing weather stream
#define ERROR_WEATHER_STREAM_ATTEMPT_APPEND		((HRESULT)(12801 | ERROR_SEVERITY_WARNING))	// failed to append data to an existing weather stream
#define ERROR_WEATHER_STREAM_ATTEMPT_OVERWRITE		((HRESULT)(12802 | ERROR_SEVERITY_WARNING))	// failed to overwrite data in an existing weather stream
///<summary>
///There were missing hours in the imported data that were filled using interpolation.
///</summary>
#define WARNING_WEATHER_STREAM_INTERPOLATE		((HRESULT)(12803))
/**
* Combined error, interpolated and invalid data.
*/
#define WARNING_WEATHER_STREAM_INTERPOLATE_BEFORE_INVALID_DATA	((HRESULT)(12805))
///<summary>
/// Attempted to load a weather stream with a start hour after noon LST.
///</summary>
#define ERROR_START_AFTER_NOON					((HRESULT)(12807))
///<summary>
///Attempted to export a fire perimeter with no points.
///</summary>
#define ERROR_ATTEMPT_EXPORT_EMPTY_FIRE			((HRESULT)(12804 | ERROR_SEVERITY_WARNING))
#define ERROR_BURNCONDITION_INVALID_TIME		((HRESULT)(12806 | ERROR_SEVERITY_WARNING))	// stats for the time requested is invalid (0)

#define ERROR_DATA_NOT_UNIQUE					((HRESULT)(12900 | ERROR_SEVERITY_WARNING))	// error condition that occurs when the data specified is not unique
#define ERROR_NAME_NOT_UNIQUE					((HRESULT)(12901 | ERROR_SEVERITY_WARNING))	// error condition that occurs when the name specified is not unique
#define ERROR_SECTOR_INVALID_INDEX				((HRESULT)(12902 | ERROR_SEVERITY_WARNING))	// error condition that occurs when the sector index specified is out of range
#define ERROR_SECTOR_TOO_SMALL					((HRESULT)(12903 | ERROR_SEVERITY_WARNING))	// error condition that occurs when the sector angle specified span an angle less than MINIMUM_SECTOR_ANGLE
#define ERROR_SPEED_OUT_OF_RANGE				((HRESULT)(12904 | ERROR_SEVERITY_WARNING))	// error condition that occurs when the wind speed specified is out of range
#define ERROR_GRID_SIZE_INCORRECT				((HRESULT)(12905 | ERROR_SEVERITY_WARNING))
#define ERROR_GRID_UNSUPPORTED_RESOLUTION		((HRESULT)(12906 | ERROR_SEVERITY_WARNING))

#define SUCCESS_CACHE_ALREADY_EXISTS			((HRESULT)12950)
#define ERROR_CACHE_NOT_FOUND					((HRESULT)(12951 | ERROR_SEVERITY_WARNING))

#define ERROR_PROTOBUF_OBJECT_INVALID			((HRESULT)(12850 | ERROR_SEVERITY_WARNING))	// the object that we're encountering isn't what it should be
#define ERROR_PROTOBUF_OBJECT_VERSION_INVALID	((HRESULT)(12851 | ERROR_SEVERITY_WARNING))	// the object that we're encountering isn't an expected version
