/**
 * WISE_Grid_Module: CWFGM_FuelMap.h
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
#include "results.h"
#include "semaphore.h"
#include "valuecache_mt.h"
//#include "ISerializeXMLStream.h"
#include "ISerializeProto.h"
#include "validation_object.h"
#include "cwfgmFuelMap.pb.h"

#ifdef HSS_SHOULD_PRAGMA_PACK
#pragma pack(push, 8)
#endif


/**
 * A CWFGM FuelMap retains relationships between CWFGM fuels and their associated grid file and internal indices.  The grid file indices are predetermined when the files are exported from the GIS.  The FuelMap object assigns but may not change internal indexes.  The FuelMap may retain up to 255 unique relationships. This object also implements the standard COM IPersistStream, IPersistStreamInit, and IPersisStorage interfaces, for use for loading and saving.  Serialization methods declared in these interfaces save the associations as well as the fuel types.  The client application must be careful not to save a fuel type twice if it maintains its own list of fuel types.  This rule is imposed to remove any unnecessary dependencies on the client code to ensure correctness of operation.  The client application must also be aware and compensate for any composite (mixed) fuel types, too.
 * 
 *		NOTE: FuelMap manages all fuel types for a document - including those that may not necessarily be used.  It also does the
 *		serialization on them so that it can guarantee load/save correctness without depending out outside client code that may not
 *		be correct, or just is simply broken.  What's the big deal?  We can't guarantee pointers (any pointers, COM or otherwise)
 *		will be the same during a re-load (actually, we can almost guarantee that they will be different).  This wouldn't be an
 *		issue if we knew that the user will always be using MFC CArchive's.  Then the solution is quite simple - we can use the
 *		MapObject() method to take care of pointers becoming invalid during the re-load.  Our other choices were:
 *		1.) add a method for the client to "fill" specific entries into this table - in which case we have to trust that we are
 *		    given good ID's, etc. - where we do no loading and saving here at all.
 *		2.) add a method to change pointers after the load (like a re-map like MapObject()) and trust that the client code hits
 *		    all entries to ensure we don't try to deref off a now-bad pointer (after a reload)
 *		Both options depend on correctness of outside code to guarantee correctness.
 *
 *		The client can still maintain another list of fuel-types.  No reason why it cannot.  But for the client to remain
 *		"computationally correct", then it shouldn't try to serialize fuel-types that are managed by this list.
 */
