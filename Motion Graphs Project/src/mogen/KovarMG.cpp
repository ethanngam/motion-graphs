#include <mogen/KovarMG.h>
#include <ctime>

int n_transition_frames = 0;
float lfootslide = 0;
float rfootslide = 0;

glm::vec3* prev_ltoe = NULL;
glm::vec3* prev_rtoe = NULL;

std::vector<float> KovarMG::getNextFrame(bool* completed) {

	*completed = false;

	Animation* currAnim = graph->getAnimation(currFrameID.animID);

	// do nothing if path is not set
	if (path.nodes.empty()) {
		currAnim->calculateFrame(currFrame);
		return currAnim->getVertices();
	}

	// reset index to loop the animation
	if (pathIdx >= path.nodes.size()) {
		currFrameID = graph->getFirstNode();
		currFrame = Animation::Frame(graph->getFrameVec(currFrameID).pose);
		currFrame.pos[3][1] = graph->getRealPos(currFrameID)[3][1];
		currPos = currFrame.pos;
		currRot = currFrame.pose["root"];
		pathIdx = 0;
		*completed = true;
	}

	// select next edge
	currFrameID = path.nodes[pathIdx];
	const auto& currFrameData = graph->getFrameVec(currFrameID);
	currFrame = currFrameData.pose;
	Animation::unNormaliseFrame(currFrame, currPos, currRot);
	currPos = currFrame.pos;
	currRot = currFrame.pose["root"];

	pathIdx++;

	currAnim->calculateFrame(currFrame);

	glm::vec3 ltoe, rtoe;
	const auto& vertices = currAnim->getVertices(&ltoe, &rtoe);

	if (currFrameID.tMode) {
		if (prev_ltoe && prev_rtoe) {
			lfootslide += 0.1 * glm::distance(*prev_ltoe, ltoe);
			rfootslide += 0.1 * glm::distance(*prev_rtoe, rtoe);
		}
		prev_ltoe = new glm::vec3(ltoe);
		prev_rtoe = new glm::vec3(rtoe);
		n_transition_frames++;
	}
	else {
		prev_ltoe = NULL;
		prev_rtoe = NULL;
	}

	if (*completed) {
		std::cout << "Number of Transition Frames: " << n_transition_frames << std::endl;
		std::cout << "Total lfootslide: " << lfootslide << std::endl;
		std::cout << "Total rfootslide: " << rfootslide << std::endl;
		std::cout << "Total footslide: " << lfootslide + rfootslide << std::endl;
		std::cout << "Avg lfootslide per frame: " << lfootslide / (float)n_transition_frames << std::endl;
		std::cout << "Avg rfootslide per frame: " << rfootslide / (float)n_transition_frames << std::endl;
		std::cout << "Avg footslide per frame: " << (lfootslide + rfootslide) / (float)n_transition_frames << std::endl;
		std::cout << "Avg lfootslide per transition: " << lfootslide * graph->getWindowSize() / (float)n_transition_frames << std::endl;
		std::cout << "Avg rfootslide per transition: " << rfootslide * graph->getWindowSize() / (float)n_transition_frames << std::endl;
		std::cout << "Avg footslide per transition: " << (lfootslide + rfootslide) * graph->getWindowSize() / (float)n_transition_frames << std::endl;
	}



	return vertices;
}

glm::vec4 KovarMG::getColour() {
	if (path.nodes.empty() || pathIdx >= path.nodes.size()) {
		return glm::vec4(0.25f, 0.25f, 0.25f, 1.0f);	// grey
	}
	else if (currFrameID.tMode) {
		return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);		// blue
	}
	else {
		return glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);		// red
	}
}

void KovarMG::setPath(const std::vector<float>& line) {

	std::vector<glm::vec3> pathline;

	// generate pathline: vector of vec3s
	// skip first point
	for (int i = 3; i < line.size(); i += 6) {
		pathline.push_back(glm::vec3(line[i + 0], line[i + 1], line[i + 2]));
	}

	// set path radius, should be equivalent to main::PATH_RADIUS
	float pathRadius = std::round(glm::distance(pathline[0], pathline[1]));


	std::clock_t start = std::clock();
	searchPath(currFrameID, pathline, pathRadius);
	std::clock_t end = std::clock();
	double elapsed_time = double(end - start) / CLOCKS_PER_SEC;
	std::cout << "CPU time used: " << elapsed_time << " seconds\n";
	// Print path cost
	std::cout << "Path Error: " << path.stack.back().cost << std::endl;

}

