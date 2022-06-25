#include "getTerrain.h"

// Initialize curlpp
curlpp::Cleanup curlpp_cleanup;

// Token embedded
std::string url("https://t{}.tianditu.gov.cn/mapservice/swdx?tk=5b14f0b6df5520545f0851e418afa219&x={}&y={}&l={}");
// std::string url("https://t{}.tianditu.gov.cn/mapservice/swdx?tk=530acfb193787c3172d42c8cbcac185a&x={}&y={}&l={}");
// Number of usable mirrors, will be used to replace the first curly brace in the url above.
constexpr int NUM_THREADS = 8;


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

void downloadTerrainSet(std::queue<tile> toDownload, const std::filesystem::path& targetDir,const std::string& user_token) {
	if (!std::filesystem::is_directory(targetDir)) {
		std::filesystem::create_directory(targetDir);
	}

	std::queue<std::string> hosts;
	for (int i = 0; i < 8; i++) {
		hosts.push(std::format("https://t{}.tianditu.gov.cn", i));
	};

	std::mutex m_mutex; // Mutex for task queue.
	bool notified = false;

	std::uniform_int_distribution<int> dice_distribution(1, 500);
	std::mt19937 random_number_engine; // pseudorandom number generator
	auto dice_roller = std::bind(dice_distribution, random_number_engine);

	auto worker = [&](int idx) {
		cURLpp::Easy request;

		// TODO: Generate template headers automatically.
		std::list<std::string> headers;
		headers.push_back("Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9");
		headers.push_back("Accept-Encoding: gzip, deflate, br");
		headers.push_back("Accept-Language: zh-CN,zh;q=0.9");
		headers.push_back("Cache-Control: no-cache");
		headers.push_back("Pragma: no-cache");
		headers.push_back("Connection: keep-alive");
		//headers.push_back("Origin: http://localhost:8080");
		//headers.push_back("Referer: http://localhost:8080/");
		headers.push_back("sec-ch-ua: \" Not A; Brand\";v=\"99\", \"Chromium\";v=\"102\", \"Google Chrome\";v=\"102\"");
		headers.push_back("sec-ch-ua-mobile: ?0");
		headers.push_back("sec-ch-ua-platform: \"Windows\"");
		headers.push_back("Sec-Fetch-Dest: document");
		headers.push_back("Sec-Fetch-Mode: navigate");
		headers.push_back("Sec-Fetch-User: ?1");
		headers.push_back("Upgrade-Insecure-Requests: 1");
		headers.push_back("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/102.0.0.0 Safari/537.36");
		request.setOpt(cURLpp::Options::HttpHeader(headers));

		while (true) {
			std::unique_lock<std::mutex> m_ulock(m_mutex);
			/*
			* Active threads will not be fewer than remaining tasks.
			*/ 
			if (toDownload.empty() || hosts.empty())
				return;

			auto tile_to_download = toDownload.front();
			auto host = hosts.front();
			toDownload.pop();
			hosts.pop();
			m_ulock.unlock();

			auto loc_headers = headers;
			loc_headers.push_back("Host: " + host);

			std::string target = std::format("/mapservice/swdx?tk={}&x={}&y={}&l={}",user_token, tile_to_download.x, tile_to_download.y, tile_to_download.l);
			target = host + target;
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

			size_t remaining_repest_times = 3;
			bool s_flag = false;
			while (remaining_repest_times && !s_flag) {
				headers = {};
				request.perform();
				int return_code = std::stoi(headers[0].substr(9, 3));
				if (return_code != 200) {
					switch (return_code) {
					case 418:
						std::cout << std::format("Thread {} failed to access target url with return_code 418.\n\t{} blocked this address. \n", idx, host);
						remaining_repest_times = 0;
						continue;
					case 403:
						std::cout << std::format("Thread {} failed to access target url with return_code 403.\n\t{} refused the credentials. \n", idx, host);
						remaining_repest_times = 0;
						continue;
					default:
						std::cout << std::format("Thread {} failed to access target url with return_code {}. Retry.\n\t", idx,return_code, host);
						break;
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(5000));
					remaining_repest_times--;
					std::cout << std::format("Thread {} failed to access target url, \n\t{} times remained. \n", idx, remaining_repest_times);
				}
				else {
					std::cout << std::format("Thread {} access target url successfully. \n\t{}\n", idx, target);
					s_flag = true;
				}
			};

			if (!s_flag) {
				std::cout << std::format("Thread {} failed to access target url, try time exhausted, skip. \n\turl : {} \n", 6, remaining_repest_times,target);
				m_ulock.lock();
				toDownload.push(tile_to_download);
				m_ulock.unlock();
				continue;
			}
			else {
				m_ulock.lock();
				hosts.push(host);
				m_ulock.unlock();
			}


			bool got_dem = headers.size() >= 3 && headers[3].size() == 40 && headers[3].substr(14, 11) == "image/jpeg;";
			if (!got_dem) {
				std::cout << std::format("Thread {} target url contains no DEM\n\turl : {} \n", idx + 1, target);
				continue;
			}

			size_t deflate_return = 0;
			char* buf = m_inflate(response.str(), &deflate_return);

			std::stringstream ss;
			std::string f_loc = targetDir.string() + std::string("/") + tile_to_download.ConvertToFilename();
			std::fstream f(f_loc, std::ios::binary | std::ios::out);
			f.write(buf,deflate_return);
			f.close();

			delete[] buf;

			std::cout << std::format("Thread {} preserving DEM to\n\tpath: {} \n", idx, f_loc);
			std::this_thread::sleep_for(std::chrono::milliseconds(500 + dice_roller()));

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

void downloadTerrainIn(double lon_start, double lon_end, double lat_start, double lat_end, int layer, const std::filesystem::path& targetDir, const std::string& token) {
	std::queue<tile> toDownload = generateTerrainSetIn(lon_start, lon_end, lat_start, lat_end, layer);

	downloadTerrainSet(toDownload, targetDir,token);
}

void test() {
	auto lat = getStartLat(12, 576);
	auto lon = getStartLon(12, 3377);
	tile t(12, 3377, 576);

	//downloadTerrainSet(std::queue<tile>({ t }));
}