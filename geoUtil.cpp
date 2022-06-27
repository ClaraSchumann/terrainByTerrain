#include <iostream>

#include "yaml-cpp/yaml.h"
#include <rapidxml/rapidxml.hpp>

#include "projection.h"
#include "getTerrain.h"
#include "geoProc.h"
#include "obj_profile.h"

extern const char* workDirectory = "C:\\2021\\BaiGe\\GeoProcess\\";
extern const char* terrainRaw = "C:\\2021\\BaiGe\\GeoProcess\\TerrainRaw\\";

extern const char* targetModel = "C:\\2021\\BaiGe\\GeoProcess\\minObj\\texture.obj";
extern double targetModel_origin_x = 471928.19247236708;
extern double targetModel_origin_y = 3439059.3636666504;
extern double targetModel_origin_h = 3408.5199999989136;
extern int targetModel_origin_EPSG = 32647;

extern double degree2meter = 111000; // Need not be precise.

/*
void snippets() {
	// Download DEM.
	double margin = 1;
	double lon_cen = 98.7026;
	double lat_cen = 31.0810;
	int layer_lowest = 7, layer_highest = 12;
	for (int layer = layer_lowest; layer <= layer_highest; layer++) {
		//downloadTerrainIn(lon_cen - margin, lon_cen + margin, lat_cen - margin, lat_cen + margin, layer);
	};

	// Test download DEM.
	test();

	// Transform terrain object to WGS84, with the lon/lats scaled up by 111000 to make is prettier.
	void terrainObjToWGS84();

	// Generate mosaicked DEM in .obj format.
	// auto [V, F] = geo_demo_3(12, 3170, 3172, 668, 672);
}
*/

int main(int argc, char* argv[])
{
	std::string configureFile;
	if (argc < 2)
		configureFile = "conf.yaml";
	else
		configureFile = argv[1];
	auto configuration = YAML::LoadFile(configureFile.c_str());

	auto obj_directory = configuration["obj_directory"].as<std::string>();
	auto obj_directory_path = std::filesystem::path(obj_directory);
	if (!std::filesystem::is_directory(obj_directory_path)) {
		throw std::exception("Please verify the directory containing .obj files");
	}

	ObjProfile src_obj_profile = getObjProfile(obj_directory_path / "metadata.xml");

	auto works = configuration["works"];
	for (int i = 0; i < works.size(); i++) {
		auto work = works[i];
		if (!work["apply"].as<bool>()) {
			continue;
		};

		switch (auto operation = hash_djb2a(work["operation"].as<std::string>())) {
		case "download_dem_raw"_sh: {
			size_t layer_lowest = work["layer_lowest"].as<size_t>();
			size_t layer_highest = work["layer_highest"].as<size_t>();
			double margin = work["margin"].as<double>();
			auto out_dem_directory = std::filesystem::path(work["out_dem_directory"].as<std::string>());
			auto token = work["tianditu_token"].as<std::string>();
			for (size_t layer = layer_lowest; layer <= layer_highest; layer++) {
				downloadTerrainIn(src_obj_profile.wgs84_lon - margin, src_obj_profile.wgs84_lon + margin, src_obj_profile.wgs84_lat - margin, src_obj_profile.wgs84_lat + margin, layer, out_dem_directory, token);
			};
			break;
		};
		case "read_obj"_sh: {
			std::string path = work["path"].as<std::string>();
			auto [V, F] = m_readObj(path);
			if (work["display"].as<bool>()) {
				showMesh(V, F);
			}
			break;
		};
		case "merge_obj"_sh: {
			std::string path = work["path"].as < std::string>();
			auto [V, F] = m_mergeObj(path);
			if (work["display"].as<bool>()) {
				showMesh(V, F);
			}
			if (work["store"].as<bool>()) {
				igl::writeOBJ(work["storePath"].as<std::string>(), V, F);
			}
			break;
		};
		case "rectification"_sh: {
			break;
		}
		default:
			std::cout << fmt::format("Operation {} not recognized!\n", operation);
			break;
		}
	}

	// 12 3170 - 3172 668 - 672 在图L12中间偏左位置
	// 11 1584 - 1586 334 - 336 
	//auto [V,F] = geo_demo_3(11, 1584, 1586, 334 , 336);

	//interpolation(100);

	// Extend: 
	// row: 341 369
	// col: 142 181

	//regenerateDEMFiles("C:\\2021\\BaiGe\\GeoProcess\\TerrainModified", "C:\\2021\\BaiGe\\GeoProcess\\L12_rectified.obj", 12, 3170, 3172, 668, 672);

	return 0;
}
