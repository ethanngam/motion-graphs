#pragma once

#include <string>
#include <vector>
#include <map>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

class Bone
{
public:
    int id;
    std::string name;
    std::map<std::string, bool> dof;
    glm::vec3 str_pos;
    glm::vec3 end_pos;
    Bone* parent;
    std::vector<Bone*> children;
    glm::mat4 Offset;
    glm::quat Axis;
    glm::quat AxisInv;
    glm::mat4 LocalTransform;

    struct BoneStruct {
        int id;
        std::string name;
        glm::vec3 direction;
        float length;
        glm::vec3 axis;
        std::map<std::string, bool> dof;
        glm::vec3 str_pos;
        glm::vec3 end_pos;
    };

    // Constructor
    Bone(BoneStruct bonedata);
};

