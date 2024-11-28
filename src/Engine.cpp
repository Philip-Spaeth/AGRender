#include "Engine.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <filesystem>
#include <algorithm>

#ifdef WIN32
#include <Windows.h>
#endif

#include <chrono>
#include <thread>
#include <string>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

inline float radians(float degrees) {
    return degrees * (M_PI / 180.0f);
}


void printMatrix(const mat4& matrix) {
    const float* data = matrix.data(); // Ersetzt dies mit der richtigen Methode
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            std::cout << data[i * 4 + j] << " ";
        }
        std::cout << std::endl;
    }
}

Engine::Engine(std::string NewDataFolder, double deltaTime, double numOfParticles, double numTimeSteps, std::vector<std::shared_ptr<Particle>>* particles) : window(nullptr), shaderProgram(0), VAO(0)
{
    dataFolder = NewDataFolder;
    this->deltaTime = deltaTime;
    this->numOfParticles = numOfParticles;
    this->numTimeSteps = numTimeSteps;
    this->particles = particles;

    // start kamera position
    cameraPosition = vec3(0, 0, 0);                  
    cameraFront = vec3(0.0, 0.0, -1.0);
    cameraUp = vec3(0.0, 1.0, 0.0);
    cameraYaw = -90.0f;
    cameraPitch = 0.0f;

    saveThread = std::thread(&Engine::saveWorker, this);
}

Engine::~Engine() {
    // Terminate the save worker thread
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        terminateThread = true;
        queueCondition.notify_all();
    }
    saveThread.join();
    if (pbo != 0) {
        glDeleteBuffers(1, &pbo);
    }
}

void glfw_error_callback(int error, const char* description)
{
    std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}

void Engine::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    Engine* engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
    if (engine)
    {
        engine->onFramebufferSizeChanged(width, height);
    }
}

void Engine::onFramebufferSizeChanged(int width, int height)
{
    // Aktualisieren Sie den Viewport
    glViewport(0, 0, width, height);
}



void APIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    if (type == GL_DEBUG_TYPE_ERROR)
    {
        std::cerr << "GL ERROR: " << message << std::endl;
    }
    else
    {
        std::cout << "GL CALLBACK: " << message << std::endl;
    }
}




bool Engine::init(double physicsFaktor) 
{
    faktor = physicsFaktor;

    // GLFW initialisieren
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // OpenGL-Version und -Profil setzen
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1); // Passe hier an, falls nötig
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24); // Tiefenpuffer

    // Fenster erstellen
    int width_Window = 1200;
    int height_Window = 800;
    if (!RenderLive) {
        width_Window = 1920;
        height_Window = 1080;
    }
    window = glfwCreateWindow(width_Window, height_Window, "Particle Rendering", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    // Kontext binden
    glfwMakeContextCurrent(window);


    // Breite und Höhe des Framebuffers holen
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    // Fallback für ungültige Breite oder Höhe
    if (width == 0) width = 800;
    if (height == 0) height = 600;

    std::cout << "Width: " << width << ", Height: " << height << std::endl;


    // OpenGL-Version prüfen
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;

    // GLEW initialisieren
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(err) << std::endl;
        return false;
    }

    // Debug-Modus aktivieren
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, nullptr);

    // Viewport setzen
    glViewport(0, 0, width, height);


    // Perspektivmatrix erstellen
    mat4 projection = mat4::perspective(45.0f, (float)width / (float)height, 0.1f, 100.0f);

    // Debug-Ausgabe der Matrizen
    std::cout << "Projection Matrix:" << std::endl;
    printMatrix(projection);


    // Tiefentest aktivieren
    glEnable(GL_DEPTH_TEST);

    // Framebuffer prüfen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer incomplete: " << framebufferStatus << std::endl;
        return false;
    }

    // Shader erstellen und aktivieren
    const char* vertexShaderSource = R"(
        #version 410 core
        uniform mat4 projection;
        uniform mat4 view;
        uniform vec3 particlePosition;
        void main() {
            gl_Position = projection * view * vec4(particlePosition, 1.0);
            gl_PointSize = 10.0;
        }
    )";
    const char* fragmentShaderSource = R"(
        #version 410 core
        out vec4 FragColor;
        uniform vec3 particleColor;
        void main() {
            FragColor = vec4(particleColor, 1.0);
        }
    )";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    checkShaderCompileStatus(vertexShader, "VERTEX");

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    checkShaderCompileStatus(fragmentShader, "FRAGMENT");

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    checkShaderLinkStatus(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Shader aktivieren
    glUseProgram(shaderProgram);

    // VAO und VBO erstellen
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);

    glBindVertexArray(0);

    // Uniform-Locations überprüfen
    GLuint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    if (projectionLoc == -1 || viewLoc == -1) {
        std::cerr << "Projection/View Uniforms not found!" << std::endl;
        return false;
    }

    // Cursor-Modus setzen
    if (RenderLive) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    return true;
}





