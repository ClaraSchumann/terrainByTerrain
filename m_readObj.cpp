#include "geoProc.h"
#include <memory>
#include <type_traits>
#include <mutex>

template<typename T>
Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> std_vector_to_Eigen(const std::vector<std::vector<T>>& src) {
	size_t rows = src.size();
	size_t cols = src[0].size();
	Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> ret(rows, cols);
	for (size_t i = 0; i < rows; i++) {
		for (size_t j = 0; j < cols; j++) {
			ret(i, j) = src[i][j];
		}
	}
	return ret;
};

// In case the forth parameter of the returned Eigen matrix in function 'std_vector_to_Eigen' be modified. 
template<typename T>
struct cvtfunc_get_type {
	using eigen_type = std::invoke_result<decltype(std_vector_to_Eigen<T>), std::vector<std::vector<T>>>::type;
	using vector_type = std::vector<eigen_type>;
};

std::tuple<Eigen::MatrixXd, Eigen::MatrixXi> m_readObj(const std::string& path) {
	std::filesystem::path p(path);
	if (!std::filesystem::is_regular_file(p)) {
		std::cout << fmt::format(".obj file {} doesn't exist!", p.string());
	}
	p = std::filesystem::absolute(p);
	std::cout << fmt::format("Reading .obj file {} \n", p.string());

	Eigen::MatrixXd V;
	Eigen::MatrixXi F;
	std::ifstream infile(p.string(), std::ios::in);
	infile.seekg(0, std::ios_base::end);
	size_t length = infile.tellg();
	infile.seekg(0, std::ios_base::beg);

	char* buffer = new char[length];
	infile.read(buffer, length);

	char c = buffer[length - 1];
	char* debug = buffer + length - 180;


	size_t num_threads = std::thread::hardware_concurrency();
	std::mutex mtx;
	if (length < 10000)
		num_threads = 1;

	// For debugging.
	//num_threads = 1;

	cvtfunc_get_type<int>::vector_type faces_gather(num_threads);
	cvtfunc_get_type<double>::vector_type vertices_gather(num_threads);

	// Under the assumption that each line ends up with a '\n', only 'v' and 'f' elements are parsed.
	std::vector<size_t> line_headers;
	size_t last_line_start = 0;
	for (size_t i = 0; i < length; i++) {
		if (buffer[i] == '\n') {
			line_headers.push_back(last_line_start);
			last_line_start = i + 1;
		}
	}

	std::vector<size_t> delimiters;
	for (size_t i = 0; i < num_threads; i++) {
		delimiters.push_back(line_headers.size() / num_threads * i);
	};
	delimiters.push_back(line_headers.size());

	auto worker = [&](size_t begin, size_t end, size_t thread_index) {
		thread_local std::vector local_line_headers(line_headers.begin() + begin, line_headers.begin() + end);
		thread_local std::vector<std::vector<double>> vertices_s;
		thread_local std::vector<std::vector<int>> faces_s;
		std::cout << fmt::format("Thread{}, from {} to {}, {} in total", thread_index, begin, end, end - begin);
		end -= begin;
		begin -= begin;
		for (auto i = begin; i < end; i++) {
			if ((i - begin) % 100000 == 0) {
				std::cout << fmt::format("Thread {} : {} / {}, {}%\n", thread_index, i, end, float(i) / end * 100) << std::flush;
			};
			char first_ch = buffer[local_line_headers[i]];
			switch (first_ch) {
			case 'v': {
				if (buffer[local_line_headers[i] + 1] != ' ' && buffer[local_line_headers[i] + 1] != '\t')
					continue;
				char* tmp = buffer + local_line_headers[i] + 2;
				double v1, v2, v3;
				v1 = std::strtod(tmp, &tmp);
				v2 = std::strtod(tmp, &tmp);
				v3 = std::strtod(tmp, &tmp);
				//sscanf_s(buffer + line_headers[i], "v %lf %lf %lf\n", &v1, &v2, &v3);
				vertices_s.push_back({ v1,v2,v3 });
				break;
			}
			case 'f': {
				if (buffer[local_line_headers[i] + 1] != ' ' && buffer[local_line_headers[i] + 1] != '\t')
					continue;
				char* tmp = buffer + local_line_headers[i] + 2;
				int v1, v2, v3;
				v1 = std::strtoll(tmp, &tmp, 10);
				while (*tmp != ' ' && *tmp != '\t') {
					tmp++;
				};
				v2 = std::strtoll(tmp, &tmp, 10);
				while (*tmp != ' ' && *tmp != '\t') {
					tmp++;
				};
				v3 = std::strtoll(tmp, &tmp, 10);
				//sscanf_s(buffer + line_headers[i], "f %llu %llu %llu\n", &v1, &v2, &v3);
				faces_s.push_back({ v1,v2,v3 });
				break;
			}
			default: {
				std::string tmp(buffer + local_line_headers[i], local_line_headers[i + 1] - local_line_headers[i]);
				std::cout << fmt::format("Invalid line {} in thread {}: {}", i, thread_index, tmp);
				break;
			}

			}
		}

		try {
			std::unique_lock<std::mutex> u_lock(mtx);
			// Some parts may contain no faces or vertices.
			if (faces_s.size()) {
				faces_gather[thread_index] = std::move(std_vector_to_Eigen(faces_s));
			}
			if (vertices_s.size()) {
				vertices_gather[thread_index] = std::move(std_vector_to_Eigen(vertices_s));
			}
		}
		catch(std::exception e){
			std::cout << e.what();
		}
	};

	std::vector<std::thread> threads;
	for (size_t i = 0; i < num_threads; i++) {
		threads.push_back(std::thread(worker, delimiters[i], delimiters[i + 1],i));
	}

	for (auto& t : threads) {
		t.join();
	}



	return {};
}