#pragma once

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include <core/Animation.h>
#include <gen/LocalMin.h>
#include <gen/Graph.h>
#include <gen/Distance.h>

class Pipeline
{
private:
	static void saveDistanceToFile(std::vector<std::vector<float>> distance, std::string filename);
	static std::vector<std::vector<float>> loadDistanceFromFile(std::string filename);

public:
	static Graph genGraph(const int WINDOW_SIZE, const int THRESHOLD, const int STEP_SIZE, const std::string graphdir);

};

