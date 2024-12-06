// Autor: Nedeljko Tesanovic
// Opis: Zestoko iskomentarisan program koji crta sareni trougao u OpenGL-u

#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES
#define STB_IMAGE_IMPLEMENTATION

 //Biblioteke za stvari iz C++-a (unos, ispis, fajlovi, itd - potrebne za kompajler sejdera) 
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <ctime>
#include <chrono>
#include <thread>
#include <filesystem>
#include <random>
#include <map>
#include "stb_image.h"
#include <ft2build.h>
#include FT_FREETYPE_H

//Biblioteke OpenGL-a
#include <GL/glew.h>   //Omogucava laksu upotrebu OpenGL naredbi
#include <GLFW/glfw3.h>//Olaksava pravljenje i otvaranje prozora (konteksta) sa OpenGL sadrzajem

unsigned int compileShader(GLenum type, const char* source); //Uzima kod u fajlu na putanji "source", kompajlira ga i vraca sejder tipa "type"
unsigned int createShader(const char* vsSource, const char* fsSource); //Pravi objedinjeni sejder program koji se sastoji od Verteks sejdera ciji je kod na putanji vsSource i Fragment sejdera na putanji fsSource

GLFWwindow* window;

int maxAmmoCount = 10;
int ammoCount = 10;
bool isGunReady = true;
bool inPeriscopeView = false;
float hydraulicVoltage = 50.0f;
float lastVoltage = 50.0f;
float vibrationEffect = 0.0f;   
float turretAngle = 0.0f;  
float turretRotationSpeed = 0.1f;
bool hydraulicOn = true; 
bool opticalView = false;
bool isHPressed = false;
bool isVPressed = false;
bool isCPressed = false;
int screenWidth, screenHeight;
float aspect = 1920.0f / 1080.0f; 
unsigned int texture = 0;
bool textureLoaded = false;

const double targetFPS = 60.0;
const std::chrono::milliseconds targetFrameDuration(static_cast<int>(1000.0 / targetFPS));

std::vector<std::pair<float, float>> targets; // Pozicije meta
bool targetsHit[3] = { false, false, false };

std::vector<float> lampVertices;

std::vector<float> tankCannonVertices;

std::vector<float> tankCannonPipeVertices = {
    0.0f, -0.55f,
    -0.02f, -0.2f,
    0.02f, -0.2f
};

float crosshairCenterX = 0.0f;
float crosshairCenterY = 0.0f;

std::vector<float> crosshairVertices = {
    -0.02f + crosshairCenterX,  0.02f * aspect + crosshairCenterY,
    0.02f + crosshairCenterX,  -0.02f * aspect + crosshairCenterY,
    -0.02f + crosshairCenterX,  -0.02f * aspect + crosshairCenterY,
    0.02f + crosshairCenterX,   0.02f * aspect + crosshairCenterY
};

std::vector<float> mouseVertices;

std::vector<float> voltmeterVertices = {
    0.8f, -0.8f,
    0.9f, -0.8f,
}; 

std::vector<float> needleRotationPoint = {
    0.8f, -0.8f,
};

std::vector<float> target1Vertices;
unsigned int target1VBO;
bool isTarget1Hit = false;
std::vector<float> target2Vertices;
unsigned int target2VBO;
bool isTarget2Hit = false;
std::vector<float> target3Vertices;
unsigned int target3VBO;
bool isTarget3Hit = false;

std::vector<float> ammoIndicatorVertices;

std::vector<float> outsideVertices;
std::vector<float> outsideTextCoords;

unsigned int outsideVbo;
unsigned int outsideTexVbo;



struct Character {
    GLuint TextureID;  // ID handle of the glyph texture
    int Size[2];       // Width and height of the glyph
    int Bearing[2];    // Offset from baseline to left/top of glyph
    unsigned int Advance; // Horizontal offset to advance to the next glyph

    Character(GLuint textureID, int sizeX, int sizeY, int bearingX, int bearingY, unsigned int advance)
        : TextureID(textureID), Advance(advance) {
        Size[0] = sizeX;
        Size[1] = sizeY;
        Bearing[0] = bearingX;
        Bearing[1] = bearingY;
    }

    // Default constructor
    Character() : TextureID(0), Advance(0) {
        Size[0] = Size[1] = 0;
        Bearing[0] = Bearing[1] = 0;
    }
};

std::map<char, Character> Characters;
GLuint textVAO, textVBO;

std::chrono::time_point<std::chrono::high_resolution_clock> lastShotTime = std::chrono::high_resolution_clock::now() - std::chrono::seconds(10);

void checkGLError(const char* label) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cout << "OpenGL Error (" << label << "): " << err << std::endl;
    }
}

unsigned int loadTexture(const char* path) {
    unsigned int textureID; 

    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // or GL_NEAREST
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // or GL_NEAREST
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // or GL_CLAMP_TO_EDGE
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // or GL_CLAMP_TO_EDGE
    }
    else {
        std::cerr << "Failed to load texture: " << path << std::endl;
    }
    stbi_image_free(data);
    return textureID;
}

