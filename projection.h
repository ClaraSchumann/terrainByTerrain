#pragma once

#include <cmath>
#include <string>
#include <locale>
#include <codecvt>
#include <sstream>
#include <fstream>
#include <iostream>

#include <zlib.h>
#include <fmt/format.h>

extern const char* workDirectory;
extern const char* terrainRaw;

extern const char* targetModel;
extern double targetModel_origin_x;
extern double targetModel_origin_y;
extern double targetModel_origin_h;
extern int targetModel_origin_EPSG;

extern double degree2meter; // Need not be precise.

double getStartLon(int layer, int x);
double getStartLat(int layer, int y);

int getStartX(int layer, double lon);
int getStartY(int layer, double lat);

std::string UTF8ToString(const std::string&);

char* m_inflate(const std::string&,size_t *);

namespace Eigen {
    template<class Matrix>
    void write_binary(const char* filename, const Matrix& matrix) {
        std::ofstream out(filename, std::ios::out | std::ios::binary | std::ios::trunc);
        typename Matrix::Index rows = matrix.rows(), cols = matrix.cols();
        out.write((char*)(&rows), sizeof(typename Matrix::Index));
        out.write((char*)(&cols), sizeof(typename Matrix::Index));
        out.write((char*)matrix.data(), rows * cols * sizeof(typename Matrix::Scalar));
        out.close();
    }
    template<class Matrix>
    void read_binary(const char* filename, Matrix& matrix) {
        std::ifstream in(filename, std::ios::in | std::ios::binary);
        typename Matrix::Index rows = 0, cols = 0;
        in.read((char*)(&rows), sizeof(typename Matrix::Index));
        in.read((char*)(&cols), sizeof(typename Matrix::Index));
        matrix.resize(rows, cols);
        in.read((char*)matrix.data(), rows * cols * sizeof(typename Matrix::Scalar));
        in.close();
    }
};

inline constexpr auto hash_djb2a(const std::string_view sv) {
    unsigned long hash{ 5381 };
    for (unsigned char c : sv) {
        hash = ((hash << 5) + hash) ^ c;
    }
    return hash;
}

inline constexpr auto operator"" _sh(const char* str, size_t len) {
    return hash_djb2a(std::string_view{ str, len });
}