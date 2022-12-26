/***********************************************************************
 * REDapp - CWFGM_GETWEATHER_INTERPOLATE.java
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

public class GETWEATHER_INTERPOLATE {
	public static final int TEMPORAL	= 1 << 17;
	public static final int SPATIAL		= 1 << 18;
	public static final int PRECIP		= 1 << 21;
	public static final int WIND		= 1 << 22;
	public static final int HISTORY		= 1 << 23;
	public static final int CALCFWI		= 1 << 24;
}
