/**
 * WISE_Grid_Module: CWFGM_VectorFilter.Serialize.cpp
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

#include "GridCom_ext.h"
#include "CWFGM_VectorFilter.h"
#include "angles.h"
#include "points.h"

#include "CoordinateConverter.h"
#include "gdalclient.h"
#include "doubleBuilder.h"

#define OLD_SERIALIZE
#ifndef OLD_SERIALIZE
#include "zlib.h"
#endif

#include <errno.h>
#include <stdio.h>

#include "url.h"
#include "geo_poly.h"

#include <boost/scoped_ptr.hpp>


#include "XYPoly.h"

HRESULT CCWFGM_VectorFilter::ImportPolylines(const std::filesystem::path & file_path, const std::vector<std::string_view> &permissible_drivers) {
	if (file_path.empty())						return E_INVALIDARG;

	bool engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)								return ERROR_SCENARIO_SIMULATION_RUNNING;

	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine;
	HRESULT hr;
	std::uint16_t xdim, ydim;
	OGRSpatialReferenceH oSourceSRS = nullptr;

	CSemaphoreEngage lock(GDALClient::GDALClient::getGDALMutex(), TRUE);

	if ((gridEngine = m_gridEngine) != nullptr) {
		if (FAILED(hr = gridEngine->GetDimensions(0, &xdim, &ydim)))					{ return hr; }

		PolymorphicAttribute var;

		if (FAILED(hr = gridEngine->GetAttribute(0, CWFGM_GRID_ATTRIBUTE_SPATIALREFERENCE, &var)))	{ 
			return hr; 
		}
		std::string projection;

		/*POLYMORPHIC CHECK*/
		try { projection = std::get<std::string>(var); } catch (std::bad_variant_access &) {
			weak_assert(false);
			return ERROR_PROJECTION_UNKNOWN;
		}

		oSourceSRS = CCoordinateConverter::CreateSpatialReferenceFromWkt(projection.c_str());
	} else {
		weak_assert(false);
		return ERROR_VECTOR_UNINITIALIZED;
	}

	XY_PolyLLSetAttributes set;
	set.SetCacheScale(m_resolution);

	if (SUCCEEDED(hr = set.ImportPoly(permissible_drivers, file_path, oSourceSRS)))
	{
		m_attributeNames.clear();

		RefNode<XY_PolyType> *node;
		while ((node = m_polyList.RemHead()) != NULL) {
			delete node->LN_Ptr();
			delete node;
		}

		XY_PolyLLAttributes* p = set.LH_Head();
		while (p->LN_Succ()) {
			XY_PolyType *pc = new XY_PolyType(*p);
			node = new RefNode<XY_PolyType>;
			node->LN_Ptr(pc);
			m_polyList.AddTail(node);
			pc->m_attributes = p->m_attributes;
			for (auto a : p->m_attributes) {
				m_attributeNames.insert(a.attributeName);
			}
			p = p->LN_Succ();
		}
		m_bRequiresSave = true;
	}
	if (oSourceSRS)
		OSRDestroySpatialReference(oSourceSRS);
	return hr;
}


#ifndef DOXYGEN_IGNORE_CODE

static std::string prepareUri(const std::string &uri)
{
	remote::url u;
	u.setUrl(uri);
	u.addParam("SERVICE", "WFS");
	u.addParam("REQUEST", "GetCapabilities");
	return u.build();
}

#endif


