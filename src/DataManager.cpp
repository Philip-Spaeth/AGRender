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
    std::cout << timeStep << std::endl;

    std::string filename = this->path + std::to_string(timeStep);
    std::string ending = ".ag";
    try
    {
        ending = ".ag";
        outputDataFormat = "ag";
        std::ifstream file(filename + ending);
        if (!file)
        {
            ending = ".agc";
            outputDataFormat = "agc";
            file = std::ifstream
            (filename + ending);
            if (!file)
            {
                ending = ".age";
                outputDataFormat = "age";
                file = std::ifstream
                (filename + ending);
                if (!file)
                {
                    ending = ".gadget";
                    outputDataFormat = "gadget";
                    file = std::ifstream
                    (filename + ending);
                    if (!file)
                    {
                        std::cerr << "Error opening datafile: " << filename << std::endl;
                        return;
                    }
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

    if (outputDataFormat == "ag")
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
        totalSize *= (sizeof(vec3) * 2 + sizeof(double) * 3 + sizeof(int) * 2 + sizeof(uint32_t));

        // Speicher für den Puffer allokieren
        char* buffer = reinterpret_cast<char*>(malloc(totalSize));
        if (buffer) {
            file.read(buffer, totalSize);
            char* ptr = buffer;

            for (int i = 0; i < totalSize / (sizeof(vec3) * 2 + sizeof(double) * 3 + sizeof(uint8_t) * 2 + sizeof(uint32_t)); i++) 
            {
                vec3 position, velocity;
                double mass, T, visualDensity;
                uint8_t type;
                uint8_t galaxyPart;
                uint32_t id;

                memcpy(&position, ptr, sizeof(vec3)); ptr += sizeof(vec3);
                memcpy(&velocity, ptr, sizeof(vec3)); ptr += sizeof(vec3);
                memcpy(&mass, ptr, sizeof(double)); ptr += sizeof(double);
                memcpy(&T, ptr, sizeof(double)); ptr += sizeof(double);
                memcpy(&visualDensity, ptr, sizeof(double)); ptr += sizeof(double);
                memcpy(&type, ptr, sizeof(uint8_t)); ptr += sizeof(uint8_t);
                memcpy(&galaxyPart, ptr, sizeof(uint8_t)); ptr += sizeof(uint8_t);
                memcpy(&id, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);

                auto particle = std::make_shared<Particle>();
                particle->position = position;
                particle->velocity = velocity;
                particle->mass = mass;
                particle->temperature = T;
                particle->density = visualDensity;
                particle->type = type;
                particle->galaxyPart = galaxyPart;
                particle->id = id;

                particles.push_back(particle);
            }
            free(buffer);
        }
    }
    else if (outputDataFormat == "agc")
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
    else if (outputDataFormat == "age")
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
        totalSize *= (sizeof(vec3) * 2 + sizeof(double) * 5 + sizeof(uint8_t)* 2 + sizeof(uint32_t));

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
                uint8_t galaxyPart;
                uint32_t id;


                memcpy(&position, ptr, sizeof(vec3)); ptr += sizeof(vec3);
                memcpy(&velocity, ptr, sizeof(vec3)); ptr += sizeof(vec3);
                memcpy(&mass, ptr, sizeof(double)); ptr += sizeof(double);
                memcpy(&T, ptr, sizeof(double)); ptr += sizeof(double);
                memcpy(&P, ptr, sizeof(double)); ptr += sizeof(double);
                memcpy(&visualDensity, ptr, sizeof(double)); ptr += sizeof(double);
                memcpy(&U, ptr, sizeof(double)); ptr += sizeof(double);
                memcpy(&type, ptr, sizeof(uint8_t)); ptr += sizeof(uint8_t);
                memcpy(&galaxyPart, ptr, sizeof(uint8_t)); ptr += sizeof(uint8_t);
                memcpy(&id, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);

                auto particle = std::make_shared<Particle>();
                particle->position = position;
                particle->velocity = velocity;
                particle->mass = mass;
                particle->temperature = T;
                particle->density = visualDensity;
                particle->type = type;
                particle->galaxyPart = galaxyPart;
                particle->id = id;

                particles.push_back(particle);
            }
            free(buffer);
        }
    }
    else if (outputDataFormat == "gadget")
    {
        // Gadget2 Header auslesen
        gadget2Header header;

        // Lesen der ersten Blockgröße vor dem Header
        unsigned int block_size_start;
        file.read(reinterpret_cast<char*>(&block_size_start), sizeof(block_size_start));
        if (!file) {
            std::cerr << "Fehler: Konnte die Start-Blockgröße nicht lesen!" << std::endl;
            return;
        }

        // Lesen des Headers
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!file) {
            std::cerr << "Fehler: Konnte den Header aus der Datei nicht lesen!" << std::endl;
            return;
        }

        // Lesen der Blockgröße nach dem Header
        unsigned int block_size_end;
        file.read(reinterpret_cast<char*>(&block_size_end), sizeof(block_size_end));
        if (!file) {
            std::cerr << "Fehler: Konnte die End-Blockgröße des Headers nicht lesen!" << std::endl;
            return;
        }

        // Überprüfen, ob die Blockgrößen übereinstimmen
        if (block_size_start != sizeof(header)) {
            std::cerr << "Fehler: Start- und End-Blockgrößen des Headers stimmen nicht überein!" << std::endl;
            return;
        }
        /*
        // Ausgabe des Headers zur Überprüfung
        std::cout << "Gadget2 Header geladen:" << std::endl;
        std::cout << "Teilchenanzahl pro Typ:" << std::endl;
        for (int i = 0; i < 6; ++i) { // Annahme: 6 Typen
            std::cout << "Typ " << i << ": " << header.npart[i] << " Partikel" << std::endl;
        }
        std::cout << "Mass per type:" << std::endl;
        for (int i = 0; i < 6; ++i) { // Annahme: 6 Typen
            std::cout << "Typ " << i << ": " << header.massarr[i] << " Mass" << std::endl;
        }
        std::cout << "Boxgröße: " << header.BoxSize << std::endl;
        std::cout << "Hubble-Parameter: " << header.HubbleParam << std::endl;
        std::cout << "Roteshift: " << header.redshift << std::endl;
        std::cout << "Simulationszeit: " << header.time << std::endl;
        */
    
        // Gesamtanzahl der Partikel berechnen
        unsigned int total_particles = 0;
        for(int i = 0; i < 6; ++i){
            total_particles += header.npart[i];
        }

        // Reservieren des Speicherplatzes für Partikel
        particles.reserve(total_particles);

        // ### Lesen des Positionsblocks (POS) ###
        
        // Lesen der Blockgröße vor dem POS-Block
        unsigned int pos_block_size_start;
        file.read(reinterpret_cast<char*>(&pos_block_size_start), sizeof(pos_block_size_start));
        if (!file) {
            std::cerr << "Fehler: Konnte die Start-Blockgröße des POS-Blocks nicht lesen!" << std::endl;
            return;
        }

        // Berechnen der erwarteten Größe für den POS-Block: N * 3 * sizeof(float)
        unsigned int expected_pos_block_size = total_particles * 3 * sizeof(float);
        if (pos_block_size_start != expected_pos_block_size) {
            std::cerr << "Warnung: Erwartete POS-Blockgröße (" << expected_pos_block_size 
                    << " bytes) stimmt nicht mit gelesener Größe (" << pos_block_size_start << " bytes) überein." << std::endl;
            // Optional: Fortfahren oder Abbruch
        }

        // Lesen der Positionsdaten
        std::vector<float> positions(total_particles * 3); // N * 3 floats
        file.read(reinterpret_cast<char*>(positions.data()), pos_block_size_start);
        if (!file) {
            std::cerr << "Fehler: Konnte die Positionsdaten nicht lesen!" << std::endl;
            return;
        }

        // Lesen der Blockgröße nach dem POS-Block
        unsigned int pos_block_size_end;
        file.read(reinterpret_cast<char*>(&pos_block_size_end), sizeof(pos_block_size_end));
        if (!file) {
            std::cerr << "Fehler: Konnte die End-Blockgröße des POS-Blocks nicht lesen!" << std::endl;
            return;
        }

        // Überprüfen, ob die Blockgrößen übereinstimmen
        if (pos_block_size_start != pos_block_size_end) {
            std::cerr << "Fehler: Start- und End-Blockgrößen des POS-Blocks stimmen nicht überein!" << std::endl;
            return;
        }

        // ### Lesen des Geschwindigkeitsblocks (VEL) ###

        // Lesen der Blockgröße vor dem VEL-Block
        unsigned int vel_block_size_start;
        file.read(reinterpret_cast<char*>(&vel_block_size_start), sizeof(vel_block_size_start));
        if (!file) {
            std::cerr << "Fehler: Konnte die Start-Blockgröße des VEL-Blocks nicht lesen!" << std::endl;
            return;
        }

        // Berechnen der erwarteten Größe für den VEL-Block: N * 3 * sizeof(float)
        unsigned int expected_vel_block_size = total_particles * 3 * sizeof(float);
        if (vel_block_size_start != expected_vel_block_size) {
            std::cerr << "Warnung: Erwartete VEL-Blockgröße (" << expected_vel_block_size 
                    << " bytes) stimmt nicht mit gelesener Größe (" << vel_block_size_start << " bytes) überein." << std::endl;
            // Optional: Fortfahren oder Abbruch
        }

        // Lesen der Geschwindigkeitsdaten
        std::vector<float> velocities(total_particles * 3); // N * 3 floats
        file.read(reinterpret_cast<char*>(velocities.data()), vel_block_size_start);
        if (!file) {
            std::cerr << "Fehler: Konnte die Geschwindigkeitsdaten nicht lesen!" << std::endl;
            return;
        }

        // Lesen der Blockgröße nach dem VEL-Block
        unsigned int vel_block_size_end;
        file.read(reinterpret_cast<char*>(&vel_block_size_end), sizeof(vel_block_size_end));
        if (!file) {
            std::cerr << "Fehler: Konnte die End-Blockgröße des VEL-Blocks nicht lesen!" << std::endl;
            return;
        }

        // Überprüfen, ob die Blockgrößen übereinstimmen
        if (vel_block_size_start != vel_block_size_end) {
            std::cerr << "Fehler: Start- und End-Blockgrößen des VEL-Blocks stimmen nicht überein!" << std::endl;
            return;
        }

        // ### Lesen des ID-Blocks (ID) ###

        // Lesen der Blockgröße vor dem ID-Block
        unsigned int id_block_size_start;
        file.read(reinterpret_cast<char*>(&id_block_size_start), sizeof(id_block_size_start));
        if (!file) {
            std::cerr << "Fehler: Konnte die Start-Blockgröße des ID-Blocks nicht lesen!" << std::endl;
            return;
        }

        // Berechnen der erwarteten Größe für den ID-Block: N * sizeof(unsigned int)
        unsigned int expected_id_block_size = total_particles * sizeof(unsigned int);
        if (id_block_size_start != expected_id_block_size) {
            std::cerr << "Warnung: Erwartete ID-Blockgröße (" << expected_id_block_size 
                    << " bytes) stimmt nicht mit gelesener Größe (" << id_block_size_start << " bytes) überein." << std::endl;
            // Optional: Fortfahren oder Abbruch
        }

        // Lesen der ID-Daten
        std::vector<unsigned int> ids(total_particles); // N unsigned ints
        file.read(reinterpret_cast<char*>(ids.data()), id_block_size_start);
        if (!file) {
            std::cerr << "Fehler: Konnte die ID-Daten nicht lesen!" << std::endl;
            return;
        }

        // Lesen der Blockgröße nach dem ID-Block
        unsigned int id_block_size_end;
        file.read(reinterpret_cast<char*>(&id_block_size_end), sizeof(id_block_size_end));
        if (!file) {
            std::cerr << "Fehler: Konnte die End-Blockgröße des ID-Blocks nicht lesen!" << std::endl;
            return;
        }

        // Überprüfen, ob die Blockgrößen übereinstimmen
        if (id_block_size_start != id_block_size_end) {
            std::cerr << "Fehler: Start- und End-Blockgrößen des ID-Blocks stimmen nicht überein!" << std::endl;
            return;
        }

        //read mass:
        // Überprüfen, ob individuelle Massen vorhanden sind (massarr[i] == 0)
        const float epsilon = 1e-10;
        bool has_individual_mass = false;
        for(int i = 0; i < 6; ++i){
            if(header.massarr[i] < epsilon)
            {
                if(header.npart[i] != 0)
                {
                    has_individual_mass = true;
                    break;
                }
                else
                {
                    //std::cout << "Mass for type: "<< i << " is 0 because Npart is 0" << std::endl;
                }
            }
            else
            {
                //std::cout << std::scientific << "Mass for type: "<< i << " is " <<  header.massarr[i] << std::endl;
            }
        }

        std::vector<float> masses; // Vektor zur Speicherung der Massen
        if(has_individual_mass)
        {
            // Lesen der Blockgröße vor dem MASS-Block
            unsigned int mass_block_size_start;
            file.read(reinterpret_cast<char*>(&mass_block_size_start), sizeof(mass_block_size_start));
            if (!file) {
                std::cerr << "Fehler: Konnte die Start-Blockgröße des MASS-Blocks nicht lesen!" << std::endl;
                return;
            }

            // Berechnen der erwarteten Größe für den MASS-Block: N * sizeof(float)
            unsigned int expected_mass_block_size = total_particles * sizeof(float);
            if (mass_block_size_start != expected_mass_block_size) {
                std::cerr << "Warnung: Erwartete MASS-Blockgröße (" << expected_mass_block_size 
                        << " bytes) stimmt nicht mit gelesener Größe (" << mass_block_size_start << " bytes) überein." << std::endl;
                // Optional: Fortfahren oder Abbruch
            }

            // Lesen der Massendaten
            masses.resize(total_particles);
            file.read(reinterpret_cast<char*>(masses.data()), mass_block_size_start);
            if (!file) {
                std::cerr << "Fehler: Konnte die Massendaten nicht lesen!" << std::endl;
                return;
            }

            // Lesen der Blockgröße nach dem MASS-Block
            unsigned int mass_block_size_end;
            file.read(reinterpret_cast<char*>(&mass_block_size_end), sizeof(mass_block_size_end));
            if (!file) {
                std::cerr << "Fehler: Konnte die End-Blockgröße des MASS-Blocks nicht lesen!" << std::endl;
                return;
            }

            // Überprüfen, ob die Blockgrößen übereinstimmen
            if (mass_block_size_start != mass_block_size_end) {
                std::cerr << "Fehler: Start- und End-Blockgrößen des MASS-Blocks stimmen nicht überein!" << std::endl;
                return;
            }
        }

        // ### Lesen des U-Blocks (interne Energie) ###

        // Überprüfen, ob Gaspartikel vorhanden sind
        if(header.npart[0] > 0) // Nur wenn es Gaspartikel gibt
        {
            std::cout << "reading U from gas particles" << std::endl;
            // Lesen der Blockgröße vor dem U-Block
            unsigned int u_block_size_start;
            file.read(reinterpret_cast<char*>(&u_block_size_start), sizeof(u_block_size_start));
            if (!file) {
                std::cerr << "Fehler: Konnte die Start-Blockgröße des U-Blocks nicht lesen!" << std::endl;
                return;
            }

            // Erwartete Größe des U-Blocks berechnen: Anzahl der Gaspartikel * sizeof(float)
            unsigned int expected_u_block_size = header.npart[0] * sizeof(float);
            if (u_block_size_start != expected_u_block_size) {
                std::cerr << "Warnung: Erwartete U-Blockgröße (" << expected_u_block_size 
                        << " Bytes) stimmt nicht mit gelesener Größe (" << u_block_size_start << " Bytes) überein." << std::endl;
                // Optional: Fortfahren oder Abbruch
            }

            // Lesen der U-Daten
            std::vector<float> u_values(header.npart[0]); // Interne Energie pro Masseneinheit für Gaspartikel
            file.read(reinterpret_cast<char*>(u_values.data()), u_block_size_start);
            if (!file) {
                std::cerr << "Fehler: Konnte die U-Daten nicht lesen!" << std::endl;
                return;
            }

            // Lesen der Blockgröße nach dem U-Block
            unsigned int u_block_size_end;
            file.read(reinterpret_cast<char*>(&u_block_size_end), sizeof(u_block_size_end));
            if (!file) {
                std::cerr << "Fehler: Konnte die End-Blockgröße des U-Blocks nicht lesen!" << std::endl;
                return;
            }

            // Überprüfen, ob die Blockgrößen übereinstimmen
            if (u_block_size_start != u_block_size_end) {
                std::cerr << "Fehler: Start- und End-Blockgrößen des U-Blocks stimmen nicht überein!" << std::endl;
                return;
            }
        
            unsigned int current_particle = 0;
            unsigned int gas_particle_index = 0;
            int count_gas = 0;
            int count_dark = 0;
            int count_star = 0;

            for(int type = 0; type < 6; ++type)
            {
                    for(unsigned int i = 0; i < (unsigned int)header.npart[type]; ++i){
                        if(current_particle >= total_particles){
                            std::cerr << "Fehler: Überschreitung der Partikelanzahl beim Erstellen der Partikel!" << std::endl;
                            return;
                        }
                        auto particle = std::make_shared<Particle>();
                        particle->id = ids[current_particle];

                        if(type == 1)  
                        {   
                            particle->type = 3; // Dark Matter
                            particle->galaxyPart = 3; // Halo
                            count_dark++;
                        }
                        if(type == 2 || type == 4 || type == 5) 
                        {
                            particle->type = 1;
                            particle->galaxyPart = 1;
                            count_star++;
                        }
                        if(type == 3)
                        {
                            particle->type = 1;
                            particle->galaxyPart = 2;
                            count_star++;
                        }
                        if(type == 0) 
                        {
                            particle->type = 2; // Gas
                            particle->galaxyPart = 1; // Disk
                            count_gas++;
                        }

                        if(has_individual_mass)
                        {
                            particle->mass = masses[current_particle];
                        }
                        else
                        {
                            particle->mass = header.massarr[type];
                        }

                        particle->position = vec3(
                            positions[3*current_particle],
                            positions[3*current_particle + 1],
                            positions[3*current_particle + 2]
                        );
                        particle->velocity = vec3(
                            velocities[3*current_particle],
                            velocities[3*current_particle + 1],
                            velocities[3*current_particle + 2]
                        );

                        // Interne Energie für Gaspartikel zuweisen
                        if(type == 0) // Gaspartikel
                        {
                            //scale to SI
                            particle->temperature = u_values[gas_particle_index];
                            //std::cout << particle->U << std::endl;
                            gas_particle_index++;
                        }

                        particles.push_back(particle);
                        current_particle++;
                    }
                }
            }
        else // Wenn keine Gaspartikel vorhanden sind
        {
            // Ihr bestehender Code zum Erstellen der Partikel
            unsigned int current_particle = 0;
            int count_gas = 0;
            int count_dark = 0;
            int count_star = 0;

            for(int type = 0; type < 6; ++type){
                for(unsigned int i = 0; i < (unsigned int)header.npart[type]; ++i){
                    if(current_particle >= total_particles){
                        std::cerr << "Fehler: Überschreitung der Partikelanzahl beim Erstellen der Partikel!" << std::endl;
                        return;
                    }
                    auto particle = std::make_shared<Particle>();
                    particle->id = ids[current_particle];

                    // Setzen des Partikeltyps und der Masse
                    if(type == 1)  
                    {   
                        particle->type = 3; // Dark Matter
                        particle->galaxyPart = 3; // Halo
                        count_dark++;
                    }
                    if(type == 2 || type == 4 || type == 5) 
                    {
                        particle->type = 1;
                        particle->galaxyPart = 1;
                        count_star++;
                    }
                    if(type == 3)
                    {
                        particle->type = 1;
                        particle->galaxyPart = 2;
                        count_star++;
                    }
                    if(type == 0) 
                    {
                        particle->type = 2; // Gas
                        particle->galaxyPart = 1; // Disk
                        count_gas++;
                    }

                    if(has_individual_mass)
                    {
                        particle->mass = masses[current_particle];
                    }
                    else
                    {
                        particle->mass = header.massarr[type];
                    }

                    particle->position = vec3(
                        positions[3*current_particle],
                        positions[3*current_particle + 1],
                        positions[3*current_particle + 2]
                    );
                    particle->velocity = vec3(
                        velocities[3*current_particle],
                        velocities[3*current_particle + 1],
                        velocities[3*current_particle + 2]
                    );

                    particles.push_back(particle);
                    current_particle++;
                }
            }
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
