#pragma once

#include <filesystem>
#include <fstream>
#include <regex>
#include <iostream>

#include <rapidxml/rapidxml.hpp>
#include <proj.h>

struct ObjProfile {
	size_t EPSG;
	double origin_x;
	double origin_y;
	double origin_h;
	double wgs84_lon;;
	double wgs84_lat;;
	double wgs84_height;

	ObjProfile(size_t _EPSG, double _origin_x, double _origin_y, double _origin_h) :EPSG(_EPSG), origin_x(_origin_x), origin_y(_origin_y), origin_h(_origin_h) {
		PJ_CONTEXT* C;
		PJ* P;
		PJ* norm;
		PJ_COORD a, b;

		C = proj_context_create();
		P = proj_create_crs_to_crs(C, "EPSG:32647", "EPSG:4326", NULL);
		a = proj_coord(origin_x, origin_y, origin_h, 0);
		b = proj_trans(P, PJ_FWD, a);
		wgs84_lat = b.enu.e;
		wgs84_lon = b.enu.n;
		wgs84_height = b.enu.u;
		proj_destroy(P);
		proj_context_destroy(C);
	};
};

ObjProfile getObjProfile(const std::filesystem::path& p);