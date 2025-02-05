#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <core/Animation.h>
#include <core/Skeleton.h>

#include <vector>
#include <map>
#include <set>
#include <stack>
#include <algorithm>
#include <fstream>

class Graph
{

public:

	struct FrameID {
		int animID = -1;		//	animationID
		int indexID = -1;		//	indexID
		int animID2 = -1;		//	2nd animation ID (for transitions)
		int indexID2 = -1;		//	2nd index ID (for transtions)
		int alpha = -1;			//	alpha value of the interpolation
		bool tMode = false;		//	flag to identify if it is a transition

		// comparison operator
		bool operator<(const FrameID& other) const {
			if (tMode != other.tMode) {
				return tMode < other.tMode;
			}
			else if (animID != other.animID) {
				return animID < other.animID;
			}
			else if (indexID != other.indexID) {
				return indexID < other.indexID;
			}
			else if (animID2 != other.animID2) {
				return animID2 < other.animID2;
			}
			else if (indexID2 != other.indexID2) {
				return indexID2 < other.indexID2;
			}
			else {
				return alpha < other.alpha;
			}
		}

		// equals operator
		bool const operator==(const FrameID& frameID) const {
			bool equals = true;
			equals = equals && (animID == frameID.animID);
			equals = equals && (indexID == frameID.indexID);
			equals = equals && (animID2 == frameID.animID2);
			equals = equals && (indexID2 == frameID.indexID2);
			equals = equals && (tMode == frameID.tMode);
			equals = equals && (alpha == frameID.alpha);

			return equals;
		}

		// not equals operator
		bool const operator!=(const FrameID& frameID) const {
			return !operator==(frameID);
		}
	};

	struct FrameVec {
		bool isStartNode = false;			// is the node the start of a transition
		bool isEndNode = false;				// is the nodee the end of a transition
		std::set<FrameID> edges;			// edges of the node (next nodes)
		std::set<FrameID> directEdges;		// directEdges of the node (next frames)
		FrameID seqEdge;					// next sequential edge of the node
		Animation::Frame pose;				// pose of the frame
		float arclen = 0.0;					// arclen walked. Includes small progress forward
		glm::vec3 truePos = glm::vec3(0.0f);// true position of the frame in animation
	};

	// Constructor
	Graph(Skeleton* _skeleton, std::vector<Animation*>& animations,
		std::vector<std::tuple<std::tuple<int, int>, std::tuple<int, int>>>& _edges,
		const int _window_size);

	// Calculate change in arclength per frame
	float dFrameArclength(FrameID currFrame, FrameID nextFrame);

	// Calculate change in arclength per edge
	float dEdgeArclength(FrameID currFrame, FrameID nextFrame);

	// Calculate change in position per edge
	glm::vec3 dEdgePosition(FrameID currFrame, FrameID nextFrame);

	// Get the real postion
	glm::mat4 getRealPos(FrameID frameID);

	// Getters
	Animation::Frame getFrame(FrameID frameID);
	FrameID getFirstNode();
	Animation* getAnimation(int animation_id);
	int getWindowSize();
	Animation* getRandomAnimation();
	std::set<FrameID> getEdges(int animID, int indexID);
	std::set<FrameID> getEdges(FrameID frameID);
	FrameID getFrameID(int animID, int indexID);
	const FrameVec& getFrameVec(FrameID frameID);
	const FrameVec& getFrameVec(int animID, int indexID);
	Skeleton* getSkeleton();
	bool isTerminalNode(int animID, int indexID);
	bool isTerminalNode(FrameID frameID);
	bool isNode(int animID, int indexID);
	bool isNode(FrameID frameID);

	// Output to string
	std::string toString(bool print = false);
	static std::string toString(FrameID frameID);
	void toFile(bool print = false);


private:
	// Attributes
	Skeleton* skeleton;
	int window_size;
	std::map<int, Animation*> anim_database;
	std::map<FrameID, FrameVec> FrameMat;
	const float SMALL_INCREMENT = 0.01;

	void pruneGraph();
	Animation::Frame blend(FrameID transID, int windowSize);
};





