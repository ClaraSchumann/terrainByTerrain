#include "geoProc.h"
#include "getTerrain.h"

bool ptInTriangle(double px, double py, double p0x, double p0y, double p1x, double p1y,
	double p2x, double p2y) {
	double dX = px - p2x;
	double dY = py - p2y;
	double dX21 = p2x - p1x;
	double dY12 = p1y - p2y;
	double D = dY12 * (p0x - p2x) + dX21 * (p0y - p2y);
	double s = dY12 * dX + dX21 * dY;
	double t = (p2y - p0y) * dX + (p0x - p2x) * dY;
	if (D < 0) return s <= 0 && t <= 0 && s + t >= D;
	return s >= 0 && t >= 0 && s + t <= D;
}

void interpolation(double lower) {
	size_t mosaicked_width = 3 * 150;

	std::filesystem::path L12("C:\\2021\\BaiGe\\GeoProcess\\L12.obj");
	std::filesystem::path model("C:\\2021\\BaiGe\\GeoProcess\\temp.obj");

	if (!std::filesystem::is_regular_file(L12)) {
		throw std::exception("Please check file path.");
	}

	if (!std::filesystem::is_regular_file(model)) {
		throw std::exception("Please check file path.");
	}

	Eigen::MatrixXd V_terrain, V_model;
	Eigen::MatrixXi F_terrain, F_model;

	/*
	igl::readOBJ(L12.string(), V_terrain, F_terrain);
	igl::readOBJ(model.string(), V_model, F_model);

	Eigen::write_binary("V_terrain.bin", V_terrain);
	Eigen::write_binary("V_model.bin", V_model);
	Eigen::write_binary("F_terrain.bin", F_terrain);
	Eigen::write_binary("F_model.bin", F_model);
	*/

	Eigen::read_binary("V_terrain.bin", V_terrain);
	Eigen::read_binary("V_model.bin", V_model);
	Eigen::read_binary("F_terrain.bin", F_terrain);
	Eigen::read_binary("F_model.bin", F_model);

	double model_max_lon = V_model(Eigen::all, 0).maxCoeff(), model_min_lon = V_model(Eigen::all, 0).minCoeff();
	double model_max_lat = V_model(Eigen::all, 1).maxCoeff(), model_min_lat = V_model(Eigen::all, 1).minCoeff();

	double number_bins = 20;
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
	for (size_t i = 0; i < num_vertices; i++) {
		size_t lon_key = hash_lon(V_model(i, 0));
		size_t lat_key = hash_lat(V_model(i, 1));
		vertices_in_bins.push_back({ lon_key,lat_key });
	}

	std::vector<std::vector<std::vector<size_t>>> all(number_bins, std::vector<std::vector<size_t>>(number_bins));
	size_t num_faces = F_model.rows();
	for (size_t i = 0; i < num_faces; i++) {
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

	size_t max_row = 0, max_col = 0, min_row = 10000, min_col = 10000;
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
			std::cout << std::format("{} isn't in any model faces. \n", i);
			continue;
		}
		else {
			std::cout << std::format("{} is located in {} faces, it is lowered from {} to {}. \n", i, count, V_terrain(i, 2), all_min_height - lower);
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

	std::cout << std::format("Modified extent in the merged DEM picture: \n row: {} {}\n col: {} {} \n", min_row, max_row, min_col, max_col) << std::endl;

	igl::writeOBJ("C:\\2021\\BaiGe\\GeoProcess\\L12_rectified.obj", V_terrain, F_terrain);

	return;
}


void regenerateDEMFiles(const std::string& loc, const std::string& modified, size_t layer, size_t x_start, size_t x_end
	, size_t y_start, size_t y_end) {
	std::filesystem::path targetDir(loc), modified_terrain(modified);

	if (!std::filesystem::is_regular_file(modified_terrain)) {
		throw std::exception("Check Input file");
	}
	if (!std::filesystem::is_directory(targetDir)) {
		throw std::exception("Check If the assigned output directories do exist.");
	}

	Eigen::MatrixXd V;
	Eigen::MatrixXi F;
	igl::readOBJ(modified_terrain.string(), V, F);

	std::vector<tmp> toStore;
	for (size_t i = y_start; i <= y_end; i++) {
		for (size_t j = x_start; j <= x_end; j++) {
			toStore.push_back(tmp(layer, j, i, DEM_TYPE(150, 150)));
		}
	}

	size_t count = 0;
	Eigen::Matrix<unsigned short, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> r((y_end - y_start + 1) * 150, (x_end - x_start + 1) * 150);
	for (int i = 0; i < r.rows(); i++) {
		for (int j = 0; j < r.cols(); j++) {
			r(i, j) = V(count, 2);
			count++;
		}
	}

	size_t row_start = 340, row_end = 370, col_start = 140, col_end = 190;
	for (int i = row_start; i < row_end; i++) {
		for (int j = col_start; j < col_end; j++) {
			std::vector<double> n;
			auto b = r.block(i - 1, j - 1, 3, 3);
			for (auto x : b.reshaped())
				n.push_back(x);
			std::sort(n.begin(), n.end());
			if (r(i, j) > n[6]) {
				double sum = 0;
				sum = std::accumulate(n.begin(), n.end(), sum);
				std::cout << std::format(" Pixel value at {} {} modified from {} to {}.\n", i, j, r(i, j), sum / 9);
				r(i, j) = sum / 9;
			}
		}
	}

	for (int i = row_start; i < row_end; i++) {
		for (int j = col_start; j < col_end; j++) {
			std::vector<double> n;
			auto b = r.block(i - 1, j - 1, 3, 3);
			for (auto x : b.reshaped())
				n.push_back(x);
			double sum = 0;
			sum = std::accumulate(n.begin(), n.end(), sum);
			r(i, j) = sum / 9;
		}
	}



	/*
	DEM_TYPE tmp = r;
	auto minimum = tmp.minCoeff(), maximum = tmp.maxCoeff();
	cv::Mat m;
	cv::eigen2cv(r, m);
	m.convertTo(m, CV_32FC1);
	std::cout << type2str(m.type()) << std::endl;
	m = (m - minimum) / (maximum - minimum);
	cv::imwrite("l12_Rectified.bmp", m * 256);
	cv::imshow("debug", m);
	cv::waitKey(0);
	*/

	for (auto& t : toStore) {
		t.d.block(0, 0, 150, 150) = r.block((t.y - y_start) * 150, (t.x - x_start) * 150, 150, 150);
		std::ofstream f(std::format("{}\\{}_{}_{}", targetDir.string(), t.layer, t.x, t.y), std::ios::binary | std::ios::out);
		f.write((char*)t.d.data(), 45000);
	}


	return;
}