void loadFont(const char* fontPath) {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "Could not initialize FreeType Library" << std::endl;
        return;
    }

    FT_Face face;
    if (FT_New_Face(ft, fontPath, 0, &face)) {
        std::cerr << "Failed to load font" << std::endl;
        return;
    }

    FT_Set_Pixel_Sizes(face, 0, 48);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Disable byte-alignment restriction

    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "Failed to load glyph: " << c << std::endl;
            continue;
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character(
            texture,
            face->glyph->bitmap.width, face->glyph->bitmap.rows,
            face->glyph->bitmap_left, face->glyph->bitmap_top,
            face->glyph->advance.x
        );
        Characters.insert(std::pair<char, Character>(c, character));
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

void initTextRendering() {
    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);

    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void renderText(GLuint shader, std::string text, float x, float y, float scale, float r, float g, float b) {
    glUseProgram(shader);
    glUniform3f(glGetUniformLocation(shader, "textColor"), r, g, b);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    for (char c : text) {
        Character ch = Characters[c];

        float xpos = x + ch.Bearing[0] * scale;
        float ypos = y - (ch.Size[1] - ch.Bearing[1]) * scale;

        float w = ch.Size[0] * scale;
        float h = ch.Size[1] * scale;

        float vertices[6][4] = {
            { xpos, ypos + h, 0.0f, 0.0f },
            { xpos, ypos, 0.0f, 1.0f },
            { xpos + w, ypos, 1.0f, 1.0f },

            { xpos, ypos + h, 0.0f, 0.0f },
            { xpos + w, ypos, 1.0f, 1.0f },
            { xpos + w, ypos + h, 1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += (ch.Advance >> 6) * scale; // Bitshift by 6 to convert from 1/64th pixels
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

std::pair<unsigned int, unsigned int> createVAO(const std::vector<float>& vertices) {
    unsigned int vao, vbo;

    // Generiši i binduj VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Generiši i binduj VBO
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Popuni VBO podacima
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Poveži podatke sa atributima u sejderu (npr. layout location 0 za pozicije)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Odveži VAO i VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return std::pair<unsigned int, unsigned int> {vao, vbo};
}

void createAmmoIndicator()
{
    float width = 0.01f;  
    float height = 0.1f; 
    float gap = 0.005f;  

    for (int i = 0; i < ammoCount; ++i) {
        float xOffset = -0.9f + (width + gap) * i; 
        float yOffset = 0.9f;  

        ammoIndicatorVertices.push_back(xOffset);      ammoIndicatorVertices.push_back(yOffset);      // Top-left
        ammoIndicatorVertices.push_back(xOffset + width); ammoIndicatorVertices.push_back(yOffset);      // Top-right
        ammoIndicatorVertices.push_back(xOffset);      ammoIndicatorVertices.push_back(yOffset - height); // Bottom-left

        ammoIndicatorVertices.push_back(xOffset + width); ammoIndicatorVertices.push_back(yOffset);      // Top-right
        ammoIndicatorVertices.push_back(xOffset + width); ammoIndicatorVertices.push_back(yOffset - height); // Bottom-right
        ammoIndicatorVertices.push_back(xOffset);      ammoIndicatorVertices.push_back(yOffset - height); // Bottom-left
    }
}

void drawAmmoIndicator(unsigned int vao, unsigned int shader)
{
    glUseProgram(shader);

    int ammoCountLocation = glGetUniformLocation(shader, "ammoCount");
    glUniform1i(ammoCountLocation, ammoCount);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, maxAmmoCount * 6);  
    glBindVertexArray(0);
}

void createCrosshair()
{
    crosshairVertices.clear();
    crosshairVertices.push_back(-0.02f + crosshairCenterX);
    crosshairVertices.push_back(0.02f * aspect + crosshairCenterY);
    crosshairVertices.push_back(0.02f + crosshairCenterX);
    crosshairVertices.push_back(-0.02f * aspect + crosshairCenterY);
    crosshairVertices.push_back(-0.02f + crosshairCenterX);
    crosshairVertices.push_back(-0.02f * aspect + crosshairCenterY);
    crosshairVertices.push_back(0.02f + crosshairCenterX);
    crosshairVertices.push_back(0.02f * aspect + crosshairCenterY);
}

void drawCrosshair(unsigned int vao, unsigned int shader)
{
    glUseProgram(shader);

    int colorLocation = glGetUniformLocation(shader, "color");
    glUniform3f(colorLocation, 1.0f, 0.0f, 0.0f);

    glBindVertexArray(vao);
    glDrawArrays(GL_LINES, 0, 4);
    glBindVertexArray(0);
}

double calculateDistance(double x1, double y1, double x2, double y2) {
    return std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2));
}

struct Point {
    double x, y;
};

Point closestPointOnCircle(double cx, double cy, double r, double px, double py) {
    // Compute direction vector
    double dx = px - cx;
    double dy = py - cy;

    // Calculate distance from center to the point
    double distance = std::sqrt(dx * dx + dy * dy);

    // Normalize the direction vector
    double nx = dx / distance;
    double ny = dy / distance;

    // Calculate the closest point on the circle
    Point closestPoint;
    closestPoint.x = cx + r * nx;
    closestPoint.y = cy + r * ny;

    return closestPoint;
}

void createMousePointer(GLFWwindow* window)
{
    mouseVertices.clear();
    double mouseXDouble, mouseYDouble;
    glfwGetCursorPos(window, &mouseXDouble, &mouseYDouble);

    mouseXDouble = (float)(2.0f * mouseXDouble / screenWidth - 1.0f);
    mouseYDouble = (float)(1.0f - 2.0f * mouseYDouble / screenHeight) / aspect;

    float range = 0.28f;
    double distance = calculateDistance(mouseXDouble, mouseYDouble, 0.0f, 0.0f);
    if (distance > range) {
        Point closest = closestPointOnCircle(0.0f, 0.0f, range, mouseXDouble, mouseYDouble);
        mouseXDouble = closest.x;
        mouseYDouble = closest.y;
    }

    mouseYDouble *= aspect;
    mouseVertices.push_back(mouseXDouble - 0.03f);
    mouseVertices.push_back(mouseYDouble);
    mouseVertices.push_back(mouseXDouble + 0.03f);
    mouseVertices.push_back(mouseYDouble);

    mouseVertices.push_back(mouseXDouble);
    mouseVertices.push_back(mouseYDouble + 0.03f * aspect);
    mouseVertices.push_back(mouseXDouble);
    mouseVertices.push_back(mouseYDouble - 0.03f * aspect);

    float offset = 0.0015f;
    if (mouseXDouble < crosshairCenterX)
        if (crosshairCenterX - mouseXDouble < offset)
            crosshairCenterX = mouseXDouble;
        else
            crosshairCenterX -= offset;
    else
        if (mouseXDouble - crosshairCenterX < offset)
            crosshairCenterX = mouseXDouble;
        else
            crosshairCenterX += offset;

    if (mouseYDouble < crosshairCenterY)
        if (crosshairCenterY - mouseYDouble < offset)
            crosshairCenterY = mouseYDouble;
        else
            crosshairCenterY -= offset;
    else
        if (mouseYDouble - crosshairCenterY < offset)
            crosshairCenterY = mouseYDouble;
        else
            crosshairCenterY += offset;
}

void drawMousePointer(unsigned int vao, unsigned int shader)
{
    glUseProgram(shader);

    int colorLocation = glGetUniformLocation(shader, "color");
    glUniform3f(colorLocation, 1.0f, 0.0f, 0.0f);

    glBindVertexArray(vao);
    glDrawArrays(GL_LINES, 0, 4);
    glBindVertexArray(0);
}

void createLamp() 
{
    float centerX = 0.8f;
    float centerY = 0.8f;
    float radius = 0.05f;
    int numSegments = 100; // Number of segments to approximate the circle

    // Add center vertex
    lampVertices.push_back(centerX);
    lampVertices.push_back(centerY);

    for (int i = 0; i <= numSegments; ++i) {
        float angle = 2.0f * M_PI * i / numSegments; // Calculate angle for this segment
        float x = centerX + radius * cos(angle);
        float y = centerY + radius * aspect * sin(angle);

        lampVertices.push_back(x);
        lampVertices.push_back(y);
    }
}

void drawLamp(unsigned int vao, unsigned int shader)
{
    glUseProgram(shader);

    float red = isGunReady ? 0.0f : 1.0f;
    float green = isGunReady ? 1.0f : 0.0f;
    float blue = 0.0f;

    int lampColorLocation = glGetUniformLocation(shader, "color");
    glUniform3f(lampColorLocation, red, green, blue);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, lampVertices.size() / 2);
    glBindVertexArray(0);
}