/* void Engine::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // Stellen Sie sicher, dass die Ansichtsportgröße dem neuen Fenster entspricht
    glViewport(0, 0, width, height);
} */

void Engine::start()
{
    // Entfernen Sie die VBO-Erstellung und -Konfiguration, da wir keine Vertex-Attribute verwenden

    // Erstellen und Binden eines leeren VAO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Wenn Sie Hintergrundsterne haben, können Sie diesen Code beibehalten
    if (BGstars)
    {
        for (int i = 0; i < amountOfStars; i++)
        {
            double x = random(-1e14, 1e14);
            double y = random(-1e14, 1e14);
            double z = random(-1e14, 1e14);
            double size = random(0.1, 2);
            bgStars.push_back(vec4(x, y, z, size));
        }
    }

    std::cout << "Data loaded" << std::endl;
}


void Engine::initializePBO() {
    if (pbo == 0) {
        glGenBuffers(1, &pbo);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
        glBufferData(GL_PIXEL_PACK_BUFFER, 3 * width * height, nullptr, GL_STREAM_READ);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }
}

void Engine::saveAsPicture(const std::string& folderName, int index) {
    // Speichern Sie das gerenderte Bild als BMP-Datei
    glfwGetFramebufferSize(window, &width, &height);

    // Initialize PBO if not already initialized
    initializePBO();

    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, 0);

    // Map the PBO to process its data on the CPU
    unsigned char* data = (unsigned char*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);

    // Copy data to a vector to pass to the saving thread
    std::vector<unsigned char> imageData(data, data + 3 * width * height);

    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    // Umkehren der Pixelreihenfolge in-place
    for (int y = 0; y < height / 2; y++) {
        for (int x = 0; x < 3 * width; x++) {
            std::swap(imageData[3 * width * y + x], imageData[3 * width * (height - 1 - y) + x]);
        }
    }

    std::string fullPath = "../Video_Output/" + videoName + "/";
    std::filesystem::create_directory(fullPath);

    std::string filename = fullPath + "Picture_" + std::to_string(index) + ".bmp";

    // Add the data to the save queue
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        saveQueue.emplace(filename, std::move(imageData));
        queueCondition.notify_one();
    }
}

void Engine::saveWorker() {
    while (true) {
        std::pair<std::string, std::vector<unsigned char>> item;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCondition.wait(lock, [this] { return !saveQueue.empty() || terminateThread; });

            if (terminateThread && saveQueue.empty()) {
                break;
            }

            item = std::move(saveQueue.front());
            saveQueue.pop();
        }

        stbi_write_bmp(item.first.c_str(), width, height, 3, item.second.data());
    }
}

void Engine::window_iconify_callback(GLFWwindow* window, int iconified) {
    if (iconified) {
        glfwRestoreWindow(window);
    }
}

void Engine::update(int index)
{
    if(index == 0)
    {
        densityAv = calcDensityAv();
    }

    //calculate the time
    if (isRunning && index != oldIndex && RenderLive)
    {
        calcTime(index);
    }
    if (RenderLive)
    {
        //continue if space is pressed
        #ifdef WIN32
        if (GetAsyncKeyState(32) & 0x8000)
        #else
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        #endif
        {
			isRunning = !isRunning;
            //small delay
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}
	}

    if (RenderLive)
    {
        processMouseInput();
    }

    if(isRunning && RenderLive == false) processInput();
    if (RenderLive) processInput();

    // set the globalScale of the system
    if (index == 0)
    {
        calculateGlobalScale();
    }

    renderParticles();

    glfwSwapBuffers(window);
    glfwPollEvents();

    //if video is rendered
    if (RenderLive == false && index != oldIndex && index != 0)
    {
        saveAsPicture(dataFolder, index);
        glfwSetWindowIconifyCallback(window, window_iconify_callback);
    }

    if(RenderLive == true)
    {
        //speed up if right arrow is pressed
    #ifdef WIN32
        if (GetAsyncKeyState(39) & 0x8000)
    #else
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
    #endif
        {
            playSpeed = playSpeed + changeSpeed;
        }
        //slow down if left arrow is pressed
    #ifdef WIN32
        if (GetAsyncKeyState(37) & 0x8000)
    #else
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
    #endif
        {
            playSpeed = playSpeed - changeSpeed;
        }

    if (GetAsyncKeyState(49) & 0x8000) renderMode = 1;
    if (GetAsyncKeyState(50) & 0x8000) renderMode = 2;
    if (GetAsyncKeyState(51) & 0x8000) renderMode = 3;
    if (GetAsyncKeyState(52) & 0x8000) renderMode = 4;
    if (GetAsyncKeyState(53) & 0x8000) renderMode = 5;
    if (GetAsyncKeyState(54) & 0x8000) renderMode = 6;
    if (GetAsyncKeyState(55) & 0x8000) renderMode = 7;
    if (GetAsyncKeyState(56) & 0x8000) renderMode = 8;
    if (GetAsyncKeyState(57) & 0x8000) renderMode = 9;
    if (GetAsyncKeyState(48) & 0x8000) renderMode = 10;
    }

    oldIndex = index;
}