HRESULT CCWFGM_VectorFilter::ImportPolylinesWFS(const std::string & url, const std::string & layer, const std::string & username, const std::string & password) {
	if (!url.length())										return E_INVALIDARG;
	if (!layer.length())										return E_INVALIDARG;

	std::string csURL(url), csLayer(layer), csUserName(username), csPassword(password);

	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine;
	HRESULT hr;
	std::uint16_t xdim, ydim;
	OGRSpatialReferenceH oSourceSRS = nullptr;

	CSemaphoreEngage lock(GDALClient::GDALClient::getGDALMutex(), TRUE);

	if ((gridEngine = m_gridEngine) != NULL) {
		if (FAILED(hr = gridEngine->GetDimensions(0, &xdim, &ydim)))					{ return hr; }

		PolymorphicAttribute var;
		if (FAILED(hr = gridEngine->GetAttribute(0, CWFGM_GRID_ATTRIBUTE_SPATIALREFERENCE, &var)))	{ return hr; }
		std::string projection;

		/*POLYMORPHIC CHECK*/
		try { projection = std::get<std::string>(var); } catch (std::bad_variant_access &) { weak_assert(false); return ERROR_PROJECTION_UNKNOWN; };

		oSourceSRS = CCoordinateConverter::CreateSpatialReferenceFromWkt(projection.c_str());
	} else {
		weak_assert(false);
		return ERROR_VECTOR_UNINITIALIZED;
	}

	XY_PolyLLSet set;
	set.SetCacheScale(m_resolution);

	std::vector<std::string> layers;
	layers.push_back(csLayer);
	XY_PolyLLSet pset;
	std::string URI = prepareUri(csURL);
	const std::vector<std::string_view> drivers;
	if (SUCCEEDED(hr = set.ImportPoly(drivers, URI.c_str(), oSourceSRS, NULL, &layers))) {
		RefNode<XY_PolyType> *node;
		while ((node = m_polyList.RemHead()) != NULL) {
			delete node->LN_Ptr();
			delete node;
		}
		XY_PolyLL *p = set.LH_Head();
		while (p->LN_Succ()) {
			XY_PolyType *pc = new XY_PolyType(*p);
			node = new RefNode<XY_PolyType>;
			node->LN_Ptr(pc);
			m_polyList.AddTail(node);
			p = p->LN_Succ();
		}
		m_gisURL = csURL;
		m_gisLayer = csLayer;
		m_gisUID = csUserName;
		m_gisPWD = csPassword;

		m_bRequiresSave = true;
	}
	if (oSourceSRS)
		OSRDestroySpatialReference(oSourceSRS);
	return hr;
}


HRESULT CCWFGM_VectorFilter::ExportPolylines(std::string_view driver_name, const std::string & bprojection, const std::filesystem::path & file_path) {
	if ((!driver_name.length()) || (file_path.empty()))				return E_INVALIDARG;

	if (!m_polyList.GetCount())
		return E_FAIL;

	CSemaphoreEngage lock(GDALClient::GDALClient::getGDALMutex(), TRUE);

	HRESULT hr;
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine;
	OGRSpatialReferenceH oSourceSRS = NULL;
	OGRSpatialReferenceH oTargetSRS = CCoordinateConverter::CreateSpatialReferenceFromStr(bprojection.c_str());

	if ((gridEngine = m_gridEngine) != NULL) {
		PolymorphicAttribute var;

		if (FAILED(hr = gridEngine->GetAttribute(0, CWFGM_GRID_ATTRIBUTE_SPATIALREFERENCE, &var)))	{ if (oTargetSRS) OSRDestroySpatialReference(oTargetSRS); return hr; }
		std::string projection;

		/*POLYMORPHIC CHECK*/
		try { projection = std::get<std::string>(var); } catch (std::bad_variant_access &) { weak_assert(false); return ERROR_PROJECTION_UNKNOWN; };

		oSourceSRS = CCoordinateConverter::CreateSpatialReferenceFromWkt(projection.c_str());
	} else {
		weak_assert(false);
		if (oTargetSRS)
			OSRDestroySpatialReference(oTargetSRS);
		return ERROR_VECTOR_UNINITIALIZED;
	}

	XY_PolyLLSet set;
	RefNode<XY_PolyType> *pc = m_polyList.LH_Head();
	while (pc->LN_Succ()) {
		XY_PolyLL *p = (XY_PolyLL *)set.NewCopy(*pc->LN_Ptr());
		p->m_publicFlags &= (~(XY_PolyLL_BaseTempl<double>::Flags::INTERPRET_POLYMASK));
		p->m_publicFlags |= pc->LN_Ptr()->m_publicFlags;
		set.AddPoly(p);
		pc = pc->LN_Succ();
	}

	set.SetCacheScale(m_resolution);

	hr = set.ExportPoly(driver_name, file_path, oSourceSRS, oTargetSRS);
	if (oSourceSRS)
		OSRDestroySpatialReference(oSourceSRS);
	if (oTargetSRS)
		OSRDestroySpatialReference(oTargetSRS);
	return hr;
}