class GRIDCOM_API CCWFGM_FuelMap : public ICWFGM_CommonBase, public ISerializeProto {

#ifndef DOXYGEN_IGNORE_CODE
	friend class CWFGM_FuelMapHelper;
public:
	CCWFGM_FuelMap();
	CCWFGM_FuelMap(const CCWFGM_FuelMap &toCopy);
	~CCWFGM_FuelMap();

#endif

public:
	/**
		Changes the state of the object with respect to access rights.  When the object is used by an active simulation, it must not be modified.
		When the object is somehow modified, it must be done so in an atomic action to prevent concerns with arising from multithreading.
		Note that these locks are primarily needed to ensure a simulation dependency is not changed while it occurs.  Locking request is
		forwarded to each attached ICWFGM_Fuel object.\n\n
		In the event of an error, then locking is undone to reflect an error state.
		\param	exclusive	TRUE if the requester wants a write lock, false for read/shared access
		\param	obtain	TRUE to obtain the lock, FALSE to release the lock.  If this is FALSE, then the 'exclusive' parameter must match the initial call used to obtain the lock.
		\sa ICWFGM_FuelMap::MT_Lock
		\sa ICWFGM_Fuel::MT_Lock
		\retval	SUCCESS_STATE_OBJECT_UNLOCKED	Lock was released.
		\retval	SUCCESS_STATE_OBJECT_LOCKED_WRITE	Exclusive/write lock obtained.
		\retval	SUCCESS_STATE_OBJECT_LOCKED_SCENARIO	A scenario successfully required a lock for purposes of simulating.
		\retval	SUCCESS_STATE_OBJECT_LOCKED_READ	Shared/read lock obtained.
		\retval	S_OK	Successful
	*/
	virtual NO_THROW HRESULT MT_Lock(bool exclusive, std::uint16_t obtain);
	/**
		Given a fuel interface, sets 'file_index' to the grid file import index, 'export_file_index' to the grid file export index, and sets 'fuel_index' to the internal index. This method can be called repeatedly to iterate through all relationships for a given fuel.  The first call to this method should place (std::uint8_t)-1 into 'fuel_index' (always).  Subsequent calls should use the previously returned 'fuel_index' value.
		\param	fuel	A CWFGM Fuel interface handle.
		\param	file_index	Return value for a grid file import index.
		\param	export_file_index	Return value for a grid file export index.
		\param	fuel_index	Return value for an internal fuel index.
		\sa ICWFGM_FuelMap::IndexOfFuel
		\retval	E_POINTER	The address provided for fuel, file_index or fuel_index is invalid.
		\retval	S_OK	Successful.
		\retval	ERROR_FUELS_FUEL_UNKNOWN	'file_index' is not in any associations.
	*/
	virtual NO_THROW HRESULT IndexOfFuel(ICWFGM_Fuel *fuel, long *file_index, long *export_file_index, std::uint8_t *fuel_index) const;
	/**
		Given a grid file index, sets 'fuel_index' to the internal index,'export_file_index' to the export grid file index, and sets 'fuel' to the appropriate CWFGM Fuel interface handle.  This call does not adjust the COM reference counters for 'fuel'; if the caller plans on retaining the pointer then it should increment the reference counter.
		\param	file_index	A grid file index.
		\param	fuel_index	Return value for an internal fuel index.
		\param	export_file_index	Return value for a grid file export index.
		\param	fuel	Return value for a CWFGM Fuel interface handle.
		\sa ICWFGM_FuelMap::FuelAtFileIndex
		\retval	E_POINTER	The address provided for fuel_index or fuel is invalid.
		\retval	S_OK	Successful.
		\retval	ERROR_FUELS_FUEL_UNKNOWN	'file_index' is not in any associations.
	*/
	virtual NO_THROW HRESULT FuelAtFileIndex(long file_index, std::uint8_t *fuel_index, long *export_file_index,  ICWFGM_Fuel **fuel) const;
	/**
		Given an internal fuel index, sets 'file_index' to the grid file import index, 'export_file_index' to the grid file export index, and sets 'fuel' to the appropriate CWFGM Fuel interface handle.  This call does not adjust the COM reference counters for 'fuel'; if the caller plans on retaining the pointer then it should increment the reference counter.
		\param	fuel_index	Internal fuel index.
		\param	file_index	Return value for a grid file import index.
		\param	export_file_index	Return value for a grid file export index.
		\param	fuel	Return value for a CWFGM Fuel interface handle.
		\sa ICWFGM_FuelMap::FuelAtIndex
		\retval	E_POINTER	The address provided for file_index, export_file_index or fuel is invalid.
		\retval	S_OK	Successful.
		\retval	ERROR_FUELS_FUEL_UNKNOWN	'fuel_index' is invalid (currently unused).
	*/
	virtual NO_THROW HRESULT FuelAtIndex(std::uint8_t fuel_index, long *file_index, long *export_file_index, ICWFGM_Fuel **fuel) const;
	/**
		Removes the association between the file index for a given fuel.  The call will also decrement the COM reference counter associated with the fuel, which will conditionally trigger deletion of the fuel.
		\param	fuel	Fuel to remove relationship for.
		\param	file_index	File index to remove relationship for.
		\sa ICWFGM_FuelMap::RemoveFuelIndex
		\retval	E_POINTER	The address provided for fuel is invalid.
		\retval	ERROR_SCENARIO_SIMULATION_RUNNING	Value cannot be changed as it is used by a currently running scenario.
		\retval	S_OK	Successful.
		\retval	ERROR_FUELS_FUEL_UNKNOWN	The fuel/index association to be removed could not be found.
	*/
	virtual NO_THROW HRESULT RemoveFuelIndex(ICWFGM_Fuel *fuel, long file_index);
	/**
		Removes a given CWFGM fuel from this FuelMap.  Removes all mappings related to this fuel object.  This call will decrement the COM reference counter associated with 'fuel' for each index that is assigned to the fuel.
		\param	fuel	Fuel to remove.
		\sa ICWFGM_FuelMap::RemoveFuel
		\retval	E_POINTER	The address provided for fuel is invalid.
		\retval	ERROR_SCENARIO_SIMULATION_RUNNING	Value cannot be changed as it is being used in a currently running scenario.
		\retval	S_OK	Successful.
		\retval	ERROR_FUELS_FUEL_UNKNOWN	'fuel' is not in the FuelMap.
	*/
	virtual NO_THROW HRESULT RemoveFuel(ICWFGM_Fuel *fuel);
	/**
		Adds a relationship between a CWFGM fuel and a grid file index, sets 'fuel_index' to the internal index that this FuelMap has assigned to it.  This call will increment the COM reference counter associated with 'fuel'.
		\param	fuel	Fuel to add.
		\param	file_index	Index for the fuel during grid import operations.
		\param	export_file_index	Index for the fuel during grid export operations.  This value is currently unused by this object and is available for any purposes that the client application may have.
		\param	fuel_index	Return value for internal fuel index.
		\sa ICWFGM_FuelMap::AddFuel
		\retval	E_POINTER	The address provided for fuel or fuel_index is invalid.
		\retval	ERROR_SCENARIO_SIMULATION_RUNNING	Value cannot be changed as it is being used in a currently running scenario.
		\retval	E_NOINTERFACE	'fuel' does not support the ICWFGM_Fuel interface.
		\retval	ERROR_NOT_ENOUGH_MEMORY	Insufficient memory. 255 Fuel entries currently exist.
		\retval	ERROR_FUELS_GRIDVALUE_ALREADY_ASSIGNED	The fuel to add would have been assigned a non-unique file identifier that is not (-1).
		\retval	SUCCESS_FUELS_FUEL_ALREADY_ADDED	The fuel is already in the
		\retval	FuelMap; a particular fuel can occur in a FuelMap more than once.
		\retval	S_OK	Successful.
	*/
	virtual NO_THROW HRESULT AddFuel( ICWFGM_Fuel *fuel, long file_index, long export_file_index, std::uint8_t *fuel_index);
	/**
		Sets 'count' to the number of fuel relationships currently in this FuelMap.  Sets 'unique_count' to the number of fuels in this FuelMap.
		\param	count	Return value for number of fuel relationships.
		\param	unique_count	Return value for number of fuels.
		\sa ICWFGM_FuelMap::GetFuelCount
		\retval	E_POINTER	The address provided for count or unique_count is invalid.
		\retval	S_OK	Successful.
	*/
	virtual NO_THROW HRESULT GetFuelCount( std::uint8_t *count, std::uint8_t *unique_count) const;
	/**
		Creates a new fuel map and places the pointer to the object in 'newFuelMap'. Duplicates all relationships maintained by the object being called in a new FuelMap object.  This call differs from 'Copy' in that all fuel types are also duplicated.
		\param	newFuelMap	Receiver pointer for the newly created, cloned fuel map object.
		\sa ICWFGM_FuelMap::Clone
		\retval	E_POINTER	The address provided for newFuelMap is invalid.
		\retval	E-OUTOFMEMORY	Insufficient memory.
		\retval	E_NOINTERFACE	'newFuelMap' is not a successfully created CWFGM FuelMap object.
		\retval	ERROR_SEVERITY_WARNING	A file exception or other miscellaneous error.
		\retval	S_OK	Successful.
	*/
	virtual NO_THROW HRESULT Clone(boost::intrusive_ptr<ICWFGM_CommonBase> *newObject) const;
	/**
		Returns an available file index.  The logic is as follows: ignore (-9999) while searching all fuel mappings for the minimum negative import fuel index value.  If no negative indicies exist, then return -2.  Otherwise, return 1 less than the minimum found value.
		\param	index	Return value for a file index.
		\sa ICWFGM_FuelMap::GetAvailableFileIndex
		\retval	E_POINTER	The address provided for index is invalid.
		\retval	S_OK	Successful.
	*/
	virtual NO_THROW HRESULT GetAvailableFileIndex(long *index) const;
	/**
		Polymorphic.  This routine retrieves an attribute/option value given the attribute/option index.
		\param	option	Currently the only valid attribute/option index supported is CWFGM_ATTRIBUTE_LOAD_WARNING.
		\param	value	Return value for the attribute/option index.
		\sa ICWFGM_FuelMap::GetAttribute
		\retval	E_POINTER	The address provided is invalid.
		\retval	S_OK	Successful.
		\retval	E_INVALIDARG	Unknown attribute/option index.
	*/
	virtual NO_THROW HRESULT GetAttribute(std::uint16_t option, PolymorphicAttribute *value) const;
	virtual NO_THROW HRESULT SetAttribute(std::uint16_t option, const PolymorphicAttribute &value);

#ifndef DOXYGEN_IGNORE_CODE
public:
	virtual std::int32_t serialVersionUid(const SerializeProtoOptions& options) const noexcept override;
	virtual WISE::GridProto::CwfgmFuelMap* serialize(const SerializeProtoOptions& options) override;
	virtual CCWFGM_FuelMap *deserialize(const google::protobuf::Message& proto, std::shared_ptr<validation::validation_object> valid, const std::string& name) override;
	virtual std::optional<bool> isdirty(void) const noexcept override { return m_bRequiresSave; }

protected:
	CRWThreadSemaphore	m_lock;

protected:
	boost::intrusive_ptr<ICWFGM_Fuel> m_fuel[256];				// these are small enough arrays that we might as well just
	long			m_fileIndex[256];			// allocate them statically as keep them on a linked list, etc.  Even though we only allow 255 entries, we
									// define an array of 256 to make the code simpler - we just leave the last entry as unassignable.
	long			m_exportFileIndex[256];

    private:
	bool ReplaceFuelIndex(long old_file_index, long new_file_index);

	std::string			m_loadWarning;

	struct fuel_cache_key {
		ICWFGM_Fuel	*fuel_pointer;
		std::uint8_t fuel_index;
	};
	struct fuel_cache_result {
		long file_index;
		long export_index;
		std::uint8_t fuel_index;
	};
	ValueCacheTempl_MT<fuel_cache_key, fuel_cache_result> m_cache;
#endif

protected:
	bool m_bRequiresSave;
};

#ifdef HSS_SHOULD_PRAGMA_PACK
#pragma pack(pop)
#endif