void createTankCannon()
{
    float centerX = 0.0f;
    float centerY = -0.5f;
    float radius = 0.02f;
    int numSegments = 100; // Number of segments to approximate the circle

    // Add center vertex
    tankCannonVertices.push_back(centerX);
    tankCannonVertices.push_back(centerY);

    for (int i = 0; i <= numSegments; ++i) {
        float angle = 2.0f * M_PI * i / numSegments; // Calculate angle for this segment
        float x = centerX + radius * cos(angle);
        float y = centerY + radius * aspect * sin(angle);

        tankCannonVertices.push_back(x);
        tankCannonVertices.push_back(y);
    }
}

void drawTankCannon(unsigned int vao, unsigned int shader)
{
    glUseProgram(shader);

    int colorLocation = glGetUniformLocation(shader, "color");
    glUniform3f(colorLocation, 0.0f, 0.0f, 0.0f);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, tankCannonVertices.size() / 2);
    glBindVertexArray(0);
}

void drawTankCannonPipe(unsigned int vao, unsigned int shader)
{
    glUseProgram(shader);

    int colorLocation = glGetUniformLocation(shader, "color");
    glUniform3f(colorLocation, 0.0f, 0.0f, 0.0f);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

float getRandomFloat(float min, float max) {
    // Create a random number generator
    std::random_device rd;  // Random device to seed the generator
    std::mt19937 gen(rd());  // Mersenne Twister generator, seeded with rd()
    std::uniform_real_distribution<> dis(min, max);  // Uniform distribution

    return dis(gen);  // Generate a random float between min and max
}

void createTargets()
{
    float minX = -0.95f;
    float maxX = 0.95f;
    float centerX = getRandomFloat(minX, maxX);

    float minY = -0.25f;
    float maxY = -0.1f;
    float centerY = getRandomFloat(minY, maxY);
    float radius = 0.02f;
    int numSegments = 50; // Number of segments to approximate the circle

    // Add center vertex
    target1Vertices.push_back(centerX);
    target1Vertices.push_back(centerY);

    for (int i = 0; i <= numSegments; ++i) {
        float angle = 2.0f * M_PI * i / numSegments; // Calculate angle for this segment
        float x = centerX + radius * cos(angle);
        float y = centerY + radius * aspect * sin(angle);

        target1Vertices.push_back(x);
        target1Vertices.push_back(y);
    }

    centerX = getRandomFloat(minX, maxX);
    centerY = getRandomFloat(minY, maxY);

    // Add center vertex
    target2Vertices.push_back(centerX);
    target2Vertices.push_back(centerY);

    for (int i = 0; i <= numSegments; ++i) {
        float angle = 2.0f * M_PI * i / numSegments; // Calculate angle for this segment
        float x = centerX + radius * cos(angle);
        float y = centerY + radius * aspect * sin(angle);

        target2Vertices.push_back(x);
        target2Vertices.push_back(y);
    }

    centerX = getRandomFloat(minX, maxX);
    centerY = getRandomFloat(minY, maxY);

    // Add center vertex
    target3Vertices.push_back(centerX);
    target3Vertices.push_back(centerY);

    for (int i = 0; i <= numSegments; ++i) {
        float angle = 2.0f * M_PI * i / numSegments; // Calculate angle for this segment
        float x = centerX + radius * cos(angle);
        float y = centerY + radius * aspect * sin(angle);

        target3Vertices.push_back(x);
        target3Vertices.push_back(y);
    }
}

void drawTargets(unsigned int vao, unsigned int shader, GLint numVertices) 
{
    glUseProgram(shader);

    int colorLocation = glGetUniformLocation(shader, "color");
    glUniform3f(colorLocation, 0.0f, 0.0f, 0.0f);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, numVertices);
    glBindVertexArray(0);
}

