#include "DataManager.h"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;
namespace fs = std::filesystem;

DataManager::DataManager(std::string path)
{
    this->path = path;
}

DataManager::~DataManager()
{
}

void DataManager::loadData(int timeStep, std::vector<std::shared_ptr<Particle>>& particles, Engine* eng)
{

    std::string filename = this->path + std::to_string(timeStep);
    std::string ending = ".agf";
    try
    {
        ending = ".agf";
        outputDataFormat = "AGF";
        std::ifstream file(filename + ending);
        if (!file)
        {
            ending = ".agfc";
            outputDataFormat = "AGFC";
            file = std::ifstream
            (filename + ending);
            if (!file)
            {
                ending = ".agfe";
                outputDataFormat = "AGFE";
                file = std::ifstream
                (filename + ending);
                if (!file)
                {
                    std::cerr << "Error opening datafile: " << filename << std::endl;
                    return;
                }
            }
        }
        file.close();
    }
    catch (const std::exception&)
    {
        std::cerr << "Error opening datafile: " << filename << std::endl;
        return;
    }

    filename = filename + ending;

    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening datafile: " << filename << std::endl;
        return;
    }

    particles.clear();

    if (outputDataFormat == "AGF")
    {
        // Header auslesen
        AGFHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!file) 
        {
            std::cerr << "Fehler: Konnte den AGF-Header nicht lesen!" << std::endl;
        }
        // Anzahl der Partikel berechnen
        unsigned int total_particles = header.numParticles[0] + header.numParticles[1] + header.numParticles[2];
    
        eng->numOfParticles = total_particles;
        eng->numTimeSteps = header.endTime / header.deltaTime;
        eng->deltaTime = header.deltaTime;
        
        // Speicherplatz für Partikel reservieren
        particles.reserve(total_particles);

        size_t totalSize = header.numParticles[0] + header.numParticles[1] + header.numParticles[2];
        totalSize *= (sizeof(vec3) * 2 + sizeof(double) * 3 + sizeof(int));

        // Speicher für den Puffer allokieren
        char* buffer = reinterpret_cast<char*>(malloc(totalSize));
        if (buffer) {
            file.read(buffer, totalSize);
            char* ptr = buffer;

            for (int i = 0; i < totalSize / (sizeof(vec3) * 2 + sizeof(double) * 3 + sizeof(uint8_t)); i++) {
                vec3 position, velocity;
                double mass, T, visualDensity;
                int type;

                memcpy(&position, ptr, sizeof(vec3)); ptr += sizeof(vec3);
                memcpy(&velocity, ptr, sizeof(vec3)); ptr += sizeof(vec3);
                memcpy(&mass, ptr, sizeof(double)); ptr += sizeof(double);
                memcpy(&T, ptr, sizeof(double)); ptr += sizeof(double);
                memcpy(&visualDensity, ptr, sizeof(double)); ptr += sizeof(double);
                memcpy(&type, ptr, sizeof(uint8_t)); ptr += sizeof(uint8_t);

                auto particle = std::make_shared<Particle>();
                particle->position = position;
                particle->velocity = velocity;
                particle->mass = mass;
                particle->temperature = T;
                particle->density = visualDensity;
                particle->type = type;

                particles.push_back(particle);
            }
            free(buffer);
        }
    }
    else if (outputDataFormat == "AGFC")
    {
        // Header auslesen
        AGFHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!file) 
        {
            std::cerr << "Fehler: Konnte den AGFE-Header nicht lesen!" << std::endl;
        }
        // Anzahl der Partikel berechnen
        unsigned int total_particles = header.numParticles[0] + header.numParticles[1] + header.numParticles[2];
    
        eng->numOfParticles = total_particles;
        eng->numTimeSteps = header.endTime / header.deltaTime;
        eng->deltaTime = header.deltaTime;
        
        // Speicherplatz für Partikel reservieren
        particles.reserve(total_particles);

        // Berechnung der Datenstrukturgröße für jedes Particle
        size_t particleDataSize = sizeof(float) * 3 + sizeof(float) + sizeof(uint8_t);

        // Speicher für das Lesen der Daten allokieren
        std::vector<char> buffer(total_particles * particleDataSize);
        file.read(buffer.data(), total_particles * particleDataSize);

        // Zeiger für das Lesen der Partikeldaten initialisieren
        const char* ptr = buffer.data();
        for (int i = 0; i < total_particles; i++) {
            
            auto particle = std::make_shared<Particle>();

            // Position einlesen
            float posX, posY, posZ;
            memcpy(&posX, ptr, sizeof(float)); ptr += sizeof(float);
            memcpy(&posY, ptr, sizeof(float)); ptr += sizeof(float);
            memcpy(&posZ, ptr, sizeof(float)); ptr += sizeof(float);
            particle->position = {posX, posY, posZ};

            // visualDensity einlesen
            float visualDensity;
            memcpy(&visualDensity, ptr, sizeof(float)); ptr += sizeof(float);
            particle->density = visualDensity;

            // type einlesen
            memcpy(&particle->type, ptr, sizeof(uint8_t)); ptr += sizeof(uint8_t);

            particles.push_back(particle);
        }
    }
    else if (outputDataFormat == "AGFE")
    {
        // Header auslesen
        AGFHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!file) 
        {
            std::cerr << "Fehler: Konnte den AGFE-Header nicht lesen!" << std::endl;
        }
        // Anzahl der Partikel berechnen
        unsigned int total_particles = header.numParticles[0] + header.numParticles[1] + header.numParticles[2];
    
        eng->numOfParticles = total_particles;
        eng->numTimeSteps = header.endTime / header.deltaTime;
        eng->deltaTime = header.deltaTime;
        
        // Speicherplatz für Partikel reservieren
        particles.reserve(total_particles);

        size_t totalSize = header.numParticles[0] + header.numParticles[1] + header.numParticles[2];
        totalSize *= (sizeof(vec3) * 2 + sizeof(double) * 5 + sizeof(uint8_t));

        // Speicher für den Puffer allokieren
        char* buffer = reinterpret_cast<char*>(malloc(totalSize));
        if (buffer) 
        {
            file.read(buffer, totalSize);
            char* ptr = buffer;

            for (int i = 0; i < totalSize / (sizeof(vec3) * 2 + sizeof(double) * 5 + sizeof(uint8_t)); i++) {
                vec3 position, velocity;
                double mass, T, P, visualDensity, U;
                uint8_t type;

                memcpy(&position, ptr, sizeof(vec3)); ptr += sizeof(vec3);
                memcpy(&velocity, ptr, sizeof(vec3)); ptr += sizeof(vec3);
                memcpy(&mass, ptr, sizeof(double)); ptr += sizeof(double);
                memcpy(&T, ptr, sizeof(double)); ptr += sizeof(double);
                memcpy(&P, ptr, sizeof(double)); ptr += sizeof(double);
                memcpy(&visualDensity, ptr, sizeof(double)); ptr += sizeof(double);
                memcpy(&U, ptr, sizeof(double)); ptr += sizeof(double);
                memcpy(&type, ptr, sizeof(uint8_t)); ptr += sizeof(uint8_t);

                auto particle = std::make_shared<Particle>();
                particle->position = position;
                particle->velocity = velocity;
                particle->mass = mass;
                particle->temperature = T;
                particle->density = visualDensity;
                particle->type = type;

                particles.push_back(particle);
            }
            free(buffer);
        }
    }
    else
    {
        std::cerr << "Unknown output data format: " << outputDataFormat << std::endl;
    }


    file.close();
}

