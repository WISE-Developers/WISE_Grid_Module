/**
 * WISE_Grid_Module: CWFGM_Asset.h
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

#ifndef __CWFGM_ASSET_H_
#define __CWFGM_ASSET_H_

#include "results.h"
#include "poly.h"
#include "ICWFGM_Asset.h"
#include "CWFGM_internal.h"
#include <vector>
#include "XY_PolyType.h"
#include "ISerializeProto.h"
#include "validation_object.h"

#include <string>
#include <boost/intrusive_ptr.hpp>
#include <boost/atomic.hpp>
#include "cwfgmFilter.pb.h"

#ifdef HSS_SHOULD_PRAGMA_PACK
#pragma pack(push, 8)
#endif

class XY_PolySetType : public XY_PolySet {
public:
	XY_PolySetType() : XY_PolySet() { m_publicFlags = 0; }
	XY_PolySetType(const XY_PolySetType& toCopy) : XY_PolySet(toCopy) { m_publicFlags = toCopy.m_publicFlags; }
	XY_PolySetType(XY_PolySetType&& toMove) : XY_PolySet(toMove) { m_publicFlags = toMove.m_publicFlags; }

	XY_PolySetType& operator=(const XY_PolyLLSet& toCopy) { (*((XY_PolySet*)(this))) = toCopy; return *this; }

	std::uint16_t m_publicFlags;
};


/**
Vector data is provided to the simulation engine by the client application.  The simulation engine defines the interface it will use to obtain the vector fuel break data.  The simulation engine is not concerned with the kind of object that implements this interface, or the implementation of the object.  It may represent the GIS vector-based data, a user-defined static vector fuelbreak, or a user-controlled break that changes over time, or some other object a programmer has built to access a specialized data set.
The simulation engine does not define any object that implements this interface.  The simulation engine only defines the interface.  Objects that implement this interface should import this interface definition.  The object implementing this interface is to be handled entirely by the client application (such as CWFGM).  Methods to import vector data or manage vector filters are not found on this COM interface, as the simulation engine has no need for these methods.
This design takes advantage of object-oriented methodologies to simplify both the design of the simulation engine, and the integration of the simulation engine into another application.  For example, to provide data from a G.I.S. directly to the simulation engine, the programmer only needs to define an object that implements this interface and communicates with the G.I.S.
Unlike ICWFGM_GridEngine interface objects that must be chained (layered) together to attain the appropriate grid layer for the simulation, objects implementing this interface don't replace or override other vector object data, they are only combined.  As such, the order is unimportant so there are no methods to chain ICWFGM_VectorEngine objects together here.  The simulation's scenario object can use many ICWFGM_VectorEngine objects simultaneously.
At this time, the simulation engine assumes that fuel breaks defined by this interface have 100% effect on the fire; the fire will not escape the confines of a vector fuel break.

The IDL of the vector object required by the simulation engine is given below.  Program developers are encouraged to obtain the latest electronic copy of the programming tools from the resources listed at the end of this document.
*/
class GRIDCOM_API CCWFGM_Asset : public ICWFGM_Asset, public ISerializeProto {
public:
#ifndef DOXYGEN_IGNORE_CODE
	CCWFGM_Asset();
	CCWFGM_Asset(const CCWFGM_Asset& toCopy);
	~CCWFGM_Asset();

public:
#endif
	/**
		Creates a new Asset object with all the same properties of the object being called, returns a handle to the new object in 'newFilter'.
		\param	newFilter Pointer to the new copy of the filter.
		\sa ICWFGM_VectorEngine::Clone
		\retval S_OK	Succesfull.
		\retval	E_POINTER	Invalid pointer.
		\retval E_OUTOFMEMORY	Out of memory.
		\retval E_NOINTERFACE	No interface
		\retval ERROR_SEVERITY_WARNING	Severity error warning
	*/
	virtual NO_THROW HRESULT Clone(boost::intrusive_ptr<ICWFGM_CommonBase>* newObject) const override;
	/**
		This method imports polylines / polygons from the specified file.  If the file is an ArcInfo Generate file, then it will use a projection file (same name, but ".prj" for an extension).  This projection file is optional.
		Other file formats will use GDAL/OGR conventions regarding projection files.  Imported data is automatically reprojected into the grid projection during import.
		\param	file_path	File path of the polyline / polygon data to import.
		\param	permissible_drivers	Array of drivers (known by GDAL) identifying the file types that are allowed to be imported.
		\sa ICWFGM_Asset::ImportPolyLines
		\retval	S_OK	Successful.
		\retval	E_POINTER	Address provided for file_path is invalid.
		\retval	ERROR_FIREBREAK_NOT_FOUND	Polygon does not exist.
		\retval	E_OUTOFMEMORY	Insufficient memory.
		\retval	ERROR_VECTOR_UNINITIALIZED	Grid object not initialized.
		\retval	ERROR_SEVERITY_WARNING	Unspecified failure.
		\retval	S_FALSE	If asking for values where the appropriate array has not been initialized.
		\retval	E_INVALIDARG	Invalid arguments.
		\retval	ERROR_FILE_NOT_FOUND	The file cannot be found.
		\retval	ERROR_TOO_MANY_OPEN_FILES	The program has too many open files.
		\retval	ERROR_ACCESS_DENIED	Access is denied.
		\retval	ERROR_INVALID_HANDLE	Generic file I/O error.
		\retval	ERROR_HANDLE_DISK_FULL	Disk full.
		\retval	ERROR_FILE_EXISTS	File already exists.
	*/
	virtual NO_THROW HRESULT ImportPolylines(const std::string & file_path, const std::vector<std::string>* permissible_drivers);
	virtual NO_THROW HRESULT ImportPolylinesWFS(const std::string& url, const std::string& layer, const std::string& username, const std::string& password);
	/**
		This method exports polylines / polygons to the specified file.  The projection file is optionally created, and will be the grid's projection.  Driver names are defined by GDAL/OGR, plus "ARCInfo Generate".
		\param	driver_name	Name of driver to be used during export.
		\param	projection	Name of projection to be used during export.
		\param	file_path	Path of the file name where Polylines will be exported.
		\sa ICWFGM_Asset::ExportPolylines
		\retval	S_OK	Successful.
		\retval E_OUTOFMEMORY	Out of memory.
		\retval	E_FAIL	Failure.
		\retval	E_POINTER	Invalid pointer in arguments
		\retval	ERROR_SCENARIO_SIMULATION_RUNNING	Unable to continue due to running simulation.
		\retval ERROR_FIREBREAK_NOT_FOUND	Unable to find firebreak.	
	*/
	virtual NO_THROW HRESULT ExportPolylines(const std::string& driver_name, const std::string& projection, const std::string& file_path);
	virtual NO_THROW HRESULT ExportPolylinesWFS(const std::string& url, const std::string& layer, const std::string& username, const std::string& password);
	/**
		This method adds a polyline / polygon defining an area to apply the vector filter.  A vector filter can contain many polylines and/or polygons, and polygons can be overlapping.
		However, only one width applies to all polylines and does not apply to polygons.  All poly data are provided in grid units.
		\param xy_pairs 2D Array of coordinates
		\param	type	Type of poly data: 0x0100 = multipoint, 0x0200 = polyline, 0x0400 = polygon (and 0x0001 for interior polygon)
		\param	index	Index for the newly added Polylines.
		\sa ICWFGM_Asset::AddPolyLine
		\retval	S_OK	Successful.
		\retval E_OUTOFMEMORY	Out of memory.
		\retval	E_FAIL	Failure.
		\retval	E_POINTER	Invalid pointer in arguments
		\retval	ERROR_SCENARIO_SIMULATION_RUNNING	Unable to continue due to running simulation.
	*/
	virtual NO_THROW HRESULT AddPolyLine(const XY_PolyConst& xy_pairs,std::uint16_t type, std::uint32_t *index);
	/**
		This method sets/edits a specific polyline / polygon defining an area to apply the vector filter.  A vector filter can contain many polylines and/or polygons, and polygons can be overlapping.
		However, only one width applies to all polylines and does not apply to polygons.  All poly data are provided in grid units.
		\param	index	Index of the new polyline / polygon.
		\param xy_pairs 2D Array of coordinates
		\param	type	Type of poly data: 0x0100 = multipoint, 0x0200 = polyline, 0x0400 = polygon (and 0x0001 for interior polygon)
		\sa ICWFGM_Asset::SetPolyLine
		\retval	S_OK	Successful.
		\retval E_OUTOFMEMORY	Out of memory.
		\retval	E_FAIL	Failure.
		\retval	E_POINTER	Invalid pointer in arguments
		\retval	ERROR_SCENARIO_SIMULATION_RUNNING	Unable to continue due to running simulation.
		\retval ERROR_FIREBREAK_NOT_FOUND	Unable to find firebreak.
	*/
	virtual NO_THROW HRESULT SetPolyLine(std::uint32_t index, const XY_PolyConst& xy_pairs,std::uint16_t type);
	/**
		This method clears and removes Polyline at provided index.   If set_break = (unsigned long) -1, then all polylines / polygons are cleared.
		\param	set_break	Index of Polyline to be cleared.
		\sa ICWFGM_Asset::ClearPolyLine
		\retval	S_OK	Successful.
		\retval	ERROR_SCENARIO_SIMULATION_RUNNING	Unable to continue due to running simulation.
		\retval ERROR_FIREBREAK_NOT_FOUND	Unable to find firebreak.
	*/
	virtual NO_THROW HRESULT ClearPolyLine(std::uint32_t index);
	/**
		This method returns the bounding box of a specific polyline / polygon, or of all polylines / polygons in the vector filter.
		\param	index	Index of polygon.  If the bounding box for all polygons is desired, then pass (unsigned long)-1.
		\param	x_min	Stores minimum x-axis value for the polyline(s) / polygon(s).
		\param	y_min	Stores minimum y-axis value for the polyline(s) / polygon(s).
		\param	x_max	Stores maximum x-axis value for the polyline(s) / polygon(s).
		\param	y_max	Stores maximum y-axis value for the polyline(s) / polygon(s).
		\sa ICWFGM_Asset::GetPolyLineRange
		\retval	S_OK	Successful.
		\retval ERROR_FIREBREAK_NOT_FOUND	Unable to find firebreak.
	*/
	virtual NO_THROW HRESULT GetPolyLineRange(std::uint32_t index, XY_Point* min_pt, XY_Point* max_pt);
	virtual NO_THROW HRESULT GetAssetRange(std::uint32_t index, const HSS_Time::WTime& time, XY_Point* min_pt, XY_Point* max_pt) override;
	/**
	  Polymorphic.  This routine retrieves an attribute/option value given the attribute/option index.
		\param option Supported / valid attribute/option index supported are:
		<ul>
		<li><code>CWFGM_ATTRIBUTE_LOAD_WARNING</code>	BSTR.  Any warnings generated by the COM object when deserializating.
		</ul>
		\param value Return value for the attribute/option index
		\sa ICWFGM_Asset::GridEngine
		\retval	S_OK	Success
		\retval	E_POINTER	Address provided for value is invalid.
		\retval	E_INVALIDARG	No valid parameters.
	*/
	virtual NO_THROW HRESULT GetAttribute(std::uint16_t option, PolymorphicAttribute* value) override;
	virtual NO_THROW HRESULT SetAttribute(std::uint16_t option, const PolymorphicAttribute& value) override;
	virtual NO_THROW HRESULT GetPolyLineAttributeCount(std::uint32_t* count) override;
	virtual NO_THROW HRESULT GetPolyLineAttributeName(std::uint32_t count, std::string* attribute_name) override;
	virtual NO_THROW HRESULT GetPolyLineAttributeValue(std::uint32_t index, const std::string& attribute_name,  PolymorphicAttribute* ignition_type) override;
	/**
		Changes the state of the object with respect to access rights.  When the object is used by an active simulation, it must not be modified.
		When the object is somehow modified, it must be done so in an atomic action to prevent concerns with arising from multithreading.
		Note that these locks are primarily needed to ensure that data contributing during a simulation is not modified while the simulation is executing.\n\n All routines in the
		ICWFGM_VectorEngine interface are necessarily NOT multithreading safe (for performance) but other interfaces for a given COM object implementing this interface must be by specification. 
		\param	exclusive	TRUE if the requester wants a write lock, false for read/shared access
		\param	obtain	TRUE to obtain the lock, FALSE to release the lock.  If this is FALSE, then the 'exclusive' parameter must match the initial call used to obtain the lock.
		\sa ICWFGM_VectorEngine::MT_Lock
		\retval	SUCCESS_STATE_OBJECT_UNLOCKED	Lock was released.
		\retval	SUCCESS_STATE_OBJECT_LOCKED_WRITE	Exclusive/write lock obtained.
		\retval	SUCCESS_STATE_OBJECT_LOCKED_SCENARIO	A scenario successfully required a lock for purposes of simulating.
		\retval	SUCCESS_STATE_OBJECT_LOCKED_READ	Shared/read lock obtained.
		\retval	S_OK	Successful.
	*/
	virtual NO_THROW HRESULT MT_Lock(bool exclusive, std::uint16_t obtain) override;
	/**
		This method returns whether this vector object has been initialized, and whether it contains all data necessary for a simulation for the simulation time identified by start_time and duration.
		If start_time and duration are both 0, then this method only returns whether the object is initialized.
		\param	start_time	Start time of the simulation.
		\param	duration	Duration of the simulation.
		\sa ICWFGM_VectorEngine::Valid
		\retval	S_OK	Successful.
		\retval	ERROR_VECTOR_UNINITIALIZED	Grid Engine property has not be set.
		\retval	ERROR_SEVERITY_WARNING	The FuelMap has not been set or no FBP data has been loaded.
	*/
	virtual NO_THROW HRESULT Valid(const HSS_Time::WTime& start_time, const HSS_Time::WTimeSpan& duration) override;
	/**
		This method returns the total number of polylines / polygons.
		\param	count	Stores the total number of Polylines.
		\sa ICWFGM_Asset::GetPolyLineCount
		\retval	S_OK	Successful.
		\retval	E_POINTER	Invalid pointer in arguments
	*/
	virtual NO_THROW HRESULT GetPolyLineCount(std::uint32_t *count);
	virtual NO_THROW HRESULT GetAssetCount(const HSS_Time::WTime & time, std::uint32_t *count) override;
	virtual NO_THROW HRESULT GetAssetSetCount(const HSS_Time::WTime& time, std::uint32_t index, std::uint32_t*count) override;
	/**
		This method gets polyline / polygon size (number of vertices).  Or, if index = -1, then this method gets the maximum size (number of vertices) of the polylines/polygons in the grid filter.
		\param	index	Index of Polyline.
		\param	size	Stores the size of the Polyline.
		\sa ICWFGM_Asset::GetPolyLineSize
		\retval	S_OK	Successful.
		\retval	E_POINTER	Invalid pointer in arguments
		\retval ERROR_FIREBREAK_NOT_FOUND	Unable to find firebreak.	
	*/
	virtual NO_THROW HRESULT GetPolyLineSize(std::uint32_t index, std::uint32_t*size);
	virtual NO_THROW HRESULT GetAssetSize(std::uint32_t index, std::uint32_t sub_index, const HSS_Time::WTime& time, std::uint32_t*size) override;
	virtual NO_THROW HRESULT GetAsset(std::uint32_t index, std::uint32_t sub_index, const HSS_Time::WTime& time, std::uint32_t*size,
			XY_Poly* xy_pairs, uint16_t *type) override;
	virtual NO_THROW HRESULT GetPolyType(std::uint32_t index, std::uint16_t *type);
	/**
		This method retrieves a polyline / polygon from the grid.  Polygons are retrieved in grid units.
		\param	index	Index of Polyline / polygon.
		\param	size	Returns the size of Polyline.
		\param xy_pairs 2D Array of coordinates
		\param	type	Returns type of the Polyline: 1 for polygon, 0 for polyline.
		\sa ICWFGM_Asset::GetPolyLine
		\retval	S_OK	Successful.
		\retval E_OUTOFMEMORY	Out of memory.
		\retval	E_FAIL	Failure.
		\retval	E_POINTER	Invalid pointer in arguments
		\retval	ERROR_SCENARIO_SIMULATION_RUNNING	Unable to continue due to running simulation.
		\retval ERROR_FIREBREAK_NOT_FOUND	Unable to find firebreak.
		\retval	E_OUTOFMEMORY	Insufficient memory.
	*/
	virtual NO_THROW HRESULT GetPolyLine(std::uint32_t index, std::uint32_t* size, XY_Poly* xy_pairs, std::uint16_t* type);
	/**
		This method returns the width value of the vector filter.  This property applies to polylines, and is stored in grid units.
		\param	pVal	Pointer to store width
		\sa ICWFGM_Asset::Width
		\retval	S_OK	Successful.
		\retval E_INVALIDARG	Invalid arguments.
		\retval ERROR_SCENARIO_SIMULATION_RUNNING	Simulation is currently running that uses this filter.
	*/
	virtual NO_THROW HRESULT get_Width(double *pVal);
	/**
		This method sets the width of the vector filter.  This property applies to polylines, and is stored in grid units.
		\param	newVal	New value for width.
		\sa ICWFGM_Asset::Width
		\retval	S_OK	Successful.
		\retval E_INVALIDARG	Invalid arguments.
		\retval ERROR_SCENARIO_SIMULATION_RUNNING	Simulation is currently running that uses this filter.
	*/
	virtual NO_THROW HRESULT put_Width(double newVal);
	/**
		Retrieves the object exposing the GridEngine interface that this VectorEngine object may refer to, to use for tasks such as bounds clipping, etc.
		\param	pVal	Value of GridEngine.
		\sa ICWFGM_VectorEngine::GridEngine
		\retval	E_POINTER	The address provided for pVal is invalid, or upon setting pVal the pointer doesn't appear to belong to an object exposing the ICWFGM_GridEngine interface.
		\retval	S_OK	Successful.
		\retval	ERROR_VECTOR_UNINITIALIZED	The Grid Engine property has not be set.
	*/
	virtual NO_THROW HRESULT get_GridEngine( boost::intrusive_ptr<ICWFGM_GridEngine>* pVal) override;
	/**
		Sets the object exposing the GridEngine interface that this VectorEngine object may refer to, to use for tasks such as bounds clipping, etc.
		\param	newVal	Replacement value for GridEngine.
		\sa ICWFGM_VectorEngine::GridEngine
		\retval	E_POINTER	The address provided for pVal is invalid, or upon setting pVal the pointer doesn't appear to belong to an object exposing the ICWFGM_GridEngine interface.
		\retval	S_OK	Successful.
		\retval	ERROR_VECTOR_UNINITIALIZED	The Grid Engine property has not be set.
		\retval	E_NOINTERFACE	The object provided does not implement the ICWFGM_GridEngine interface.
	*/
	virtual NO_THROW HRESULT put_GridEngine(ICWFGM_GridEngine * newVal) override;

