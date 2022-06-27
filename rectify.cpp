#include "geoProc.h"
#include "getTerrain.h"

std::tuple<Eigen::MatrixXd, size_t, size_t, size_t, size_t> rectify(double lower,
	const Eigen::MatrixXd& V_model,
	const Eigen::MatrixXd& V_terrain_src,
	const Eigen::MatrixXi& F_model,
	const Eigen::MatrixXi& F_terrain,
	size_t number_bins,
	size_t mosaicked_width
) {

	auto V_terrain = V_terrain_src;
	double model_max_lon = V_model(Eigen::all, 0).maxCoeff(), model_min_lon = V_model(Eigen::all, 0).minCoeff();
	double model_max_lat = V_model(Eigen::all, 1).maxCoeff(), model_min_lat = V_model(Eigen::all, 1).minCoeff();

	auto  hash_lon = [&](double lon)-> size_t {
		static size_t model_min_lon_st = size_t(model_min_lon),
			model_max_lon_st = size_t(model_max_lon);
		static size_t hash_seg = (model_max_lon_st - model_min_lon_st) / number_bins + 1;
		size_t ret = ((size_t)lon - model_min_lon_st) / hash_seg;
		assert(ret < number_bins);
		return ret;
	};
	auto  hash_lat = [&](double lat)-> size_t {
		static size_t model_min_lat_st = size_t(model_min_lat),
			model_max_lat_st = size_t(model_max_lat);
		static size_t hash_seg = (model_max_lat_st - model_min_lat_st) / number_bins + 1;
		size_t ret = ((size_t)lat - model_min_lat_st) / hash_seg;
		assert(ret < number_bins);
		return ret;
	};

	size_t num_vertices = V_model.rows();
	std::vector<std::vector<size_t>> vertices_in_bins;
	std::cout << "Distributing points into bins, step 1 / 2 \n";
	for (size_t i = 0; i < num_vertices; i++) {
		size_t lon_key = hash_lon(V_model(i, 0));
		size_t lat_key = hash_lat(V_model(i, 1));
		vertices_in_bins.push_back({ lon_key,lat_key });
		if (i % 100000 == 0) {
			std::cout << fmt::format("{} / {} {}% \n", i, num_vertices, float(i) / num_vertices * 100);
		}
	}

	std::vector<std::vector<std::vector<size_t>>> all(number_bins, std::vector<std::vector<size_t>>(number_bins));
	size_t num_faces = F_model.rows();
	std::cout << "Distributing points into bins, step 2 / 2 \n";
	for (size_t i = 0; i < num_faces; i++) {
		if (i % 100000 == 0) {
			std::cout << fmt::format("{} / {} {}% \n", i, num_faces, float(i) / num_faces * 100);
		}
		for (size_t v_idx = 0; v_idx < 3; v_idx++) {
			size_t v = F_model(i, v_idx);
			all[vertices_in_bins[v][0]][vertices_in_bins[v][1]].push_back(i);
		}
	};

	for (auto& lons : all) {
		for (auto& lats : lons) {
			auto  unique_end = std::unique(lats.begin(), lats.end());
			lats.erase(unique_end, lats.end());
		}
	}

	size_t max_row = 0, max_col = 0, min_row = 99999, min_col = 99999;
	for (int i = 0; i < V_terrain.rows(); i++) {
		static size_t model_min_lon_st = size_t(model_min_lon),
			model_max_lon_st = size_t(model_max_lon);
		static size_t model_min_lat_st = size_t(model_min_lat),
			model_max_lat_st = size_t(model_max_lat);

		size_t lon = (size_t)V_terrain(i, 0);
		size_t lat = (size_t)V_terrain(i, 1);
		if (lon < model_min_lon_st || lon > model_max_lon_st) {
			continue;
		}
		if (lat < model_min_lat_st || lat > model_max_lat_st) {
			continue;
		}

		auto& candidates = all[hash_lon(lon)][hash_lat(lat)];
		size_t all_min_height = 99999;
		size_t count = 0;
		for (auto idx : candidates) {
			size_t v0 = F_model(idx, 0),
				v1 = F_model(idx, 1),
				v2 = F_model(idx, 2);

			bool isIn = ptInTriangle(V_terrain(i, 0), V_terrain(i, 1),
				V_model(v0, 0), V_model(v0, 1),
				V_model(v1, 0), V_model(v1, 1),
				V_model(v2, 0), V_model(v2, 1));

			//isIn = true;
			if (isIn) {
				count++;
				double min_height = std::min(V_model(v0, 2), std::min(V_model(v1, 2), V_model(v2, 2)));
				if (min_height < all_min_height) {
					all_min_height = min_height;
				}
			}
		}

		if (count == 0) {
			std::cout << fmt::format("{} isn't in any model faces. \n", i);
			continue;
		}
		else {
			std::cout << fmt::format("{} is located in {} faces, it is lowered from {} to {}. \n", i, count, V_terrain(i, 2), all_min_height - lower);
			size_t row_idx = i / mosaicked_width;
			size_t col_idx = i % mosaicked_width;
			if (row_idx < min_row)
				min_row = row_idx;
			if (row_idx > max_row)
				max_row = row_idx;
			if (col_idx < min_col)
				min_col = col_idx;
			if (col_idx > max_col)
				max_col = col_idx;
			V_terrain(i, 2) = all_min_height - lower;
		}
	}

	std::cout << fmt::format("Modified extent in the merged DEM picture: \n row: {} {}\n col: {} {} \n", min_row, max_row, min_col, max_col) << std::endl;

	//igl::writeOBJ("C:\\2021\\BaiGe\\GeoProcess\\L12_rectified.obj", V_terrain, F_terrain);

	return { V_terrain ,min_row, max_row, min_col,max_col };

}