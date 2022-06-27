#include <tuple>
#include <filesystem>
#include <exception>
#include <regex>
#include <algorithm>
#include <thread>

#include <igl/writeOBJ.h>
#include <igl/readOBJ.h>
#include <igl/opengl/glfw/Viewer.h>

#include <opencv2/core/eigen.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>

#include <pcl/point_cloud.h>
#include <pcl/kdtree/kdtree_flann.h>

#include "projection.h"

#include "proj.h"

using DEM_TYPE = Eigen::Matrix<unsigned short, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
struct tmp {
	size_t layer;
	size_t x;
	size_t y;
	DEM_TYPE d;

	tmp(size_t l_, size_t x_, size_t y_, DEM_TYPE&& d_) : layer(l_), x(x_), y(y_), d(d_) {};
};

void geo_demo_1();

std::tuple<Eigen::MatrixXd, Eigen::MatrixXi> getTerrainVF();

void showMesh(const Eigen::MatrixXd&, const Eigen::MatrixXi&);

Eigen::MatrixXd VStack(const std::vector<Eigen::MatrixXd>& mat_vec);

void terrainObjToWGS84();

std::vector<tmp> getTerrainPartsAll(int layer);

std::vector<tmp> getTerrainPartsWithLimit(int layer, size_t x_start, size_t x_end, size_t y_start, size_t y_end);

std::tuple<Eigen::MatrixXd, Eigen::MatrixXi> mergeTerrainParts(const std::vector<tmp>&);

Eigen::MatrixXd getVertices(DEM_TYPE value,
	size_t x_start, size_t y_start, size_t layer
);

Eigen::MatrixXi getTriangleMesh(size_t rows, size_t cols);

void geo_demo_2(size_t);

std::tuple<Eigen::MatrixXd, Eigen::MatrixXi> geo_demo_3(size_t, size_t, size_t, size_t, size_t);

std::string type2str(int type);

bool ptInTriangle(pcl::PointXY p, pcl::PointXY p0, pcl::PointXY p1, pcl::PointXY p2);

void interpolation(double);

void regenerateDEMFiles(const std::string& loc, const std::string& modified, size_t layer, size_t x_start, size_t x_end
	, size_t y_start, size_t y_end);

std::tuple<Eigen::MatrixXd, Eigen::MatrixXi> m_readObj(const std::string& path);

std::tuple<Eigen::MatrixXd, Eigen::MatrixXi> m_mergeObj(const std::string& path);
