#include "geoProc.h"

std::tuple<Eigen::MatrixXd, Eigen::MatrixXi> m_mergeObj(const std::string& path) {
	std::filesystem::path p(path);
	if (!std::filesystem::is_directory(p)) {
		std::cout << fmt::format("Directory file {} doesn't exist!", p.string());
	}

	std::vector<Eigen::MatrixXd> vertices;
	std::vector<Eigen::MatrixXi> faces;

	size_t debug_count = 0;
	for (auto& f : std::filesystem::directory_iterator(p)) {
		if (debug_count == 3)
			;
		auto file = f.path();
		if (file.extension() != ".obj" && file.extension() != ".OBJ") {
			continue;
		}
		debug_count++;
		auto [V, F] = m_readObj(file.string());
		vertices.push_back(std::move(V));
		faces.push_back(std::move(F));
	}

	size_t total_vertices_rows = 0, total_faces_row = 0;
	for (auto& m : vertices) {
		total_vertices_rows += m.rows();
	};
	for (auto& f : faces) {
		total_faces_row += f.rows();
	}
	Eigen::MatrixXd v_m(total_vertices_rows, 3);
	Eigen::MatrixXi f_m(total_faces_row, 3);

	size_t next_v = 0, next_f = 0;
	for(size_t i = 0; i < vertices.size(); i++){
		size_t r_v = vertices[i].rows(), r_f = faces[i].rows();
		v_m.block(next_v, 0, r_v, 3) = vertices[i];
		f_m.block(next_f, 0, r_f, 3) = (faces[i].array() += next_v, faces[i]);
		next_v += r_v;
		next_f += r_f;
	}

	return { v_m,f_m };
}
