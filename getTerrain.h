#pragma once

#include <string>
#include <format>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <sstream>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <condition_variable>
#include <random>
#include <functional>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Exception.hpp>

#include "projection.h"


struct tile {
	int l;
	int x;
	int y;
	char status;
	tile(int l_, int x_, int y_) : status(0), l(l_), x(x_), y(y_) {};
	tile() : status(0), l(1), x(0), y(0) {};
	~tile() {};

	std::string ConvertToFilename();
	std::string ConvertToQueryParameter();
};

void downloadTerrainIn(double lon_start, double lon_end, double lat_start, double lat_end, int layer, const std::filesystem::path& targetDir, const std::string&);

std::queue<tile> generateTerrainSetIn(double lon_start, double lon_end, double lat_start, double lat_end, int layer);

void downloadTerrainSet(std::queue<tile>, const std::filesystem::path&, const std::string&);

void test();