double Engine::calcDensityAv()
{
    //calc the middle value of the particles densities
    double densityAv = 0;
    for (const auto& particle : *particles)
    {
        densityAv += particle->density;
    }
    densityAv = densityAv / particles->size();
    return densityAv;
}

vec3 jetColorMap(double value) 
{
    double r = value < 0.5 ? 0.0 : 2.0 * value - 1.0;
    double g = value < 0.25 ? 0.0 : value < 0.75 ? 1.0 : 1.0 - 4.0 * (value - 0.75);
    double b = value < 0.5 ? 1.0 : 1.0 - 2.0 * value;

    return vec3(r, g, b);
}




mat4 perspective(float fovY, float aspect, float zNear, float zFar) {
    float tanHalfFovy = tan(fovY * M_PI / 180.0f / 2.0f);

    mat4 result; // Standardkonstruktor verwenden
    float mat[4][4] = { 0.0f }; // Temporäres Array für die Matrixdaten

    mat[0][0] = 1.0f / (aspect * tanHalfFovy);
    mat[1][1] = 1.0f / tanHalfFovy;
    mat[2][2] = -(zFar + zNear) / (zFar - zNear);
    mat[2][3] = -1.0f;
    mat[3][2] = -(2.0f * zFar * zNear) / (zFar - zNear);

    result = mat4(mat); // Setze die Matrixdaten
    return result;
}






/* void Engine::renderParticles()
{
    GLenum error;

    // Framebuffer binden und Bildschirm löschen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer incomplete: " << framebufferStatus << std::endl;
        return;
    }

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // Hintergrundfarbe setzen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    // Shader-Programm verwenden
    glUseProgram(shaderProgram);

    // Projektions- und Sichtmatrix erstellen
    //mat4 projection = mat4::perspective(45.0f, (float)width / (float)height, 0.1f, cameraViewDistance);
    mat4 projection = mat4::perspective(45.0f, (float)width / (float)height, 0.1f, 100.0f);
    std::cout << "Width: " << width << ", Height: " << height << std::endl;
    mat4 viewMatrix = mat4::lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp);

    // Debug: Ausgabe der Matrizen
    std::cout << "Projection Matrix:" << std::endl;
    printMatrix(projection);
    std::cout << "View Matrix:" << std::endl;
    printMatrix(viewMatrix);


    // Matrizen im Shader setzen
    GLuint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    if (projectionLoc != -1) {
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projection.data());
    }

    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    if (viewLoc != -1) {
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, viewMatrix.data());
    }

    // VAO binden
    glBindVertexArray(VAO);

    glPointSize(10.0f);
    glBegin(GL_POINTS);
    glVertex3f(0.0f, 0.0f, -2.0f); // Punkt sichtbar in der Mitte des Bildschirms
    glEnd();



    // Hintergrundsterne rendern
    if (BGstars)
    {
        for (const auto& star : bgStars)
        {
            glPointSize(static_cast<GLfloat>(star.w));

            GLfloat posArray[3] = {
                static_cast<GLfloat>(star.x),
                static_cast<GLfloat>(star.y),
                static_cast<GLfloat>(star.z)
            };
            glUniform3fv(glGetUniformLocation(shaderProgram, "particlePosition"), 1, posArray);

            GLfloat colorArray[3] = { 1.0f, 1.0f, 1.0f }; // Weiß
            glUniform3fv(glGetUniformLocation(shaderProgram, "particleColor"), 1, colorArray);

            glDrawArrays(GL_POINTS, 0, 1);
        }
    }


    // Partikel rendern
    glPointSize(5.0f);
    for (const auto& particle : *particles)
    {

        // Debug-Ausgabe: Position der Partikel prüfen
        std::cout << "Particle Position: (" 
              << particle->position.x << ", " 
              << particle->position.y << ", " 
              << particle->position.z << ")" << std::endl;

        vec3 color = vec3(1.0, 1.0, 1.0); // Standardfarbe Weiß
        if (particle->galaxyPart == 1)
            color = vec3(1.0, 0.0, 0.0); // Rot
        else if (particle->galaxyPart == 2)
            color = vec3(0.0, 1.0, 0.0); // Grün
        else if (particle->galaxyPart == 3)
            color = vec3(0.0, 0.0, 1.0); // Blau

        GLfloat scaledPosArray[3] = {
            static_cast<GLfloat>(particle->position.x * globalScale),
            static_cast<GLfloat>(particle->position.y * globalScale),
            static_cast<GLfloat>(particle->position.z * globalScale)
        };
        glUniform3fv(glGetUniformLocation(shaderProgram, "particlePosition"), 1, scaledPosArray);

        GLfloat colorArray[3] = {
            static_cast<GLfloat>(color.x),
            static_cast<GLfloat>(color.y),
            static_cast<GLfloat>(color.z)
        };
        glUniform3fv(glGetUniformLocation(shaderProgram, "particleColor"), 1, colorArray);

        glDrawArrays(GL_POINTS, 0, 1);
    }

    glBindVertexArray(0);

} */



