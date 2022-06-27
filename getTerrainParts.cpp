#include "geoProc.h"

std::tuple<Eigen::MatrixXd, Eigen::MatrixXi> getTerrainParts(size_t layer, size_t x_min, size_t x_max, size_t y_min, size_t y_max, const std::string& src_dem_dir) {
	auto p = getTerrainPartsWithLimit(layer, x_min, x_max, y_min, y_max,src_dem_dir);
	auto [V, F] = mergeTerrainParts(p);

	return { V, F };
}