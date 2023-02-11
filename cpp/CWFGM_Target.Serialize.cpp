/**
 * WISE_Grid_Module: CWFGM_Target.Serialize.h
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

#ifdef _MSC_VER
#include <winerror.h>
#else
#include "hresult.h"
#endif
#include "GridCom_ext.h"
#include "CWFGM_Target.h"
#include "angles.h"
#include "points.h"
#include "CoordinateConverter.h"
#include "gdalclient.h"
#include "doubleBuilder.h"

#include <errno.h>
#include <stdio.h>
#include "url.h"
#include "geo_poly.h"
#include <boost/scoped_ptr.hpp>

#ifndef _MSC_VER
using namespace ResultCodes;
#endif



HRESULT CCWFGM_Target::ImportPointSet(const std::filesystem::path& file_path, const std::vector<std::string_view> &permissible_drivers) {
	if (file_path.empty())						return E_POINTER;

	SEM_BOOL engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)								return ERROR_SCENARIO_SIMULATION_RUNNING;

	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine;
	HRESULT hr;
	std::uint16_t xdim, ydim;
	OGRSpatialReferenceH oSourceSRS = nullptr;

	CSemaphoreEngage lock(GDALClient::GDALClient::getGDALMutex(), TRUE);

	double gridResolution;

	if ((gridEngine = m_gridEngine) != nullptr) {
		if (FAILED(hr = gridEngine->GetDimensions(0, &xdim, &ydim)))								{ return hr; }

		PolymorphicAttribute var;
		if (FAILED(hr = gridEngine->GetAttribute(0, CWFGM_GRID_ATTRIBUTE_PLOTRESOLUTION, &var)))	{ return hr; } VariantToDouble_(var, &gridResolution);

		if (FAILED(hr = gridEngine->GetAttribute(0, CWFGM_GRID_ATTRIBUTE_SPATIALREFERENCE, &var))) {
			return hr;
		}
		std::string projection;

		/*POLYMORPHIC CHECK*/
		try { projection = std::get<std::string>(var); }
		catch (std::bad_variant_access&) { 
			weak_assert(false);
			return ERROR_PROJECTION_UNKNOWN;
		}

		oSourceSRS = CCoordinateConverter::CreateSpatialReferenceFromWkt(projection.c_str());
	}
	else {
		weak_assert(false);
		return ERROR_VECTOR_UNINITIALIZED;
	}

	XY_PolyLLSetAttributes set;
	set.SetCacheScale(gridResolution);

	if (SUCCEEDED(hr = set.ImportPoly(permissible_drivers, file_path, oSourceSRS, nullptr, nullptr, nullptr, true))) {
		XY_PolyLLAttributes* pa = set.LH_Head();
		while (pa->LN_Succ()) {
			if (!pa->IsMultiPoint())		// targets must be only (multi)point data
				return ERROR_INVALID_DATATYPE | ERROR_SEVERITY_WARNING;
			pa = pa->LN_Succ();
		}

		m_attributeNames.clear();
		RefNode<XY_PolyType>* node;
		while ((node = m_targets.RemHead()) != NULL) {
			delete node->LN_Ptr();
			delete node;
		}

		XY_PolyLLAttributes* p = set.LH_Head();
		while (p->LN_Succ()) {
			XY_PolyType* pc = new XY_PolyType(*p);
			node = new RefNode<XY_PolyType>;
			node->LN_Ptr(pc);
			m_targets.AddTail(node);
			pc->m_attributes = p->m_attributes;
			for (auto a : p->m_attributes) {
				m_attributeNames.insert(a.attributeName);
			}
			p = p->LN_Succ();
		}
		rescanRanges();
		m_bRequiresSave = true;
	}
	if (oSourceSRS)
		OSRDestroySpatialReference(oSourceSRS);
	return hr;
}


#ifndef DOXYGEN_IGNORE_CODE

static std::string prepareUri(const std::string& uri)
{
	remote::url u;
	u.setUrl(uri);
	u.addParam("SERVICE", "WFS");
	u.addParam("REQUEST", "GetCapabilities");
	return u.build();
}

#endif


