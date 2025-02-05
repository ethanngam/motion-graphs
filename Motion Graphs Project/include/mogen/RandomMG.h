#pragma once

#include <mogen/MotionGenerator.h>

class RandomMG : public MotionGenerator
{

public:
	RandomMG(Graph* _graph) : MotionGenerator(_graph) {}
	std::vector<float> getNextFrame(bool* completed = NULL);
	glm::vec4 getColour();
};

