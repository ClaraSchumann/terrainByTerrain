#include "geoProc.h"
#include <fmt/format.h>

auto transformationToWgs84(Eigen::MatrixXd V, const ObjProfile& p, double scale) -> decltype(V){
	size_t rows = V.rows();
	std::cout << "Rows : " << rows << std::endl;
	size_t num_threads = std::thread::hardware_concurrency();
	size_t span = rows / num_threads;

	std::vector<Eigen::MatrixXd> results(num_threads);
	std::mutex t_m;

	std::string source_EPSG = fmt::format("EPSG:{}", p.EPSG);
	std::cout << "Coordinates Transfrom Start\n";
	auto worker = [&](int idx, int start, int end) {
		std::cout << fmt::format("Threads {}, begin {} , end {}.\n", idx, start, end);
		int local_rows = end - start;
		Eigen::MatrixXd l(local_rows, 3);

		PJ_CONTEXT* C;
		PJ* P;
		PJ_COORD a, b;

		C = proj_context_create();
		P = proj_create_crs_to_crs(C, source_EPSG.c_str(), "EPSG:4326", NULL);

		for (size_t i = start; i < end; i++) {
			if ((i - start) % 300000 == 0) {
				std::cout << fmt::format("Thread {} : {} / {}, {}%\n", idx, i, end, float(i) / end * 100) << std::flush;
			};
			a = proj_coord(p.origin_x + V(i, 0), p.origin_y + V(i, 1), p.origin_h + V(i, 2), 0);
			b = proj_trans(P, PJ_FWD, a);
			l(i - start, 0) = b.enu.n * scale; // The n for NORTH here is actually longitude.
			l(i - start, 1) = b.enu.e * scale;
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

	return V;
}