void Engine::renderParticles()
{
    // Binden des Framebuffers
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Deaktivieren Sie den Tiefentest und das Z-Buffering
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // In der renderParticles-Funktion
    glUseProgram(shaderProgram);

    // Erstellen der Projektionsmatrix und Sichtmatrix
    mat4 projection = mat4::perspective(45.0f, 800.0f / 600.0f, 0.1f, cameraViewDistance);
    mat4 viewMatrix = mat4::lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp);

    // Setzen der Matrizen im Shader
    GLuint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");

    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projection.data());
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, viewMatrix.data());

    // Vertex Array Object (VAO) binden
    glBindVertexArray(VAO);

    if (BGstars)
    {
        //render the background bgStars
        for (int i = 0; i < amountOfStars; i++)
        {
            glPointSize(bgStars[i].w);

            // Setzen der Position im Shader
            vec3 pos(bgStars[i].x, bgStars[i].y, bgStars[i].z);
            float posArray[3];
            pos.toFloatArray(posArray);
            glUniform3fv(glGetUniformLocation(shaderProgram, "particlePosition"), 1, posArray);

            // Setzen der Farbe im Shader
            vec3 color(1.0f, 1.0f, 1.0f);
            float colorArray[3];
            color.toFloatArray(colorArray);
            glUniform3fv(glGetUniformLocation(shaderProgram, "particleColor"), 1, colorArray);

            // Zeichnen des Punktes
            glDrawArrays(GL_POINTS, 0, 1);
        }
    }

    for (const auto& particle : *particles)
    {
        if(particle->galaxyPart == 3 && (renderMode == 2 || renderMode == 3 || renderMode == 4 || renderMode == 7 || renderMode == 8 || renderMode == 9))
        {
            continue;
        } 
        if (particle->galaxyPart == 2 && (renderMode == 3 || renderMode == 5 || renderMode == 8 || renderMode == 10))
        {
            continue;
        }
        if(particle->galaxyPart == 1 && (renderMode == 5 || renderMode == 4 || renderMode == 9 || renderMode == 10))
        {
            continue;
        }

        double red = 1;
        double green = 1;
        double blue = 1;

        vec3 color = vec3(red, green, blue);
        if(renderMode <= 5)
        {
            if (densityAv != 0) 
            {
                color.x = particle->density * 10 / densityAv;
                color.y = 0;
                color.z = densityAv / particle->density;

                double plus = 0;
                if(particle->density * 100 / densityAv > 1) plus = particle->density / densityAv / 10;

                color.x += plus * 10;
                color.y += plus;
                color.z += plus;
            }
        }
        else
        {
            if(particle->galaxyPart == 1)
            {
                color = vec3(1, 0, 0);
            }
            if(particle->galaxyPart == 2)
            {
                color = vec3(0, 1, 0);
            }
            if(particle->galaxyPart == 3)
            {
                color = vec3(0, 0, 1);
            }
        }

        vec3 scaledPosition = particle->position * globalScale;

        // Setzen Position im Shader
        float scaledPosArray[3];
        scaledPosition.toFloatArray(scaledPosArray);
        glUniform3fv(glGetUniformLocation(shaderProgram, "particlePosition"), 1, scaledPosArray);

        // Setzen Farbe im Shader
        float colorArray[3];
        color.toFloatArray(colorArray);
        glUniform3fv(glGetUniformLocation(shaderProgram, "particleColor"), 1, colorArray);

        glPointSize(0.5f); // Setzen der Punktgröße auf 5 Pixel

        // Zeichnen Punkt
        glDrawArrays(GL_POINTS, 0, 1);
    }

    // VAO lösen
    glBindVertexArray(0);
}



