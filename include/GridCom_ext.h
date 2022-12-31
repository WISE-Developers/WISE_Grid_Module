/**
 * WISE_Grid_Module: GridCom_ext.h
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

#ifndef __GRIDCOM_EXT_H
#define __GRIDCOM_EXT_H

#include <cstdint>

#define CWFGM_ATTRIBUTE_LOAD_WARNING		10000
#define CWFGM_GRID_ATTRIBUTE_LATITUDE		10001
#define CWFGM_GRID_ATTRIBUTE_LONGITUDE		10002
#define CWFGM_GRID_ATTRIBUTE_XLLCORNER		10003
#define CWFGM_GRID_ATTRIBUTE_YLLCORNER		10004
#define CWFGM_GRID_ATTRIBUTE_XURCORNER		10005
#define CWFGM_GRID_ATTRIBUTE_YURCORNER		10006
#define CWFGM_GRID_ATTRIBUTE_PLOTRESOLUTION	10007
#define CWFGM_GRID_ATTRIBUTE_SPATIALREFERENCE	10008
#define CWFGM_GRID_ATTRIBUTE_ASCII_GRIDFILE_HEADER	10009
#define CWFGM_GRID_ATTRIBUTE_PROJECTION_UNITS	10010

#define CWFGM_GRID_ATTRIBUTE_DEFAULT_ELEVATION	10100
#define CWFGM_GRID_ATTRIBUTE_DEFAULT_ELEVATION_SET	10109
#define CWFGM_GRID_ATTRIBUTE_MEDIAN_ELEVATION	10101
#define CWFGM_GRID_ATTRIBUTE_MEAN_ELEVATION	10102
#define CWFGM_GRID_ATTRIBUTE_MIN_ELEVATION	10103
#define CWFGM_GRID_ATTRIBUTE_MAX_ELEVATION	10104
#define CWFGM_GRID_ATTRIBUTE_MIN_SLOPE		10105
#define CWFGM_GRID_ATTRIBUTE_MAX_SLOPE		10106
#define CWFGM_GRID_ATTRIBUTE_MIN_AZIMUTH	10107
#define CWFGM_GRID_ATTRIBUTE_MAX_AZIMUTH	10108

#define CWFGM_GRID_ATTRIBUTE_FUELS_PRESENT	10300
#define CWFGM_GRID_ATTRIBUTE_DEM_PRESENT	10301
#define CWFGM_GRID_ATTRIBUTE_DEM_NODATA_EXISTS	10302
#define CWFGM_GRID_ATTRIBUTE_DEFAULT_FMC	10303
#define CWFGM_GRID_ATTRIBUTE_DEFAULT_FMC_ACTIVE 10304

#define CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_RH		10400
#define CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_FWI		10401
#define CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MIN_ISI		10402
#define CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_MAX_WS		10403
#define CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START	10404				// as stored in the object
#define CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END	10405				// as stored in the object
#define CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START_INTERPRET	10412	// how to interpret the value
#define CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END_INTERPRET		10413	// how to interpret the value
#define CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_START_COMPUTED		10414	// start time which is PERIOD_START with interpretation applied
#define CWFGM_GRID_ATTRIBUTE_BURNINGCONDITION_PERIOD_END_COMPUTED		10415	// start time which is PERIOD_END with interpretation applied

#define CWFGM_FUELGRID_ATTRIBUTE_X_START					10406	// x start of a given cell that contains 'x'
#define CWFGM_FUELGRID_ATTRIBUTE_X_MID						10407	// x mid-point of a given cell that contains 'x'
#define CWFGM_FUELGRID_ATTRIBUTE_X_END						10408	// x end of a given cell that contains 'x'
#define CWFGM_FUELGRID_ATTRIBUTE_Y_START					10409	// y start of a given cell that contains 'y'
#define CWFGM_FUELGRID_ATTRIBUTE_Y_MID						10410	// y mid-point of a given cell that contains 'y'
#define CWFGM_FUELGRID_ATTRIBUTE_Y_END						10411	// y end of a given cell that contains 'y'

#define CWFGM_GRID_ATTRIBUTE_GIS_CANRESIZE					10450	// can use GIS data from a URL to expand the initial dataset
#define CWFGM_GRID_ATTRIBUTE_GIS_URL						10451
#define CWFGM_GRID_ATTRIBUTE_GIS_LAYER						10452
#define CWFGM_GRID_ATTRIBUTE_GIS_UID						10453
#define CWFGM_GRID_ATTRIBUTE_GIS_PWD						10454
#define CWFGM_GRID_ATTRIBUTE_INITIALSIZE					10455	// initial buffer size to place around the ignition(s)
#define CWFGM_GRID_ATTRIBUTE_GROWTHSIZE						10456	// size that a buffer should grow when it needs to grow
#define CWFGM_GRID_ATTRIBUTE_BUFFERSIZE						10457	// size of the buffer to place around a simulation state to determine when it's time to acquire more data

#define CWFGM_GETEVENTTIME_FLAG_SEARCH_FORWARD		0x00000000
#define CWFGM_GETEVENTTIME_FLAG_SEARCH_BACKWARD		0x00000001
#define CWFGM_GETEVENTTIME_FLAG_SEARCH_SUNRISE		0x00000002
#define CWFGM_GETEVENTTIME_FLAG_SEARCH_SUNSET		0x00000004
#define CWFGM_GETEVENTTIME_FLAG_SEARCH_SOLARNOON	0x00000006
#define CWFGM_GETEVENTTIME_QUERY_PRIMARY_WX_STREAM	((std::uint64_t)(1ull << 25))
#define CWFGM_GETEVENTTIME_QUERY_ANY_WX_STREAM		((std::uint64_t)(1ull << 26))

#define CWFGM_GETWEATHER_INTERPOLATE_TEMPORAL		((std::uint64_t)(1ull << 17))
#define CWFGM_GETWEATHER_INTERPOLATE_SPATIAL		((std::uint64_t)(1ull << 18))
#define CWFGM_GETWEATHER_INTERPOLATE_PRECIP			((std::uint64_t)(1ull << 21))
#define CWFGM_GETWEATHER_INTERPOLATE_WIND			((std::uint64_t)(1ull << 22))
#define CWFGM_GETWEATHER_INTERPOLATE_WIND_VECTOR	((std::uint64_t)(1ull << 27))
#define CWFGM_GETWEATHER_INTERPOLATE_TEMP_RH		((std::uint64_t)(1ull << 29))
#define CWFGM_GETWEATHER_INTERPOLATE_HISTORY		((std::uint64_t)(1ull << 23))
#define CWFGM_GETWEATHER_INTERPOLATE_CALCFWI		((std::uint64_t)(1ull << 24))

namespace grid {
	enum class TerrainValue : std::uint8_t {
		NOT_SET = 0b00,
		SET = 0b01,
		DEFAULT = 0b11
	};

	enum class AttributeValue : std::uint8_t {
		NOT_SET = 0b00,
		SET = 0b01,
		DEFAULT = 0b11
	};
}

#endif
