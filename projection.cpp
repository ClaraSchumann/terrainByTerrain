#include "projection.h"
#include <cmath>


double getStartLon(int layer, int x) {
	return 360.f / pow(2, layer) * x - 180.f;
};

double getStartLat(int layer, int y) {
	return 90.f - 360.f / pow(2, layer) * y;
};

int getStartX(int layer, double lon) {
	return (lon + 180.f) * pow(2, layer) / 360.f;
};

int getStartY(int layer, double lat) {
	return (90.f - lat) * pow(2, layer) / 360.f;
};

std::string UTF8ToString(const std::string& utf8Data) {
	std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
	std::wstring wString = conv.from_bytes(utf8Data);
	std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>> convert(new std::codecvt<wchar_t, char, std::mbstate_t>("CHS"));
	std::string str = convert.to_bytes(wString);

	return str;
}

char* m_inflate(const std::string& d, size_t* length) {
    const char* src = d.data();
    size_t sourse_size = d.size();

    size_t to_alloc = 200000;
    char * buffer  = new char [to_alloc];
    memset(buffer, 0, to_alloc);
    size_t target_size = to_alloc;

    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;

    infstream.avail_in = (uInt)sourse_size; // size of input
    infstream.next_in = (Bytef*)src; // input char array
    infstream.avail_out = (uInt)target_size; // size of output
    infstream.next_out = (Bytef*)buffer; // output char array

    // the actual DE-compression work.
    inflateInit(&infstream);
    inflate(&infstream, Z_NO_FLUSH);
    inflateEnd(&infstream);

    *length = infstream.total_out;

    /*
    std::stringstream ss;
    std::fstream f("check.bin", std::ios::binary | std::ios::out);
    f.write(buffer, infstream.total_out);
    f.close();
    */

    return buffer;
}