void Engine::processInput()
{
    if (RenderLive)
    {
        // Toggle zwischen Kameramodi mit "L"
        static bool lKeyWasPressedLastFrame = false;

        // Prüfen, ob die Taste "L" gerade gedrückt wurde
        bool lKeyPressed =
#ifdef WIN32
        (GetAsyncKeyState('L') & 0x8000) != 0;
#else
            glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS;
#endif

        // Umschalten des Kameramodus nur, wenn die Taste neu gedrückt wurde
        if (lKeyPressed && !lKeyWasPressedLastFrame)
        {
            focusedCamera = !focusedCamera;
        }

        // Aktualisieren des letzten Tastendruckzustands
        lKeyWasPressedLastFrame = lKeyPressed;

        if (RenderLive)
        {
#ifdef WIN32
            if (GetAsyncKeyState(VK_CONTROL) < 0)
#else
            if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
#endif
            {
                // Kamerabewegung
                float index = 0.1f;
                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                    cameraPosition += rushSpeed * index * cameraFront;
                if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                    cameraPosition -= rushSpeed * index * cameraFront;
                if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                    cameraPosition -= rushSpeed * index * cameraFront.cross(cameraUp);
                if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                    cameraPosition += rushSpeed * index * cameraFront.cross(cameraUp);
            }
            else
            {
                // Kamerabewegung
                float index = 0.1f;
                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                    cameraPosition += cameraSpeed * index * cameraFront;
                if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                    cameraPosition -= cameraSpeed * index * cameraFront;
                if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                    cameraPosition -= cameraSpeed * index * cameraFront.cross(cameraUp);
                if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                    cameraPosition += cameraSpeed * index * cameraFront.cross(cameraUp);
            }
        }
    }
    if (RenderLive == false)
    {
        float index = 0.1f; 
        cameraPosition += cameraSpeed * index * cameraFront.cross(cameraUp);
        //std::cout << "Camera Position: " << cameraPosition.x << " " << cameraPosition.y << " " << cameraPosition.z << std::endl;
    }

    
    if (focusedCamera) {
        // Richtet die Kamera auf den Ursprung aus
        vec3 direction = (vec3(0, 0, 0) - cameraPosition).normalize();
        cameraFront = direction;
    }
    // Aktualisieren der Ansichtsmatrix (Kameraposition und Blickrichtung)
    view = mat4::lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp);

    // Setzen der Matrizen im Shader
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view.data());
}

void Engine::processMouseInput()
{
    if (focusedCamera == false)
    {
        // Erfassen Sie die Mausbewegung
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        static double lastMouseX = mouseX;
        static double lastMouseY = mouseY;

        double xOffset = mouseX - lastMouseX;
        double yOffset = lastMouseY - mouseY; // Umgekehrtes Vorzeichen für umgekehrte Mausrichtung

        lastMouseX = mouseX;
        lastMouseY = mouseY;

        const float sensitivity = 0.05f;
        xOffset *= sensitivity;
        yOffset *= sensitivity;

        cameraYaw += xOffset;
        cameraPitch += yOffset;

        // Begrenzen Sie die Kamerapitch, um ein Überdrehen zu verhindern
        if (cameraPitch > 89.0f)
            cameraPitch = 89.0f;
        if (cameraPitch < -89.0f)
            cameraPitch = -89.0f;

        // Berechnen der neuen Kamerarichtung
        vec3 newFront;
        newFront.x = cos(radians(cameraYaw)) * cos(radians(cameraPitch));
        newFront.y = sin(radians(cameraPitch));
        newFront.z = sin(radians(cameraYaw)) * cos(radians(cameraPitch));
        cameraFront = newFront.normalize();

        // Aktualisieren der Ansichtsmatrix
        view = mat4::lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp);
    }
}


