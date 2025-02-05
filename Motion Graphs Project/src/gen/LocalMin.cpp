#include <gen/LocalMin.h>
#include <algorithm>

std::vector<std::tuple<int, int>> LocalMin::localMinima(std::vector<std::vector<float>> distance_2d, int threshold, int STEP_SIZE) {

	std::vector<float> minimums;
	
	std::vector<std::tuple<int, int>> result;
	const int direction_x[8] = {1, 1, 0, -1, -1, -1, 0, 1};
	const int direction_y[8] = {0, 1, 1, 1, 0, -1, -1, -1};

	int size = distance_2d.size();

	// For each pixel in distance_2d
	for (int i = 0; i < distance_2d.size(); i++) {
		for (int j = 0; j < distance_2d[0].size(); j++) {

			// Check if all edges are larger
			bool is_minimum = true;
			for (int k = 0; k < 8; k++) {
				int new_i = i + direction_x[k];
				int new_j = j + direction_y[k];

				// Skip if pixel out of range
				if (new_i < 0 || new_i >= distance_2d.size() || new_j < 0 || new_j >= distance_2d[0].size()) {
					continue;
				}
				
				if (distance_2d[i][j] >= distance_2d[new_i][new_j]) {
					is_minimum = false;
					break;
				}
			}

			// add if node if node is minimum, not zero, and is below threshold. (Thresholding is ignored if threshold == -1)
			if (is_minimum && (threshold == -1 || distance_2d[i][j] <= threshold) || distance_2d[i][j] == 0) {
				std::tuple<int, int> t = std::make_tuple(i * STEP_SIZE, j * STEP_SIZE);
				result.push_back(t);
			}

			if (is_minimum) {
				minimums.push_back(distance_2d[i][j]);
			}

		}
	}

	if (result.size() == 0) {
		std::cout << "Threshold Might Be Too Small. No Local Mins Found" << std::endl;
		abort();
	}

	std::sort(minimums.begin(), minimums.end());

	std::vector<float> percentiles = { 0.2, 0.4, 0.6, 0.8, 0.99 };
	for (const auto& percentile : percentiles) {
		int index = std::floor(minimums.size() * percentile);
		std::cout << "Threshold Percentile " << percentile << " : " << minimums[index] << std::endl;
	}

	return result;
}