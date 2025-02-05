#include <core/Bone.h>

Bone::Bone(BoneStruct bonedata) {
    id = bonedata.id;
    name = bonedata.name;
    dof = bonedata.dof;
    str_pos = bonedata.str_pos;
    end_pos = bonedata.end_pos;

    Offset = glm::translate(glm::mat4(1.0f), bonedata.length * bonedata.direction);
    Axis = glm::quat(glm::radians(glm::vec3(bonedata.axis[0], bonedata.axis[1], bonedata.axis[2])));
    AxisInv = glm::inverse(Axis);
    LocalTransform = glm::mat4(1.0f);
}