void Engine::checkShaderCompileStatus(GLuint shader, const char* shaderType)
{
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Error compiling " << shaderType << " shader:\n" << infoLog << std::endl;
    }
}

void Engine::checkShaderLinkStatus(GLuint program)
{
    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Error linking shader program:\n" << infoLog << std::endl;
    }
}

bool Engine::clean()
{
    // Aufräumen und beenden
    glfwTerminate();
    return true;
}

void Engine::calcTime(int index) 
{
    calcTime(vec3(0, 0, 0), index);
}

void Engine::calcTime(vec3 position, int index)
{
    passedTime = (index * faktor) * deltaTime;
    //std::cout << deltaTime << std::endl;

    int passedTimeInSec = passedTime / 86400;

    std::string Unit;

    //set the right unit
    if (passedTime < 60) { Unit = " s"; }
    else if (passedTime < 3600) { passedTime /= 60; Unit = " min"; }
    else if (passedTime < 86400) { passedTime /= 3600; Unit = " h"; }
    else if (passedTime < 31536000) { passedTime /= 86400; Unit = " days"; }
    else { passedTime /= 31536000; Unit = " years"; }

    std::string time; 
    //set the time to like millions, billions, trillions, ...
    if (passedTime < 1000) 
    { 
        //passed time to string with 1 decimal after the point
        std::string newTime = std::to_string(passedTime);
        newTime = newTime.substr(0, newTime.find(".") + 2);
        time = newTime; 
    }
	else if (passedTime < 1000000) 
    { 
        passedTime /= 1000; 
        //passed time to string with 1 decimal after the point
        std::string newTime = std::to_string(passedTime);
        newTime = newTime.substr(0, newTime.find(".") + 2);
        time = newTime + " thousand"; 
    }
	else if (passedTime < 1000000000) 
    { 
        passedTime /= 1000000; 
		//passed time to string with 1 decimal after the point
		std::string newTime = std::to_string(passedTime);
		newTime = newTime.substr(0, newTime.find(".") + 2);
		time = newTime + " million";
    }
	else if (passedTime < 1000000000000) 
    {
        passedTime /= 1000000000;
        //passed time to string with 1 decimal after the point
        std::string newTime = std::to_string(passedTime);
        newTime = newTime.substr(0, newTime.find(".") + 2);
        time = newTime + " billion";
    }
	else if (passedTime < 1000000000000000) 
    { 
        passedTime /= 1000000000000; 
		//passed time to string with 1 decimal after the point
		std::string newTime = std::to_string(passedTime);
		newTime = newTime.substr(0, newTime.find(".") + 2);
		time = newTime + " trillion";
    }
	else 
    { 
        passedTime /= 1000000000000000;
        //passed time to string with 1 decimal after the point
        std::string newTime = std::to_string(passedTime);
        newTime = newTime.substr(0, newTime.find(".") + 2);
        time = newTime + " quadrillion";
    }

    std::cout << "Passed time: " << time << Unit << std::endl;
}

double Engine::random(double min, double max)
{
    // Generiere eine zufällige Gleitkommazahl zwischen min und max
    double randomFloat = min + static_cast<double>(rand()) / static_cast<double>(RAND_MAX) * (max - min);

    return randomFloat;
}

void Engine::calculateGlobalScale()
{
    double maxDistance = 0;
    for (const auto& particle : *particles)
    {
        // Skip ungültige oder extreme Werte
        if (particle->position.x == 0 && particle->position.y == 0 && particle->position.z == 0)
            continue;
        if (std::isinf(particle->position.x) || std::isinf(particle->position.y) || std::isinf(particle->position.z) ||
            std::isnan(particle->position.x) || std::isnan(particle->position.y) || std::isnan(particle->position.z))
            continue;

        double distance = sqrt(pow(particle->position.x, 2) + pow(particle->position.y, 2) + pow(particle->position.z, 2));
        if (distance > maxDistance)
            maxDistance = distance;
    }

    // Vermeide die Verarbeitung, wenn alle Positionen ungültig sind
    if (maxDistance == 0) {
        globalScale = 1; // Standardwert oder Fehlerwert setzen
        return;
    }

    maxDistance = maxDistance * 2; // Durchmesser des Systems
    int exponent = static_cast<int>(std::floor(std::log10(std::abs(maxDistance))));
    globalScale = maxDistance / pow(10, exponent * 2 - 2);
}