#ifdef _WIN32
void setConsoleColor(WORD color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}
#else
void setConsoleColor(int color) {
    // No color setting for Linux in this version
}
#endif
void DataManager::printProgress(double currentStep, double steps, std::string text) 
{
    static const int barWidth = 70;

    if (!timerStarted) {
        std::cout << std::endl;
        startTime = std::chrono::high_resolution_clock::now();
        timerStarted = true;
    }

    double progress = (currentStep + 1) / steps;
    int pos = static_cast<int>(barWidth * progress);
    
    // Set color based on progress
#ifdef _WIN32
    WORD progressColor = (currentStep < steps - 1) ? FOREGROUND_RED : FOREGROUND_GREEN;
#endif

    std::cout << "[";
    for (int i = 0; i < barWidth; ++i) {
#ifdef _WIN32
        if (i < pos) {
            setConsoleColor(progressColor);
            std::cout << "=";
        }
        else if (i == pos && currentStep < steps - 1) {
            setConsoleColor(FOREGROUND_RED);
            std::cout << ">";
        }
        else {
            setConsoleColor(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
            std::cout << " ";
        }
#else
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
#endif
    }

    double remainingTime = 0;

    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = currentTime - startTime;
    double estimatedTotalTime = (elapsed.count() / (currentStep + 1)) * steps;
    double estimatedRemainingTime = estimatedTotalTime - elapsed.count();
    remainingTime = estimatedRemainingTime;

    std::string unit;
    if (remainingTime < 60) {
        unit = "s";
    } else if (remainingTime < 3600) {
        remainingTime /= 60;
        unit = "min";
    } else {
        remainingTime /= 3600;
        unit = "h";
    }

    std::ostringstream timeStream;
    timeStream << std::fixed << std::setprecision(1) << remainingTime;
    std::string timeleft = timeStream.str();

#ifdef _WIN32
    setConsoleColor(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
#endif
    std::cout << "] " << int(progress * 100.0) << " %  Estimated remaining time: " << timeleft << unit << "  "<< text << "       " << "\r";

    if (currentStep == steps - 1) {
        std::cout << std::endl;
        std::cout << std::endl;
    }
}