bool isHit(float targetX, float targetY, float x, float y) {
    return sqrt(pow(targetX - x, 2) + pow(targetY - y, 2)) < 0.05;
}

void drawVoltmeterNeedle(unsigned int vao, unsigned int shader)
{
    glUseProgram(shader);
    
    float angle = -(1.0f - hydraulicVoltage / 100.0f) * M_PI;

    float needleX = 0.2f * cos(angle * M_PI / 180.0f);
    float needleY = 0.2f * sin(angle * M_PI / 180.0f);

    vibrationEffect += 0.1f;
    if (vibrationEffect > 6.28f) 
        vibrationEffect = 0.0f;

    glUniform1f(glGetUniformLocation(shader, "rotationAngle"), angle);
    glUniform1f(glGetUniformLocation(shader, "voltage"), hydraulicVoltage);
    glUniform1f(glGetUniformLocation(shader, "vibrationEffect"), vibrationEffect);
    glUniform2f(glGetUniformLocation(shader, "rotationPoint"), needleRotationPoint[0], needleRotationPoint[1]);
    glUniform1i(glGetUniformLocation(shader, "hydraulicsEnabled"), hydraulicOn);
    glUniform1i(glGetUniformLocation(shader, "hydraulicsEnabled"), aspect);

    glBindVertexArray(vao);
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);
}

void createOutside()
{
    float centerX = 0.0f;
    float centerY = 0.0f;
    float radius = 0.3f;
    int numSegments = 100; // Number of segments to approximate the circle

    // Add center vertex
    outsideVertices.push_back(centerX);
    outsideVertices.push_back(centerY);

    outsideTextCoords.push_back(0.5f);
    outsideTextCoords.push_back(0.5f);

    float texCenterX = 0.4f;  
    float texCenterY = 0.25f;

    for (int i = 0; i <= numSegments; ++i) {
        float angle = 2.0f * M_PI * i / numSegments; // Calculate angle for this segment
        float x = centerX + radius * cos(angle);
        float y = centerY + radius * aspect * sin(angle);

        outsideVertices.push_back(x);
        outsideVertices.push_back(y);

        // Calculate texture coordinates for this point (maps to the texture's edge)
        float texX = texCenterX + 0.2f * (cos(angle) + 1.0f) * 0.5f;
        float texY = (sin(angle) + 1.0f) * 0.5f; 

        //std::cout << texX << ", " << texY << std::endl;
        texX = texX - floor(texX);

        outsideTextCoords.push_back(texX);
        outsideTextCoords.push_back(texY);
    }
}

