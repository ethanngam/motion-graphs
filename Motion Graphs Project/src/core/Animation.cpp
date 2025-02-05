#include <core/Animation.h>

Animation::Animation(Skeleton* _skeleton, std::string amcpath) {
    skeleton = _skeleton;

    // get the id from the amc filename
    id = std::stoi(amcpath.substr(amcpath.size() - 6, 2));

    // read the file
    std::string data;
    std::ifstream f(amcpath);
    std::string line;

    // check if file is opened successfully
    if (!f.is_open()) {
        perror(("error while opening file " + amcpath).c_str());
    }

    // ========== Read the AMC File ==========
    bool build_flag = false;
    while (getline(f, line)) {
        char* char_after_number;
        long frame_number = strtol(line.c_str(), &char_after_number, 10);

        // if the line contains a number only, create a new frame, enable the build flag
        if (!*char_after_number) {
            Frame new_frame;
            frame.push_back(new_frame);
            build_flag = true;
            continue;
        }
        else if (build_flag == true) {
            // tokenise the line
            std::vector<std::string> line_tokens;
            boost::split(line_tokens, line, [](char c) {return c == ' '; });
            std::erase_if(line_tokens, [](const auto& s) {
                return s.find_first_not_of(" ") == std::string::npos;
                });

            std::string bone_name = line_tokens.at(0);

            Bone* bone = skeleton->getBoneByName(bone_name);

            if (bone_name == "root") {
                glm::vec3 position = glm::vec3(
                    strtof(line_tokens.at(1).c_str(), NULL),
                    strtof(line_tokens.at(2).c_str(), NULL),
                    strtof(line_tokens.at(3).c_str(), NULL)
                );

                frame.back().pos = glm::translate(glm::mat4(1.0f), position);
                line_tokens.erase(line_tokens.begin(), line_tokens.begin() + 3);
            }

            // read dof
            std::array<float, 3> data = { 0,0,0 };

            int j = 1;
            std::string key;
            for (int i = 0; i < 3; i++) {
                if (i == 0) { key = "rx"; }
                else if (i == 1) { key = "ry"; }
                else if (i == 2) { key = "rz"; }

                // if the bone has the corresponding dof, read the data
                if (bone->dof[key] == true) {
                    data[i] = strtof(line_tokens.at(j).c_str(), NULL);
                    j++;
                }
            }

            // store a quaternions
            frame.back().pose[bone_name] = glm::quat(glm::radians(glm::vec3(data[0], data[1], data[2])));
        }
    }

    // close the file
    f.close();
}

Animation::Animation(Skeleton* _skeleton, std::vector<Frame> frames) {
    skeleton = _skeleton;
    frame = frames;
}

// Use DFS down the bone hierarchy to get all vertices
std::vector<float> Animation::getVertices(glm::vec3* ltoe, glm::vec3* rtoe) {
    vertices.clear();

    std::stack<Bone*> sStack;
    sStack.push(skeleton->getBoneByName("root"));

    Bone* current_node;
    Bone* child_node;
    while (sStack.empty() == false) {
        current_node = sStack.top();
        sStack.pop();

        // Add the child of the node to the stack
        for (int i = 0; i < current_node->children.size(); i++) {
            child_node = current_node->children[i];
            sStack.push(child_node);
        }

        // Push the coordinates to vertices
        calculateBoneCoord(current_node);
        if (current_node->name != "root") {
            vertices.push_back(current_node->str_pos[0]);
            vertices.push_back(current_node->str_pos[1]);
            vertices.push_back(current_node->str_pos[2]);
            vertices.push_back(current_node->end_pos[0]);
            vertices.push_back(current_node->end_pos[1]);
            vertices.push_back(current_node->end_pos[2]);
        }

        if (rtoe && current_node->name == "rtoes") {
            *rtoe = glm::vec3(current_node->str_pos);
        }
        else if (ltoe && current_node->name == "ltoes") {
            *ltoe = glm::vec3(current_node->str_pos);
        }
    }

    return vertices;
}

// Calculate the coordinates using the global transform
void Animation::calculateBoneCoord(Bone* bone) {
    if (bone->name == "root") {
        bone->str_pos = { 0,0,0 };
        bone->end_pos = { 0,0,0 };
        return;
    }

    glm::vec4 start_pos = { 0.0f, 0.0f, 0.0f, 1.0f };
    glm::vec4 end_pos = bone->Offset * start_pos;

    glm::mat4 G = calculateGlobalTransform(bone);

    start_pos = G * start_pos;
    end_pos = G * end_pos;

    bone->str_pos = start_pos;
    bone->end_pos = end_pos;
}

