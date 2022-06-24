#include "geoProc.h"

void geo_demo_1() {
	Eigen::MatrixXd V;
	Eigen::MatrixXi F;

	// Load a mesh in OFF format
	igl::readOBJ("C:\\2021\\BaiGe\\GeoProcess\\minObj\\texture.obj", V, F);

	// Plot the mesh
	igl::opengl::glfw::Viewer viewer;
	viewer.data().set_mesh(V, F);
	viewer.launch();
}

void showMesh(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F) {
	// Plot the mesh
	igl::opengl::glfw::Viewer viewer;
	viewer.data().set_mesh(V, F);
	viewer.launch();

}

Eigen::MatrixXd VStack(const std::vector<Eigen::MatrixXd>& mat_vec) {
	assert(!mat_vec.empty());
	long num_cols = mat_vec[0].cols();
	size_t num_rows = 0;
	for (size_t mat_idx = 0; mat_idx < mat_vec.size(); ++mat_idx) {
		assert(mat_vec[mat_idx].cols() == num_cols);
		num_rows += mat_vec[mat_idx].rows();
	}
	Eigen::MatrixXd vstacked_mat(num_rows, num_cols);
	size_t row_offset = 0;
	for (size_t mat_idx = 0; mat_idx < mat_vec.size(); ++mat_idx) {
		long cur_rows = mat_vec[mat_idx].rows();
		vstacked_mat.middleRows(row_offset, cur_rows) = mat_vec[mat_idx];
		row_offset += cur_rows;
	}
	return vstacked_mat;
}

std::tuple<Eigen::MatrixXd, Eigen::MatrixXi> getTerrainVF() {
	Eigen::MatrixXd V;
	Eigen::MatrixXi F;

	igl::readOBJ("C:\\2021\\BaiGe\\GeoProcess\\minObj\\texture.obj", V, F);

	// It should be READONLY in multi-thread context.
	size_t rows = V.rows();
	std::cout << "Rows : " << rows << std::endl;
	size_t num_threads = 12;
	size_t span = rows / num_threads;

	std::vector<Eigen::MatrixXd> results(num_threads);
	std::mutex t_m;

	std::cout << "Coordinates Transfrom Start\n";
	auto worker = [&](int idx, int start, int end) {
		std::cout << std::format("Threads {}, begin {} , end {}.\n", idx, start, end);
		int local_rows = end - start;
		Eigen::MatrixXd l(local_rows, 3);

		PJ_CONTEXT* C;
		PJ* P;
		PJ_COORD a, b;

		C = proj_context_create();
		P = proj_create_crs_to_crs(C, "EPSG:32647", "EPSG:4326", NULL);

		for (size_t i = start; i < end; i++) {
			a = proj_coord(targetModel_origin_x + V(i, 0), targetModel_origin_y + V(i, 1), targetModel_origin_h + V(i, 2), 0);
			b = proj_trans(P, PJ_FWD, a);
			l(i - start, 0) = b.enu.n * 111000; // The n for NORTH here is actually longitude.
			l(i - start, 1) = b.enu.e * 111000;
			l(i - start, 2) = b.enu.u;
		}

		std::unique_lock<std::mutex> u(t_m);
		results[idx] = l;
		u.unlock();

		proj_destroy(P);
		proj_context_destroy(C);
	};

	std::vector<std::thread> ts;
	size_t last = 0;
	for (int i = 0; i < num_threads - 1; i++) {
		ts.push_back(std::thread(worker, i, last, last + span));
		last += span;
	}
	ts.push_back(std::thread(worker, num_threads - 1, last, rows));

	for (auto& t : ts) {
		t.join();
	}

	V = VStack(results);

	return { V,F };
}

void terrainObjToWGS84() {
	auto [V, F] = getTerrainVF();

	std::cout << V(1, 0) << '\t' << V(1, 1) << '\t' << V(1, 2);

	//showMesh(V, F);

	igl::writeOBJ("temp.obj", V, F);

	return;
}

Eigen::MatrixXi getTriangleMesh(size_t rows, size_t cols) {
	size_t num_face = (rows - 1) * (cols - 1) * 2;
	Eigen::MatrixXi ret(num_face, 3);
	size_t count = 0;
	for (int i = 0; i < rows - 1; i++) {
		for (int j = 0; j < cols - 1; j++) {
			ret(count, 0) = i * cols + j;
			ret(count, 1) = i * cols + j + 1;
			ret(count, 2) = (i + 1) * cols + j;
			count++;
			ret(count, 0) = i * cols + j + 1;
			ret(count, 1) = (i + 1) * cols + j + 1;
			ret(count, 2) = (i + 1) * cols + j;
			count++;
		}
	}

	return ret;
}

