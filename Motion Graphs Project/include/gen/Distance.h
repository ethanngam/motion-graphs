#pragma once

#include <core/Animation.h>
#include <core/Skeleton.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <map>
#include <numeric>
#include <cmath>

class Distance
{

public:

	struct Clip {
		Animation* animation;
		int start;
		int end;

		Clip(Animation* _A, int _start, int _end) {
			animation = _A;
			start = _start;
			end = _end;
		}
	};

	struct Point {
		float x;
		float y;
		float z;

		Point(float _x, float _y, float _z) {
			x = _x;
			y = _y;
			z = _z;
		}
	};

	Animation* A1;
	Animation* A2;
	int SIZE;

	Distance(Animation* _A1, Animation* _A2, int _SIZE);
	std::vector<std::vector<float>> distance(const int STEP_SIZE = 1);

private:
	float distanceOfClip(Clip C1, Clip C2);
	void genPointCloud(Animation* A, int frame_id, std::map<char, std::vector<float>>* result, std::vector<float>* weights, glm::mat4 norm_pos, glm::quat norm_rot);
	void printCSV(std::map<char, std::vector<float>> cloud);

};

