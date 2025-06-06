#ifndef ENGINE_H
#define ENGINE_H

#include <iostream>
#include <vector>
#include <memory>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "vec3.h"
#include "mat4.h"
#include "vec4.h"
#include "Particle.h"
#include <cmath>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

#ifdef WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

class Engine {
public:
    Engine(std::string dataFolder, double deltaTime, double numOfParticles, double numTimeSteps, std::vector<std::shared_ptr<Particle>>* particles);
    ~Engine();

    float particleAlpha = 0.2f;

    double deltaTime;
    double numOfParticles;
    double numTimeSteps;
    
    std::vector<std::shared_ptr<Particle>>* particles;

    std::string dataFolder;
    vec3 agColorMap(Particle* particle ,double densityAV);
    vec3 jetColorMap(Particle* particle ,double densityAV);
    bool init(double physicsFaktor);
    void start();
    void update(int index);
    bool clean();

    GLuint pbo = 0;
    std::queue<std::pair<std::string, std::vector<unsigned char>>> saveQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    std::thread saveThread;
    bool terminateThread = false;
    int width, height;

    void initializePBO();
    void saveWorker();

    void initializePBO(int width, int height);
    void saveAsPicture(const std::string& folderName, int index);

    static void window_iconify_callback(GLFWwindow* window, int iconified);
    bool RenderLive = true;
    std::string videoName;

    GLFWwindow* window;

    bool isRunning = false;

    int maxNumberOfParticles = 1e7;

    double playSpeed = 1;
    double changeSpeed = 1;


    //render Modes:
    int renderMode = 1;
    // all with viusalDensity = 1
    int ALL_WITH_VISUAL_DENSITY = 1;
    // only Stars and gas with viusalDensity = 2
    int STARS_AND_GAS_WITH_VISUAL_DENSITY = 2;
    // only Stars with viusalDensity = 3
    int STARS_WITH_VISUAL_DENSITY = 3;
    // only gas with viusalDensity = 4
    int GAS_WITH_VISUAL_DENSITY = 4;
    // only dark matter with viusalDensity = 5
    int DARK_MATTER_WITH_VISUAL_DENSITY = 5;

    // all with diffrent colors
    int ALL_WITH_DIFFRENT_COLORS = 6;
    // only Stars and gas with diffrent colors
    int STARS_AND_GAS_WITH_DIFFRENT_COLORS = 7;
    // only Stars with diffrent colors
    int STARS_WITH_DIFFRENT_COLORS = 8;
    // only gas with diffrent colors
    int GAS_WITH_DIFFRENT_COLORS = 9;
    // only dark matter with diffrent colors
    int DARK_MATTER_WITH_DIFFRENT_COLORS = 10;

    int colorMode = 2;

    double passedTime = 0;

    double globalScale = 1e-9;
    void calculateGlobalScale();

    bool focusedCamera = false; 
    vec3 cameraPosition;
    vec3 cameraFront;
    vec3 cameraUp;
    double cameraYaw;
    double cameraPitch;
    double cameraSpeed = 100;
    double rushSpeed = 1000;

    void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    
    double dFromCenter = 0;

    
    //static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    void onFramebufferSizeChanged(int width, int height); // Instanzfunktion


private:
    struct bgStar
    {
        vec3 position;
        double size;
        vec3 color;
        double alpha;
    };
    double calcDensityAv();
    double densityAv = 0;
    int oldIndex = -1;
    bool BGstars = true;
    int amountOfStars = 5000;
    std::vector<bgStar> bgStars;

    bool tracks = false;
    double cameraViewDistance = 1e15;
    mat4 view;

    // Funktionen für Kamerasteuerung 
    void processMouseInput();
    void processInput();

    GLuint shaderProgram;
    bool shouldClose = false;
    GLuint VAO;
    GLuint instanceVBO;
    void renderParticles();
    void checkShaderCompileStatus(GLuint shader, const char* shaderType);
    void checkShaderLinkStatus(GLuint program);
    void calcTime(int index);
    void calcTime(vec3 position, int index);
    double faktor = -1;
    double random(double min, double max);
};

#endif // ENGINE_H
