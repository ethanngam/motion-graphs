#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb-master/stb_image.h>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <core/Animation.h>
#include <core/Skeleton.h>
#include <core/Camera.h>
#include <core/Shader.h>
#include <gen/Pipeline.h>
#include <gen/Graph.h>
#include <gen/Pathline.h>
#include <mogen/KovarMG.h>
#include <mogen/RandomMG.h>
#include <mogen/MotionGenerator.h>

#include <iostream>
#include <fstream> 
#include <vector>
#include <stack>
#include <chrono>
#include <thread>
#include <algorithm>
#include <Windows.h>
#include <filesystem>
#include <limits>

// mode to determine what process to run
enum Mode { PLAY_ANIMATION, PLAY_GRAPH };

// function declarations
int init();
std::map<std::string, int> loadGraphConfig(std::string config_path);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void processInput(GLFWwindow* window);
int playGraph(MotionGenerator& motionGenerator);
int playAnimation(Animation& animation);
void reset(bool ignorePathline = false);

// settings
const int scale = 1;
const int SCR_WIDTH = 800 * scale;
const int SCR_HEIGHT = 600 * scale;
const int PANEL_WIDTH = 200 * scale;
const int TOOLBAR_HEIGHT = 20 * scale;
const int MAX_FPS = 600.0;
const int MIN_FPS = 1.0;
const float CAMERA_SENSITIVITY = 0.1f;
const float PATH_RADIUS = 1.0;          // how far away to place new point for path

// playback attributes
bool play = true;
bool draw_mode = false;
bool cursor_lock = false;
static int animation_fps = 120;
int current_frame = 0;
int increment_frame = 0;
bool normaliseY = true;
bool isRunning = false;

// input attributes
float lastX = 400;
float lastY = 300;
std::map<int, int> prev_keystates;
std::vector<std::string> pathfiles = Pathline::searchDirectory();

// global attributes
GLFWwindow* window;
Shader* skeletonShader;
Shader* floorShader;

// drawing attributes
std::vector<float> pathline;
std::vector<float> rootline;

// buffers
unsigned int framebuffer;
unsigned int textureColorbuffer;
unsigned int floor_texture;
unsigned int floorVBO, flootVAO;
unsigned int avatarVBO, avatarVAO;
unsigned int shadowVBO, shadowVAO;
unsigned int rootlineVBO, rootlineVAO;
unsigned int pathlineVBO, pathlineVAO;

// others
glm::vec3 root_pos;
float ortho_width = 160;
float ortho_height = ortho_width * SCR_HEIGHT / SCR_WIDTH;
MotionGenerator* MoGen;

// camera object
Camera camera(
    glm::vec3(50, 15, 50),
    glm::vec3(0, 5, 0),
    glm::vec3(0, 1, 0),
    1.0f);

int main(int argc, char* argv[]) {
    std::cout << "Select Mode:" << std::endl;
    std::cout << "1. Play Animation" << std::endl;
    std::cout << "2. Play Motion Graph (Recommended)" << std::endl;

    int mode;
    std::cin >> mode;
    mode -= 1;

    int graphType;
    if (mode == PLAY_GRAPH) {
        std::cout << "Select Graph Type" << std::endl;
        std::cout << "1. Random Graph Walk" << std::endl;
        std::cout << "2. Kovar Graph Walk (Recommended)" << std::endl;

        std::cin >> graphType;
    }

    // initialise GLFW, GLAD, and Scene
    if (init() == -1) {
        std::cout << "Error in Initialising OpenGL" << std::endl;
        abort();
    }

    if (mode == PLAY_ANIMATION) {
        // subject 91
        Skeleton skeleton("data/mocap/91.asx");
        Animation animation(&skeleton, "data/mocap/91_01.amc");

        playAnimation(animation);
    }
    else if (mode == PLAY_GRAPH) {

        // Subject 91
        auto config = loadGraphConfig("data/graphs/graph91/config.txt");
        Graph graph = Pipeline::genGraph(config["window_size"], config["threshold"], config["step_size"], "data/graphs/graph91/");

        if (graphType == 1) {
            RandomMG randomMG = RandomMG(&graph);
            MoGen = &randomMG;
            playGraph(randomMG);
        }
        else if (graphType == 2) {
            KovarMG kovarMG = KovarMG(&graph);
            MoGen = &kovarMG;
            playGraph(kovarMG);
        }
        else {
            std::cout << "Error, Not a Valid Mode!" << std::endl;
            abort();
        }
    }
    else {
        std::cout << "Error, Not a Valid Mode!" << std::endl;
        abort();
    }
}