HRESULT CCWFGM_VectorFilter::ExportPolylinesWFS(const std::string & url, const std::string & layer, const std::string & username, const std::string & password) {
	return E_NOTIMPL;
}


std::int32_t CCWFGM_VectorFilter::serialVersionUid(const SerializeProtoOptions& options) const noexcept {
	return options.fileVersion();
}


WISE::GridProto::CwfgmVectorFilter* CCWFGM_VectorFilter::serialize(const SerializeProtoOptions& options) {
	auto filter = new WISE::GridProto::CwfgmVectorFilter();
	filter->set_version(serialVersionUid(options));

	if (m_firebreakWidth != 0.0)
		filter->set_allocated_firebreakwidth(DoubleBuilder().withValue(m_firebreakWidth).forProtobuf(options.useVerboseFloats()));

	XY_PolyLLSet set;
	RefNode<XY_PolyType> *pc = m_polyList.LH_Head();
	while (pc->LN_Succ()) {
		XY_PolyLL *p = (XY_PolyLL *)set.NewCopy(*pc->LN_Ptr());
		p->m_publicFlags &= (~(XY_PolyLL_BaseTempl<double>::Flags::INTERPRET_POLYMASK));
		p->m_publicFlags |= pc->LN_Ptr()->m_publicFlags;
		set.AddPoly(p);
		pc = pc->LN_Succ();
	}

	GeoPoly geo(&set);
	geo.setStoredUnits(GeoPoly::UTM);
	filter->set_allocated_polygons(geo.getProtobuf(options.useVerboseFloats()));

	return filter;
}


