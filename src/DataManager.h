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

    void loadData(int timeStep, std::vector<std::shared_ptr<Particle>>& particles, Engine* eng);

    void printProgress(double currentStep, double steps, std::string text);

private:
    std::chrono::_V2::system_clock::time_point startTime;
    bool timerStarted = false;

    //AGF header
    struct AGFHeader
    {
        int numParticles[3];
        double deltaTime;
        double endTime;
        double currentTime;
    };
    //gadget2 header
    struct gadget2Header
    {
        unsigned int npart[6];
        double massarr[6];
        double time;
        double redshift;
        int flag_sfr;
        int flag_feedback;
        unsigned int npartTotal[6];
        int flag_cooling;
        int num_files;
        double BoxSize;
        double Omega0;
        double OmegaLambda;
        double HubbleParam;
        int flag_stellarage;
        int flag_metals;
        unsigned int npartTotalHighWord[6];
        int flag_entropy_instead_u;
        char fill[60]; // zur Auffüllung auf 256 Bytes
    };
};