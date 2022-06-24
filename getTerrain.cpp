#include "getTerrain.h"

// Initialize curlpp
curlpp::Cleanup curlpp_cleanup;

// Token embedded
std::string url("https://t{}.tianditu.gov.cn/mapservice/swdx?tk=5b14f0b6df5520545f0851e418afa219&x={}&y={}&l={}");
// std::string url("https://t{}.tianditu.gov.cn/mapservice/swdx?tk=530acfb193787c3172d42c8cbcac185a&x={}&y={}&l={}");
// Number of usable mirrors, will be used to replace the first curly brace in the url above.
constexpr int NUM_THREADS = 8;

std::mutex m_mutex;

std::string tile::ConvertToFilename() {
	return std::format("{}_{}_{}", l, x, y);
}

std::string tile::ConvertToQueryParameter() {
	return std::format("&x={}&y={}&l={}", x, y, l);
}

std::queue<tile> generateTerrainSetIn(double lon_start, double lon_end, double lat_start, double lat_end, int layer) {
	std::queue<tile> toDownload;
	int x_start = getStartX(layer, lon_start);
	int x_end = getStartX(layer, lon_end);
	int y_start = getStartY(layer, lat_start);
	int y_end = getStartY(layer, lat_end);
	if (x_start > x_end) {
		std::swap(x_start, x_end);
	}
	if (y_start > y_end) {
		std::swap(y_start, y_end);
	}

	for (int i = x_start; i <= x_end; i++) {
		for (int j = y_start; j <= y_end; j++) {
			toDownload.push(tile{ layer,i,j });
		}
	}

	return toDownload;
}

void downloadTerrainSet(std::queue<tile> toDownload) {

	auto worker = [&](int idx) {
		cURLpp::Easy request;
		std::list<std::string> headers;
		headers.push_back("Accept: */*");
		headers.push_back("Accept-Encoding: gzip, deflate, br");
		headers.push_back("Accept-Language: zh-CN,zh;q=0.9");
		headers.push_back("Cache-Control: no-cache");
		headers.push_back("Connection: keep-alive");
		//headers.push_back("Host: t5.tianditu.gov.cn");
		//headers.push_back("Origin: http://localhost:8080");
		//headers.push_back("Pragma: no-cache");
		//headers.push_back("Referer: http://localhost:8080/");
		headers.push_back("sec-ch-ua: \" Not A; Brand\";v=\"99\", \"Chromium\";v=\"102\", \"Google Chrome\";v=\"102\"");
		headers.push_back("sec-ch-ua-mobile: ?0");
		headers.push_back("sec-ch-ua-platform: \"Windows\"");
		headers.push_back("Sec-Fetch-Dest: empty");
		headers.push_back("Sec-Fetch-Mode: cors");
		headers.push_back("Sec-Fetch-Site: cross-site");
		headers.push_back("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/102.0.0.0 Safari/537.36");
		request.setOpt(cURLpp::Options::HttpHeader(headers));

		while (true) {
			std::unique_lock<std::mutex> m_ulock(m_mutex);
			if (toDownload.empty())
				return;

			auto t = toDownload.front();
			toDownload.pop();
			m_ulock.unlock();

			std::string target = std::format("https://t{}.tianditu.gov.cn/mapservice/swdx?tk=5b14f0b6df5520545f0851e418afa219&x={}&y={}&l={}", idx+1, t.x, t.y, t.l);
			std::cout << std::format("Thread {} is retrieving file from : \n\t{} \n", idx + 1, target);

			std::vector<std::string> headers;
			auto HeaderCallback = [&](char* ptr, size_t size, size_t nmemb)->size_t
			{
				int totalSize = size * nmemb;
				headers.push_back(std::string(ptr, totalSize));
				return totalSize;
			};
			request.setOpt(curlpp::options::HeaderFunction(HeaderCallback));
			request.setOpt(cURLpp::Options::Url(target));
			std::stringstream response;
			request.setOpt(new curlpp::options::WriteStream(&response));

			/*
			auto* pOpt = new curlpp::Options::HttpHeader;
			request.getOpt(pOpt);
			std::cout << pOpt->getValue() << std::endl;
			*/

			size_t max_repeat_times = 10;
			bool s_flag = false;
			while (max_repeat_times && !s_flag) {
				headers = {};
				request.perform();
				int return_code = std::stoi(headers[0].substr(9, 3));
				if (return_code != 200) {
					std::this_thread::sleep_for(std::chrono::milliseconds(5000));
					max_repeat_times--;
					std::cout << std::format("Thread {} failed to access target url, \n\t{} times remained. \n", idx + 1, max_repeat_times);
				}
				else {
					std::cout << std::format("Thread {} access target url successfully. \n\t{}\n", idx + 1, target);
					s_flag = true;
				}
			};

			if (!s_flag) {
				std::cout << std::format("Thread {} failed to access target url, try time exhausted, skip. \n\turl : {} \n", 6, max_repeat_times,target);
				continue;
			}

			bool got_dem = headers.size() >= 3 && headers[3].size() == 40 && headers[3].substr(14, 11) == "image/jpeg;";
			if (!got_dem) {
				std::cout << std::format("Thread {} target url contains no DEM\n\turl : {} \n", idx + 1, max_repeat_times, target);
				continue;
			}

			size_t deflate_return = 0;
			char* buf = m_inflate(response.str(), &deflate_return);

			std::stringstream ss;
			std::string f_loc = terrainRaw + std::string("/") + t.ConvertToFilename();
			std::fstream f(f_loc, std::ios::binary | std::ios::out);
			f.write(buf,deflate_return);
			f.close();

			delete[] buf;

			std::cout << std::format("Thread {} preserving DEM to\n\tpath: {} \n", idx + 1, f_loc);
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));

			//m_inflate(response.str());

			//auto p = UTF8ToString(response.str());
			//std::cout << p << std::endl;

			/*
			std::ofstream fout;
			fout.open("try.txt", std::ios::out | std::ios::app);
			fout << os.str() << std::endl;
			fout.close();
			*/
		}
	};

	std::vector<std::thread> ts;
	for (int i = 0; i < NUM_THREADS; i++) {
		ts.push_back(std::thread(worker, i));
	}

	for (auto& t : ts) {
		t.join();
	}

	return;

}

void downloadTerrainIn(double lon_start, double lon_end, double lat_start, double lat_end, int layer) {
	std::queue<tile> toDownload = generateTerrainSetIn(lon_start, lon_end, lat_start, lat_end, layer);

	downloadTerrainSet(toDownload);
}

void test() {
	auto lat = getStartLat(12, 576);
	auto lon = getStartLon(12, 3377);
	tile t(12, 3377, 576);

	downloadTerrainSet(std::queue<tile>({ t }));
}