#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <array>
#include <iostream>
#include <glm/glm.hpp>

class Pathline
{
public:
	static std::vector<float> loadFromDir(float segmentLength, glm::vec3 origin);
	static std::vector<float> loadFromFile(std::string filepath, float segmentLength, glm::vec3 origin);
	static std::vector<std::string> searchDirectory();


private:
	static std::vector<glm::vec3> makeEven(const std::vector<glm::vec3>& pathline, float segmentLength);
};