void updateTurretView(bool isRight)
{
    float centerX = 0.0f;
    float centerY = 0.0f;
    float radius = 0.3f;
    int numSegments = 100;

    outsideVertices.clear();
    outsideTextCoords.clear();

    outsideVertices.push_back(centerX);
    outsideVertices.push_back(centerY);

    float textureOffsetX = 0.1f; 
    float textureOffsetY = 0.0f;  


    outsideTextCoords.push_back(0.5f + turretAngle * turretRotationSpeed);  
    outsideTextCoords.push_back(0.5f);

    float texCenterX = 0.4f;  
    float texCenterY = 0.5f;  

    for (int i = 0; i <= numSegments; ++i) {
        float angle = 2.0f * M_PI * i / numSegments;
        //angle += turretAngle * M_PI / 180.0f; // Adjust the angle by the turret rotation

        // Calculate vertex positions based on rotation and aspect ratio
        float x = centerX + radius * cos(angle);
        float y = centerY + radius * aspect * sin(angle);

        outsideVertices.push_back(x);
        outsideVertices.push_back(y);

        // Map the angle to texture coordinates
        float texX = texCenterX + 0.2f * (cos(angle) + 1.0f) * 0.5f;  // Horizontal texture mapping
        float texY = (sin(angle) + 1.0f) * 0.5f;  // Vertical texture mapping

        // Apply the offset to the texture coordinates
        texX += turretAngle * turretRotationSpeed;
        texY += textureOffsetY;

        // Wrap the texture coordinates in the [0.0, 1.0] range to avoid out-of-bounds errors
        //texX = texX - floor(texX);  // Wrap texX
        //texY = texY - floor(texY);  // Wrap texY

        // Add the updated texture coordinates to the vector
        outsideTextCoords.push_back(texX);
        outsideTextCoords.push_back(texY);
    }

    radius = 0.02f;
    numSegments = 50;
    float offset = turretRotationSpeed / 35;
    if (isRight)
        target1Vertices[0] = target1Vertices[0] + offset;
    else
        target1Vertices[0] = target1Vertices[0] - offset;

    for (int i = 0; i <= numSegments * 2; i += 2) {
        float angle = 2.0f * M_PI * i / numSegments; // Calculate angle for this segment
        float x = target1Vertices[0] + radius * cos(angle);
        float y = target1Vertices[1] + radius * aspect * sin(angle);

        target1Vertices[i + 2] = x;
        target1Vertices[i + 3] = y;
    }

    if (isRight)
        target2Vertices[0] = target2Vertices[0] + offset;
    else
        target2Vertices[0] = target2Vertices[0] - offset;

    for (int i = 0; i <= numSegments * 2; i += 2) {
        float angle = 2.0f * M_PI * i / numSegments; // Calculate angle for this segment
        float x = target2Vertices[0] + radius * cos(angle);
        float y = target2Vertices[1] + radius * aspect * sin(angle);

        target2Vertices[i + 2] = x;
        target2Vertices[i + 3] = y;
    }

    if (isRight)
        target3Vertices[0] = target3Vertices[0] + offset;
    else
        target3Vertices[0] = target3Vertices[0] - offset;

    for (int i = 0; i <= numSegments * 2; i += 2) {
        float angle = 2.0f * M_PI * i / numSegments; // Calculate angle for this segment
        float x = target3Vertices[0] + radius * cos(angle);
        float y = target3Vertices[1] + radius * aspect * sin(angle);

        target3Vertices[i + 2] = x;
        target3Vertices[i + 3] = y;
    }
}

void loadAndBindTexture(unsigned int shader)
{
    glUseProgram(shader);
    if (!textureLoaded) {
        texture = loadTexture("outside.jpg");
        textureLoaded = true;
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(shader, "texture1"), 0);
}

void drawOutisde(unsigned int vao, unsigned int shader)
{
    glUseProgram(shader);

    loadAndBindTexture(shader);
    // Bind the VAO for drawing
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, outsideVertices.size() / 2);
    glBindVertexArray(0);
}