CCWFGM_VectorFilter* CCWFGM_VectorFilter::deserialize(const google::protobuf::Message& proto, std::shared_ptr<validation::validation_object> valid, const std::string& name) {
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine;
	if (!(gridEngine = m_gridEngine)) {
		if (valid)
			/// <summary>
			/// The gridEngine is not initialized but should be by this time in deserialization.
			/// </summary>
			/// <type>internal</type>
			valid->add_child_validation("WISE.GridProto.CwfgmVectorFilter", name, validation::error_level::SEVERE,
				validation::id::initialization_incomplete, "gridEngine");
		weak_assert(false);
		m_loadWarning = "Error: WISE.GridProto.CwfgmVectorFilter: No grid engine";
		throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmVectorFilter: Incomplete initialization");
	}

	auto filter = dynamic_cast_assert<const WISE::GridProto::CwfgmVectorFilter*>(&proto);
	double dValue;

	if (!filter)
	{
		if (valid)
			/// <summary>
			/// The object passed as a vector filter is invalid. An incorrect object type was passed to the parser.
			/// </summary>
			/// <type>internal</type>
			valid->add_child_validation("WISE.GridProto.CwfgmVectorFilter", name, validation::error_level::SEVERE, validation::id::object_invalid, proto.GetDescriptor()->name());
		weak_assert(false);
		m_loadWarning = "Error: WISE.GridProto.CwfgmVectorFilter: Protobuf object invalid";
		throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmVectorFilter: Protobuf object invalid", ERROR_PROTOBUF_OBJECT_INVALID);
	}
	if ((filter->version() != 1) && (filter->version() != 2))
	{
		if (valid)
			/// <summary>
			/// The object version is not supported. The asset is not supported by this version of Prometheus.
			/// </summary>
			/// <type>user</type>
			valid->add_child_validation("WISE.GridProto.CwfgmVectorFilter", name, validation::error_level::SEVERE, validation::id::version_mismatch, std::to_string(filter->version()));
		weak_assert(false);
		m_loadWarning = "Error: WISE.GridProto.CwfgmVectorFilter: Version is invalid";
		throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmVectorFilter: Version is invalid", ERROR_PROTOBUF_OBJECT_VERSION_INVALID);
	}

	auto vt = validation::conditional_make_object(valid, "WISE.GridProto.CwfgmVectorFilter", name);
	auto v = vt.lock();

	if (filter->has_firebreakwidth())
	{
		dValue = DoubleBuilder().withProtobuf(filter->firebreakwidth()).getValue();

		if ((dValue < 1.0) && (dValue != 0.0)) { 
			m_loadWarning = "Error: WISE.GridProto.CwfgmVectorFilter: Invalid fire break width value";
			if (v)
				/// <summary>
				/// The firebreak width must be between 1.0 and 250.0m, inclusive.
				/// </summary>
				/// <type>user</type>
				v->add_child_validation("Math.Double", "fireBreakWidth", validation::error_level::WARNING, validation::id::value_invalid, std::to_string(dValue), { true, 1.0 }, { true, 250.0 }, "m");
			else
				throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmVectorFilter: Invalid m_firebreakWidth value");
		}
		if (dValue > 250.0) {
			m_loadWarning = "Error: WISE.GridProto.CwfgmVectorFilter: Invalid fire break width value";
			if (v)
				/// <summary>
				/// The firebreak width must be between 1.0 and 250.0m, inclusive.
				/// </summary>
				/// <type>user</type>
				v->add_child_validation("Math.Double", "fireBreakWidth", validation::error_level::WARNING, validation::id::value_invalid, std::to_string(dValue), { true, 1.0 }, { true, 250.0 }, "m");
			else
				throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmVectorFilter: Invalid m_firebreakWidth value");
		}

		m_firebreakWidth = DoubleBuilder().withProtobuf(filter->firebreakwidth()).getValue();
	} else
		m_firebreakWidth = 0.0;

	PolymorphicAttribute var;
	if (FAILED(m_gridEngine->GetAttribute(0, CWFGM_GRID_ATTRIBUTE_SPATIALREFERENCE, &var))) {
		if (valid)
			/// <summary>
			/// The plot resolution is not readable but should be by this time in deserialization.
			/// </summary>
			/// <type>internal</type>
			valid->add_child_validation("WISE.GridProto.CwfgmVectorFilter", name, validation::error_level::SEVERE,
				validation::id::initialization_incomplete, "projection");
		weak_assert(false);
		m_loadWarning = "Error: CWISE.GridProto.CwfgmVectorFilter: Incomplete initialization";
		throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmVectorFilter: Incomplete initialization");
	}

	std::string projection;
	projection = std::get<std::string>(var);

	boost::scoped_ptr<CCoordinateConverter> convert(new CCoordinateConverter());
	convert->SetSourceProjection(projection.c_str());

	GeoPoly geo(filter->polygons(), GeoPoly::TYPE_LINKED_LIST);
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

	XY_PolyLLSet *set = geo.getLinkedList(true, v, "polygons");
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
					v->add_child_validation("Geography.GeoPoly", "polygons", validation::error_level::SEVERE, validation::id::out_of_memory, "Failed to create vector data from protobuf definitions.");
				break;
			}
			if (pn) {
				if (pn->LN_Ptr())
					m_polyList.AddTail(pn);
				else
					delete pn;
			}
			p = p->LN_Succ();
		}
		delete set;
	}

	return this;
}
