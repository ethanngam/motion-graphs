#pragma once

#include <gen/Graph.h>

#include <vector>
#include <glm/glm.hpp>

class MotionGenerator
{
public:
	struct Buffer {
		unsigned int VAO = -1;
		unsigned int VBO = -1;
		unsigned int glType = -1;
		glm::vec4 colour;
	};

	MotionGenerator(Graph* _graph) {
		graph = _graph;
		currFrameID = graph->getFirstNode();
		currFrame = Animation::Frame(graph->getFrameVec(currFrameID).pose);
		currFrame.pos[3][1] = graph->getRealPos(currFrameID)[3][1];
		currPos = currFrame.pos;
		currRot = currFrame.pose["root"];
	}

	virtual glm::vec4 getColour() {
		return glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	virtual glm::vec3 getRoot() {
		return glm::vec3(currFrame.pos[3]);
	}

	virtual void setPath(const std::vector<float>& line) { /* Do Nothing */ };

	virtual void getDrawBuffers() { /* Do Nothing */ };

	virtual void reset() {
		currFrameID = graph->getFirstNode();
		currFrame = Animation::Frame(graph->getFrameVec(currFrameID).pose);
		currFrame.pos[3][1] = graph->getRealPos(currFrameID)[3][1];
		currPos = currFrame.pos;
		currRot = currFrame.pose["root"];
	}

	virtual std::vector<float> getNextFrame(bool *completed=NULL) = 0;

protected:
	Graph* graph;
	Graph::FrameID currFrameID;
	Animation::Frame currFrame;
	glm::mat4 currPos = glm::mat4(1.0f);
	glm::quat currRot = glm::mat4(1.0f);
	std::map<std::string, Buffer> buffers;
};