unsigned int setupOutsideVAO()
{
    unsigned int vao;

    // Create the VAO, VBO (for vertex data), and texVbo (for texture data)
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &outsideVbo);    // Vertex positions VBO
    glGenBuffers(1, &outsideTexVbo); // Texture coordinates VBO

    // Bind the VAO
    glBindVertexArray(vao);

    // Bind the VBO for vertex positions and upload data
    glBindBuffer(GL_ARRAY_BUFFER, outsideVbo);
    glBufferData(GL_ARRAY_BUFFER, outsideVertices.size() * sizeof(float), outsideVertices.data(), GL_STATIC_DRAW);

    // Position attribute (location = 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Bind the texVbo for texture coordinates and upload data
    glBindBuffer(GL_ARRAY_BUFFER, outsideTexVbo);
    glBufferData(GL_ARRAY_BUFFER, outsideTextCoords.size() * sizeof(float), outsideTextCoords.data(), GL_STATIC_DRAW);

    // Texture coordinate attribute (location = 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    // Unbind the VBOs and VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return vao;
}

void updateBuffers()
{
    // Update VBO with new vertex positions
    glBindBuffer(GL_ARRAY_BUFFER, outsideVbo); // Bind the VBO for vertices
    glBufferSubData(GL_ARRAY_BUFFER, 0, outsideVertices.size() * sizeof(float), outsideVertices.data()); // Update vertex data

    // Update texVbo with new texture coordinates
    glBindBuffer(GL_ARRAY_BUFFER, outsideTexVbo); // Bind the VBO for texture coordinates
    glBufferSubData(GL_ARRAY_BUFFER, 0, outsideTextCoords.size() * sizeof(float), outsideTextCoords.data()); // Update texture data

    // Unbind the buffers (optional, but good practice)
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void updateTargetVBO(unsigned int vbo, std::vector<float>& vertices) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

bool isPointInCircle(Point circleCenter, double radius, Point point) {
    // Calculate squared distance from the point to the center of the circle
    double dx = point.x - circleCenter.x;
    double dy = point.y - circleCenter.y;
    double distanceSquared = dx * dx + dy * dy;

    // Compare with the squared radius
    return distanceSquared <= radius * radius;
}

void processInput() 
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS && hydraulicVoltage < 100.0f && hydraulicOn) {
        hydraulicVoltage += 1.0f;
        lastVoltage = hydraulicVoltage;
    }

    if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS && hydraulicVoltage > 0.0f && hydraulicOn) {
        hydraulicVoltage -= 1.0f;
        lastVoltage = hydraulicVoltage;
    }

    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
        if (!isHPressed) {
            hydraulicOn = !hydraulicOn;
            if (hydraulicOn)
                hydraulicVoltage = lastVoltage;
            else
                hydraulicVoltage = 0.0f;
            isHPressed = true;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_RELEASE) {
        isHPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
        if (!isVPressed) {
            opticalView = true;
            isVPressed = true;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_RELEASE) {
        isVPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        if (!isVPressed) {
            opticalView = false;
            isVPressed = true;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE) {
        isVPressed = false;
    }

    if (hydraulicOn) {
        turretRotationSpeed = 0.1f + hydraulicVoltage / 100.0f; 
    }
    else {
        turretRotationSpeed = 0.01f;  
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        turretAngle -= 0.01f;
        updateTurretView(true);
        updateBuffers();
        updateTargetVBO(target1VBO, target1Vertices);
        updateTargetVBO(target2VBO, target2Vertices);
        updateTargetVBO(target3VBO, target3Vertices);
    }

    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        turretAngle += 0.01f;
        updateTurretView(false);
        updateBuffers();
        updateTargetVBO(target1VBO, target1Vertices);
        updateTargetVBO(target2VBO, target2Vertices);
        updateTargetVBO(target3VBO, target3Vertices);
    }

    auto now = std::chrono::high_resolution_clock::now();

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && isGunReady && ammoCount > 0) {
        ammoCount--;
        isGunReady = false;
        lastShotTime = now;
        std::cout << "Gun fired! Ammo left: " << ammoCount << "\n";

        /*std::cout << "mouse" << crosshairCenterX << ", " << crosshairCenterY << std::endl;
        std::cout << "target 1 " << target1Vertices[0] << ", " << target1Vertices[1] << std::endl;
        std::cout << "target 2 " << target2Vertices[0] << ", " << target2Vertices[1] << std::endl;
        std::cout << "target 3 " << target3Vertices[0] << ", " << target3Vertices[1] << std::endl;*/
        if (isPointInCircle({target1Vertices[0], target1Vertices[1]}, 0.02f, {crosshairCenterX, crosshairCenterY})) {
            std::cout << "Hit target 1!" << std::endl;
            isTarget1Hit = true;
        }

        if (isPointInCircle({ target2Vertices[0], target2Vertices[1] }, 0.02f, { crosshairCenterX, crosshairCenterY })) {
            std::cout << "Hit target 2!" << std::endl;
            isTarget2Hit = true;
        }

        if (isPointInCircle({ target3Vertices[0], target3Vertices[1] }, 0.02f, { crosshairCenterX, crosshairCenterY })) {
            std::cout << "Hit target 3!" << std::endl;
            isTarget3Hit = true;
        }
    }
}

