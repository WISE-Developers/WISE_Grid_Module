/***********************************************************************
 * REDapp - IWXData.java
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

/**
 * Weather data.
 */
public class IWXData {
	public double temperature;
	public double dewPointTemperature;
	public double rh;
	public double precipitation;
	public double windSpeed;
	public double windDirection;
	public long specifiedBits;
	
	public IWXData() { }
	
	public IWXData(IWXData toCopy) {
		this.temperature = toCopy.temperature;
		this.dewPointTemperature = toCopy.dewPointTemperature;
		this.rh = toCopy.rh;
		this.precipitation = toCopy.precipitation;
		this.windSpeed = toCopy.windSpeed;
		this.windDirection = toCopy.windDirection;
		this.specifiedBits = toCopy.specifiedBits;
	}
	
	public static class SPECIFIED {
		public static final long TEMPERATURE			= 0x00000001;
		public static final long DEWPOINTTEMPERATURE	= 0x00000002;
		public static final long RH						= 0x00000004;
		public static final long PRECIPITATION			= 0x00000008;
		public static final long WINDSPEED				= 0x00000010;
		public static final long WINDDIRECTION			= 0x00000020;
		public static final long ALL					= TEMPERATURE | DEWPOINTTEMPERATURE | RH | PRECIPITATION | WINDSPEED | WINDDIRECTION;
		public static final long INTERPOLATED			= 0x00000040;
		public static final long ENSEMBLE               = 0x00000080;
        public static final long INVALID_DATA           = 0x00000100;
	}
}