void KovarMG::searchPath(Graph::FrameID start_node, const std::vector<glm::vec3>& pathline, float pathRadius) {

	const auto& startFrameData = graph->getFrameVec(start_node);

	State startState(start_node, currPos, currRot, 0, 0, SEARCH);
	startState.nextFrames = sortedNextFrames(startState, pathline, pathRadius);

	path.stack.push_back(startState);
	path.nodes.push_back(start_node);

	while (!isComplete(path, pathline)) {
		std::cout << "Frames Constructed: " << path.nodes.size() << std::endl;

		if (path.nodes.size() > MAX_PATH_LENGTH && MAX_PATH_LENGTH != -1) {
			std::cout << "Warning, Maximum Path Limit Reached " << std::endl;
			break;
		}

		// branch and bound iteratively
		const auto& nextIteration = iterateBnB(path, pathline, pathRadius);

		// keep first KEEP number of nodes
		// offset by one as we do not want to include the first node
		for (int i = 1; i < KEEP + 1; i++) {
			if (i >= nextIteration.nodes.size()) {
				assert(isComplete(nextIteration, pathline));
				assert(isComplete(path, pathline));
				break;
			}

			path.stack.push_back(nextIteration.stack[i]);	
			path.nodes.push_back(nextIteration.nodes[i]);
		}
	}
}

KovarMG::Path KovarMG::iterateBnB(const Path& globalPath, const std::vector<glm::vec3>& pathline, float pathRadius) {

	float lowerBound = std::numeric_limits<float>::max();

	Path currPath;
	Path bestPath;

	currPath.stack.reserve(SEARCH + 1);
	currPath.nodes.reserve(SEARCH + 1);

	const auto& lastState = globalPath.stack.back();

	// Reset depth
	State startState(lastState.frameID, lastState.position, lastState.rotation, lastState.arclen, lastState.cost, SEARCH);
	startState.nextFrames = lastState.nextFrames;

	// Initialise current path with the previous last state
	currPath.stack.push_back(startState);
	currPath.nodes.push_back(globalPath.nodes.back());

	while (!currPath.stack.empty()) {

		State& currState = currPath.stack.back();

		const bool prune = currState.cost >= lowerBound;
		const bool noNextFrames = currState.index >= currState.nextFrames.size();

		// Break conditions
		if (prune || noNextFrames) {
			currPath.stack.pop_back();
			currPath.nodes.pop_back();
			continue;
		}

		const bool firstEvaluation = currState.index == 0;
		const bool depthReached = currState.depth == 0;
		const bool completed = isComplete(currPath, pathline);

		// End conditions
		if (firstEvaluation && (depthReached || completed)) {
			if (currState.cost < lowerBound) {
				lowerBound = currState.cost;
				bestPath = Path(currPath);
			}
			currPath.stack.pop_back();
			currPath.nodes.pop_back();
			continue;
		}

		const auto& nextFrameID = currState.nextFrames[currState.index];
		const auto& nextFrameData = graph->getFrameVec(nextFrameID);
		currState.index++;

		currPath.stack.emplace_back(nextState(currState, pathline, pathRadius, nextFrameID));
		currPath.nodes.push_back(nextFrameID);
	}

	assert(bestPath.stack.size() > 1);
	return bestPath;
}