float calculateAngleToTarget(float cx, float cy, float targetX, float targetY) {
    return atan2(targetY - cy, targetX - cx) * 180.0f / M_PI;
}

void updateTankPipeVBO(unsigned int vbo) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, tankCannonPipeVertices.size() * sizeof(float), tankCannonPipeVertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void rotateVerticesAroundPoint(std::vector<float>& vertices, float angle) {
    // Calculate the middle point of the cannon pipe (the average of the two points)
    float middleX = (vertices[0] + vertices[2]) / 2.0f;
    float middleY = (vertices[1] + vertices[3]) / 2.0f;

    // Rotation matrix elements
    float cosAngle = cos(angle);
    float sinAngle = sin(angle);

    for (size_t i = 0; i < vertices.size(); i += 2) {
        // Translate point to origin (relative to the middle point)
        float x = vertices[i] - middleX;
        float y = vertices[i + 1] - middleY;

        // Rotate the point
        float newX = x * cosAngle - y * sinAngle;
        float newY = x * sinAngle + y * cosAngle;

        // Translate the point back to its original position
        vertices[i] = newX + middleX;
        vertices[i + 1] = newY + middleY;
    }
}

void gameLoop() 
{
    createLamp();
    createTargets();
    createAmmoIndicator();
    createOutside();
    createTankCannon();

    unsigned int basicShader = createShader("basic.vs", "basic.fs");
    unsigned int crosshairShader = createShader("crosshair.vs", "crosshair.fs");
    unsigned int ammoShader = createShader("ammo.vs", "ammo.fs");
    unsigned int voltmeterShader = createShader("voltmeter.vs", "voltmeter.fs");
    unsigned int outsideShader = createShader("outside.vs", "outside.fs");
    unsigned int textShader = createShader("text.vs", "text.fs");

    loadFont("LiberationSans-Regular.ttf");
    initTextRendering();
    loadAndBindTexture(outsideShader);

    unsigned int lampVAO = createVAO(lampVertices).first;
    unsigned int ammoIndicatorVAO = createVAO(ammoIndicatorVertices).first;
    unsigned int voltmeterVAO = createVAO(voltmeterVertices).first;
    unsigned int outsideVAO = setupOutsideVAO();
    unsigned int tankCannonVAO = createVAO(tankCannonVertices).first;
    std::pair<unsigned int, unsigned int> target1VAO = createVAO(target1Vertices);
    target1VBO = target1VAO.second;
    std::pair<unsigned int, unsigned int> target2VAO = createVAO(target2Vertices);
    target2VBO = target2VAO.second;
    std::pair<unsigned int, unsigned int> target3VAO = createVAO(target3Vertices);
    target3VBO = target3VAO.second;
    std::pair<unsigned int, unsigned int> tankCannonPipeVAOAndVBO = createVAO(tankCannonPipeVertices);

    auto previous = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(window)) {
        auto now = std::chrono::high_resolution_clock::now();
        auto frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - previous);

        if (frameDuration < targetFrameDuration) {
            auto sleepDuration = targetFrameDuration - frameDuration;
            std::this_thread::sleep_for(sleepDuration);
        }
        previous = now;

        std::chrono::duration<float> elapsed = now - lastShotTime;

        if (!isGunReady && elapsed.count() >= 7.5f) {
            isGunReady = true;
            std::cout << "Gun is ready again!\n";
        }

        unsigned int crosshairVAO;
        unsigned int mousePointerVAO;
        processInput(); 
        glClear(GL_COLOR_BUFFER_BIT);        
        renderText(textShader, "Hello, World!", 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f);


        if (opticalView) {
            createMousePointer(window);
            createCrosshair();
            crosshairVAO = createVAO(crosshairVertices).first;
            mousePointerVAO = createVAO(mouseVertices).first;
            drawOutisde(outsideVAO, outsideShader);
            drawTankCannon(tankCannonVAO, basicShader);

            /*float middleX = (tankCannonPipeVertices[0] + tankCannonPipeVertices[2]) / 2.0f;
            float middleY = (tankCannonPipeVertices[1] + tankCannonPipeVertices[3]) / 2.0f;
            float dx = 1.0f - middleX;
            float dy = 1.0f - middleY;
            float angle = 1;
            rotateVerticesAroundPoint(tankCannonPipeVertices, angle);*/

            updateTankPipeVBO(tankCannonPipeVAOAndVBO.second);
            drawTankCannonPipe(tankCannonPipeVAOAndVBO.first, basicShader);
            drawMousePointer(mousePointerVAO, basicShader);
            if (!isTarget1Hit)
                drawTargets(target1VAO.first, basicShader, target1Vertices.size() / 2);
            if (!isTarget2Hit)
                drawTargets(target2VAO.first, basicShader, target2Vertices.size() / 2);
            if (!isTarget3Hit)
                drawTargets(target3VAO.first, basicShader, target3Vertices.size() / 2);
        }
        else {
            crosshairVAO = createVAO(crosshairVertices).first;
        }
        drawLamp(lampVAO, basicShader);
        drawCrosshair(crosshairVAO, crosshairShader);
        drawAmmoIndicator(ammoIndicatorVAO, ammoShader);
        drawVoltmeterNeedle(voltmeterVAO, voltmeterShader);

        glFlush();
        glfwSwapBuffers(window);
        glfwPollEvents();
        glDeleteVertexArrays(1, &mousePointerVAO);
        glDeleteVertexArrays(1, &crosshairVAO);
    }

    glDeleteVertexArrays(1, &lampVAO);
    glDeleteVertexArrays(1, &target1VAO.first);
    glDeleteVertexArrays(1, &target2VAO.first);
    glDeleteVertexArrays(1, &target3VAO.first);
    glDeleteVertexArrays(1, &ammoIndicatorVAO);
    glDeleteVertexArrays(1, &voltmeterVAO);
    glDeleteVertexArrays(1, &outsideVAO);
}

