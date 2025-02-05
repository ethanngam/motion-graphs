#include <mogen/RandomMG.h>

std::vector<float> RandomMG::getNextFrame(bool* completed) {

	// select random edge
	const auto& directEdges = graph->getFrameVec(currFrameID).directEdges;
	auto directEdgesIt = directEdges.begin();
	for (int i = 1; i < rand() % graph->getFrameVec(currFrameID).directEdges.size(); i++) {
		directEdgesIt++;
	}

	currFrameID = *directEdgesIt;
	currFrame = graph->getFrameVec(currFrameID).pose;
	Animation::unNormaliseFrame(currFrame, currPos, currRot);
	currPos = currFrame.pos;
	currRot = currFrame.pose["root"];

	Animation* currAnim = graph->getAnimation(currFrameID.animID);
	currAnim->calculateFrame(currFrame);
	return currAnim->getVertices();
}

glm::vec4 RandomMG::getColour() {
	if (currFrameID.tMode) {
		return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);		// blue
	}
	else {
		return glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);		// red
	}
}