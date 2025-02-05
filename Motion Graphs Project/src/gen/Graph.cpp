#include <gen/graph.h>

Graph::Graph(Skeleton* _skeleton, std::vector<Animation*>& animations, std::vector<std::tuple<std::tuple<int, int>, std::tuple<int, int>>>& _edges, const int _window_size) {

	// set skeleton
	skeleton = _skeleton;
	window_size = _window_size;

	// load animations
	for (int i = 0; i < animations.size(); i++) {
		anim_database[animations[i]->getID()] = animations[i];
	}

	// generate empty frame mat
	for (auto const& [animID, anim] : anim_database) {
		for (int j = 0; j < anim->getFrameSize(); j++) {
			FrameID newFrameID;
			FrameVec newFrameVec;

			newFrameID.animID = anim->getID();
			newFrameID.indexID = j;

			FrameMat[newFrameID] = newFrameVec;
		}
	}

	// load transitional edges
	for (auto undirectedEdge : _edges) {
		std::array<std::tuple<std::tuple<int, int>, std::tuple<int, int>>, 2> directedEdges;

		directedEdges[0] = std::make_tuple(get<0>(undirectedEdge), get<1>(undirectedEdge));
		directedEdges[1] = std::make_tuple(get<1>(undirectedEdge), get<0>(undirectedEdge));

		for (auto edge : directedEdges) {

			FrameID node1;
			FrameID node2;

			node1.animID = get<0>(get<0>(edge));
			node1.indexID = get<1>(get<0>(edge));
			node2.animID = get<0>(get<1>(edge));
			node2.indexID = get<1>(get<1>(edge));

			if (node1 == node2) {
				continue;
			}

			node2.indexID += window_size;

			FrameVec& frameVec1 = FrameMat[node1];
			FrameVec& frameVec2 = FrameMat[node2];

			// set isNode
			frameVec1.isStartNode = true;
			frameVec2.isEndNode = true;

			// add frame1 -> frame2
			if (!frameVec1.edges.contains(node2)) {
				frameVec1.edges.insert(node2);
			}
		}
	}

	// to store previous nodes (animID, indexID)
	std::map<int, int> prev_nodes;

	// load sequential edges
	for (auto const& [frameID, frameVec] : FrameMat) {
		int animID = frameID.animID;
		int indexID = frameID.indexID;

		// load animation to prev_nodes
		if (!prev_nodes.contains(animID)) {
			prev_nodes[animID] = -1;
		}

		// connect the nodes
		if (frameVec.isStartNode || frameVec.isEndNode) {

			// if not first node, connect with previous node
			if (prev_nodes[animID] != -1) {
				FrameID key;
				key.animID = animID;
				key.indexID = prev_nodes[animID];
				FrameMat[key].edges.insert(frameID);
				FrameMat[key].seqEdge = frameID;
			}

			// set previous node
			prev_nodes[animID] = indexID;
		}
	}

	// prune the graph
	pruneGraph();

	// connect each frame to the next direct frame
	bool connect = false;
	for (auto const& [frameID, frameVec] : FrameMat) {

		if (isTerminalNode(frameID)) {
			continue;
		}

		FrameID nextFrameID = FrameID(frameID);
		nextFrameID.indexID += 1;

		if (FrameMat.contains(nextFrameID)) {
			FrameMat[frameID].directEdges.insert(nextFrameID);
		}
	}

	// generate transitions
	for (auto const& [frameID, frameVec] : FrameMat) {
		for (auto const& nextFrameID : FrameMat[frameID].edges) {
			if (isNode(frameID) && FrameMat[frameID].seqEdge != nextFrameID) {		// if it is a transition edge
				FrameID prevFrameID = frameID;
				for (int i = 1; i < window_size; i++) {
					FrameID tFrameID;
					FrameVec tFrameVec;

					// set up transition frame
					tFrameID.animID = frameID.animID;
					tFrameID.indexID = frameID.indexID + i;
					tFrameID.animID2 = nextFrameID.animID;
					tFrameID.indexID2 = nextFrameID.indexID - window_size + i;
					tFrameID.tMode = true;
					tFrameID.alpha = i;

					// insert transition frame
					FrameMat[tFrameID] = tFrameVec;

					// connect to previous frame
					FrameMat[prevFrameID].directEdges.insert(tFrameID);

					// store this frame as previous frame
					prevFrameID = tFrameID;

					// debug
					assert(!(tFrameID.animID == tFrameID.animID2 && tFrameID.indexID == tFrameID.indexID2));
				}

				// connect last transition frame with next animation
				FrameMat[prevFrameID].directEdges.insert(nextFrameID);
			}
		}
	}

	// generate the pose & arclen all nodes
	FrameID prevFrameID;
	for (auto const& [frameID, frameVec] : FrameMat) {
		if (!frameID.tMode) {	// sequential nodes
			// get pose
			auto pose = anim_database[frameID.animID]->getFrame(frameID.indexID);

			// normalize frame relative to previous frame
			if (prevFrameID.animID != -1 && prevFrameID.animID == frameID.animID) {
				Animation::Frame prevFrame = anim_database[prevFrameID.animID]->getFrame(prevFrameID.indexID);
				Animation::normaliseFrame(pose, prevFrame.pos, prevFrame.pose["root"]);

				const auto distance = glm::distance(glm::vec3(pose.pos[3][0], 0.0f, pose.pos[3][2]), glm::vec3(0.0f));
				FrameMat[frameID].arclen = FrameMat[prevFrameID].arclen + std::max(distance, SMALL_INCREMENT);
				FrameMat[frameID].truePos = FrameMat[prevFrameID].truePos + glm::vec3(pose.pos[3]);
			}
			
			FrameMat[frameID].pose = pose;
			prevFrameID = frameID;
		}
		else {	// transitional nodes
			const auto pose = blend(frameID, window_size);
			FrameMat[frameID].pose = pose;

			if (frameID.alpha == 1) {
				const auto distance = glm::distance(glm::vec3(pose.pos[3][0], 0.0f, pose.pos[3][2]), glm::vec3(0.0f));
				FrameMat[frameID].arclen = std::max(distance, SMALL_INCREMENT);
				FrameMat[frameID].truePos = glm::vec3(pose.pos[3]);
			}
			else if (frameID.alpha > 1) {
				prevFrameID.animID = frameID.animID;
				prevFrameID.indexID = frameID.indexID - 1;
				prevFrameID.animID2 = frameID.animID2;
				prevFrameID.indexID2 = frameID.indexID2 - 1;
				prevFrameID.alpha = frameID.alpha - 1;
				prevFrameID.tMode = frameID.tMode;

				const auto distance = glm::distance(glm::vec3(pose.pos[3][0], 0.0f, pose.pos[3][2]), glm::vec3(0.0f));
				FrameMat[frameID].arclen = FrameMat[prevFrameID].arclen + std::max(distance, SMALL_INCREMENT);
				FrameMat[frameID].truePos = FrameMat[prevFrameID].truePos + glm::vec3(pose.pos[3]);
			}
			else {
				throw new std::exception("Fatal Error, Invalid Alpha");
			}
		}
	}

	// print graph
	std::cout << "Saving & Printing Graph" << std::endl;
	toFile(true);
	std::cout << "End Printing Graph" << std::endl;

	const auto& n_edges = _edges.size();
	const auto& n_frames = FrameMat.size();
	const auto& avg_edge_length = (FrameMat.size() / n_edges);

	std::cout << "Number of Edges: " << n_edges << std::endl;
	std::cout << "Number of Frames: " << n_frames << std::endl;
	std::cout << "Avg Edge Length: " << avg_edge_length << std::endl;
}

