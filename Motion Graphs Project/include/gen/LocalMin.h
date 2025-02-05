#pragma once

#include <vector>
#include <tuple>
#include <iostream>

class LocalMin
{
public:
	static std::vector<std::tuple<int, int>> localMinima(std::vector<std::vector<float>> distance_2d, int threshold, int STEP_SIZE);
};

