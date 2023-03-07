/**
 * WISE_Grid_Module: CWFGM_PolyReplaceGridFilter.Serialize.cpp
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

#include "ICWFGM_Fuel.h"
#include "GridCom_ext.h"
#include "FireEngine_ext.h"
#include "CWFGM_PolyReplaceGridFilter.h"
#include "angles.h"

#include <errno.h>
#include <stdio.h>

#include "CoordinateConverter.h"
#include "gdalclient.h"
#include "doubleBuilder.h"
#include "url.h"
#include "geo_poly.h"

#include <boost/scoped_ptr.hpp>

#ifdef DEBUG
#include <assert.h>
#endif


HRESULT CCWFGM_PolyReplaceGridFilter::ImportPolygons(const std::filesystem::path & file_path, const std::vector<std::string_view> &permissible_drivers) {
	if (file_path.empty())
		return E_INVALIDARG;

	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine;
	HRESULT hr;
	std::uint16_t xdim, ydim;
	OGRSpatialReferenceH oSourceSRS = nullptr;
	CSemaphoreEngage lock(GDALClient::GDALClient::getGDALMutex(), TRUE);

	if ((m_xllcorner == -999999999.0) && (m_yllcorner == -999999999.0) && (m_resolution == -1.0)) {
		weak_assert(false);
		fixResolution();
	}

	if (gridEngine = m_gridEngine(nullptr)) {
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
	}
	else {
		weak_assert(false);
		return ERROR_GRID_UNINITIALIZED;
	}

	XY_PolyLLSet set;
	set.SetCacheScale(m_resolution);

	if (SUCCEEDED(hr = set.ImportPoly(permissible_drivers, file_path, oSourceSRS))) {

		m_polySet.RemoveAllPolys();
		XY_PolyLL *p;
		while (p = set.RemHead())
			m_polySet.AddPoly(p);
		m_bRequiresSave = true;
	}
	if (oSourceSRS)
		OSRDestroySpatialReference(oSourceSRS);
	return hr;
}


#ifndef DOXYGEN_IGNORE_CODE

static std::string prepareUri(const std::string& uri) {
	remote::url u;
	u.setUrl(uri);
	u.addParam("SERVICE", "WFS");
	u.addParam("REQUEST", "GetCapabilities");
	return u.build();
}

#endif


HRESULT CCWFGM_PolyReplaceGridFilter::ImportPolygonsWFS( const std::string & url,  const std::string & layer,  const std::string & username,  const std::string & password) {
	if (!url.length())											return E_INVALIDARG;
	if (!layer.length())										return E_INVALIDARG;

	std::string csURL(url), csLayer(layer), csUserName(username), csPassword(password);

	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine;
	HRESULT hr;
	std::uint16_t xdim, ydim;
	OGRSpatialReferenceH oSourceSRS = NULL;

	if ((m_xllcorner == -999999999.0) && (m_yllcorner == -999999999.0) && (m_resolution == -1.0)) {
		weak_assert(false);
		fixResolution();
	}

	CSemaphoreEngage lock(GDALClient::GDALClient::getGDALMutex(), TRUE);

	if (gridEngine = m_gridEngine(nullptr)) {
		if (FAILED(hr = gridEngine->GetDimensions(0, &xdim, &ydim)))					{ return hr; }

		PolymorphicAttribute var;
		if (FAILED(hr = gridEngine->GetAttribute(0, CWFGM_GRID_ATTRIBUTE_SPATIALREFERENCE, &var)))	{ return hr; }
		std::string projection;

		/*POLYMORPHIC CHECK*/
		try { projection = std::get<std::string>(var); } catch (std::bad_variant_access &) { weak_assert(false); return ERROR_PROJECTION_UNKNOWN; };

		oSourceSRS = CCoordinateConverter::CreateSpatialReferenceFromWkt(projection.c_str());
	}
	else {
		weak_assert(false);
		return ERROR_GRID_UNINITIALIZED;
	}
	XY_PolyLLSet set;
	set.SetCacheScale(m_resolution);
	
	std::vector<std::string> layers;
	layers.push_back(csLayer);
	XY_PolyLLSet pset;
	std::string URI = prepareUri(csURL);
	const std::vector<std::string_view> drivers;
	if (SUCCEEDED(hr = set.ImportPoly(drivers, URI.c_str(), oSourceSRS, NULL, &layers))) {

		m_polySet.RemoveAllPolys();
		XY_PolyLL *p;
		while (p = set.RemHead())
			m_polySet.AddPoly(p);

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


HRESULT CCWFGM_PolyReplaceGridFilter::ExportPolygons(std::string_view driver_name,  const std::string & bprojection, const std::filesystem::path & file_path) {
	if ((!driver_name.length()) || (file_path.empty()))
		return E_INVALIDARG;

	if (!m_polySet.NumPolys())
		return E_FAIL;

	CSemaphoreEngage lock(GDALClient::GDALClient::getGDALMutex(), TRUE);

	HRESULT hr;
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine;
	OGRSpatialReferenceH oSourceSRS = NULL;
	OGRSpatialReferenceH oTargetSRS = CCoordinateConverter::CreateSpatialReferenceFromStr(bprojection.c_str());

	if ((m_xllcorner == -999999999.0) && (m_yllcorner == -999999999.0) && (m_resolution == -1.0)) {
		weak_assert(false);
		fixResolution();
	}

	if (gridEngine = m_gridEngine(nullptr)) {
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
		return ERROR_GRID_UNINITIALIZED;
	}

	XY_PolyLLSet set;
	XY_PolyLL *pc = m_polySet.LH_Head();
	while (pc->LN_Succ()) {
		XY_PolyLL *p = new XY_PolyLL(*pc);
		p->m_publicFlags &= (~(XY_PolyLL_BaseTempl<double>::Flags::INTERPRET_POLYMASK));
		p->m_publicFlags |= XY_PolyLL_BaseTempl<double>::Flags::INTERPRET_POLYGON;
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


HRESULT CCWFGM_PolyReplaceGridFilter::ExportPolygonsWFS( const std::string & url,  const std::string & layer,  const std::string & username,  const std::string & password) {
	return E_NOTIMPL;
}


std::int32_t CCWFGM_PolyReplaceGridFilter::serialVersionUid(const SerializeProtoOptions& options) const noexcept {
	return options.fileVersion();
}


WISE::GridProto::CwfgmPolyReplaceGridFilter *CCWFGM_PolyReplaceGridFilter::serialize(const SerializeProtoOptions& options) {
	auto filter = new WISE::GridProto::CwfgmPolyReplaceGridFilter();
	filter->set_version(serialVersionUid(options));

	GeoPoly geo(&m_polySet);
	geo.setStoredUnits(GeoPoly::UTM);
	filter->set_allocated_polygons(geo.getProtobuf(options.useVerboseFloats()));

	return filter;
}


CCWFGM_PolyReplaceGridFilter *CCWFGM_PolyReplaceGridFilter::deserialize(const google::protobuf::Message& proto, std::shared_ptr<validation::validation_object> valid, const std::string& name) {
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine;
	if (!(gridEngine = m_gridEngine(0))) {
		if (valid)
			/// <summary>
			/// The gridEngine is not initialized but should be by this time in deserialization.
			/// </summary>
			/// <type>internal</type>
			valid->add_child_validation("WISE.GridProto.CwfgmPolyReplaceGridFilter", name, validation::error_level::SEVERE,
				validation::id::initialization_incomplete, "gridEngine");
		weak_assert(false);
		m_loadWarning = "Error: WISE.GridProto.CwfgmPolyReplaceGridFilter: No grid engine";
		throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmPolyReplaceGridFilter: Incomplete initialization");
	}

	auto filter = dynamic_cast_assert<const WISE::GridProto::CwfgmPolyReplaceGridFilter*>(&proto);

	if (!filter) {
		if (valid)
			/// <summary>
			/// The object passed as an asset is invalid. An incorrect object type was passed to the parser.
			/// </summary>
			/// <type>internal</type>
			valid->add_child_validation("WISE.GridProto.CwfgmPolyReplaceGridFilter", name, validation::error_level::SEVERE, validation::id::object_invalid, proto.GetDescriptor()->name());
		weak_assert(false);
		m_loadWarning = "Error: WISE.GridProto.CwfgmPolyReplaceGridFilter: Protobuf object invalid";
		throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmPolyReplaceGridFilter: Protobuf object invalid", ERROR_PROTOBUF_OBJECT_INVALID);
	}
	if ((filter->version() != 1) && (filter->version() != 2)) {
		if (valid)
			/// <summary>
			/// The object version is not supported. The filter is not supported by this version of Prometheus.
			/// </summary>
			/// <type>user</type>
			valid->add_child_validation("WISE.GridProto.CwfgmPolyReplaceGridFilter", name, validation::error_level::SEVERE, validation::id::version_mismatch, std::to_string(filter->version()));
		weak_assert(false);
		m_loadWarning = "Error: WISE.GridProto.CwfgmPolyReplaceGridFilter: Version is invalid";
		throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmPolyReplaceGridFilter: Version is invalid", ERROR_PROTOBUF_OBJECT_VERSION_INVALID);
	}

	auto vt = validation::conditional_make_object(valid, "WISE.GridProto.CwfgmPolyReplaceGridFilter", name);
	auto v = vt.lock();

	PolymorphicAttribute var;

	if (FAILED(gridEngine->GetAttribute(0, CWFGM_GRID_ATTRIBUTE_SPATIALREFERENCE, &var))) {
		if (valid)
			/// <summary>
			/// The plot resolution is not readable but should be by this time in deserialization.
			/// </summary>
			/// <type>internal</type>
			valid->add_child_validation("WISE.GridProto.CwfgmPolyReplaceGridFilter", name, validation::error_level::SEVERE,
				validation::id::initialization_incomplete, "projection");
		weak_assert(false);
		m_loadWarning = "Error: WISE.GridProto.CwfgmPolyReplaceGridFilter: Incomplete initialization";
		throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmPolyReplaceGridFilter: Incomplete initialization");
	}

	std::string projection;
	projection = std::get<std::string>(var);

	boost::scoped_ptr<CCoordinateConverter> convert(new CCoordinateConverter());
	convert->SetSourceProjection(projection.c_str());

	if (filter->has_polygons()) {
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
		m_polySet.RemoveAllPolys();
		XY_PolyLLSet* set = geo.getLinkedList(true, v, "polygons");
		if (set) {
			XY_PolyLL* p;
			while ((p = set->RemHead()) != NULL)
				m_polySet.AddPoly(p);
			delete set;
		}
	}

	return this;
}
