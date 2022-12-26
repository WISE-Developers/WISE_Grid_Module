/***********************************************************************
 * REDapp - DFWIData.java
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
 * Daily FWI data.
 */
public class DFWIData {
	public double dFFMC;
	public double dISI;
	public double dFWI;
	public double dDMC;
	public double dBUI;
	public double dDC;
	public long dSpecifiedBits;
	
	public DFWIData() { }
	
	public static class SPECIFIED {
		public static final long FFMC	= 0x00001000;
		public static final long DMC	= 0x00002000;
		public static final long DC		= 0x00004000;
		public static final long BUI	= 0x00008000;
		public static final long ISI	= 0x00010000;
		public static final long FWI	= 0x00020000;
		public static final long ALL	= FFMC | DMC | DC | BUI | ISI | FWI;
	}
}
