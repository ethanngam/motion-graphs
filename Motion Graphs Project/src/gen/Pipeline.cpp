#include <gen/Pipeline.h>

void Pipeline::saveDistanceToFile(std::vector<std::vector<float>> distance, std::string filename) {
	std::ofstream DistanceCSV(filename);
	for (int i = 0; i < distance.size(); i++) {
		for (int j = 0; j < distance[0].size(); j++) {
			DistanceCSV << distance[i][j] << " ";
		}
		DistanceCSV << "\n";
	}
	DistanceCSV.close();
}

std::vector<std::vector<float>> Pipeline::loadDistanceFromFile(std::string filename) {

	std::ifstream infile(filename);

	if (!infile.is_open()) {
		perror(("error while opening file " + filename).c_str());
	}

	std::vector<std::vector<float>> result;

	for (std::string line; getline(infile, line); )
	{
		std::vector<float> tokens;

		std::stringstream ss(line);
		std::string word;
		while (ss >> word) {
			tokens.push_back(std::stof(word));
		}

		result.push_back(tokens);
	}

	return result;
}

Graph Pipeline::genGraph(const int WINDOW_SIZE, const int THRESHOLD, const int STEP_SIZE, const std::string graphdir) {

	// variables
	std::string asf_file;
	std::vector<std::string> amc_files;

	// setup path to mocap and distance directories
	std::string mocap_dir = graphdir + "mocap/";
	std::string distance_dir = graphdir + "distance/";

	// get all files in mocap directory
	for (const auto& entry : std::filesystem::directory_iterator(mocap_dir)) {

		// get skeleton file
		if (entry.path().extension() == ".asx") {
			std::cout << "Found " + entry.path().string() << std::endl;
			asf_file = entry.path().string();
		}

		// get all motion files
		if (entry.path().extension() == ".amc") {
			std::cout << "Found " + entry.path().string() << std::endl;
			amc_files.push_back(entry.path().string());
		}
	}

	// check if skeleton file exist
	if (asf_file.empty()) {
		std::cout << "Error, No .asf File Found." << std::endl;
		abort();
	}

	// check if amc files exist
	if (amc_files.empty()) {
		std::cout << "Error, Not Enough .amc Files Found. (" << amc_files.size() << " files)" << std::endl;
		abort();
	}

	// get all .dist files in distance directory
	std::vector<std::string> dist_files;
	for (const auto& entry : std::filesystem::directory_iterator(distance_dir)) {
		if (entry.path().extension() == ".dist") {
			dist_files.push_back(entry.path().string());
		}
	}

	// generate skeleton
	Skeleton* skeleton = new Skeleton(asf_file);

	// for all combinations of motion (M X M), if distance matrix exists, load it, otherwise, generate it
	// sort the filenames to ensure that [MOTION_1] < [MOTION_2] for consistency
	std::sort(amc_files.begin(), amc_files.end());

	std::map<std::tuple<int, int>, std::vector<std::vector<float>>> distance_mats;
	for (int i = 0; i < amc_files.size(); i++) {
		for (int j = i; j < amc_files.size(); j++) {		// for all combinations
			// get amc file
			auto amc_file1 = amc_files[i];
			auto amc_file2 = amc_files[j];

			// get amc id
			auto amc_id1 = amc_file1.substr(amc_file1.size() - 6, 2);
			auto amc_id2 = amc_file2.substr(amc_file2.size() - 6, 2);

			// generate distance filenames from [MOTION 1]_[MOTION_2]_w[WINDOW_SIZE]_s[STEP_SIZE]
			std::string distname = amc_id1 + "_" + amc_id2 + "_w" + std::to_string(WINDOW_SIZE) + "_s" + std::to_string(STEP_SIZE) + ".dist";

			std::string dist_path = distance_dir + distname;

			// make a pair of int denoting the first and second animation id
			auto animation_pair = std::make_tuple(std::stoi(amc_id1), std::stoi(amc_id2));

			// search of dist_path in dist_files. Load it if it exists, generate it if otherwise
			if (std::find(dist_files.begin(), dist_files.end(), dist_path) != dist_files.end()) {
				std::cout << "Loading " << dist_path << std::endl;

				distance_mats[animation_pair] = loadDistanceFromFile(dist_path);
			}
			else {
				std::cout << "Generating " << dist_path << std::endl;
				Animation A1(skeleton, amc_file1);
				Animation A2(skeleton, amc_file2);
				Distance distance_obj(&A1, &A2, WINDOW_SIZE);

				auto distance_mat = distance_obj.distance(STEP_SIZE);

				saveDistanceToFile(distance_mat, dist_path);

				distance_mats[animation_pair] = distance_mat;
			}
		}
	}

	// get all edges from all distance matrix
	std::cout << "Generating Edges" << std::endl;
	std::vector<std::tuple<std::tuple<int, int>, std::tuple<int, int>>> edges;
	for (auto const& [key, val] : distance_mats) {
		auto localminima = LocalMin::localMinima(val, THRESHOLD, STEP_SIZE);
		for (auto lm : localminima) {
			std::tuple<int, int> node1 = std::make_tuple(get<0>(key), get<0>(lm));
			std::tuple<int, int> node2 = std::make_tuple(get<1>(key), get<1>(lm));
			edges.push_back(std::make_tuple(node1, node2));
		}
	}

	// load all animations
	std::cout << "Creating All Animations" << std::endl;
	std::vector<Animation*> animations;
	for (auto amc_file : amc_files) {
		animations.push_back(new Animation(skeleton, amc_file));
	}

	// generate the graph using all local minimums
	std::cout << "Creating The Graph" << std::endl;
	Graph graph(skeleton, animations, edges, WINDOW_SIZE);

	// print graph
	//std::cout << "Start Printing Graph" << std::endl;
	//std::cout << graph.toString() << std::endl;
	//std::cout << "End Printing Graph" << std::endl;

	return graph;
}



