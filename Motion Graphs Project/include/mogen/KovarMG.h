#pragma once

#include <gen/Graph.h>
#include <mogen/MotionGenerator.h>

class KovarMG : public MotionGenerator
{
private:
	
	struct State {
		std::vector<Graph::FrameID> nextFrames;
		const Graph::FrameID frameID;
		const glm::mat4 position;
		const glm::quat rotation;
		const float arclen = 0;
		const float cost = 0;
		const int depth = 0;
		int index = 0;

		State(
			Graph::FrameID _frameID,
			glm::mat4 _position,
			glm::quat _rotation,
			float _arclen,
			float _cost,
			int _depth) :
			frameID(_frameID),
			position(_position),
			rotation(_rotation),
			arclen(_arclen),
			cost(_cost),
			depth(_depth) {}
	};

	struct Path {
		std::vector<State> stack;
		std::vector<Graph::FrameID> nodes;
	};

	Path path;
	std::vector<Graph::FrameID> pathnodes;
 	int pathIdx = 0;
	const int FPS = 120;
	const int SEARCH = FPS * 2;
	const int KEEP = SEARCH * 0.5;
	const int MARGIN = 3;
	const int MAX_PATH_LENGTH = FPS * 60 * 10;

	void searchPath(Graph::FrameID start_node, const std::vector<glm::vec3>& pathline, float pathRadius);
	float pathCost(const State& currState, const std::vector<glm::vec3>& pathline, float pathRadius);
	bool isComplete(const Path& localPath, const std::vector<glm::vec3>& pathline);
	State nextState(const State& currState, const std::vector<glm::vec3>& pathline, float pathRadius, Graph::FrameID nextFrame);
	glm::vec3 getTruePos(float arclen, float pathRadius, const std::vector<glm::vec3>& pathline);

public:
	KovarMG(Graph* _graph) : MotionGenerator(_graph) {
		// [WIP]
		// initialise path point buffer
		Buffer pathPointBuffer;
		pathPointBuffer.colour = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);		// black
		pathPointBuffer.glType = GL_LINES;
		glGenVertexArrays(1, &pathPointBuffer.VAO);
		glGenBuffers(1, &pathPointBuffer.VBO);
		buffers["pathPoint"] = pathPointBuffer;

		// [WIP]
		// initialise root point buffer
		Buffer rootPointBuffer;
		rootPointBuffer.colour = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);		// red
		rootPointBuffer.glType = GL_LINES;
		glGenVertexArrays(1, &rootPointBuffer.VAO);
		glGenBuffers(1, &rootPointBuffer.VBO);
		buffers["rootPoint"] = rootPointBuffer;
	}

	void setPath(const std::vector<float>& line);
	Path iterateBnB(const Path& globalPath, const std::vector<glm::vec3>& pathline, float pathRadius);
	std::vector<Graph::FrameID> sortedNextFrames(State currState, const std::vector<glm::vec3>& pathline, float pathRadius);
	std::vector<float> getNextFrame(bool* completed = NULL);
	glm::vec4 getColour();
	void reset();
};

