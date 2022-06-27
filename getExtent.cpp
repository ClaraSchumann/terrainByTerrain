#include "geoProc.h"
#include "projection.h"

std::tuple<size_t, size_t, size_t, size_t> getExtent(size_t layer, const Eigen::MatrixXd& vertices, double scale) {
	size_t rows = vertices.rows();
	double min_lon = vertices.block(0, 0, rows, 1).minCoeff() / scale,
		max_lon = vertices.block(0, 0, rows, 1).maxCoeff() / scale,
		min_lat = vertices.block(0, 1, rows, 1).minCoeff() / scale,
		max_lat = vertices.block(0, 1, rows, 1).maxCoeff() / scale;

	size_t min_x = getStartX(layer, min_lon), max_x = getStartX(layer, max_lon),
		min_y = getStartY(layer, min_lat), max_y = getStartY(layer, max_lat);

	if (min_x > max_x) {
		std::swap(min_x, max_x);
	}
	if (min_y > max_y) {
		std::swap(min_y, max_y);
	}

	return { min_x - 1, max_x + 1, min_y - 1, max_y + 1 };
}