	virtual std::int32_t serialVersionUid(const SerializeProtoOptions& options) const noexcept override;
	virtual WISE::GridProto::CwfgmAsset* serialize(const SerializeProtoOptions& options) override;
	virtual CCWFGM_Asset* deserialize(const google::protobuf::Message& proto, std::shared_ptr<validation::validation_object> valid, const std::string& name) override;
	virtual std::optional<bool> isdirty(void) const noexcept override { return m_bRequiresSave; }

	/**
		This object does not generate any events.
		\param	flags	Flags related to event time requested.
		\param	from_time	A GMT time provided as seconds since January 1st, 1600.
		\param	next_event	A GMT time provided as seconds since January 1st, 1600, representing the time for the next event, based on 'flags'.
		\sa ICWFGM_VectorEngine::GetEventTime
		\retval	ERROR_VECTOR_UNINITIALIZED	No object in the grid layering to forward the request to.
		\retval	S_OK	Successful.
	*/
	virtual NO_THROW HRESULT GetEventTime(std::uint32_t flags, const HSS_Time::WTime& from_time,  HSS_Time::WTime* next_event);
	/**
		This object requires no work to be performed in this call.
		\param	time	A GMT time provided as seconds since January 1st, 1600.
		\param	mode	Calculation mode.
		\sa ICWFGM_VectorEngine::PreCalculationEvent
		\retval	S_OK	Always successful.
	*/
	virtual NO_THROW HRESULT PreCalculationEvent(const HSS_Time::WTime& time, std::uint32_t mode, CalculationEventParms* parms);
	/**
		This object requires no work to be performed in this call.
		\param	time	A GMT time provided as seconds since January 1st, 1600.
		\param	mode	Calculation mode.
		\sa ICWFGM_VectorEngine::PostCalculationEvent
		\retval	S_OK	Always successful
	*/
	virtual NO_THROW HRESULT PostCalculationEvent(const HSS_Time::WTime& time, std::uint32_t mode, CalculationEventParms* parms);

#ifndef DOXYGEN_IGNORE_CODE
protected:
	boost::intrusive_ptr<ICWFGM_GridEngine>	m_gridEngine;
	CRWThreadSemaphore						m_lock, m_calcLock;