Animation::Frame Graph::blend(FrameID transID, int windowSize) {

	// to convert "amount" to [0,1] and ensure C1 continuity
	auto interpolate = [](int amount, int windowSize) {
		// convert to float
		float pPlus1 = (float)amount + 1;
		float k = (float)windowSize;

		// calculate using the equation in the paper
		float result = 2 * std::pow(pPlus1 / k, 3) - 3 * std::pow(pPlus1 / k, 2) + 1;

		return result;
	};

	Animation::Frame output;
	Animation::Frame frame1 = getFrameVec(transID.animID, transID.indexID).pose;
	Animation::Frame frame2 = getFrameVec(transID.animID2, transID.indexID2).pose;

	// get interpolation value
	float ap = interpolate(transID.alpha, windowSize);

	// interpolate the root1 & root2
	glm::mat4 root1 = frame1.pos;
	glm::mat4 root2 = frame2.pos;
	glm::mat4 root = ap * root1 + (1 - ap) * root2;
	output.pos = root;

	// interpolate the joint angles
	for (std::string bone : skeleton->getBonenames()) {

		// skip if bone is not in motion
		// this can happen as lhipjoint and rhipjoint may not be specified / rotated at all
		if (!frame1.pose.contains(bone) || !frame2.pose.contains(bone)) {
			continue;
		}

		// initialise the quartenions
		glm::quat quat1 = frame1.pose.at(bone);
		glm::quat quat2 = frame2.pose.at(bone);

		// interpolate
		glm::quat quat;
		quat = glm::mix(quat1, quat2, 1 - ap);

		output.pose[bone] = quat;
	}

	return output;
}