HRESULT CCWFGM_Target::ImportPointSetWFS(const std::string& url, const std::string& layer, const std::string& username, const std::string& password) {
	if (!url.length())										return E_INVALIDARG;
	if (!layer.length())										return E_INVALIDARG;

	std::string csURL(url), csLayer(layer), csUserName(username), csPassword(password);

	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine;
	HRESULT hr;
	double gridResolution;
	std::uint16_t xdim, ydim;
	OGRSpatialReferenceH oSourceSRS = nullptr;

	CSemaphoreEngage lock(GDALClient::GDALClient::getGDALMutex(), TRUE);

	if ((gridEngine = m_gridEngine) != nullptr) {
		if (FAILED(hr = gridEngine->GetDimensions(0, &xdim, &ydim))) { return hr; }

		PolymorphicAttribute var;
		if (FAILED(hr = gridEngine->GetAttribute(0, CWFGM_GRID_ATTRIBUTE_PLOTRESOLUTION, &var))) { return hr; } VariantToDouble_(var, &gridResolution);
		if (FAILED(hr = gridEngine->GetAttribute(0, CWFGM_GRID_ATTRIBUTE_SPATIALREFERENCE, &var))) { return hr; }
		std::string projection;

		/*POLYMORPHIC CHECK*/
		try { projection = std::get<std::string>(var); }
		catch (std::bad_variant_access&) { weak_assert(false); return ERROR_PROJECTION_UNKNOWN; };

		oSourceSRS = CCoordinateConverter::CreateSpatialReferenceFromWkt(projection.c_str());
	}
	else {
		weak_assert(false);
		return ERROR_VECTOR_UNINITIALIZED;
	}

	XY_PolyLLSet set;
	set.SetCacheScale(gridResolution);

	std::vector<std::string> layers;
	layers.push_back(csLayer);
	XY_PolyLLSet pset;
	std::string URI = prepareUri(csURL);
	const std::vector<std::string_view> drivers;
	if (SUCCEEDED(hr = set.ImportPoly(drivers, URI.c_str(), oSourceSRS, NULL, &layers))) {
		RefNode<XY_PolyType>* node;
		while ((node = m_targets.RemHead()) != NULL) {
			delete node->LN_Ptr();
			delete node;
		}
		XY_PolyLL* p = set.LH_Head();
		while (p->LN_Succ()) {
			XY_PolyType* pc = new XY_PolyType(*p);
			node = new RefNode<XY_PolyType>;
			node->LN_Ptr(pc);
			m_targets.AddTail(node);
			p = p->LN_Succ();
		}
		m_gisURL = csURL;
		m_gisLayer = csLayer;
		m_gisUID = csUserName;
		m_gisPWD = csPassword;

		rescanRanges();
		m_bRequiresSave = true;
	}
	if (oSourceSRS)
		OSRDestroySpatialReference(oSourceSRS);
	return hr;
}


HRESULT CCWFGM_Target::ExportPointSet(std::string_view driver_name, const std::string& bprojection, const std::filesystem::path& file_path) {
	if ((!driver_name.length()) || (file_path.empty()))				return E_INVALIDARG;

	if (!m_targets.GetCount())
		return E_FAIL;

	CSemaphoreEngage lock(GDALClient::GDALClient::getGDALMutex(), TRUE);

	HRESULT hr;
	double gridResolution;
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine;
	OGRSpatialReferenceH oSourceSRS = NULL;
	OGRSpatialReferenceH oTargetSRS = CCoordinateConverter::CreateSpatialReferenceFromStr(bprojection.c_str());

	if ((gridEngine = m_gridEngine) != NULL) {
		PolymorphicAttribute var;
		if (FAILED(hr = gridEngine->GetAttribute(0, CWFGM_GRID_ATTRIBUTE_PLOTRESOLUTION, &var)))	{ if (oTargetSRS) OSRDestroySpatialReference(oTargetSRS); return hr; } VariantToDouble_(var, &gridResolution);

		if (FAILED(hr = gridEngine->GetAttribute(0, CWFGM_GRID_ATTRIBUTE_SPATIALREFERENCE, &var))) { if (oTargetSRS) OSRDestroySpatialReference(oTargetSRS); return hr; }
		std::string projection;

		/*POLYMORPHIC CHECK*/
		try { projection = std::get<std::string>(var); }
		catch (std::bad_variant_access&) { weak_assert(false); return ERROR_PROJECTION_UNKNOWN; };

		oSourceSRS = CCoordinateConverter::CreateSpatialReferenceFromWkt(projection.c_str());
	}
	else {
		weak_assert(false);
		if (oTargetSRS)
			OSRDestroySpatialReference(oTargetSRS);
		return ERROR_VECTOR_UNINITIALIZED;
	}

	XY_PolyLLSet set;
	RefNode<XY_PolyType>* pc = m_targets.LH_Head();
	while (pc->LN_Succ()) {
		XY_PolyLL* p = (XY_PolyLL *)set.NewCopy(*pc->LN_Ptr());
		p->m_publicFlags = XY_PolyLL_BaseTempl<double>::Flags::INTERPRET_MULTIPOINT;
		set.AddPoly(p);
		pc = pc->LN_Succ();
	}

	set.SetCacheScale(gridResolution);
	hr = set.ExportPoly(driver_name, file_path, oSourceSRS, oTargetSRS);
	if (oSourceSRS)
		OSRDestroySpatialReference(oSourceSRS);
	if (oTargetSRS)
		OSRDestroySpatialReference(oTargetSRS);
	return hr;
}


HRESULT CCWFGM_Target::ExportPointSetWFS(const std::string& url, const std::string& layer, const std::string& username, const std::string& password) {
	return E_NOTIMPL;
}


std::int32_t CCWFGM_Target::serialVersionUid(const SerializeProtoOptions& options) const noexcept {
	return options.fileVersion();
}


