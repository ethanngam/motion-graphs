#include <gen/Distance.h>

Distance::Distance(Animation* _A1, Animation* _A2, int _SIZE) {
	A1 = _A1;
	A2 = _A2;
	SIZE = _SIZE;
}

std::vector<std::vector<float>> Distance::distance(const int STEP_SIZE) {

	std::vector<std::vector<float>> result;

	int A1_size = A1->getFrameSize();
	int A2_size = A2->getFrameSize();

	for (int ai = 0; ai < A1_size - (SIZE - 1); ai += STEP_SIZE) {

		std::cout << ai << "/" << A1_size - (SIZE - 1) << std::endl;

		std::vector<float> result_row;
		for (int bi = 0; bi < A2_size - (SIZE - 1); bi += STEP_SIZE) {

			//std::cout << bi << " ";

			Clip clip1(A1, ai, ai + SIZE);
			Clip clip2(A2, bi, bi + SIZE);

			result_row.push_back(distanceOfClip(clip1, clip2));
		}

		//std::cout << std::endl;

		result.push_back(result_row);
	}

	return result;
}

float Distance::distanceOfClip(Clip C1, Clip C2) {
	
	std::map<char, std::vector<float>> cloud1, cloud2;
	std::vector<float> weights1, weights2;

	cloud1['x'], cloud1['y'], cloud1['z'] = {};
	cloud2['x'], cloud2['y'], cloud2['z'] = {};

	auto root1_pos = C1.animation->getFramePos(C1.start);
	auto root1_rot = C1.animation->getFrameRot(C1.start);
	auto root2_pos = C2.animation->getFramePos(C2.start);
	auto root2_rot = C2.animation->getFrameRot(C2.start);

	for (int ai = C1.start; ai < C1.end; ai++) {
		genPointCloud(C1.animation, ai, &cloud1, &weights1, root1_pos, root1_rot);
	}

	for (int bi = C2.start; bi < C2.end; bi++) {
		genPointCloud(C2.animation, bi, &cloud2, &weights2, root2_pos, root2_rot);
	}

	float sumOfSquaredDistance = 0.0;
	for (int i = 0; i < cloud1['x'].size(); i++) {
		float x = cloud1['x'][i] - cloud2['x'][i];
		float y = cloud1['y'][i] - cloud2['y'][i];
		float z= cloud1['z'][i] - cloud2['z'][i];

		float squaredDistance = pow(x, 2) + pow(y, 2) + pow(y, 2);

		sumOfSquaredDistance += squaredDistance;
	}

	return sumOfSquaredDistance;
}

void Distance::genPointCloud(Animation* A, int frame_id, std::map<char, std::vector<float>>* result, std::vector<float>* weights, glm::mat4 norm_pos, glm::quat norm_rot) {

	A->calculateFrameWithNormalisation(frame_id, norm_pos, norm_rot);
	std::vector<float> vertices = A->getVertices();
	
	for (int i = 3; i < vertices.size(); i += 2 * 3) {
		result->at('x').push_back(vertices[i + 0]);
		result->at('y').push_back(vertices[i + 1]);
		result->at('z').push_back(vertices[i + 2]);
		weights->push_back(1.0f);
	}
}

void Distance::printCSV(std::map<char, std::vector<float>> cloud) {
	for (int i = 0; i < cloud['x'].size(); i++) {
		std::cout
		<< cloud['x'][i] << ","
		<< cloud['y'][i] << ","
		<< cloud['z'][i] <<
		std::endl;
	}
}