// Calculate the global transform by going up the hierarchy,
// and multplying the local transforms
glm::mat4 Animation::calculateGlobalTransform(Bone* bone) {
    if (bone->name == "root") {
        return glm::mat4(1.0f);
    }

    glm::mat4 GlobalTransform = glm::mat4(1.0f);
    while (bone->parent != NULL) {
        GlobalTransform = (bone->LocalTransform) * GlobalTransform;
        bone = bone->parent;
    }

    // Multiply with the skeleton's global transform
    GlobalTransform = skeleton->getBoneByName("root")->LocalTransform * GlobalTransform;

    return GlobalTransform;
}

void Animation::calculateFrame(Frame f) {

    // Calculate the local transform for all bones via
    // Local Transform = Parent Offet * Axis * Rotation * AxisInv
    for (int i = 0; i < skeleton->getBoneCount(); i++) {
        std::string bone_name = skeleton->getBonenameByIndex(i);
        Bone* bone = skeleton->getBoneByName(bone_name);

        glm::mat4 Offset = glm::mat4(1.0f);
        if (bone_name == "root") {
            bone->LocalTransform = f.pos * glm::toMat4(bone->Axis * f.pose[bone_name] * bone->AxisInv);
        }
        else {
            bone->LocalTransform = bone->parent->Offset * glm::toMat4(bone->Axis * f.pose[bone_name] * bone->AxisInv);
        }
    }
}

void Animation::calculateFrame(int frame_num) {
    if (frame_num >= frame.size()) {
        return;
    }

    calculateFrame(frame.at(frame_num));
}

void Animation::normaliseTransform(glm::mat4& current_pos, glm::quat& current_rot, glm::mat4 starting_pos, glm::quat starting_rot) {
    // find yaw
    starting_rot.x = 0;
    starting_rot.z = 0;
    starting_rot = glm::normalize(starting_rot);
    
    // translate position
    current_pos = glm::inverse(starting_pos) * current_pos;

    // normalise rotation
    current_rot = glm::inverse(starting_rot) * current_rot;
    current_pos[3] = glm::toMat4(glm::inverse(starting_rot)) * current_pos[3];
}

void Animation::unNormaliseTransform(glm::mat4& current_pos, glm::quat& current_rot, glm::mat4 starting_pos, glm::quat starting_rot) {
    // find yaw
    starting_rot.x = 0;
    starting_rot.z = 0;
    starting_rot = glm::normalize(starting_rot);
    
    // normalise rotation
    current_rot = starting_rot * current_rot;
    current_pos[3] = glm::toMat4(starting_rot) * current_pos[3];

    // translate position
    current_pos = starting_pos * current_pos;
}

void Animation::normaliseFrame(Frame& frame, glm::mat4 starting_pos, glm::quat starting_rot) {
    normaliseTransform(frame.pos, frame.pose["root"], starting_pos, starting_rot);
}

void Animation::unNormaliseFrame(Frame& frame, glm::mat4 starting_pos, glm::quat starting_rot) {
    unNormaliseTransform(frame.pos, frame.pose["root"], starting_pos, starting_rot);
}

void Animation::calculateFrameWithNormalisation(int frame_num, glm::mat4 starting_pos, glm::quat starting_rot) {
    Frame new_frame = Frame(frame[frame_num]);
    normaliseFrame(new_frame, starting_pos, starting_rot);
    calculateFrame(new_frame);
}

int Animation::getID() const {
    return id;
}

int Animation::getFrameSize() const {
    return frame.size();
}

// get copy of a frame
Animation::Frame Animation::getFrame(int indexID) const {
    Frame new_frame(frame.at(indexID));     // to ensure that the frame is a copy
    return new_frame;
}

// get frame position
glm::mat4 Animation::getFramePos(int indexID) const {
    return frame.at(indexID).pos;
}

// get frame rotation
glm::quat Animation::getFrameRot(int indexID) const {
    return frame.at(indexID).pose.at("root");
}

std::string Animation::frameToString(Frame& frame) {
    return "Animation::frameToString() Not Implemented!";
}
