/***********************************************************************
 * REDapp - CWFGM_GRID_ATTRIBUTE.java
 * Copyright (C) 2015-2019 The REDapp Development Team
 * Homepage: http://redapp.org
 * 
 * REDapp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * REDapp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with REDapp. If not see <http://www.gnu.org/licenses/>. 
 **********************************************************************/

package ca.wise.grid;

public abstract class GRID_ATTRIBUTE {
	public static final int LOAD_WARNING			= 10000;
	public static final int LATITUDE				= 10001;
	public static final int LONGITUDE				= 10002;
	public static final int XLLCORNER				= 10003;
	public static final int YLLCORNER				= 10004;
	public static final int PLOTRESOLUTION			= 10005;
	public static final int SPATIALREFERENCE		= 10006;
	public static final int ASCII_GRIDFILE_HEADER	= 10007;
	public static final int DEFAULT_ELEVATION		= 10100;
	public static final int MEDIAN_ELEVATION		= 10101;
	public static final int MEAN_ELEVATION			= 10102;
	public static final int MIN_ELEVATION			= 10103;
	public static final int MAX_ELEVATION			= 10104;
	public static final int MIN_SLOPE				= 10105;
	public static final int MAX_SLOPE				= 10106;
	public static final int MIN_AZIMUTH				= 10107;
	public static final int MAX_AZIMUTH				= 10108;
	public static final int TIMEZONE				= 10200;
	public static final int DAYLIGHT_SAVINGS		= 10201;
	public static final int DST_START				= 10203;
	public static final int DST_END					= 10204;
	public static final int FUELS_PRESENT			= 10300;
	public static final int DEM_PRESENT				= 10301;
	public static final int DEM_NODATA_EXISTS		= 10302;
	public static final int DEFAULT_FMC				= 10303;
}
