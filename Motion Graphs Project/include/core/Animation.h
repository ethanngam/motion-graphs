#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream> 
#include <vector>
#include <stack>
#include <map>
#include <string>
#include <tuple>
#include <array>
#include <utility>

#include <core/Skeleton.h>
#include <core/Bone.h>

class Animation
{
public:

    // Struct to describe a frame
    struct Frame {
        std::map < std::string, glm::quat> pose;
        glm::mat4 pos = glm::mat4(1.0f);
    };

    // Load animation from amc file
    Animation(Skeleton* _skeleton, std::string amcpath);

    // Load animation from a vector of frames
    Animation(Skeleton* _skeleton, std::vector<Frame> frames);

    // Compute the local transforms for a frame
    void calculateFrame(int frame_num);
    void calculateFrame(Frame f);

    // Compute the local transforms for a frame with a given normalisation
    void calculateFrameWithNormalisation(int frame_num, glm::mat4 starting_pos, glm::quat starting_rot);

    // Compute the vertices for a frame
    std::vector<float> getVertices(glm::vec3* ltoe = NULL, glm::vec3* rtoe = NULL);

    // Normalise / unNormalise transforms
    static void normaliseTransform(glm::mat4& current_pos, glm::quat& current_rot, glm::mat4 starting_pos, glm::quat starting_rot);
    static void unNormaliseTransform(glm::mat4& current_pos, glm::quat& current_rot, glm::mat4 starting_pos, glm::quat starting_rot);

    // Normalise / unNormalise frame
    static void normaliseFrame(Frame& frame, glm::mat4 starting_pos, glm::quat starting_rot);
    static void unNormaliseFrame(Frame& frame, glm::mat4 starting_pos, glm::quat starting_rot);

    // Output frame as a string
    static std::string frameToString(Frame& frame);

    // Getters
    int getID() const;
    int getFrameSize() const;
    Frame getFrame(int indexID) const;
    glm::mat4 getFramePos(int indexID) const;
    glm::quat getFrameRot(int indexID) const;

    const Skeleton* getSkeleton() {
        return skeleton;
    }
    

private:
    int id;
    const Skeleton* skeleton;
    std::vector<float> vertices;
    std::vector<Frame> frame;

    glm::mat4 calculateGlobalTransform(Bone* bone);
    void calculateBoneCoord(Bone* bone);
};