void getMonitorResolution(int& width, int& height)
{
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);
    width = videoMode->width;
    height = videoMode->height;
}

bool initOpenGL() 
{
    if (!glfwInit()) 
    {
        std::cout << "GLFW Biblioteka se nije ucitala! :(\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    glfwSwapInterval(1); 

    getMonitorResolution(screenWidth, screenHeight);
    const char wTitle[] = "Tenk - Trener";
    window = glfwCreateWindow(screenWidth, screenHeight, wTitle, glfwGetPrimaryMonitor(), NULL);

    if (window == NULL) 
    {
        std::cout << "Prozor nije napravljen! :(\n";
        glfwTerminate(); 
        return false;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) 
    {
        std::cout << "GLEW nije mogao da se ucita! :'(\n";
        return false;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, screenWidth, screenHeight);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); 

    return true;
}

int main(void)
{
    if (!initOpenGL()) {
        return -1;
    }

    targets = { {-0.3f, 0.4f}, {0.5f, -0.2f}, {0.1f, 0.1f} };

    gameLoop();
    return 0;
}

unsigned int compileShader(GLenum type, const char* source)
{
    //Uzima kod u fajlu na putanji "source", kompajlira ga i vraca sejder tipa "type"
    //Citanje izvornog koda iz fajla
    std::string content = "";
    std::ifstream file(source);
    std::stringstream ss;
    if (file.is_open())
    {
        ss << file.rdbuf();
        file.close();
        std::cout << "Uspjesno procitao fajl sa putanje \"" << source << "\"!" << std::endl;
    }
    else {
        ss << "";
        std::cout << "Greska pri citanju fajla sa putanje \"" << source << "\"!" << std::endl;
    }
     std::string temp = ss.str();
     const char* sourceCode = temp.c_str(); //Izvorni kod sejdera koji citamo iz fajla na putanji "source"

    int shader = glCreateShader(type); //Napravimo prazan sejder odredjenog tipa (vertex ili fragment)
    
    int success; //Da li je kompajliranje bilo uspjesno (1 - da)
    char infoLog[512]; //Poruka o gresci (Objasnjava sta je puklo unutar sejdera)
    glShaderSource(shader, 1, &sourceCode, NULL); //Postavi izvorni kod sejdera
    glCompileShader(shader); //Kompajliraj sejder

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success); //Provjeri da li je sejder uspjesno kompajliran
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog); //Pribavi poruku o gresci
        if (type == GL_VERTEX_SHADER)
            printf("VERTEX");
        else if (type == GL_FRAGMENT_SHADER)
            printf("FRAGMENT");
        printf(" sejder ima gresku! Greska: \n");
        printf(infoLog);
    }
    return shader;
}
unsigned int createShader(const char* vsSource, const char* fsSource)
{
    //Pravi objedinjeni sejder program koji se sastoji od Vertex sejdera ciji je kod na putanji vsSource

    unsigned int program; //Objedinjeni sejder
    unsigned int vertexShader; //Verteks sejder (za prostorne podatke)
    unsigned int fragmentShader; //Fragment sejder (za boje, teksture itd)

    program = glCreateProgram(); //Napravi prazan objedinjeni sejder program

    vertexShader = compileShader(GL_VERTEX_SHADER, vsSource); //Napravi i kompajliraj vertex sejder
    fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSource); //Napravi i kompajliraj fragment sejder

    //Zakaci verteks i fragment sejdere za objedinjeni program
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program); //Povezi ih u jedan objedinjeni sejder program
    glValidateProgram(program); //Izvrsi provjeru novopecenog programa

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success); //Slicno kao za sejdere
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(program, 512, NULL, infoLog);
        std::cout << "Objedinjeni sejder ima gresku! Greska: \n";
        std::cout << infoLog << std::endl;
    }

    //Posto su kodovi sejdera u objedinjenom sejderu, oni pojedinacni programi nam ne trebaju, pa ih brisemo zarad ustede na memoriji
    glDetachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    return program;
}
