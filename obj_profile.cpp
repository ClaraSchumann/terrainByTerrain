#include "obj_profile.h"

ObjProfile getObjProfile(const std::filesystem::path& p) {
	if (!std::filesystem::is_regular_file(p)) {
		throw std::exception("metadata.xml does not exist.");
	}

	std::ifstream f(p.string(), std::ios::in);
	std::stringstream buffer;
	buffer << f.rdbuf();
	auto xml_string = buffer.str();

	std::cout << xml_string;

	using namespace rapidxml;
	xml_document<> doc;

	doc.parse<0>(const_cast<char*>(xml_string.c_str()));

	auto node = doc.first_node()->first_node();
	auto SRS_string = std::string(node->value());
	auto SRSOrigin = std::string(node->next_sibling()->value());

	std::regex r("EPSG:(.*)");
	std::smatch matches;
	std::regex_search(SRS_string, matches, r);
	size_t EPSG_idx = std::stoull(matches.str(1));

	r = std::regex("(.*),(.*),(.*)");
	std::regex_search(SRSOrigin, matches, r);
	double x = std::stod(matches.str(1));
	double y = std::stod(matches.str(2));
	double h = std::stod(matches.str(3));

	return ObjProfile(EPSG_idx, x, y, h);
}