int init() {
    // glfw initialization and configuration
    glfwInit();

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    window = glfwCreateWindow(SCR_WIDTH + PANEL_WIDTH, SCR_HEIGHT + TOOLBAR_HEIGHT, "Motion Player", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // glad load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // set callbacks for the window
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // settings
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // setup shaders
    skeletonShader = new Shader("skeletonShader.vs", "skeletonShader.fs");
    floorShader = new Shader("floorShader.vs", "floorShader.fs");


    // ========== Setup FrameBufferObject ==========

    // initialise FrameBuffer
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // generate texture to store the pixel colours
    glGenTextures(1, &textureColorbuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // attach it to currently bound framebuffer object
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);

    // generate renderBufferObject for depth testing
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // attach it to currently bound framebuffer object
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    // check framebuffer status
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
        return -1;
    }

    // unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // ========== Setup Floor ==========

    // set up floor vertices
    float floor[] = {
         500.0,  -0.1,  500.0,   25.0,  25.0,
         500.0,  -0.1, -500.0,   25.0, -25.0,
        -500.0,  -0.1, -500.0,  -25.0, -25.0,
         500.0,  -0.1,  500.0,   25.0,  25.0,
        -500.0,  -0.1,  500.0,  -25.0,  25.0,
        -500.0,  -0.1, -500.0,  -25.0, -25.0
    };

    // set up floor VAOs, VBO, and Attribute Pointers
    glGenVertexArrays(1, &flootVAO);
    glGenBuffers(1, &floorVBO);

    // bind and set floor vertices
    glBindVertexArray(flootVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floor), floor, GL_STATIC_DRAW);

    // set floor vertex attribute pointer
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // set texture attribute pointer
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // setup texture
    glGenTextures(1, &floor_texture);
    glBindTexture(GL_TEXTURE_2D, floor_texture);

    // load and generate the texture
    int width, height, nrChannels;
    unsigned char* data = stbi_load("data/textures/checker.png", &width, &height, &nrChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);  // free data in memory
    }
    else {
        std::cout << "Failed to load texture" << std::endl;
        return -1;
    }

    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // unbind VBO and VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    // ========== Initialise other VAOs and VBOs ==========

    // generate avatar VAOs and VBOs
    glGenVertexArrays(1, &avatarVAO);
    glGenBuffers(1, &avatarVBO);

    // generate shadow VAOs and VBOs
    glGenVertexArrays(1, &shadowVAO);
    glGenBuffers(1, &shadowVBO);

    // generate rootline VAOs and VBOs
    glGenVertexArrays(1, &rootlineVAO);
    glGenBuffers(1, &rootlineVBO);

    // generate pathline VAOs and VBOs
    glGenVertexArrays(1, &pathlineVAO);
    glGenBuffers(1, &pathlineVBO);

    return 0;
}