Eigen::MatrixXd getVertices(DEM_TYPE value,
	size_t x_start, size_t y_start, size_t layer
) {
	size_t rows = value.rows(), cols = value.cols();
	Eigen::MatrixXd V(rows * cols, 3);

	double start_lon = getStartLon(layer, x_start), start_lat = getStartLat(layer, y_start);
	double seg = pow(2, layer);
	size_t count = 0;
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			V(count, 1) = (start_lat - 360.f / seg * i / 150.) * degree2meter;
			V(count, 0) = (start_lon + 360.f / seg * j / 150.) * degree2meter;
			V(count, 2) = value(i, j);
			count++;
		}
	};

	return V;
}

std::vector<tmp> getTerrainPartsAll(int layer) {
	std::filesystem::path terrainDir(terrainRaw);
	if (!std::filesystem::is_directory(terrainDir)) {
		throw std::exception("TerrainDir is not a valid path.");
	}

	std::vector<tmp> DEM_parts;
	std::regex re("([0-9]+)_([0-9]+)_([0-9]+)");
	for (const auto& entry : std::filesystem::directory_iterator(terrainDir)) {
		auto f = entry.path().filename().string();

		std::smatch matches;
		if (std::regex_search(f, matches, re)) {
			size_t l_layer = std::stoi(matches[1].str());
			if (layer != l_layer)
				continue;

			// Get filesize
			std::ifstream f(entry.path(), std::ios::binary | std::ios::in);
			size_t temp = f.tellg();
			f.seekg(0, std::ios_base::end);
			size_t length = f.tellg();
			f.seekg(temp);

			assert(length == 45000);
			DEM_TYPE DEM_part(150, 150);
			f.read((char*)DEM_part.data(), 45000);

			DEM_parts.push_back(tmp(std::stoi(matches[1].str()), std::stoi(matches[2].str()), std::stoi(matches[3].str()), std::move(DEM_part)));
		}
		else {
			continue;
		}
	}

	std::sort(DEM_parts.begin(), DEM_parts.end(), [](const tmp& l, const tmp& r) -> bool {
		if (l.y != r.y) {
			return l.y < r.y;
		}

		return l.x < r.x;
		});

	return DEM_parts;
};

std::vector<tmp> getTerrainPartsWithLimit(int layer, size_t x_start, size_t x_end, size_t y_start, size_t y_end) {
	std::filesystem::path terrainDir(terrainRaw);
	if (!std::filesystem::is_directory(terrainDir)) {
		throw std::exception("TerrainDir is not a valid path.");
	}

	std::vector<tmp> DEM_parts;
	std::regex re("([0-9]+)_([0-9]+)_([0-9]+)");
	for (const auto& entry : std::filesystem::directory_iterator(terrainDir)) {
		auto f = entry.path().filename().string();

		std::smatch matches;
		if (std::regex_search(f, matches, re)) {
			size_t l_layer = std::stoi(matches[1].str()), x = std::stoi(matches[2].str()), y = std::stoi(matches[3].str());
			if (layer != l_layer)
				continue;
			if (x < x_start || x > x_end)
				continue;
			if (y<y_start || y > y_end)
				continue;

			// Get filesize
			std::ifstream f(entry.path(), std::ios::binary | std::ios::in);
			size_t temp = f.tellg();
			f.seekg(0, std::ios_base::end);
			size_t length = f.tellg();
			f.seekg(temp);

			assert(length == 45000);
			DEM_TYPE DEM_part(150, 150);
			f.read((char*)DEM_part.data(), 45000);

			DEM_parts.push_back(tmp(std::stoi(matches[1].str()), std::stoi(matches[2].str()), std::stoi(matches[3].str()), std::move(DEM_part)));
		}
		else {
			continue;
		}
	}

	std::sort(DEM_parts.begin(), DEM_parts.end(), [](const tmp& l, const tmp& r) -> bool {
		if (l.y != r.y) {
			return l.y < r.y;
		}

		return l.x < r.x;
		});

	return DEM_parts;

}

std::string type2str(int type) {
	std::string r;

	uchar depth = type & CV_MAT_DEPTH_MASK;
	uchar chans = 1 + (type >> CV_CN_SHIFT);

	switch (depth) {
	case CV_8U:  r = "8U"; break;
	case CV_8S:  r = "8S"; break;
	case CV_16U: r = "16U"; break;
	case CV_16S: r = "16S"; break;
	case CV_32S: r = "32S"; break;
	case CV_32F: r = "32F"; break;
	case CV_64F: r = "64F"; break;
	default:     r = "User"; break;
	}

	r += "C";
	r += (chans + '0');

	return r;
} 

