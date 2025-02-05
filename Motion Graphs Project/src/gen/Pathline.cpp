#include <gen/Pathline.h>

std::vector<float> Pathline::loadFromDir(float segmentLength, glm::vec3 origin) {
	// Setup path
	std::string pathDir = "data/paths/";

	// get all files in mocap directory
	std::vector<std::string> pathFiles;
	for (const auto& entry : std::filesystem::directory_iterator(pathDir)) {
		// get all path file
		if (entry.path().extension() == ".path") {
			pathFiles.push_back(entry.path().string());
		}
	}

	// Let user select path
	int index = 1;
	std::cout << "Select Path File" << std::endl;
	for (auto const& filepath : pathFiles) {
		std::cout << index << ". " << filepath << std::endl;
		index++;
	}

	std::string input;
	std::cin >> input;
	int selection = std::stoi(input);

	const auto& file = pathFiles[selection - 1];
	return loadFromFile(file, segmentLength, origin);
}

std::vector<float> Pathline::loadFromFile(std::string filepath, float segmentLength, glm::vec3 origin) {

	// Open file
	std::ifstream infile(filepath);

	if (!infile.is_open()) {
		perror(("error while opening file " + filepath).c_str());
		abort();
	}

	// Read file into pathline
	std::vector<glm::vec3> pathline;

	for (std::string line; getline(infile, line); ) {

		std::stringstream ss(line);
		std::string f1;
		std::string f2;
		ss >> f1;
		ss >> f2;

		const float x = std::stof(f1);
		const float y = std::stof(f2);

		pathline.push_back(glm::vec3(x, 0, y));
	}

	auto evenPathline = makeEven(pathline, segmentLength);

	// Translate path so that it starts at the origin
	glm::vec3 shiftToOrigin = origin - evenPathline[0];
	for (auto& point : evenPathline) {
		point += shiftToOrigin;
	}

	// Convert to buffer form
	std::vector<float> bufferline;
	bufferline.push_back(evenPathline[0][0]);
	bufferline.push_back(evenPathline[0][1]);
	bufferline.push_back(evenPathline[0][2]);
	for (auto& point : evenPathline) {
		bufferline.push_back(point[0]);
		bufferline.push_back(point[1]);
		bufferline.push_back(point[2]);
		bufferline.push_back(point[0]);
		bufferline.push_back(point[1]);
		bufferline.push_back(point[2]);
	}

	return bufferline;
}

std::vector<std::string> Pathline::searchDirectory() {
	// Setup path
	std::string pathDir = "data/paths/";

	// get all files in mocap directory
	std::vector<std::string> pathFiles;
	for (const auto& entry : std::filesystem::directory_iterator(pathDir)) {
		// get all path file
		if (entry.path().extension() == ".path") {
			pathFiles.push_back(entry.path().string());
		}
	}

	return pathFiles;
}

std::vector<glm::vec3> Pathline::makeEven(const std::vector<glm::vec3>& pathline, float segmentLength) {
	std::vector<glm::vec3> stdPath;
	stdPath.push_back(pathline[0]);

	float distRemaining = segmentLength;
	glm::vec3 P = pathline[0];
	glm::vec3 S = pathline[0];
	glm::vec3 E = pathline[1];

	int index = 0;
	while (true) {
		if (glm::distance(P, E) >= distRemaining) {
			glm::vec3 SE = glm::normalize(E - S);
			P = P + SE * distRemaining;
			stdPath.push_back(P);
			distRemaining = segmentLength;
		}
		else {
			distRemaining -= glm::distance(P, E);
			assert(distRemaining >= 0);

			index++;

			if (index + 1 >= pathline.size()) {
				break;
			}

			P = pathline[index];
			S = pathline[index];
			E = pathline[index + 1];
		}
	}

	return stdPath;
}