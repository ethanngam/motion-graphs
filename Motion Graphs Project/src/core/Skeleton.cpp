#include <core/Skeleton.h>

Skeleton::Skeleton(std::string filepath) {


    // ========== Parse the asf file ==========

    // open file
    std::string data;
    std::ifstream f(filepath);
    std::string line;

    if (!f.is_open()) {
        perror(("Error while opening file " + filepath).c_str());
        abort();
    }

    std::vector<std::string> bonedata = {};             // each line correspoding to a bone
    std::vector<std::vector<std::string>> hierarchy;    // the hierarchy of bones

    // Read the file
    // 1. Store the bonedata lines into bonedata
    // 2. Store the hierarchy lines into hierarchy
    bool hierarchy_flag = false;
    while (getline(f, line)) {

        std::vector<std::string> line_tokens;
        boost::split(line_tokens, line, [](char c) {return c == ' '; });

        std::erase_if(line_tokens, [](const auto& s) {
            return s.find_first_not_of(" ") == std::string::npos;
            });

        if (line_tokens[0] == ":hierarchy") {
            hierarchy_flag = true;
        }

        if (hierarchy_flag == false) {
            bonedata.insert(bonedata.end(), line_tokens.begin(), line_tokens.end());
        }
        else if (hierarchy_flag == true) {
            hierarchy.push_back(line_tokens);
        }

    }

    // Create first root bone
    Bone::BoneStruct root = clearBoneStruct();
    root.id = 0;
    root.name = "root";
    root.dof["rx"] = true;
    root.dof["ry"] = true;
    root.dof["rz"] = true;
    bonepile[root.name] = new Bone(root);
    bonenames.push_back("root");

    // Read the lines in bone data and construct the bone
    bool build_flag = false;
    Bone::BoneStruct bone;
    for (unsigned int i = 0; i < bonedata.size(); i++) {
        if (bonedata[i] == "begin") {
            bone = clearBoneStruct();
            build_flag = true;
        }
        else if (bonedata[i] == "end") {
            bonepile[bone.name] = new Bone(bone);
            bonenames.push_back(bone.name);
            build_flag = false;
        }
        else if (build_flag == true) {
            if (bonedata[i] == "id") {
                bone.id = stoi(bonedata[i + 1]);
            }
            else if (bonedata[i] == "name") {
                bone.name = bonedata[i + 1];
            }
            else if (bonedata[i] == "direction") {
                bone.direction[0] = stof(bonedata[i + 1]);
                bone.direction[1] = stof(bonedata[i + 2]);
                bone.direction[2] = stof(bonedata[i + 3]);
            }
            else if (bonedata[i] == "length") {
                bone.length = stof(bonedata[i + 1]);
            }
            else if (bonedata[i] == "axis") {
                bone.axis[0] = stof(bonedata[i + 1]);
                bone.axis[1] = stof(bonedata[i + 2]);
                bone.axis[2] = stof(bonedata[i + 3]);
            }
            else if (bonedata[i] == "dof") {
                for (unsigned int j = 1; j < 4; j++) {
                    if (bonedata[i + j] == "rx") {
                        bone.dof["rx"] = true;
                    }
                    else if (bonedata[i + j] == "ry") {
                        bone.dof["ry"] = true;
                    }
                    else if (bonedata[i + j] == "rz") {
                        bone.dof["rz"] = true;
                    }
                }
            }
        }
    }

    // Read the hierarchy data and build the hierarchy
    build_flag = false;
    for (int i = 0; i < hierarchy.size(); i++) {

        std::string tkn = hierarchy[i][0];
        if (tkn == "begin") {
            build_flag = true;
            continue;
        }
        else if (tkn == "end") {
            build_flag = false;
            continue;
        }
        else if (build_flag == true) {
            Bone* parent_node = getBoneByName(tkn);
            for (int j = 1; j < hierarchy[i].size(); j++) {
                Bone* child_node = getBoneByName(hierarchy[i][j]);
                parent_node->children.push_back(child_node);
                child_node->parent = parent_node;
            }
        }
    }

    // Close the file
    f.close();
}

// Clear out data a bone struct
Bone::BoneStruct Skeleton::clearBoneStruct() {
    Bone::BoneStruct bone;

    bone.id = -1;
    bone.name = "None";
    bone.direction = { 0,0,0 };
    bone.length = 0;
    bone.axis = { 0,0,0 };
    bone.dof["rx"] = false;
    bone.dof["ry"] = false;
    bone.dof["rz"] = false;

    return bone;
}

Bone* Skeleton::getBoneByName(std::string bone_name) const {
    return bonepile.at(bone_name);
}

std::vector<std::string> Skeleton::getBonenames() {
    return bonenames;
}

int Skeleton::getBoneCount() const {
    return bonenames.size();
}

std::string Skeleton::getBonenameByIndex(int index) const {
    return bonenames[index];
}