std::vector<Graph::FrameID> KovarMG::sortedNextFrames(State currState, const std::vector<glm::vec3>& pathline, float pathRadius) {

	const auto& currFrameID = currState.frameID;
	const auto& currFrameData = graph->getFrameVec(currFrameID);

	// Convert currFrameDate.edges from set to vector
	std::vector<Graph::FrameID> edges;
	edges.assign(currFrameData.edges.begin(), currFrameData.edges.end());

	// Sort the edges
	if (edges.size() > 1) {
		std::sort(edges.begin(), edges.end(), [&](const Graph::FrameID& f1, const Graph::FrameID& f2) {
			const auto dArclen1 = graph->dEdgeArclength(currState.frameID, f1);
		const auto dArclen2 = graph->dEdgeArclength(currState.frameID, f2);
		const auto dPosition1 = graph->dEdgePosition(currState.frameID, f1);
		const auto dPosition2 = graph->dEdgePosition(currState.frameID, f2);
		const auto truePos1 = getTruePos(currState.arclen + dArclen1, pathRadius, pathline);
		const auto truePos2 = getTruePos(currState.arclen + dArclen2, pathRadius, pathline);
		const auto currPos1 = glm::vec3(currState.position[3]) + dPosition1;
		const auto currPos2 = glm::vec3(currState.position[3]) + dPosition2;
		const auto distance1 = glm::distance(truePos1, currPos1);
		const auto distance2 = glm::distance(truePos2, currPos2);

		return distance1 < distance2;
			});
	}

	/* --- Generate Next Frames --- */

	std::vector<Graph::FrameID> nextFrames;

	if (edges.empty()) {						// in between nodes
		Graph::FrameID nextFrameID;
		nextFrameID = *currFrameData.directEdges.begin();
		nextFrames.push_back(nextFrameID);
		assert(!graph->isNode(currFrameID));
		assert(currFrameData.directEdges.contains(nextFrameID));
	}
	else {										// nodes
		assert(graph->isNode(currState.frameID));
		for (auto const& nextNode : edges) {
			Graph::FrameID nextFrameID;
			if (currFrameData.seqEdge == nextNode) {	// sequential edge
				nextFrameID = currFrameID;
				nextFrameID.indexID += 1;
			}
			else {										// transitional edge
				nextFrameID.animID = currFrameID.animID;
				nextFrameID.indexID = currFrameID.indexID + 1;
				nextFrameID.animID2 = nextNode.animID;
				nextFrameID.indexID2 = nextNode.indexID - graph->getWindowSize() + 1;
				nextFrameID.tMode = true;
				nextFrameID.alpha = 1;
			}
			nextFrames.push_back(nextFrameID);
			assert(currFrameData.directEdges.contains(nextFrameID));
		}
	}
	assert(!nextFrames.empty());
	assert(nextFrames.size() == currFrameData.directEdges.size());

	return nextFrames;
}

float KovarMG::pathCost(const State& currState, const std::vector<glm::vec3>& pathline, float pathRadius) {
	glm::vec3 currPos = glm::vec3(currState.position[3][0], 0.0f, currState.position[3][2]);
	glm::vec3 truePos = getTruePos(currState.arclen, pathRadius, pathline);

	float distance = glm::distance(currPos, truePos);

	if (std::isnan(distance)) {
		throw new std::exception("Distance is nan!");
	}

	return distance;
}

bool KovarMG::isComplete(const Path& localPath, const std::vector<glm::vec3>& pathline) {

	glm::vec3 lastPos = pathline.back();
	glm::vec3 currPos = localPath.stack.back().position[3];
	currPos[1] = 0.0f;	// cast to floor

	bool isClose = glm::distance(currPos, lastPos) < MARGIN;
	bool isGoodLength = localPath.stack.back().arclen > pathline.size() * 0.9;

	return isClose && isGoodLength;
}

glm::vec3 KovarMG::getTruePos(float arclen, float pathRadius, const std::vector<glm::vec3>& pathline) {

	// get index in float
	float indexf = arclen / pathRadius;

	// get real index and its remainder
	int index = floor(indexf);
	float remainder = indexf - index;

	if (index + 1 < pathline.size()) {
		// get the two start and end points that indexf lies in
		glm::vec3 S = pathline[index];
		glm::vec3 E = pathline[index + 1];

		// interpolate between them
		glm::vec3 P = S * (1 - remainder) + E * (remainder);

		return P;
	}
	else {
		return pathline.back();
	}
}

KovarMG::State KovarMG::nextState(const State& currState, const std::vector<glm::vec3>& pathline, float pathRadius, Graph::FrameID nextFrame) {

	const auto& nexFrameData = graph->getFrameVec(nextFrame);

	glm::mat4 position = nexFrameData.pose.pos;
	glm::quat rotation = nexFrameData.pose.pose.at("root");

	Animation::unNormaliseTransform(position, rotation, currState.position, currState.rotation);
	float arclen = currState.arclen + graph->dFrameArclength(currState.frameID, nextFrame);
	float cost = currState.cost + std::pow(pathCost(currState, pathline, pathRadius), 2);	// sum of squared error
	int depth = currState.depth - 1;

	State nextState(nextFrame, position, rotation, arclen, cost, depth);
	nextState.nextFrames = sortedNextFrames(nextState, pathline, pathRadius);

	return nextState;
}

void KovarMG::reset() {
	currFrameID = graph->getFirstNode();
	currFrame = Animation::Frame(graph->getFrameVec(currFrameID).pose);
	currFrame.pos[3][1] = graph->getRealPos(currFrameID)[3][1];
	currPos = currFrame.pos;
	currRot = currFrame.pose["root"];
	path = Path();
	pathnodes = std::vector<Graph::FrameID>();
	pathIdx = 0;
}