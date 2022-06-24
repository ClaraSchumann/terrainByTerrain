

#include <iostream>


#include "projection.h"
#include "getTerrain.h"
#include "geoProc.h"

extern const char* workDirectory = "C:\\2021\\BaiGe\\GeoProcess\\";
extern const char* terrainRaw = "C:\\2021\\BaiGe\\GeoProcess\\TerrainRaw\\";

extern const char* targetModel = "C:\\2021\\BaiGe\\GeoProcess\\minObj\\texture.obj";
extern double targetModel_origin_x = 471928.19247236708;
extern double targetModel_origin_y = 3439059.3636666504;
extern double targetModel_origin_h = 3408.5199999989136;
extern int targetModel_origin_EPSG = 32647;

extern double degree2meter = 111000; // Need not be precise.

void snippets() {
    // Download DEM.
    double margin = 1;
    double lon_cen = 98.7026;
    double lat_cen = 31.0810;
    int layer_lowest = 7, layer_highest = 12;
    for (int layer = layer_lowest; layer <= layer_highest; layer++) {
        downloadTerrainIn(lon_cen - margin, lon_cen + margin, lat_cen - margin, lat_cen + margin, layer);
    };

    // Test download DEM. 
    test();

    // Transform terrain object to WGS84, with the lon/lats scaled up by 111000 to make is prettier.
    void terrainObjToWGS84();

    // Generate mosaicked DEM in .obj format.
    auto [V, F] = geo_demo_3(12, 3170, 3172, 668, 672);
}

int main()
{
    // 12 3170 - 3172 668 - 672 在图L12中间偏左位置
    // 11 1584 - 1586 334 - 336 
    //auto [V,F] = geo_demo_3(11, 1584, 1586, 334 , 336);

    interpolation(100);

    // Extend: 
    // row: 341 369
    // col: 142 181

    regenerateDEMFiles("C:\\2021\\BaiGe\\GeoProcess\\TerrainModified", "C:\\2021\\BaiGe\\GeoProcess\\L12_rectified.obj", 12, 3170, 3172, 668, 672);

    return 0;
}