int playAnimation(Animation &animation) {

    // avatar vertices
    std::vector<float> vertices;
    cursor_lock = true;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    while (!glfwWindowShouldClose(window)) {

        // start timer
        std::chrono::system_clock::time_point frame_start_time = std::chrono::system_clock::now();

        // play pause function
        if (play == true) {
            current_frame++;
        }
        else { 
            // if paused, pause the frame.
            current_frame += increment_frame;   // add step for frame by frame playback
            increment_frame = 0;
        }

        // if end of animation, loop back to start
        if (current_frame >= animation.getFrameSize()) {
            current_frame = 0;
        }

        // calculate frame and get vertices
        animation.calculateFrame(current_frame);
        vertices = animation.getVertices();

        // bind and set data in buffer
        glBindVertexArray(avatarVAO);
        glBindBuffer(GL_ARRAY_BUFFER, avatarVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_DYNAMIC_DRAW);

        // set attribute pointers
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        /* For Debug: to get data in buffer */
        GLint bufferSize;
        float myBuffer[300];
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
        glGetBufferSubData(GL_ARRAY_BUFFER, 0, bufferSize, myBuffer);
        std::vector<float> x(myBuffer, myBuffer + bufferSize / sizeof(float));
        /**/

        // unbind buffer
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // set up MVP
        glm::mat4 Projection = glm::perspective(glm::radians(45.0f), (float)(SCR_WIDTH + PANEL_WIDTH) / (float)(SCR_HEIGHT + TOOLBAR_HEIGHT), 0.1f, 500.0f);
        glm::mat4 View = camera.generateView();
        glm::mat4 Model = glm::mat4(1.0f);
        glm::mat4 MVP = Projection * View * Model;


        // ========== Draw Scene ==========

        // set clear colour
        glClearColor(33.0f / 255, 158.0f / 255, 188.0f / 255, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw floor
        floorShader->setMat4("MVP", MVP);
        floorShader->use();
        glBindTexture(GL_TEXTURE_2D, floor_texture);
        glBindVertexArray(flootVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // draw avatar
        skeletonShader->setMat4("MVP", MVP);
        skeletonShader->use();
        glBindVertexArray(avatarVAO);
        glDrawArrays(GL_LINES, 0, vertices.size() / 3);

        // process inputs
        processInput(window);
        camera.processInput(window);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();

        // stop timer and calculate time elapsed
        std::chrono::system_clock::time_point frame_end_time = std::chrono::system_clock::now();
        std::chrono::duration<double, std::milli> frame_delta = frame_end_time - frame_start_time;

        // introduct sleep to control fps
        double frame_time = 1000.0 / animation_fps;
        if (frame_delta.count() < frame_time) {
            std::chrono::duration<double, std::milli> delta_ms(frame_time - frame_delta.count());
            auto delta_ms_duration = std::chrono::duration_cast<std::chrono::milliseconds>(delta_ms);
            std::this_thread::sleep_for(std::chrono::milliseconds(delta_ms_duration.count()));
        }
    }
    glfwTerminate();
    return 0;
}

int playGraph(MotionGenerator& motionGenerator) {
    
    int shadow_spacing = 120 * 6;
    bool transition_mode = false;
    std::vector<float> vertices;
    std::vector<float> shadow;
    while (!glfwWindowShouldClose(window)) {
        std::chrono::system_clock::time_point frame_start_time = std::chrono::system_clock::now();

        if (play == true) {
            bool completed = false;
            vertices = motionGenerator.getNextFrame(&completed);

            // global variable to draw rootline
            root_pos = motionGenerator.getRoot();
            
            // push root pos into rootline
            if (rootline.size() == 0) {
                // initialise
                rootline.push_back(root_pos[0]);
                rootline.push_back(0);
                rootline.push_back(root_pos[2]);
            }
            else {
                // push last 3 elements onto rootline
                // keep in mind that rootlint.size() grows while adding elements
                rootline.push_back(rootline[rootline.size() - 3]);
                rootline.push_back(rootline[rootline.size() - 3]);
                rootline.push_back(rootline[rootline.size() - 3]);
            }

            rootline.push_back(root_pos[0]);
            rootline.push_back(0);
            rootline.push_back(root_pos[2]);

            // clear root line if the character has completed the path
            if (completed) {
                //play = false;
                rootline.clear();
            }

            if (false && current_frame % shadow_spacing == 0) {

                // get minimum height
                float min = 100000000.0;
                for (int i = 1; i < vertices.size(); i += 3) {
                    const auto& vertex = vertices[i];
                    min = vertex < min ? vertex : min;
                }
                
                // append accounting for minimum height
                for (int i = 0; i < vertices.size(); i++) {
                    auto vertex = vertices[i];

                    if (i % 3 == 1) {
                        vertex -= min;
                    }

                    shadow.push_back(vertex);
                }
            }
            current_frame++;
            
        }

        // get skeleton colour
        //glm::vec4 colour = motionGenerator.getColour();
        glm::vec4 colour(1.0f, 0.0f, 0.0f, 1.0f);

        // bind and set avatar buffer data
        glBindVertexArray(avatarVAO);
        glBindBuffer(GL_ARRAY_BUFFER, avatarVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // bind and set avatar buffer data
        glBindVertexArray(shadowVAO);
        glBindBuffer(GL_ARRAY_BUFFER, shadowVBO);
        glBufferData(GL_ARRAY_BUFFER, shadow.size() * sizeof(float), &shadow[0], GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        /* For Debug: to get data from buffer 
        GLint bufferSize;
        float myBuffer[300];
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
        glGetBufferSubData(GL_ARRAY_BUFFER, 0, bufferSize, myBuffer);
        std::vector<float> x(myBuffer, myBuffer + bufferSize / sizeof(float));
        /**/

        // unbind buffer
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // generate rootline
        // bind and set data and pointers
        if (!rootline.empty()) {
            glBindVertexArray(rootlineVAO);
            glBindBuffer(GL_ARRAY_BUFFER, rootlineVBO);
            glBufferData(GL_ARRAY_BUFFER, rootline.size() * sizeof(float), &rootline[0], GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
        }

        // unbind buffer
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // generate pathline
        // bind and set data and pointers
        if (pathline.size() > 0) {
            glBindVertexArray(pathlineVAO);
            glBindBuffer(GL_ARRAY_BUFFER, pathlineVBO);
            glBufferData(GL_ARRAY_BUFFER, pathline.size() * sizeof(float), &pathline[0], GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
        }

        // unbind buffer
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // get the smallest y value
        float minY = std::numeric_limits<float>::infinity();
        for (int i = 0; i + 3 < vertices.size(); i += 3) {
            const auto y = vertices[i + 1];

            if (y < minY) {
                minY = y;
            }
        }

        // set up MVP for everything
        glm::mat4 mvp;
        if (draw_mode == false) {       // use persepective camera
            glm::mat4 Projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 5000.0f);
            glm::mat4 View = camera.generateView();
            glm::mat4 Model = glm::mat4(1.0f);

            mvp = Projection * View * Model;
        }
        else if (draw_mode == true) {   // use orthogonal camera looking down
            glm::mat4 Projection = glm::ortho(-ortho_width/2, ortho_width/2, -ortho_height/2, ortho_height/2, 0.1f, 5000.0f);
            glm::mat4 View = glm::lookAt(glm::vec3(0, 100, 0), glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));
            glm::mat4 Model = glm::mat4(1.0f);

            mvp = Projection * View * Model;
        }

        // set up MVP for skeleton
        glm::mat4 mvpSkeleton;
        if (draw_mode == false) {       // use persepective camera
            glm::mat4 Projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 5000.0f);
            glm::mat4 View = camera.generateView();
            glm::mat4 Model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -minY, 0.0f));    // to normalise y

            mvpSkeleton = Projection * View * Model;
        }
        else if (draw_mode == true) {   // use orthogonal camera looking down
            glm::mat4 Projection = glm::ortho(-ortho_width / 2, ortho_width / 2, -ortho_height / 2, ortho_height / 2, 0.1f, 5000.0f);
            glm::mat4 View = glm::lookAt(glm::vec3(0, 100, 0), glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));
            glm::mat4 Model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -minY, 0.0f));    // to normalise y

            mvpSkeleton = Projection * View * Model;
        }


        // ========== Draw Scene To Frame Buffer ==========
        
        // bind framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

        // set clear colour
        glClearColor(33.0f / 255, 158.0f / 255, 188.0f / 255, 1.0f);
        //glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw floor
        floorShader->use();
        floorShader->setMat4("MVP", mvp);
        glBindTexture(GL_TEXTURE_2D, floor_texture);
        glBindVertexArray(flootVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // draw avatar
        skeletonShader->use();
        if (normaliseY) {
            skeletonShader->setMat4("MVP", mvpSkeleton);
        }
        else {
            skeletonShader->setMat4("MVP", mvp);
        }
        skeletonShader->setVec4("color", colour);
        glBindVertexArray(avatarVAO);
        glDrawArrays(GL_LINES, 0, vertices.size() / 3);
        glDrawArrays(GL_POINTS, 0, vertices.size() / 3);

        if (!shadow.empty()) {
            skeletonShader->use();
            skeletonShader->setMat4("MVP", mvp);
            skeletonShader->setVec4("color", colour);
            glBindVertexArray(shadowVAO);
            glDrawArrays(GL_LINES, 0, shadow.size() / 3);
            glDrawArrays(GL_POINTS, 0, shadow.size() / 3);
        }

        // draw rootline
        skeletonShader->use();
        skeletonShader->setMat4("MVP", mvp);
        skeletonShader->setVec4("color", colour);
        glBindVertexArray(rootlineVAO);
        glDrawArrays(GL_LINES, 0, rootline.size() / 3);

        // draw pathline
        skeletonShader->use();
        skeletonShader->setMat4("MVP", mvp);
        skeletonShader->setVec4("color", glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
        glBindVertexArray(pathlineVAO);
        glDrawArrays(GL_LINES, 0, pathline.size() / 3);


        // ========== Draw To Screen ==========

        // unbind framebuffer (bind to defualt)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, SCR_WIDTH + PANEL_WIDTH, SCR_HEIGHT);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // setup menu bar
        ImGui::BeginMainMenuBar();
        if (ImGui::BeginMenu("Path")) {
            // 1.1. Draw Path
            if (ImGui::MenuItem("Draw Path")) {
                std::cout << "PRESS";
                draw_mode = true;
                cursor_lock = false;
            }
            // 1.2. Load Path
            if (ImGui::BeginMenu("Load Path")) {
                for (const auto& file : pathfiles) {
                    if (ImGui::MenuItem(file.c_str())) {
                        reset();
                        glm::vec3 origin(root_pos[0], 0, root_pos[2]);
                        pathline = Pathline::loadFromFile(file, PATH_RADIUS, origin);
                    }
                }
                ImGui::EndMenu();
            }
            // 1.3. Reset Path
            if (ImGui::MenuItem("Reset Path")) {
                reset();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            // 2.1 Draw Mode Camera
            if (ImGui::MenuItem("Draw Mode")) {
                draw_mode = true;
                cursor_lock = false;
            }
            // 2.2 Normal Camera
            if (ImGui::MenuItem("Camera Mode")) {
                draw_mode = false;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();

        // Main OpenGL Scene Window
        bool isOpen = true;
        ImGui::Begin("Motion Graphs", &isOpen,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        {
            ImGui::SetWindowSize(ImVec2(SCR_WIDTH, SCR_HEIGHT));
            ImGui::SetWindowPos(ImVec2(0, TOOLBAR_HEIGHT));
            ImGui::Image((ImTextureID)textureColorbuffer, ImVec2(SCR_WIDTH, SCR_HEIGHT), ImVec2(0, 1), ImVec2(1, 0));
        }
        ImGui::End();

        // Sidebar Window
        bool runSearch = false;
        ImGui::Begin("Panel", &isOpen, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);
        {
            ImGui::SetWindowSize(ImVec2(PANEL_WIDTH, SCR_HEIGHT));
            ImGui::SetWindowPos(ImVec2(SCR_WIDTH, TOOLBAR_HEIGHT));
            ImGui::Text("======== Controls ========");
            ImGui::Text("Look Around    (right click)");
            ImGui::Text("Move Forward   (w)");
            ImGui::Text("Move Backward  (s)");
            ImGui::Text("Move Left      (a)");
            ImGui::Text("Move Right     (d)");
            ImGui::Text("Move Up        (shift)");
            ImGui::Text("Move Down      (ctrl)");
            ImGui::Text("");
            ImGui::Text("======= Play Back ========");
            // Play Pause
            if (play) { ImGui::Text("PLAY / Pause     (space)"); }
            else { ImGui::Text("Play / PAUSE     (space)"); }

            // Fps Slider
            ImGui::Text("FPS    ");
            ImGui::SameLine();
            ImGui::SliderInt("##fps", &animation_fps, 0, 240);

            // Y Normalisation Checkbox
            ImGui::Text("Normalise Y    ");
            ImGui::SameLine();
            ImGui::Checkbox("##norm", &normaliseY);
            
            ImGui::Text("");
            ImGui::Text("======= Draw Mode ========");
            ImGui::Text("Draw Mode      (/)");
            ImGui::Text("Draw Path      (left click)");
            ImGui::Text("Zoom           (scroll)");
            ImGui::Text("");
            ImGui::Text("========= Others =========");
            ImGui::Text("Quit           (esc)");
            ImGui::Text("");

            // Run Button
            if (pathline.empty()) {
                ImGui::BeginDisabled();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.0f, 0.0f, 1.0f));
                ImGui::SetWindowFontScale(3);
                ImGui::Button("RUN", ImVec2(180, 125));
                ImGui::SetWindowFontScale(1);
                ImGui::PopStyleColor();
                ImGui::EndDisabled();
                ImGui::Text("Please Draw/Load Path");
            }
            else {
                int progress = 0;
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.0f, 0.0f, 1.0f));
                ImGui::SetWindowFontScale(3);
                if (ImGui::Button("RUN", ImVec2(180, 125))) {
                    runSearch = true;
                }
                ImGui::SetWindowFontScale(1);
                ImGui::PopStyleColor();
                if (runSearch) {
                    ImGui::Text("Searching Path...");
                    ImGui::Text("Please Wait");
                }
                else {
                    ImGui::Text("Press RUN to Start\nGraph Search");
                }
            }
        }
        ImGui::End();

        // process input
        processInput(window);
        camera.processInput(window);

        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();

        // Search Path
        if (runSearch) {
            if (isRunning) {
                reset(true);
            }
            MoGen->setPath(pathline);
            isRunning = true;
            runSearch = false;
        }

        // stop timer and calculate time elapsed
        std::chrono::system_clock::time_point frame_end_time = std::chrono::system_clock::now();
        std::chrono::duration<double, std::milli> frame_delta = frame_end_time - frame_start_time;

        // introduce sleep to control fps
        double frame_time = 1000.0 / animation_fps;
        if (frame_delta.count() < frame_time) {
            std::chrono::duration<double, std::milli> delta_ms(frame_time - frame_delta.count());
            auto delta_ms_duration = std::chrono::duration_cast<std::chrono::milliseconds>(delta_ms);
            std::this_thread::sleep_for(std::chrono::milliseconds(delta_ms_duration.count()));
        }
    }

    // ImGui Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow* window)
{
    // ESC: quit the program
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // SPACE: toggle play/pause
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        // initiallise
        if (!prev_keystates.contains(GLFW_KEY_SPACE)) {
            prev_keystates[GLFW_KEY_SPACE] = GLFW_RELEASE;
        }

        if (prev_keystates[GLFW_KEY_SPACE] == GLFW_RELEASE) {
            prev_keystates[GLFW_KEY_SPACE] = GLFW_PRESS;
            play = !play;
        }
    }
    else {
        prev_keystates[GLFW_KEY_SPACE] = GLFW_RELEASE;
    }

    // SLASH: toggle camera mode
    if (glfwGetKey(window, GLFW_KEY_SLASH) == GLFW_PRESS) {
        // initiallise
        if (!prev_keystates.contains(GLFW_KEY_SLASH)) {
            prev_keystates[GLFW_KEY_SLASH] = GLFW_RELEASE;
        }

        if (prev_keystates[GLFW_KEY_SLASH] == GLFW_RELEASE) {
            prev_keystates[GLFW_KEY_SLASH] = GLFW_PRESS;
            draw_mode = !draw_mode;

            if (draw_mode) {
                cursor_lock = false;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            else {
                glm::vec3 target = glm::vec3(root_pos[0], root_pos[1], root_pos[2]);
                camera.lookAt(target);
            }

        }
    }
    else {
        prev_keystates[GLFW_KEY_SLASH] = GLFW_RELEASE;
    }

    // LEFT ARROW: decrement frame
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        increment_frame = -1;
    }

    // RIGHT ARROW: increment frame
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        increment_frame = 1;
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {

    float xoffset = lastX - xpos;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    // draw path
    if (draw_mode) {
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            auto debug = pathline.size();

            // clear pathline
            if (prev_keystates[GLFW_MOUSE_BUTTON_LEFT] == GLFW_RELEASE) {
                reset();
            }

            prev_keystates[GLFW_MOUSE_BUTTON_LEFT] = GLFW_PRESS;

            // [0, 1] interval mapping: x' = x * (b - a) + a; {0 <= x <= 1}
            float x_coord = (xpos / SCR_WIDTH) * ortho_width - ortho_width / 2;
            float y_coord = ((ypos - TOOLBAR_HEIGHT) / SCR_HEIGHT) * ortho_height - ortho_height / 2;
            y_coord *= -1;  // height is from top to bottom
            x_coord *= -1;  // height is from top to bottom

            // initialise
            if (pathline.empty()) {
                // line start
                pathline.push_back(root_pos[0]);
                pathline.push_back(0.0f);
                pathline.push_back(root_pos[2]);

                // line end
                pathline.push_back(root_pos[0]);
                pathline.push_back(0.0f);
                pathline.push_back(root_pos[2]);
                return;
            }

            glm::vec3 currPos = glm::vec3(x_coord, 0.0f, y_coord);
            glm::vec3 lastPos = glm::vec3(pathline[pathline.size() - 3], pathline[pathline.size() - 2], pathline[pathline.size() - 1]);

            // if more distance to last point more than path radius
            if (glm::distance(lastPos, currPos) >= PATH_RADIUS) {
                // construct a unit vector lastPos to currPos
                glm::vec3 SE = glm::normalize(currPos - lastPos);

                // place a point on S + SE * PATH_RADIUS
                glm::vec3 P = lastPos + SE * PATH_RADIUS;

                // push last 3 elements onto pathline
                // keep in mind that pathline.size() grows while adding elements
                pathline.push_back(pathline[pathline.size() - 3]);
                pathline.push_back(pathline[pathline.size() - 3]);
                pathline.push_back(pathline[pathline.size() - 3]);

                // line end
                pathline.push_back(P[0]);
                pathline.push_back(P[1]);
                pathline.push_back(P[2]);
            }
        }
        else {
            prev_keystates[GLFW_MOUSE_BUTTON_LEFT] = GLFW_RELEASE;
        }
    }
    // Move Camera Direction
    else if (cursor_lock) {
        xoffset *= CAMERA_SENSITIVITY;
        yoffset *= CAMERA_SENSITIVITY;
        camera.rotateYaw(xoffset);
        camera.rotatePitch(yoffset);
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    // LEFT MOUSE BUTTON: set pathline when released
    // the pathline is spaced evenly with a distance per point of PATH RADIUS
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        prev_keystates[GLFW_MOUSE_BUTTON_RIGHT] = GLFW_PRESS;

        if (!draw_mode) {
            cursor_lock = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    else {
        // initialise
        if (!prev_keystates.contains(GLFW_MOUSE_BUTTON_RIGHT)) {
            prev_keystates[GLFW_MOUSE_BUTTON_RIGHT] = GLFW_RELEASE;
        }

        if (prev_keystates[GLFW_MOUSE_BUTTON_RIGHT] == GLFW_PRESS) {
            prev_keystates[GLFW_MOUSE_BUTTON_RIGHT] = GLFW_RELEASE;
            cursor_lock = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

// zoom in or out
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (draw_mode) {
        ortho_width *= 1 - yoffset / 10;    // change by 10%, inverse yoffset
        ortho_height = ortho_width * SCR_HEIGHT / SCR_WIDTH;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Load Graph Configuration File
std::map<std::string, int> loadGraphConfig(std::string config_path) {

    std::ifstream f(config_path);
    
    // check if file is opened successfully
    if (!f.is_open()) {
        perror(("error while opening file " + config_path).c_str());
        abort();
    }

    // fill up config map
    std::map<std::string, int> config;
    std::string line;
    while (getline(f, line)) {
        std::vector<std::string> line_tokens;
        boost::split(line_tokens, line, [](char c) {return c == ' '; });

        config[line_tokens[0]] = std::stoi(line_tokens[1]);
    }

    return config;
}

// reset the state for motion graph
void reset(bool ignorePathline) {
    MoGen->reset();
    root_pos = MoGen->getRoot();
    isRunning = false;
    rootline.clear();

    if (!ignorePathline) {
        pathline.clear();
    }
};