std::tuple<Eigen::MatrixXd, Eigen::MatrixXi> mergeTerrainParts(const std::vector<tmp>& DEM_parts) {
	size_t x_start = DEM_parts[0].x, y_start = DEM_parts[0].y;
	size_t x_end = DEM_parts.back().x, y_end = DEM_parts.back().y;
	size_t span_x = x_end - x_start + 1;
	size_t span_y = y_end - y_start + 1;

	DEM_TYPE all(150 * span_y, 150 * span_x);
	for (auto& t : DEM_parts) {
		all.block((t.y - y_start) * 150, (t.x - x_start) * 150, 150, 150) = t.d.block(0, 0, 150, 150);
	};

	/* Check mosaicked DEM
	DEM_TYPE tmp = all;
	auto minimum = tmp.minCoeff(), maximum = tmp.maxCoeff();
	cv::Mat m;
	cv::eigen2cv(all, m);
	m.convertTo(m, CV_32FC1);
	std::cout << type2str(m.type()) << std::endl;
	m = (m - minimum) / (maximum - minimum);
	//cv::imwrite("l12.bmp", m * 256);
	cv::imshow("debug", m);
	cv::waitKey(0);
	*/

	auto F = getTriangleMesh(150 * span_y, 150 * span_x);
	auto V = getVertices(all, DEM_parts[0].x, DEM_parts[0].y, DEM_parts[0].layer);

	return { V,F };

}

void geo_demo_2(size_t layer) {
	auto p = getTerrainPartsAll(layer);
	mergeTerrainParts(p);

	return;
}

std::tuple<Eigen::MatrixXd, Eigen::MatrixXi> geo_demo_3(size_t layer, size_t x_start, size_t x_end, size_t y_start, size_t y_end) {
	auto p = getTerrainPartsWithLimit(layer, x_start, x_end, y_start, y_end);
	auto [V, F] = mergeTerrainParts(p);

	igl::writeOBJ("L12.obj", V, F);

	return { V,F };
}

bool ptInTriangle(pcl::PointXY p, pcl::PointXY p0, pcl::PointXY p1, pcl::PointXY p2) {
	double dX = p.x - p2.x;
	double dY = p.y - p2.y;
	double dX21 = p2.x - p1.x;
	double dY12 = p1.y - p2.y;
	double D = dY12 * (p0.x - p2.x) + dX21 * (p0.y - p2.y);
	double s = dY12 * dX + dX21 * dY;
	double t = (p2.y - p0.y) * dX + (p0.x - p2.x) * dY;
	if (D < 0) return s <= 0 && t <= 0 && s + t >= D;
	return s >= 0 && t >= 0 && s + t <= D;
}

void interpolation_old() {
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
	igl::readOBJ(L12.string(), V_terrain, F_terrain);
	igl::readOBJ(model.string(), V_model, F_model);

	double model_max_lon = V_model(Eigen::all, 0).maxCoeff(), model_min_lon = V_model(Eigen::all, 0).minCoeff();
	double model_max_lat = V_model(Eigen::all, 1).maxCoeff(), model_min_lat = V_model(Eigen::all, 1).minCoeff();

	pcl::PointCloud<pcl::PointXY>::Ptr cloud(new pcl::PointCloud<pcl::PointXY>);
	cloud->width = V_model.rows();
	cloud->height = 1;
	cloud->points.resize(cloud->width * cloud->height);
	for (int i = 0; i < V_model.rows(); i++) {
		(*cloud)[i].x = V_model(i, 0);
		(*cloud)[i].y = V_model(i, 1);
	}

	pcl::KdTreeFLANN<pcl::PointXY> kdtree;
	kdtree.setInputCloud(cloud);

	for (int i = 0; i < V_terrain.rows(); i++) {
		double lon = V_terrain(i, 0), lat = V_terrain(i, 1);
		if (lon < model_min_lon || lon > model_max_lon)
			continue;
		if (lat < model_min_lat || lat > model_max_lat)
			continue;

		pcl::PointXY searchPoint;
		searchPoint.x = lon;
		searchPoint.y = lat;
		std::vector<int> pointIdxKNNSearch(3);
		std::vector<float> pointKNNSquaredDistance(3);
		if (kdtree.nearestKSearch(searchPoint, 3, pointIdxKNNSearch, pointKNNSquaredDistance) > 0)
		{
			bool inTriangle = ptInTriangle(searchPoint, (*cloud)[pointIdxKNNSearch[0]], (*cloud)[pointIdxKNNSearch[1]], (*cloud)[pointIdxKNNSearch[2]]);
			inTriangle = true;
			if (!inTriangle) {
				std::cout << std::format("Point {} is in the rectangular bounding box but not on the model. \n", i);
				continue;
			}

			double lower = 500;
			double model_min = std::min(V_model(pointIdxKNNSearch[0], 2), std::min(V_model(pointIdxKNNSearch[1], 2), V_model(pointIdxKNNSearch[2], 2)));
			double tmp = V_terrain(i, 2);
			V_terrain(i, 2) = model_min - lower;
			std::cout << std::format("The height of point {} has been rectified from {} to {}. \n", i, tmp, model_min - 2);
			std::cout << std::format("Target point, Latitude : {}, Lontitude:{} \n", V_terrain(i, 1) / degree2meter, V_terrain(i, 0) / degree2meter);
		}
	}

	igl::writeOBJ("C:\\2021\\BaiGe\\GeoProcess\\L12_rectified.obj", V_terrain, F_terrain);

	return;
}
