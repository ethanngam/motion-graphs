#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

#include <core/Bone.h>

#include <iostream>
#include <fstream> 
#include <vector>
#include <stack>
#include <map>
#include <string>
#include <tuple>
#include <array>
#include <utility>

class Skeleton
{
public:
    // Constructor
	Skeleton(std::string filepath);

    // Returns the bone via given a bonename
    Bone* getBoneByName(std::string bone_name) const;

    // Get all bonenames
    std::vector<std::string> getBonenames();

    // Get the number of bones
    int getBoneCount() const;

    // Get the bonename by index in bonepile
    std::string getBonenameByIndex(int index) const;

private:
    std::map<std::string, Bone*> bonepile;
    std::vector<std::string> bonenames;
    static Bone::BoneStruct clearBoneStruct();
};