// uses Tarjan's algorithm to generate strongly connected components
// adapted from https://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm
void Graph::pruneGraph() {

	// define vertex struct for algorithm
	struct Vertex {
		FrameID frameID;
		int index = -1;
		int lowlink = -1;
		bool onStack = false;
	};

	// generate set of vertices
	std::map<FrameID, Vertex> vertices;
	for (auto const& [frameID, frameVec] : FrameMat) {
		if (isNode(frameID)) {
			Vertex v;
			v.frameID = frameID;
			vertices[v.frameID] = v;
		}
	}

	int index = 0;	// to give nodes unique indexes
	std::stack<Vertex> stack;
	std::map<int, std::set<FrameID>> SCCs;

	// define lambda function for strong connect
	std::function<void(Vertex&)> strongConnect;
	strongConnect = [&](Vertex& v) -> void {
		// setup
		v.index = index;
		v.lowlink = index;
		v.onStack = true;
		stack.push(v);
		index++;

		std::set<FrameID>& edges = FrameMat[v.frameID].edges;
		for (auto const& w_id : edges) {

			Vertex& w = vertices[w_id];

			if (w.index == -1) {
				strongConnect(w);
				v.lowlink = std::min(v.lowlink, w.lowlink);
			}
			else if (w.onStack) {
				v.lowlink = std::min(v.lowlink, w.index);
			}
		}

		Vertex u;
		if (v.lowlink == v.index) {
			std::set<FrameID> set;

			do {
				u = stack.top();
				stack.pop();

				u.onStack = false;
				set.insert(u.frameID);
			} while (u.index != v.index);

			SCCs[v.index] = set;
		}
	};

	for (std::map<FrameID, Vertex>::iterator it = vertices.begin(); it != vertices.end(); it++) {
		if (it->second.index == -1) {
			strongConnect(it->second);
		}
	}

	// start pruning
	// get largest SCC
	int largestSCC = 0;
	for (auto it = SCCs.begin(); it != SCCs.end(); it++) {
		if (it->second.size() > SCCs[largestSCC].size()) {
			largestSCC = it->first;
		}
	}

	// remove all edges that do not connect two nodes in the SCC
	for (auto const& [frameID, vertex] : vertices) {

		// copy edges as you cannot delete elements from the same set while looping
		std::set<FrameID> edges = std::set<FrameID>(FrameMat[frameID].edges);
		for (auto it = edges.begin(); it != edges.end(); it++) {
			if (!SCCs[largestSCC].contains(frameID) || !SCCs[largestSCC].contains(*it)) {
				FrameMat[frameID].edges.erase(*it);
				if (FrameMat[frameID].seqEdge == *it) {		// unset sequential edges as well
					FrameMat[frameID].seqEdge.animID = -1;
				}
			}
		}
	}

	// remove all nodes without edges
	for (auto const& [frameID, vertex] : vertices) {
		if (FrameMat[frameID].edges.size() == 0) {
			FrameMat[frameID].isStartNode = false;
			FrameMat[frameID].isEndNode = false;
		}
	}
}

float Graph::dFrameArclength(FrameID currFrame, FrameID nextFrame) {

	assert(
		(currFrame.animID == nextFrame.animID
			&& currFrame.indexID + 1 == nextFrame.indexID)
		|| currFrame.tMode
		|| nextFrame.tMode);

	float arclen = -1.0;
	if (currFrame.tMode == nextFrame.tMode) {
		arclen = FrameMat[nextFrame].arclen - FrameMat[currFrame].arclen;
	}
	else if (!currFrame.tMode && nextFrame.tMode) {
		arclen = FrameMat[nextFrame].arclen;
	}
	else if (currFrame.tMode && !nextFrame.tMode) {
		FrameID currSeqFrame;
		currSeqFrame.animID = currFrame.animID2;
		currSeqFrame.indexID = currFrame.indexID2;

		arclen = FrameMat[nextFrame].arclen - FrameMat[currSeqFrame].arclen;
	}

	assert(arclen >= 0);
	return arclen;
}

