#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <chrono>
#include "Particle.h"
#include "vec3.h"
#include "Engine.h"

class DataManager
{
public:
    DataManager(std::string path);
    ~DataManager();
    std::string path;    
    std::string outputDataFormat;


    void readInfoFile(double& deltaTime, double& timeSteps, double& numberOfParticles);
    void loadData(int timeStep, std::vector<std::shared_ptr<Particle>>& particles, Engine* eng);

    void printProgress(double currentStep, double steps, std::string text);

private:
    std::chrono::_V2::system_clock::time_point startTime;
    bool timerStarted = false;

    struct AGFHeader
    {
        int numParticles[3];
        double deltaTime;
        double endTime;
        double currentTime;
    };
};