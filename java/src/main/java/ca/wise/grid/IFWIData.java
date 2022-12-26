/***********************************************************************
 * REDapp - IFWIData.java
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
 * Hourly FWI data.
 */
public class IFWIData {
	public double dFFMC;
	public double dISI;
	public double dFWI;
	public long dSpecifiedBits;
	
	public IFWIData() { }
	
	public static class SPECIFIED {
		public static final long FFMC = 0x00000100;
		public static final long ISI  = 0x00000200;
		public static final long FWI  = 0x00000400;
		public static final long ALL  = FFMC | ISI | FWI;
	}
}