	HRESULT fixResolution();

	std::string						m_gisURL, m_gisLayer, m_gisUID, m_gisPWD;
	double							m_xmin, m_ymin, m_xmax, m_ymax;
	RefList<XY_PolyType>			m_polyList;
	std::set<std::string>			m_attributeNames;

	double							m_assetBoundaryWidth;
	std::vector<XY_PolySetType>		m_assets;
	std::string						m_loadWarning;
	unsigned long					m_flags;					// see CWFGM_internal.h for available options
	bool							m_bRequiresSave;

	void rescanRanges();

	HRESULT getPolyRange(std::uint32_t index, double* x_min, double* y_min, double* x_max, double* y_max) const;
	HRESULT getPolyMaxSize(std::uint32_t* size) const;
	HRESULT getPolyCount(std::uint32_t* count) const;
	HRESULT getPolySize(std::uint32_t index, std::uint32_t* count) const;
	HRESULT getType(std::uint32_t index, std::uint16_t* type) const;
	HRESULT getPoly(std::uint32_t index, std::uint32_t* size, XY_Poly* xy_pairs, std::uint16_t* type) const;

	HRESULT getBreakRange(std::uint32_t index, const HSS_Time::WTime& time, double* x_min, double* y_min, double* x_max, double* y_max);
	HRESULT getBreakMaxSize(const HSS_Time::WTime& time, std::uint32_t* size) const;
	HRESULT getBreakCount(const HSS_Time::WTime& time, std::uint32_t* count) const;
	HRESULT getBreakSetCount(const HSS_Time::WTime& time, std::uint32_t index, std::uint32_t* count) const;
	HRESULT getBreakSize(std::uint32_t index, std::uint32_t sub_index, const HSS_Time::WTime& time, std::uint32_t* count) const;
	HRESULT getBreak(std::uint32_t index, std::uint32_t sub_index, const HSS_Time::WTime& time, std::uint32_t* size, XY_Poly* xy_pairs, std::uint16_t *type) const;

	__INLINE void clearFirebreak()	{ m_assets.clear(); };
	void buildFirebreak();
#endif
};

#ifdef HSS_SHOULD_PRAGMA_PACK
#pragma pack(pop)
#endif

#endif //__CWFGM_ASSET_H_