float Graph::dEdgeArclength(FrameID currFrame, FrameID nextFrame) {
	assert(isNode(currFrame) && isNode(nextFrame));

	float arclen = -1.0;
	if (FrameMat.at(currFrame).seqEdge == nextFrame) {
		arclen = FrameMat[nextFrame].arclen - FrameMat[currFrame].arclen;
	}
	else {
		arclen = FrameMat[nextFrame].arclen;
	}

	assert(arclen >= 0);
	return arclen;
}

glm::vec3 Graph::dEdgePosition(FrameID currFrame, FrameID nextFrame) {

	assert(isNode(currFrame) && isNode(nextFrame));

	if (FrameMat.at(currFrame).seqEdge == nextFrame) {
		return FrameMat[nextFrame].truePos - FrameMat[currFrame].truePos;
	}
	else {
		return FrameMat[nextFrame].truePos;
	}
}

// get a copy of the frame
Animation::Frame Graph::getFrame(FrameID frameID) {
	return FrameMat[frameID].pose;
}

glm::mat4 Graph::getRealPos(FrameID frameID) {
	assert(!frameID.tMode);
	return anim_database[frameID.animID]->getFrame(frameID.indexID).pos;
}

Graph::FrameID Graph::getFirstNode() {
	for (auto const& [frameID, frameVec] : FrameMat) {
		if (isNode(frameID)) {
			return frameID;
		}
	}
	throw new std::exception("Warning: No Node Found!");
}

Animation* Graph::getAnimation(int animation_id) {
	return anim_database[animation_id];
}

int Graph::getWindowSize() {
	return window_size;
}

Animation* Graph::getRandomAnimation() {
	return anim_database.begin()->second;
}

std::set<Graph::FrameID> Graph::getEdges(int animID, int indexID) {
	return getFrameVec(animID, indexID).edges;
}

std::set<Graph::FrameID> Graph::getEdges(FrameID frameID) {
	return FrameMat[frameID].edges;
}

Graph::FrameID Graph::getFrameID(int animID, int indexID) {
	FrameID frameID;
	frameID.animID = animID;
	frameID.indexID = indexID;
	return frameID;
}

const Graph::FrameVec& Graph::getFrameVec(FrameID frameID) {
	assert(FrameMat.contains(frameID));
	return FrameMat.at(frameID);
}

const Graph::FrameVec& Graph::getFrameVec(int animID, int indexID) {
	FrameID key;
	key.animID = animID;
	key.indexID = indexID;

	return getFrameVec(key);
}

Skeleton* Graph::getSkeleton() {
	return skeleton;
}

bool Graph::isTerminalNode(int animID, int indexID) {
	FrameID frameID = getFrameID(animID, indexID);

	return isTerminalNode(frameID);
}

bool Graph::isTerminalNode(FrameID frameID) {

	if ((FrameMat[frameID].isStartNode || FrameMat[frameID].isEndNode) && FrameMat[frameID].seqEdge.animID == -1) {
		// return true if there is no sequential edges
		return true;
	}
	else {
		return false;
	}
}

// prints the graph
std::string Graph::toString(bool print) {
	std::string output = "";

	for (auto const& [frameID, frameVec] : FrameMat) {

		if (isNode(frameID)) {
			output += toString(frameID);
			output += " --> ";

			for (auto const& edge : frameVec.edges) {
				output += toString(edge) + " ";
			}

			output += "\n";
		}
	}

	if (print) {
		std::cout << output << std::endl;
	}

	return output;
}

// prints the node as (animID, indexID)
std::string Graph::toString(FrameID frameID) {
	if (!frameID.tMode) {
		return "(" + std::to_string(frameID.animID) + "," + std::to_string(frameID.indexID) + ")";
	}
	else {
		return "(" + std::to_string(frameID.animID) + "," + std::to_string(frameID.indexID) + ")"
			+ "(" + std::to_string(frameID.animID2) + "," + std::to_string(frameID.indexID2) + ")"
			+ std::to_string(frameID.alpha);
	}
}

void Graph::toFile(bool print) {
	std::ofstream save_file;
	save_file.open("output/graph.txt");

	if (!save_file.is_open()) {
		throw new std::exception("Error in Writing to File");
	}

	save_file << toString(print);
	save_file.close();
}

bool Graph::isNode(int animID, int indexID) {
	FrameID key = getFrameID(animID, indexID);
	return isNode(key);
}

bool Graph::isNode(FrameID frameID) {
	if (FrameMat[frameID].isStartNode || FrameMat[frameID].isEndNode) {
		return true;
	}
	else {
		return false;
	}
}