WISE::GridProto::CwfgmTarget* CCWFGM_Target::serialize(const SerializeProtoOptions& options) {
	auto filter = new WISE::GridProto::CwfgmTarget();
	filter->set_version(serialVersionUid(options));

	XY_PolyLLSet set;
	RefNode<XY_PolyType>* pc = m_targets.LH_Head();
	while (pc->LN_Succ()) {
		XY_PolyLL* p = (XY_PolyLL*)set.NewCopy(*pc->LN_Ptr());
		p->m_publicFlags = XY_PolyLL_BaseTempl<double>::Flags::INTERPRET_MULTIPOINT;
		set.AddPoly(p);
		pc = pc->LN_Succ();
	}

	GeoPoly geo(&set);
	geo.setStoredUnits(GeoPoly::UTM);
	filter->set_allocated_targets(geo.getProtobuf(options.useVerboseFloats()));

	return filter;
}


CCWFGM_Target* CCWFGM_Target::deserialize(const google::protobuf::Message& proto, std::shared_ptr<validation::validation_object> valid, const std::string& name)
{
	if (!m_gridEngine) {
		if (valid)
			/// <summary>
			/// The gridEngine is not initialized but should be by this time in deserialization.
			/// </summary>
			/// <type>internal</type>
			valid->add_child_validation("WISE.GridProto.CwfgmTarget", name, validation::error_level::SEVERE,
				validation::id::initialization_incomplete, "gridEngine");
		weak_assert(false);
		m_loadWarning = "Error: WISE.GridProto.CwfgmTarget: No grid engine";
		throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmTarget: Incomplete initialization");
	}

	auto filter = dynamic_cast<const WISE::GridProto::CwfgmTarget*>(&proto);

	if (!filter)
	{
		if (valid)
			/// <summary>
			/// The object passed as a target is invalid. An incorrect object type was passed to the parser.
			/// </summary>
			/// <type>internal</type>
			valid->add_child_validation("WISE.GridProto.CwfgmTarget", name, validation::error_level::SEVERE, validation::id::object_invalid, proto.GetDescriptor()->name());
		weak_assert(false);
		m_loadWarning = "Error: WISE.GridProto.CwfgmTarget: Protobuf object invalid";
		throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmTarget: Protobuf object invalid", ERROR_PROTOBUF_OBJECT_INVALID);
	}

	if ((filter->version() != 1) && (filter->version() != 2))
	{
		if (valid)
			/// <summary>
			/// The object version is not supported. The target is not supported by this version of Prometheus.
			/// </summary>
			/// <type>user</type>
			valid->add_child_validation("WISE.GridProto.CwfgmTarget", name, validation::error_level::SEVERE, validation::id::version_mismatch, std::to_string(filter->version()));
		weak_assert(false);
		m_loadWarning = "Error: WISE.GridProto.CwfgmTarget: Version is invalid";
		throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmTarget: Version is invalid", ERROR_PROTOBUF_OBJECT_VERSION_INVALID);
	}

	auto vt = validation::conditional_make_object(valid, "WISE.GridProto.CwfgmTarget", name);
	auto v = vt.lock();

	PolymorphicAttribute var;
	HRESULT hr;

	if (FAILED(hr = m_gridEngine->GetAttribute(0, CWFGM_GRID_ATTRIBUTE_SPATIALREFERENCE, &var)))
	{
		if (valid)
			/// <summary>
			/// The projection is not readable but should be by this time in deserialization.
			/// </summary>
			/// <type>internal</type>
			valid->add_child_validation("WISE.GridProto.CwfgmTarget", name, validation::error_level::SEVERE,
				validation::id::initialization_incomplete, "projection");
		weak_assert(false);
		m_loadWarning = "Error: WISE.GridProto.CwfgmTarget: Incomplete initialization";
		throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmTarget: Incomplete initialization");
	}

	std::string projection;
	projection = std::get<std::string>(var);

	boost::scoped_ptr<CCoordinateConverter> convert(new CCoordinateConverter());
	convert->SetSourceProjection(projection.c_str());

	GeoPoly geo(filter->targets(), GeoPoly::TYPE_LINKED_LIST);
	geo.setStoredUnits(GeoPoly::UTM);
	geo.setConverter([&convert](std::uint8_t type, double x, double y, double z) -> std::tuple<double, double, double>
		{
			XY_Point loc = convert->start()
				.fromPoints(x, y, z)
				.asLatLon()
				.endInUTM()
				.to2DPoint();
			return std::make_tuple(loc.x, loc.y, 0.0);
		});

	XY_PolyLLSet *set = geo.getLinkedList(true, v, "targets");
	if (set) {
		XY_PolyLL* p = set->LH_Head();
		while (p->LN_Succ()) {
			RefNode<XY_PolyType>* pn = nullptr;
			try {
				pn = new RefNode<XY_PolyType>();
				pn->LN_Ptr(new XY_PolyType(*p));
			}
			catch (std::exception& e) {
				if (v)
					v->add_child_validation("Geography.GeoPoly", "targets", validation::error_level::SEVERE, validation::id::out_of_memory, "Failed to create targets from protobuf definitions.");
				break;
			}
			if (pn) {
				if (pn->LN_Ptr())
					m_targets.AddTail(pn);
				else
					delete pn;
			}
			p = p->LN_Succ();
		}
		rescanRanges();
		delete set;
	}

	return this;
}
