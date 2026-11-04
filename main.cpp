#include<glad/glad.h>
#include<iostream>
#include<vector>
#include<cmath>

#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include "Enemy.h"
#include "Player.h"

extern const int SCREEN_W = 1280;
const int SCREEN_H = 720;

const int NUM_LANES = 4;
const float HUD_H = 120.0f;
const float HOME_W = 130.0f;
const float FIELD_X = 130.0f;
const float FIELD_Y = 130.0f;
const float FIELD_W = 1150.0f;
const float LANE_H = 120.0f;

float laneTop(int lane)
{
    return FIELD_Y + lane * LANE_H;
}

float laneCenterY(int lane)
{
    return laneTop(lane) + LANE_H / 2;
}

int laneAt(float x, float y)
{
    if (x < FIELD_X || x >= FIELD_X + FIELD_W) {
        return -1;
    }

    int result = -1;
    for (int lane = 0;lane < NUM_LANES;lane++) {
        if (y >= laneTop(lane) && y < laneTop(lane) + LANE_H) {
            result = lane;
        }
    }
    return result;
}

GLuint VAO, VBO, EBO;
GLuint shaderProgram;
GLint locTransform;
GLint locRed;
GLint locGreen;
GLint locBlue;

const int CIRCLE_START = 4;
int circleVertexCount = 0;
int triangleStart = 0;

const char* vertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec3 aPos;

uniform mat4 u_Transform;

void main()
{
    gl_Position = u_Transform * vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core

uniform float u_Red;
uniform float u_Green;
uniform float u_Blue;
out vec4 FragColor;

void main()
{
    FragColor = vec4(u_Red, u_Green, u_Blue, 1.0);
}
)";

GLuint compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cout << "Vertex/fragment shader failed to compile:\n" << infoLog << std::endl;
    }
    return shader;
}

void createShaderProgram()
{
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cout << "Shader program failed to link:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    locTransform = glGetUniformLocation(shaderProgram, "u_Transform");
    locRed = glGetUniformLocation(shaderProgram, "u_Red");
    locGreen = glGetUniformLocation(shaderProgram, "u_Green");
    locBlue = glGetUniformLocation(shaderProgram, "u_Blue");
}

void createShapes()
{
    std::vector<float>vertices = {
        -0.5f,-0.5f,0.0f,
        0.5f,-0.5f,0.0f,
        0.5f,0.5f,0.0f,
        -0.5f,0.5f,0.0f };

    GLuint indices[] = {
        0,1,2,
        2,3,0 };

    const int segments = 30;
    vertices.insert(vertices.end(), { 0.0f,0.0f,0.0f });
    for (int i = 0;i <= segments;++i) {
        float angle = 2.0f * 3.14f * (float)i / (float)segments;
        vertices.insert(vertices.end(), { 0.5f * cosf(angle),0.5f * sinf(angle),0.0f });
    }
    circleVertexCount = segments + 2;

    triangleStart = CIRCLE_START + circleVertexCount;
    vertices.insert(vertices.end(), { -0.5f,-0.5f,0.0f,
                                     0.5f,-0.5f,0.0f,
                                     0.0f,0.5f,0.0f });

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(
        GL_ARRAY_BUFFER,
        VBO
    );
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(
        GL_ELEMENT_ARRAY_BUFFER,
        EBO
    );
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(indices),
        indices,
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void setShape(float cx, float cy, float w, float h, float angle, int r, int g, int b)
{
    float ndcX = (2.0f * cx) / SCREEN_W - 1.0f;
    float ndcY = 1.0f - (2.0f * cy) / SCREEN_H;

    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, glm::vec3(ndcX, ndcY, 0.0f));
    transform = glm::scale(transform, glm::vec3(2.0f / SCREEN_W, 2.0f / SCREEN_H, 1.0f));
    transform = glm::rotate(transform, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));
    transform = glm::scale(transform, glm::vec3(w, h, 1.0f));

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(locTransform, 1, GL_FALSE, glm::value_ptr(transform));
    glUniform1f(locRed, r / 255.0f);
    glUniform1f(locGreen, g / 255.0f);
    glUniform1f(locBlue, b / 255.0f);
    glBindVertexArray(VAO);
}

void drawRect(float x, float y, float w,
    float h, int r, int g, int b)
{
    setShape(x + w / 2, y + h / 2,
        w, h, 0.0f, r, g, b);
    glDrawElements(GL_TRIANGLES,
        6, GL_UNSIGNED_INT, 0);
}

void drawRotatedRect(float cx, float cy, float w, float h, float angle, int r, int g, int b)
{
    setShape(cx, cy, w, h, angle, r, g, b);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void drawRectOutline(float x, float y, float w, float h, float t, int r, int g, int b)
{
    drawRect(x, y, w, t, r, g, b);
    drawRect(x, y + h - t, w, t, r, g, b);
    drawRect(x, y, t, h, r, g, b);
    drawRect(x + w - t, y, t, h, r, g, b);
}

void drawCircle(float cx, float cy, float radius, int r, int g, int b)
{
    setShape(cx, cy, radius * 2, radius * 2, 0.0f, r, g, b);
    glDrawArrays(GL_TRIANGLE_FAN, CIRCLE_START, circleVertexCount);
}

void drawTriangle(float cx, float cy, float w, float h, float angle,
    int r, int g, int b)
{
    setShape(cx, cy, w, h, angle, r, g, b);
    glDrawArrays(GL_TRIANGLES,
        triangleStart, 3);
}

void drawDigit(int d, float x, float y, float s, int r, int g, int b)
{
    if (d != 1 && d != 4) {
        drawRect(x, y, 3 * s, s, r, g, b);
    }
    if (d != 5 && d != 6) {
        drawRect(x + 2 * s, y, s, 3 * s, r, g, b);
    }
    if (d != 2) {
        drawRect(x + 2 * s, y + 2 * s, s, 3 * s, r, g, b);
    }
    if (d != 1 && d != 4 && d != 7) {
        drawRect(x, y + 4 * s, 3 * s, s, r, g, b);
    }
    if (d == 0 || d == 2 || d == 6 || d == 8) {
        drawRect(x, y + 2 * s, s, 3 * s, r, g, b);
    }
    if (d != 1 && d != 2 && d != 3 && d != 7) {
        drawRect(x, y, s, 3 * s, r, g, b);
    }
    if (d != 0 && d != 1 && d != 7) {
        drawRect(x, y + 2 * s, 3 * s, s, r, g, b);
    }
}

void drawNumber(int n, float x, float y, float s, int r, int g, int b)
{
    int count;
    count = 1;
    if (n >= 10) {
        count = 2;
    }
    if (n >= 100) {
        count = 3;
    }
    if (n >= 1000) {
        count = 4;
    }
    if (n >= 10000) {
        count = 5;
    }

    int place = 1;
    for (int i = 1;i < count;i++) {
        place = place * 10;
    }

    for (int i = 0;i < count;i++) {
        int d = (n / place) % 10;
        drawDigit(d, x + s, y + s, s, 0, 0, 0);
        drawDigit(d, x, y, s, r, g, b);
        x = x + 4 * s;
        place = place / 10;
    }
}

const int STATE_MENU = 0;
const int STATE_CUTSCENE = 1;
const int STATE_PLAYING = 2;
const int STATE_PAUSED = 3;
const int STATE_WON = 4;
const int STATE_LOST = 5;
int gameState = STATE_MENU;

const int MELEE = 0;
const int RANGED = 1;
const int DEFENSE = 2;

const int START_COINS = 30;
int coins = START_COINS;

float mouseX = 0;
float mouseY = 0;
bool laneSelected[4];
int hoveredLane = -1;

int cardPrice[3] = { 10,20,30 };
int cardUnlockAt[3] = { 10,20,30 };

float laneFlashTime = 0;
float coinFlashTime = 0;
void drawBed(float cx, float cy);

// --- Pixel font rendering ---
unsigned short getGlyphMask(char c)
{
    if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
    switch (c) {
        case 'A': return 0x7bed;
        case 'B': return 0x6bae;
        case 'C': return 0x7927;
        case 'D': return 0x6b6e;
        case 'E': return 0x79a7;
        case 'F': return 0x79a4;
        case 'G': return 0x796f;
        case 'H': return 0x5bed;
        case 'I': return 0x7497;
        case 'J': return 0x126a;
        case 'K': return 0x5bad;
        case 'L': return 0x4927;
        case 'M': return 0x5f6d;
        case 'N': return 0x6b6d;
        case 'O': return 0x7b6f;
        case 'P': return 0x7be4;
        case 'Q': return 0x7b79;
        case 'R': return 0x7bf5;
        case 'S': return 0x79cf;
        case 'T': return 0x7492;
        case 'U': return 0x5b6f;
        case 'V': return 0x5b6a;
        case 'W': return 0x5b7d;
        case 'X': return 0x5aad;
        case 'Y': return 0x5a92;
        case 'Z': return 0x72a7;
        case '!': return 0x2482;
        case '.': return 0x0002;
        case ':': return 0x0410;
        case '-': return 0x01c0;
        case '?': return 0x6282;
        case '\'': return 0x2400;
        case ',': return 0x0014;
        case '0': return 0x7b6f;
        case '1': return 0x2c97;
        case '2': return 0x73e7;
        case '3': return 0x73cf;
        case '4': return 0x5bc9;
        case '5': return 0x79cf;
        case '6': return 0x79ef;
        case '7': return 0x7252;
        case '8': return 0x7bef;
        case '9': return 0x7bcf;
        default: return 0x0000;
    }
}

void drawLetter(char c, float x, float y, float s, int r, int g, int b)
{
    unsigned short mask = getGlyphMask(c);
    if (!mask) return;
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 3; col++) {
            int bit = (4 - row) * 3 + (2 - col);
            if (mask & (1 << bit)) {
                drawRect(x + col * s, y + row * s, s, s, r, g, b);
            }
        }
    }
}

void drawString(const char* text, float x, float y, float s, int r, int g, int b)
{
    if (!text) return;
    float curX = x;
    for (int i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        if (c == ' ') {
            curX += 4.0f * s;
            continue;
        }
        drawLetter(c, curX + 1.0f, y + 1.0f, s, 0, 0, 0);
        drawLetter(c, curX, y, s, r, g, b);
        curX += 4.0f * s;
    }
}

// --- Environment state & models ---
struct Cloud {
    float x;
    float y;
    float speed;
    float scale;
};

Cloud envClouds[5] = {
    { 80.0f, 20.0f, 12.0f, 1.0f },
    { 340.0f, 45.0f, 16.0f, 0.85f },
    { 620.0f, 15.0f, 10.0f, 1.25f },
    { 910.0f, 50.0f, 14.0f, 0.9f },
    { 1180.0f, 30.0f, 11.0f, 1.1f }
};

float envTorchTime = 0.0f;

void updateEnvironment(float dt)
{
    envTorchTime += dt;
    for (int i = 0; i < 5; i++) {
        envClouds[i].x += envClouds[i].speed * dt;
        if (envClouds[i].x > SCREEN_W + 120.0f) {
            envClouds[i].x = -160.0f;
        }
    }
}

void drawCloud(float x, float y, float scale)
{
    float w = 90.0f * scale;
    float h = 26.0f * scale;
    drawRect(x + 12 * scale, y, w - 24 * scale, h, 255, 255, 255);
    drawRect(x, y + 8 * scale, w, h - 8 * scale, 255, 255, 255);
    drawRect(x + 20 * scale, y - 8 * scale, w - 44 * scale, 8 * scale, 255, 255, 255);
    drawRect(x + 4 * scale, y + h - 4 * scale, w - 8 * scale, 4 * scale, 215, 225, 235);
}

void drawOakTree(float x, float y)
{
    drawRect(x - 10, y, 20, 52, 105, 75, 40);
    drawRect(x - 10, y + 10, 20, 4, 85, 60, 30);
    drawRect(x - 10, y + 28, 20, 4, 85, 60, 30);

    drawRect(x - 38, y - 36, 76, 36, 45, 115, 35);
    drawRect(x - 30, y - 56, 60, 24, 55, 135, 45);
    drawRect(x - 18, y - 70, 36, 16, 70, 160, 55);

    drawRect(x - 24, y - 32, 14, 10, 75, 165, 55);
    drawRect(x + 10, y - 48, 12, 10, 75, 165, 55);
}

void drawTorch(float x, float y, float time)
{
    drawRect(x - 3, y - 10, 6, 26, 120, 85, 45);
    drawRect(x - 4, y - 14, 8, 4, 50, 45, 45);

    float flicker = sinf(time * 18.0f) * 2.0f;
    float flameH = 14.0f + flicker;
    drawRect(x - 5, y - 14 - flameH, 10, flameH, 255, 150, 20);
    drawRect(x - 3, y - 12 - flameH * 0.7f, 6, flameH * 0.7f, 255, 230, 80);
    drawRect(x - 1, y - 10 - flameH * 0.4f, 2, flameH * 0.4f, 255, 255, 200);
}

void drawFlower(float x, float y, bool isYellow)
{
    drawRect(x - 1, y - 12, 2, 14, 60, 130, 40);
    drawRect(x - 3, y - 6, 2, 3, 60, 130, 40);
    if (isYellow) {
        drawRect(x - 4, y - 16, 8, 5, 245, 215, 35);
        drawRect(x - 2, y - 18, 4, 2, 255, 235, 80);
        drawRect(x - 1, y - 15, 2, 2, 200, 150, 20);
    } else {
        drawRect(x - 4, y - 16, 8, 5, 220, 40, 40);
        drawRect(x - 2, y - 18, 4, 2, 240, 70, 70);
        drawRect(x - 1, y - 15, 2, 2, 35, 20, 20);
    }
}

void drawChest(float x, float y)
{
    drawRect(x - 14, y - 12, 28, 24, 160, 110, 50);
    drawRectOutline(x - 14, y - 12, 28, 24, 2, 55, 40, 20);
    drawRect(x - 14, y - 2, 28, 2, 55, 40, 20);
    drawRect(x - 3, y - 4, 6, 7, 220, 220, 225);
    drawRect(x - 1, y - 2, 2, 3, 40, 40, 40);
}

void drawCraftingTable(float x, float y)
{
    drawRect(x - 14, y - 14, 28, 28, 150, 100, 50);
    drawRectOutline(x - 14, y - 14, 28, 28, 2, 80, 50, 25);
    drawRect(x - 12, y - 12, 24, 6, 180, 130, 70);
    drawRect(x - 8, y + 2, 6, 8, 80, 50, 25);
    drawRect(x + 2, y + 2, 6, 8, 80, 50, 25);
}

void drawHomeBaseBed(float cx, float cy)
{
    drawRect(cx - 36, cy + 10, 8, 14, 90, 60, 30);
    drawRect(cx + 28, cy + 10, 8, 14, 90, 60, 30);
    drawRect(cx - 38, cy + 2, 76, 10, 150, 110, 60);
    drawRect(cx - 38, cy - 14, 76, 17, 205, 40, 40);
    drawRect(cx - 38, cy - 2, 76, 3, 155, 25, 25);
    drawRect(cx - 38, cy - 18, 22, 18, 245, 245, 245);
    drawRectOutline(cx - 38, cy - 18, 22, 18, 1, 200, 200, 200);
}

void drawEnvironment()
{
    // Sky gradient layers
    drawRect(0, 0, SCREEN_W, 130, 115, 180, 240);
    drawRect(0, 70, SCREEN_W, 60, 140, 200, 250);

    // Clouds
    for (int i = 0; i < 5; i++) {
        drawCloud(envClouds[i].x, envClouds[i].y + 60.0f, envClouds[i].scale);
    }

    // Horizon trees
    drawOakTree(220, 130);
    drawOakTree(520, 130);
    drawOakTree(820, 130);
    drawOakTree(1120, 130);

    // 4 Battle lanes
    for (int lane = 0; lane < NUM_LANES; lane++) {
        float y = laneTop(lane);
        int r = (lane % 2 == 0) ? 100 : 90;
        int g = (lane % 2 == 0) ? 175 : 160;
        int b = (lane % 2 == 0) ? 65 : 55;
        drawRect(FIELD_X, y, FIELD_W, LANE_H, r, g, b);

        drawRect(FIELD_X, y, FIELD_W, 3, 80, 145, 45);
        for (int col = 0; col < 20; col++) {
            float px = FIELD_X + col * 58.0f + ((lane * 17) % 30);
            drawRect(px, y + 4, 4, 3, 75, 135, 40);
        }

        if (lane < NUM_LANES - 1) {
            float sepY = y + LANE_H - 4;
            drawRect(FIELD_X, sepY, FIELD_W, 4, 120, 80, 45);
            for (int k = 0; k < 12; k++) {
                drawRect(FIELD_X + k * 95.0f + 20, sepY, 12, 4, 100, 100, 105);
            }
        }
    }

    // Flowers on lanes
    drawFlower(260, laneCenterY(0) + 25, false);
    drawFlower(640, laneCenterY(0) - 20, true);
    drawFlower(980, laneCenterY(0) + 15, false);

    drawFlower(410, laneCenterY(1) + 20, true);
    drawFlower(780, laneCenterY(1) - 22, false);
    drawFlower(1100, laneCenterY(1) + 18, true);

    drawFlower(320, laneCenterY(2) - 18, false);
    drawFlower(580, laneCenterY(2) + 22, true);
    drawFlower(890, laneCenterY(2) - 15, false);

    drawFlower(490, laneCenterY(3) + 20, true);
    drawFlower(750, laneCenterY(3) - 20, false);
    drawFlower(1040, laneCenterY(3) + 15, true);

    // Torches along lanes
    for (int lane = 0; lane < NUM_LANES; lane++) {
        float ly = laneCenterY(lane);
        drawTorch(450, ly - 32, envTorchTime + lane);
        drawTorch(850, ly - 32, envTorchTime + lane * 1.5f);
        drawTorch(1180, ly - 32, envTorchTime + lane * 2.2f);
    }

    // Fortress wall on safe zone
    drawRect(0, FIELD_Y, HOME_W, SCREEN_H - FIELD_Y, 100, 100, 105);
    for (int r = 0; r < 14; r++) {
        float by = FIELD_Y + r * 42.0f;
        drawRect(0, by, HOME_W, 2, 60, 60, 65);
        int offset = (r % 2) * 32;
        for (int c = 0; c < 3; c++) {
            float bx = c * 64 - offset;
            if (bx >= 0 && bx < HOME_W) {
                drawRect(bx, by, 2, 42, 60, 60, 65);
            }
        }
    }
    drawRect(HOME_W - 4, FIELD_Y, 4, SCREEN_H - FIELD_Y, 65, 65, 70);

    // Castle props
    drawTorch(25, FIELD_Y + 45, envTorchTime);
    drawTorch(105, FIELD_Y + 45, envTorchTime + 0.8f);
    drawChest(32, laneCenterY(0) + 10);
    drawCraftingTable(95, laneCenterY(1) - 10);
    drawChest(32, laneCenterY(2) + 20);

    // Bed objective at Home Base
    drawHomeBaseBed(65, laneCenterY(1) + 40);

    // Bottom dirt cliff
    float botY = laneTop(NUM_LANES - 1) + LANE_H;
    drawRect(0, botY, SCREEN_W, SCREEN_H - botY, 125, 80, 45);
    drawRect(0, botY, SCREEN_W, 5, 85, 55, 30);
    for (int col = 0; col < 22; col++) {
        float px = col * 60.0f;
        drawRect(px + 10, botY + 12, 14, 10, 100, 65, 35);
        drawRect(px + 35, botY + 28, 12, 8, 90, 60, 30);
    }
}

// --- Cutscene implementation ---
float cutsceneTimer = 0.0f;
const float CUTSCENE_DURATION = 15.0f;
bool cutsceneDone = false;

void startCutscene()
{
    cutsceneTimer = 0.0f;
    cutsceneDone = false;
}

void updateCutscene(float dt)
{
    cutsceneTimer += dt;
    if (cutsceneTimer >= CUTSCENE_DURATION) {
        cutsceneDone = true;
    }
}

bool cutsceneFinished()
{
    return cutsceneDone;
}

void skipCutscene()
{
    cutsceneDone = true;
}

void drawDialogueBox(const char* speaker, const char* line1, const char* line2, int badgeR, int badgeG, int badgeB)
{
    float bx = 80.0f;
    float by = 555.0f;
    float bw = 1120.0f;
    float bh = 135.0f;

    drawRect(bx - 3, by - 3, bw + 6, bh + 6, 20, 20, 25);
    drawRect(bx, by, bw, bh, 35, 35, 42);
    drawRectOutline(bx, by, bw, bh, 3, badgeR, badgeG, badgeB);

    drawRect(bx + 16, by - 16, 130, 24, badgeR, badgeG, badgeB);
    drawRectOutline(bx + 16, by - 16, 130, 24, 2, 20, 20, 25);
    drawString(speaker, bx + 28, by - 12, 2.5f, 255, 255, 255);

    drawString(line1, bx + 28, by + 28, 3.0f, 245, 245, 250);
    drawString(line2, bx + 28, by + 74, 3.0f, 245, 245, 250);

    drawString("PRESS SPACE TO SKIP", SCREEN_W - 270, 30, 2.0f, 255, 255, 255);
}

void drawCutsceneSteve(float x, float y, float legSwing, float armAngle, bool swordRaised)
{
    drawRect(x - 14, y - 38, 28, 28, 210, 165, 125);
    drawRect(x - 14, y - 38, 28, 8, 75, 45, 25);
    drawRect(x - 14, y - 30, 7, 10, 75, 45, 25);
    drawRect(x + 7, y - 30, 7, 10, 75, 45, 25);

    drawRect(x - 6, y - 24, 4, 3, 50, 70, 190);
    drawRect(x + 2, y - 24, 4, 3, 50, 70, 190);
    drawRect(x - 2, y - 19, 4, 3, 175, 125, 95);

    drawRect(x - 13, y - 10, 26, 30, 40, 185, 205);

    drawRect(x - 12, y + 20 + legSwing, 10, 20, 45, 55, 130);
    drawRect(x + 2, y + 20 - legSwing, 10, 20, 45, 55, 130);

    drawRect(x - 19, y - 10, 6, 26, 40, 185, 205);
    drawRect(x - 19, y + 16, 6, 8, 210, 165, 125);

    if (swordRaised) {
        drawRect(x + 13, y - 26, 6, 22, 40, 185, 205);
        drawRect(x + 13, y - 32, 6, 6, 210, 165, 125);
        drawRotatedRect(x + 16, y - 56, 8, 38, -15.0f, 90, 225, 235);
        drawRotatedRect(x + 16, y - 42, 18, 6, -15.0f, 110, 70, 35);
    } else {
        drawRect(x + 13, y - 10, 6, 26, 40, 185, 205);
        drawRect(x + 13, y + 16, 6, 8, 210, 165, 125);
        drawRotatedRect(x + 22, y + 6, 6, 30, 30.0f, 90, 225, 235);
        drawRotatedRect(x + 22, y + 16, 16, 5, 30.0f, 110, 70, 35);
    }
}

void drawCutsceneCreeper(float x, float y, float stride, bool flashWhite)
{
    int r = flashWhite ? 245 : 45;
    int g = flashWhite ? 245 : 185;
    int b = flashWhite ? 245 : 45;

    drawRect(x - 18, y - 40, 36, 32, r, g, b);

    int eyeR = flashWhite ? 180 : 20;
    drawRect(x - 12, y - 32, 7, 7, eyeR, eyeR, eyeR);
    drawRect(x + 5, y - 32, 7, 7, eyeR, eyeR, eyeR);
    drawRect(x - 4, y - 23, 8, 12, eyeR, eyeR, eyeR);
    drawRect(x - 8, y - 17, 4, 9, eyeR, eyeR, eyeR);
    drawRect(x + 4, y - 17, 4, 9, eyeR, eyeR, eyeR);

    drawRect(x - 13, y - 8, 26, 32, r - 10, g - 25, b - 10);

    drawRect(x - 16 + stride, y + 24, 13, 16, r - 20, g - 50, b - 20);
    drawRect(x + 3 - stride, y + 24, 13, 16, r - 20, g - 50, b - 20);
}

void drawCutscenePhantom(float x, float y, float flapAngle)
{
    drawRotatedRect(x - 30, y - 8, 38, 12, -20.0f - flapAngle, 50, 60, 110);
    drawRotatedRect(x + 30, y - 8, 38, 12, 20.0f + flapAngle, 50, 60, 110);
    drawRect(x - 18, y - 12, 36, 24, 65, 80, 140);
    drawRect(x - 28, y - 8, 14, 16, 40, 45, 80);
    drawRect(x - 25, y - 5, 5, 5, 185, 255, 60);
    drawRect(x + 18, y - 4, 20, 8, 40, 45, 80);
}

void drawCutscene()
{
    float t = cutsceneTimer;

    int skyR = 110, skyG = 175, skyB = 235;
    if (t >= 5.0f && t < 10.0f) {
        float f = (t - 5.0f) / 5.0f;
        skyR = 110 + (int)(f * 15);
        skyG = 175 - (int)(f * 95);
        skyB = 235 - (int)(f * 95);
    } else if (t >= 10.0f) {
        float f = (t - 10.0f) / 5.0f;
        skyR = 125 + (int)(f * 40);
        skyG = 80 - (int)(f * 20);
        skyB = 140 - (int)(f * 70);
    }
    drawRect(0, 0, SCREEN_W, 460, skyR, skyG, skyB);

    drawCloud(180, 60, 1.2f);
    drawCloud(780, 80, 1.0f);

    drawRect(0, 460, SCREEN_W, 260, 95, 170, 60);
    drawRect(0, 460, SCREEN_W, 6, 75, 140, 45);
    drawRect(0, 490, SCREEN_W, 230, 125, 85, 55);
    drawRect(0, 490, SCREEN_W, 6, 85, 55, 35);

    drawOakTree(60, 460);
    drawOakTree(1220, 460);

    drawBed(140, 430);
    drawRect(95, 360, 90, 22, 20, 20, 25);
    drawRectOutline(95, 360, 90, 22, 2, 255, 215, 60);
    drawString("OUR BED", 108, 364, 2.0f, 255, 215, 60);

    if (t < 5.0f) {
        float walkProgress = sinf(t * 2.5f);
        float steveX = 350.0f + walkProgress * 70.0f;
        float legSwing = sinf(t * 8.0f) * 4.0f;
        drawCutsceneSteve(steveX, 420.0f, legSwing, 0.0f, false);

        drawDialogueBox("STEVE",
                        "ANOTHER PEACEFUL DAY GUARDING OUR BEDWARS ISLAND.",
                        "AS LONG AS THIS BED IS SAFE, OUR RESPAWNS ARE SECURE!",
                        40, 185, 205);
    }
    else if (t < 10.0f) {
        float steveX = 410.0f;
        drawCutsceneSteve(steveX, 420.0f, 0.0f, 0.0f, false);

        float progress = (t - 5.0f) / 5.0f;
        float creeperX = 1250.0f - progress * 400.0f;
        float stride = sinf(t * 10.0f) * 4.0f;
        drawCutsceneCreeper(creeperX, 420.0f, stride, false);

        float phantomX = 1250.0f - progress * 450.0f;
        float phantomY = 120.0f + sinf(t * 5.0f) * 20.0f;
        float flap = sinf(t * 14.0f) * 35.0f;
        drawCutscenePhantom(phantomX, phantomY, flap);

        drawDialogueBox("CREEPER",
                        "SSSSS... NOT FOR LONG, STEVE!",
                        "DESTROY THAT BED AND THEY WILL NEVER RESPAWN!",
                        220, 50, 45);
    }
    else {
        float steveX = 410.0f;
        drawCutsceneSteve(steveX, 420.0f, 0.0f, 0.0f, true);

        float creeperX = 850.0f + sinf(t * 20.0f) * 2.0f;
        bool flash = ((int)(t * 8.0f) % 2 == 0);
        drawCutsceneCreeper(creeperX, 420.0f, 0.0f, flash);

        float phantomX = 800.0f;
        float phantomY = 130.0f + sinf(t * 4.0f) * 15.0f;
        float flap = sinf(t * 12.0f) * 30.0f;
        drawCutscenePhantom(phantomX, phantomY, flap);

        drawDialogueBox("STEVE",
                        "NOT ON MY WATCH! DEFENDERS, TO YOUR LANES!",
                        "HOLD THE LINE AND PROTECT THE BED AT ALL COSTS!",
                        255, 205, 50);
    }
}

std::vector<Player> activePlayers;

struct PlayerArrow {
    glm::vec2 position;
    int lane;
    bool active;
};

std::vector<PlayerArrow> playerArrows;
enum class CollectibleType {
    COIN,
    HEAL
};
struct Collectible {
    glm::vec2 position;
    int lane;
    CollectibleType type;
    int value;
    bool active;
    float bobTimer;
};

std::vector<Collectible> activeCollectibles;


void drawCoinItem(float cx, float cy) {
    drawRect(cx - 7, cy - 5, 14, 10, 245, 205, 40);
    drawRect(cx - 5, cy - 3, 10, 6, 255, 235, 100);
    drawRectOutline(cx - 7, cy - 5, 14, 10, 1, 160, 110, 20);
}

void drawHealItem(float cx, float cy) {
    // Red Heart / Golden Apple
    drawRect(cx - 8, cy - 6, 7, 7, 230, 40, 40);
    drawRect(cx + 1, cy - 6, 7, 7, 230, 40, 40);
    drawTriangle(cx, cy + 3, 16, 12, 180.0f, 230, 40, 40);
    // Shine highlight
    drawRect(cx - 5, cy - 4, 3, 3, 255, 200, 200);
}

void drawCollectibles() {
    for (const auto& item : activeCollectibles) {
        if (!item.active) continue;

        float bobY = item.position.y + sinf(item.bobTimer * 5.0f) * 3.0f;

        if (item.type == CollectibleType::COIN) {
            drawCoinItem(item.position.x, bobY);
        }
        else {
            drawHealItem(item.position.x, bobY);
        }
    }
}
void resetPlayers()
{
    activePlayers.clear();
    playerArrows.clear();
}
void jumpPlayers()
{
    for (auto& player : activePlayers) {
        if (laneSelected[player.getLane()]) {
            player.jump();
        }
    }
}

void duckPlayers()
{
    for (auto& player : activePlayers) {
        if (laneSelected[player.getLane()]) {
            player.duck();
        }
    }
}


bool spawnPlayer(int type, int lane)
{
    if (lane < 0 || lane >= NUM_LANES) {
        return false;
    }

    if (type < MELEE || type > DEFENSE) {
        return false;
    }

    float x = FIELD_X + 35.0f;
    float y = laneCenterY(lane);

    if (type == DEFENSE) {

        x = FIELD_X + 325.0f;

        for (const auto& player : activePlayers) {

            if (!player.isActive() || !player.isAlive()) continue;
            if (player.getLane() != lane) continue;
            if (player.getType() != PlayerType::DEFENSE) continue;

            float nextX = player.getPosition().x - 70.0f;

            if (nextX < x) {
                x = nextX;
            }
        }

        if (x < FIELD_X + 95.0f) {
            return false;
        }
    }

    activePlayers.emplace_back(
        static_cast<PlayerType>(type),
        lane,
        glm::vec2(x, y)
    );

    return true;
}

void updatePlayers(float dt)
{
    for (auto& player : activePlayers) {
        player.update(dt);
    }
}

void removeDefeatedPlayers()
{
    for (auto it = activePlayers.begin(); it != activePlayers.end();) {

        if (!it->isActive() || !it->isAlive()) {
            it = activePlayers.erase(it);
        }
        else {
            ++it;
        }
    }
}

void drawPlayers()
{
    for (const auto& player : activePlayers) {
        player.render();
    }
}
std::vector<Enemy> activeEnemies;
std::vector<float> hardEnemyAttackTimers;
float enemySpawntimer = 0.0f;
float enemiesSpawnedInWave = 0;

int wave = 1;
bool enemyAtBase = false;
bool wavesDone = false;
float waveBreakTimer = 0.0f;
void resetEnemies() {
    wave = 1;
    enemyAtBase = false;
    wavesDone = false;
    activeEnemies.clear();
    hardEnemyAttackTimers.clear();
    enemySpawntimer = 0.0f;
    enemiesSpawnedInWave = 0;
    waveBreakTimer = 0.0f;
    activeCollectibles.clear();
}
int getBalancedSpawnLane()
{
    // Count how many enemies currently occupy each lane
    int laneCounts[NUM_LANES] = { 0, 0, 0, 0 };
    for (const auto& enemy : activeEnemies) {
        if (enemy.isAlive() && enemy.getLane() >= 0 && enemy.getLane() < NUM_LANES) {
            laneCounts[enemy.getLane()]++;
        }
    }

    // Find the minimum number of enemies in any lane
    int minCount = laneCounts[0];
    for (int i = 1; i < NUM_LANES; i++) {
        if (laneCounts[i] < minCount) {
            minCount = laneCounts[i];
        }
    }

    // Gather all candidate lanes that share this lowest count
    std::vector<int> candidateLanes;
    for (int i = 0; i < NUM_LANES; i++) {
        if (laneCounts[i] == minCount) {
            candidateLanes.push_back(i);
        }
    }

    // Randomly pick among the least crowded lanes
    int chosenIndex = std::rand() % static_cast<int>(candidateLanes.size());
    return candidateLanes[chosenIndex];
}
void updateEnemies(float dt)
{
    int maxWaveEnemies = (wave == 1) ? 3 : ((wave == 2) ? 5 : 10);

    if (enemiesSpawnedInWave < maxWaveEnemies) {
        enemySpawntimer += dt;
        if (enemySpawntimer >= 5.0f) {
            enemySpawntimer = 0.0f;
            int spawnLane = getBalancedSpawnLane();
            float spawnY = laneCenterY(spawnLane);
            float spawnX = 1200.0f;

            EnemyType typeToSpawn = EnemyType::EASY;
            if (wave == 1) {
                typeToSpawn = EnemyType::EASY;
            }
            if (wave == 2) {
                if (enemiesSpawnedInWave == 0) typeToSpawn = EnemyType::EASY;
                else if (enemiesSpawnedInWave == 1) typeToSpawn = EnemyType::MEDIUM;
                else typeToSpawn = (std::rand() % 2 == 0 ? EnemyType::EASY : EnemyType::MEDIUM);
            }
            else if (wave == 3) {
                if (enemiesSpawnedInWave == 0) typeToSpawn = EnemyType::EASY;
                else if (enemiesSpawnedInWave == 1) typeToSpawn = EnemyType::MEDIUM;
                else if (enemiesSpawnedInWave == 2) typeToSpawn = EnemyType::HARD;
                else {
                    int r = std::rand() % 3;
                    typeToSpawn = (r == 0) ? EnemyType::EASY : ((r == 1) ? EnemyType::MEDIUM : EnemyType::HARD);
                }
            }

            activeEnemies.emplace_back(typeToSpawn, spawnLane, glm::vec2(spawnX, spawnY));
            hardEnemyAttackTimers.push_back(0.0f);
            enemiesSpawnedInWave++;
        }
    }

    for (auto it = activeEnemies.begin(); it != activeEnemies.end();) {
        it->update(dt);
        if (it->getPosition().x <= FIELD_X) {
            enemyAtBase = true;
            return;
        }


        if (!it->isAlive()) {
            int index = static_cast<int>(it - activeEnemies.begin());

            // 1. ALWAYS drop coins from defeated enemies so player never goes broke!
            activeCollectibles.push_back({
                it->getPosition(),
                it->getLane(),
                CollectibleType::COIN,
                it->getReward(),
                true,
                0.0f
                });

            // 2. Extra 30% chance to ALSO drop a separate Heart pickup
            if ((std::rand() % 10) < 3) {
                activeCollectibles.push_back({
                    glm::vec2(it->getPosition().x - 25.0f, it->getPosition().y),
                    it->getLane(),
                    CollectibleType::HEAL,
                    2,
                    true,
                    0.0f
                    });
            }

            hardEnemyAttackTimers.erase(hardEnemyAttackTimers.begin() + index);
            it = activeEnemies.erase(it);
        }
        else {
            ++it;
        }
    }

    if (enemiesSpawnedInWave >= maxWaveEnemies && activeEnemies.empty()) {
        if (wave < 3) {
            waveBreakTimer += dt;
            if (waveBreakTimer >= 3.0f) {
                wave++;
                enemiesSpawnedInWave = 0;
                enemySpawntimer = 0.0f;
            }
        }
        else {
            wavesDone = true;
        }
    }
}

void drawEnemies()
{
    for (const auto& enemy : activeEnemies) {
        enemy.render();
    }
}


void checkPlayerEnemyCollisions()
{
    for (auto& player : activePlayers) {
        player.setIsColliding(false);
    }

    for (auto& enemy : activeEnemies) {
        enemy.setIsColliding(false);
    }

    for (auto& player : activePlayers) {
        if (!player.isActive() || !player.isAlive()) continue;

        for (auto& enemy : activeEnemies) {
            if (!enemy.isAlive()) continue;

            if (player.getLane() != enemy.getLane()) continue;

            float distance = std::abs(player.getPosition().x - enemy.getPosition().x);

            if (distance < 28.0f) {
                player.setIsColliding(true);
                enemy.setIsColliding(true);

           if (player.getType() == PlayerType::MELEE && player.canAttack()) {
          enemy.takeDamage(1);
          player.startSwing();
         player.resetAttackTimer();
          }   
                 }
        }
    }
}

void updateCollectibles(float dt) {
    for (auto it = activeCollectibles.begin(); it != activeCollectibles.end();) {
        if (!it->active) {
            it = activeCollectibles.erase(it);
            continue;
        }

        it->bobTimer += dt;

        bool collected = false;

        // Check 1: Did an active player in this lane touch it?
        for (auto& player : activePlayers) {
            if (!player.isActive() || !player.isAlive()) continue;
            if (player.getLane() != it->lane) continue;

            // Generous collision box (45.0f) so it picks up smoothly as units pass
            float distX = std::abs(player.getPosition().x - it->position.x);
            float distY = std::abs(player.getPosition().y - it->position.y);

            if (distX < 45.0f && distY < 40.0f) {
                if (it->type == CollectibleType::COIN) {
                    coins += it->value;
                }
                else if (it->type == CollectibleType::HEAL) {
                    player.takeHeal(it->value);
                }
                collected = true;
                break;
            }
        }

        if (collected) {
            it = activeCollectibles.erase(it);
        }
        else {
            ++it;
        }
    }
}
void checkHardEnemyAttacks(float dt)
{
    for (int i = 0; i < static_cast<int>(activeEnemies.size()); i++) {

        Enemy& enemy = activeEnemies[i];

        if (!enemy.isAlive()) continue;
        if (enemy.getType() != EnemyType::HARD) continue;

        Player* target = nullptr;

        for (auto& player : activePlayers) {

            if (!player.isActive() || !player.isAlive()) continue;
            if (player.getLane() != enemy.getLane()) continue;

            float distance = std::abs(
                player.getPosition().x - enemy.getPosition().x
            );

            if (distance < 28.0f) {

                if (target == nullptr ||
                    player.getPosition().x > target->getPosition().x) {
                    target = &player;
                }
            }
        }

        if (target != nullptr) {

            hardEnemyAttackTimers[i] += dt;

            if (hardEnemyAttackTimers[i] >= enemy.getAttackCooldown()) {
                target->takeDamage(2);
                hardEnemyAttackTimers[i] = 0.0f;
            }
        }
        else {
            hardEnemyAttackTimers[i] = 0.0f;
        }
    }
}


void checkEnemyProjectileHits()
{
    for (auto& enemy : activeEnemies) {
        for (auto& projectile : enemy.getProjectiles()) {

            if (!projectile.Active) continue;

            Player* target = nullptr;

            for (auto& player : activePlayers) {

                if (!player.isActive() || !player.isAlive()) continue;
                if (player.getLane() != enemy.getLane()) continue;

                float distanceX = std::abs(
                    player.getPosition().x - projectile.position.x
                );

                float distanceY = std::abs(
                    player.getPosition().y - projectile.position.y
                );

                if (distanceX < 20.0f && distanceY < 38.0f) {

                    if (target == nullptr ||
                        player.getPosition().x > target->getPosition().x) {
                        target = &player;
                    }
                }
            }

            if (target == nullptr) continue;

            if (projectile.isHighAtatck && target->isDucking()) {
                continue;
            }

            if (!projectile.isHighAtatck && target->isJumping()) {
                continue;
            }

            target->takeDamage(1);
            projectile.Active = false;
        }
    }
}

void updateRangedAttacks(float dt)
{
    for (auto& player : activePlayers) {

        if (!player.isActive() || !player.isAlive()) continue;
        if (player.getType() != PlayerType::RANGED) continue;
        if (!player.canAttack()) continue;

        for (auto& enemy : activeEnemies) {

            if (!enemy.isAlive()) continue;
            if (player.getLane() != enemy.getLane()) continue;

            float distance = enemy.getPosition().x - player.getPosition().x;

            if (distance > 0.0f && distance < 350.0f) {

                PlayerArrow arrow;
                arrow.position = player.getPosition();
                arrow.position.x += 22.0f;
                arrow.lane = player.getLane();
                arrow.active = true;
                playerArrows.push_back(arrow);
                player.startBowShot();
                player.resetAttackTimer();
                break;            }
        }
    }

    for (auto& arrow : playerArrows) {

        if (!arrow.active) continue;

        arrow.position.x += 320.0f * dt;

        if (arrow.position.x > FIELD_X + FIELD_W) {
            arrow.active = false;
            continue;
        }

        for (auto& enemy : activeEnemies) {

            if (!enemy.isAlive()) continue;
            if (arrow.lane != enemy.getLane()) continue;

            float distance = std::abs(
                arrow.position.x - enemy.getPosition().x
            );

            if (distance < 18.0f) {
                enemy.takeDamage(1);
                arrow.active = false;
                break;
            }
        }
    }
}

void removeInactivePlayerArrows()
{
    for (auto it = playerArrows.begin(); it != playerArrows.end();) {

        if (!it->active) {
            it = playerArrows.erase(it);
        }
        else {
            ++it;
        }
    }
}

void drawPlayerArrows()
{
    for (const auto& arrow : playerArrows) {

        if (!arrow.active) continue;

        float x = arrow.position.x;
        float y = arrow.position.y;

        drawRect(x - 8, y - 2, 16, 4, 150, 95, 45);
        drawRect(x + 8, y - 4, 5, 8, 200, 200, 200);
    }
}

bool insideRect(float px, float py, float x, float y, float w, float h)
{
    bool insideX = (px >= x && px <= x + w);
    bool insideY = (py >= y && py <= y + h);

    return insideX && insideY;
}

bool isUnlocked(int type)
{
    return (coins >= cardUnlockAt[type]);
}

const float CARD_Y = 12.0f;
const float CARD_W = 130.0f;
const float CARD_H = 98.0f;

const float card_start_x = 20.0f;
const float card_spacing = 142.0f;

float cardX(int i)
{
    return card_start_x + i * card_spacing;
}

int cardUnderMouse()
{
    int result = -1;
    for (int i = 0;i < 3;i++) {
        if (insideRect(mouseX, mouseY, cardX(i), CARD_Y, CARD_W, CARD_H) && isUnlocked(i)) {
            result = i;
        }
    }
    return result;
}


void startNewGame()
{
    coins = START_COINS;

    for (int i = 0; i < NUM_LANES; i++) {
        laneSelected[i] = false;
    }

    laneFlashTime = 0;
    coinFlashTime = 0;

    resetPlayers();
    resetEnemies();
    activeCollectibles.clear();
}
void placePlayers(int type)
{
    bool anyLane = false;
    for (int i = 0;i < NUM_LANES;i++) {
        if (laneSelected[i] == true) {
            anyLane = true;
        }
    }
    if (!anyLane) {
        laneFlashTime = 0.5f;
        return;
    }
    if (!isUnlocked(type)) {
        return;
    }

    int placed = 0;
    for (int lane = 0;lane < NUM_LANES;lane++) {

if (laneSelected[lane] && coins >= cardPrice[type]) {

    if (spawnPlayer(type, lane)) {
        coins -= cardPrice[type];
        placed++;
    }
}
    }
    if (placed == 0) {
        coinFlashTime = 0.5f;
    }
}

const float BUTTON_X = 490.0f;
const float BUTTON_W = 300.0f;
const float BUTTON_H = 72.0f;

const int ICON_PLAY = 0;
const int ICON_EXIT = 1;
const int ICON_RESTART = 2;
const int ICON_HOME = 3;

float buttonY(int i)
{
    return 300.0f + i * 92.0f;
}

int buttonUnderMouse(int count)
{
    int result = -1;
    for (int i = 0;i < count;i++) {
        if (insideRect(mouseX, mouseY, BUTTON_X, buttonY(i), BUTTON_W, BUTTON_H)) {
            result = i;
        }
    }
    return result;
}

void drawButton(int i, int icon)
{
    float y = buttonY(i);
    float cx = BUTTON_X + BUTTON_W / 2;
    float cy = y + BUTTON_H / 2;

    int r = 115, g = 115, b = 115;
    if (insideRect(mouseX, mouseY, BUTTON_X, y, BUTTON_W, BUTTON_H)) {
        r = 120;
        g = 135;
        b = 215;
    }

    drawRect(BUTTON_X - 4, y - 4, BUTTON_W + 8, BUTTON_H + 8, 10, 10, 10);
    drawRect(BUTTON_X, y, BUTTON_W, BUTTON_H, r, g, b);
    drawRect(BUTTON_X, y, BUTTON_W, 4, r + 70, g + 65, b + 40);
    drawRect(BUTTON_X, y + BUTTON_H - 6, BUTTON_W, 6, r - 55, g - 55, b - 55);

    if (icon == ICON_PLAY) {
        drawTriangle(cx + 3, cy + 3, 40, 40, -90.0f, 0, 0, 0);
        drawTriangle(cx, cy, 40, 40, -90.0f, 90, 230, 90);
    }
    if (icon == ICON_EXIT) {
        drawRotatedRect(cx, cy, 50, 12, 45.0f, 230, 60, 60);
        drawRotatedRect(cx, cy, 50, 12, -45.0f, 230, 60, 60);
    }
    if (icon == ICON_RESTART) {
        drawCircle(cx, cy, 22, 255, 255, 255);
        drawCircle(cx, cy, 14, r, g, b);
        drawRect(cx, cy - 24, 24, 20, r, g, b);
        drawTriangle(cx + 4, cy - 18, 16, 16, -90.0f, 255, 255, 255);
    }
    if (icon == ICON_HOME) {
        drawTriangle(cx, cy - 12, 52, 24, 0.0f, 255, 255, 255);
        drawRect(cx - 18, cy - 1, 36, 22, 255, 255, 255);
        drawRect(cx - 5, cy + 7, 10, 14, r, g, b);
    }
}

void drawBed(float cx, float cy)
{
    drawRect(cx - 150, cy + 30, 22, 30, 90, 60, 30);
    drawRect(cx + 128, cy + 30, 22, 30, 90, 60, 30);
    drawRect(cx - 155, cy + 8, 310, 26, 150, 110, 60);
    drawRect(cx - 155, cy - 32, 310, 42, 200, 40, 40);
    drawRect(cx - 155, cy + 2, 310, 8, 150, 25, 25);
    drawRect(cx - 155, cy - 44, 95, 50, 240, 240, 240);
    drawRectOutline(cx - 155, cy - 44, 95, 50, 3, 200, 200, 200);
}

void drawTrophy(float cx, float cy)
{
    drawCircle(cx - 52, cy - 30, 24, 230, 180, 40);
    drawCircle(cx - 52, cy - 30, 14, 25, 25, 30);
    drawCircle(cx + 52, cy - 30, 24, 230, 180, 40);
    drawCircle(cx + 52, cy - 30, 14, 25, 25, 30);
    drawRect(cx - 50, cy - 65, 100, 70, 255, 205, 50);
    drawRect(cx - 12, cy + 5, 24, 30, 230, 180, 40);
    drawRect(cx - 45, cy + 35, 90, 18, 200, 150, 30);
    drawRect(cx - 35, cy - 55, 12, 45, 255, 240, 150);
}

void drawPauseSign(float cx, float cy)
{
    drawRect(cx - 38, cy - 55, 28, 110, 255, 255, 255);
    drawRect(cx + 10, cy - 55, 28, 110, 255, 255, 255);
}

void drawLostSign(float cx, float cy)
{
    drawBed(cx, cy);
    drawRotatedRect(cx, cy, 260, 24, 25.0f, 230, 50, 50);
    drawRotatedRect(cx, cy, 260, 24, -25.0f, 230, 50, 50);
}

void drawMenuBackground()
{
    drawRect(0, 0, SCREEN_W, SCREEN_H, 110, 175, 235);
    drawRect(0, 0, SCREEN_W, 260, 135, 195, 245);

    drawRect(150, 60, 140, 34, 255, 255, 255);
    drawRect(174, 44, 80, 24, 255, 255, 255);
    drawRect(900, 110, 160, 34, 255, 255, 255);
    drawRect(930, 94, 90, 24, 255, 255, 255);

    for (int row = 0;row < 3;row++) {
        for (int col = 0;col < 20;col++) {
            float x = col * 64.0f;
            float y = 560.0f + row * 64.0f;
            if (row == 0) {
                drawRect(x, y, 64, 64, 95, 170, 60);
            }
            else if ((col + row) % 2 == 0) {
                drawRect(x, y, 64, 64, 125, 85, 55);
            }
            else {
                drawRect(x, y, 64, 64, 140, 98, 62);
            }
            drawRectOutline(x, y, 64, 64, 2, 70, 55, 40);
        }
    }
}

void cardRect(bool unlocked, float x, float y, float w, float h, int r, int g, int b)
{
    if (!unlocked) {
        int gray = (r + g + b) / 3;
        r = gray;
        g = gray;
        b = gray;
    }
    drawRect(x, y, w, h, r, g, b);
}

void drawCoin(float cx, float cy, float radius)
{
    drawCircle(cx, cy, radius, 150, 100, 0);
    drawCircle(cx, cy, radius - 3, 255, 200, 40);
    drawCircle(cx - radius / 4, cy - radius / 4, radius / 3, 255, 235, 130);
}

void drawLock(float cx, float cy)
{
    drawCircle(cx, cy - 4, 8, 200, 200, 200);
    drawCircle(cx, cy - 4, 4, 98, 98, 98);
    drawRect(cx - 9, cy - 2, 18, 14, 200, 200, 200);
    drawRect(cx - 2, cy + 2, 4, 6, 98, 98, 98);
}

void drawSword(float cx, float cy, bool u)
{
    cardRect(u, cx - 4, cy - 22, 8, 26, 90, 225, 235);
    cardRect(u, cx - 1, cy - 22, 2, 24, 220, 255, 255);
    cardRect(u, cx - 12, cy + 4, 24, 5, 110, 70, 30);
    cardRect(u, cx - 3, cy + 9, 6, 8, 140, 95, 45);
    cardRect(u, cx - 5, cy + 17, 10, 4, 230, 190, 50);
}

void drawBow(float cx, float cy, bool u)
{
    cardRect(u, cx - 6, cy - 20, 8, 8, 140, 95, 45);
    cardRect(u, cx + 0, cy - 12, 8, 8, 140, 95, 45);
    cardRect(u, cx + 4, cy - 6, 8, 12, 140, 95, 45);
    cardRect(u, cx + 0, cy + 4, 8, 8, 140, 95, 45);
    cardRect(u, cx - 6, cy + 12, 8, 8, 140, 95, 45);
    cardRect(u, cx - 7, cy - 16, 2, 32, 235, 235, 235);
    cardRect(u, cx - 22, cy - 1, 44, 3, 205, 205, 205);
    cardRect(u, cx + 20, cy - 5, 4, 11, 90, 90, 100);
    cardRect(u, cx - 24, cy - 5, 4, 11, 220, 60, 60);
}

void drawShield(float cx, float cy, bool u)
{
    float rowY[5] = { -20.0f,-11.0f,-2.0f,5.0f,12.0f };
    float rowW[5] = { 36.0f,36.0f,28.0f,20.0f,10.0f };
    float rowH[5] = { 9.0f,9.0f,7.0f,7.0f,5.0f };

    for (int i = 0;i < 5;i++) {
        cardRect(u, cx - rowW[i] / 2 - 3, cy + rowY[i] - 2, rowW[i] + 6, rowH[i] + 4, 60, 60, 70);
    }
    for (int i = 0;i < 5;i++) {
        cardRect(u, cx - rowW[i] / 2, cy + rowY[i], rowW[i], rowH[i], 185, 190, 200);
    }

    cardRect(u, cx - 3, cy - 16, 6, 26, 60, 95, 205);
    cardRect(u, cx - 10, cy - 8, 20, 6, 60, 95, 205);
}

void drawCards()
{
    int hovered = cardUnderMouse();

    for (int i = 0;i < 3;i++) {
        bool u = isUnlocked(i);
        float x = cardX(i);
        float y = CARD_Y;
        if (i == hovered) {
            y -= 4;
        }

        cardRect(u, x - 3, y - 3, CARD_W + 6, CARD_H + 6, 15, 15, 15);
        if (i == hovered) {
            cardRect(u, x, y, CARD_W, CARD_H, 135, 135, 150);
        }
        else {
            cardRect(u, x, y, CARD_W, CARD_H, 98, 98, 98);
        }
        cardRect(u, x, y, CARD_W, 3, 190, 190, 190);
        cardRect(u, x, y, 3, CARD_H, 190, 190, 190);
        cardRect(u, x, y + CARD_H - 3, CARD_W, 3, 55, 55, 55);
        cardRect(u, x + CARD_W - 3, y, 3, CARD_H, 55, 55, 55);
        if (i == hovered) {
            drawRectOutline(x - 3, y - 3, CARD_W + 6, CARD_H + 6, 3, 255, 235, 90);
        }

        if (i == MELEE) {
            drawSword(x + CARD_W / 2, y + 36, u);
        }
        if (i == RANGED) {
            drawBow(x + CARD_W / 2, y + 36, u);
        }
        if (i == DEFENSE) {
            drawShield(x + CARD_W / 2, y + 36, u);
        }

        if (u) {
            drawCoin(x + 40, y + CARD_H - 22, 10);
            drawNumber(cardPrice[i], x + 58, y + CARD_H - 32, 4, 255, 225, 90);
        }
        else {
            drawLock(x + 40, y + CARD_H - 22);
            drawNumber(cardUnlockAt[i], x + 58, y + CARD_H - 32, 4, 230, 230, 230);
        }
    }
}

void drawWaves()
{
    for (int i = 0;i < 3;i++) {
        float x = 610.0f + i * 50.0f;
        drawRect(x, 40, 36, 36, 20, 20, 22);
        if (i < wave) {
            drawRect(x + 5, 45, 26, 26, 200, 40, 40);
        }
        drawRectOutline(x, 40, 36, 36, 3, 200, 200, 200);
    }
}

void drawHUD()
{
    drawRect(0, 0, SCREEN_W, HUD_H, 70, 70, 74);
    for (int row = 0;row < 4;row++) {
        for (int col = 0;col < 40;col++) {
            if ((col * 17 + row * 31) % 5 < 2) {
                drawRect(col * 32.0f, row * 32.0f, 32, 32, 60, 60, 64);
            }
        }
    }
    drawRect(0, HUD_H - 6, SCREEN_W, 6, 30, 30, 32);

    drawCards();
    drawWaves();

    drawRect(SCREEN_W - 290, 22, 270, 76, 20, 20, 22);
    if (coinFlashTime > 0) {
        drawRectOutline(SCREEN_W - 290, 22, 270, 76, 4, 255, 60, 60);
    }
    else {
        drawRectOutline(SCREEN_W - 290, 22, 270, 76, 4, 255, 210, 60);
    }
    drawCoin(SCREEN_W - 252, 60, 22);
    drawNumber(coins, SCREEN_W - 212, 40, 8, 255, 225, 90);
}

void drawLaneHighlights()
{
    for (int lane = 0;lane < NUM_LANES;lane++) {
        float y = laneTop(lane);
        if (laneFlashTime > 0) {
            drawRectOutline(FIELD_X, y, FIELD_W, LANE_H, 6, 255, 60, 60);
        }
        if (laneSelected[lane]) {
            drawRectOutline(FIELD_X, y, FIELD_W, LANE_H, 6, 255, 225, 60);
        }
        if (lane == hoveredLane) {
            drawRectOutline(FIELD_X + 6, y + 6, FIELD_W - 12, LANE_H - 12, 4, 255, 255, 255);
        }
    }
}

void updateGame(float dt)
{
    updateEnvironment(dt);

    if (laneFlashTime > 0) {
        laneFlashTime -= dt;
    }
    if (coinFlashTime > 0) {
        coinFlashTime -= dt;
    }

    if (gameState == STATE_CUTSCENE) {
        updateCutscene(dt);
        if (cutsceneFinished()) {
            gameState = STATE_PLAYING;
        }
    }
    else if (gameState == STATE_PLAYING) {
       updatePlayers(dt);
       updateEnemies(dt);
       updateCollectibles(dt);
       checkPlayerEnemyCollisions();
       checkHardEnemyAttacks(dt);
       checkEnemyProjectileHits();
       removeDefeatedPlayers();
       removeInactivePlayerArrows();
       updateRangedAttacks(dt);
        if (enemyAtBase) {
            gameState = STATE_LOST;
        }
        else if (wavesDone) {
            gameState = STATE_WON;
        }
    }
}

void drawGame()
{
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (gameState == STATE_MENU) {
        drawMenuBackground();
        drawBed(640, 190);
        drawButton(0, ICON_PLAY);
        drawButton(1, ICON_EXIT);
    }
    else if (gameState == STATE_CUTSCENE) {
        drawCutscene();
    }
    else {
        drawEnvironment();
        drawLaneHighlights();
        drawCollectibles();
        drawPlayers();
        drawPlayerArrows();
        drawEnemies();
        drawHUD();

        if (gameState != STATE_PLAYING) {
            drawRect(440, 110, 400, 490, 25, 25, 30);
            drawRectOutline(440, 110, 400, 490, 4, 90, 90, 96);
        }
        if (gameState == STATE_PAUSED) {
            drawPauseSign(640, 200);
            drawButton(0, ICON_PLAY);
            drawButton(1, ICON_RESTART);
            drawButton(2, ICON_HOME);
        }
        if (gameState == STATE_WON) {
            drawTrophy(640, 200);
            drawButton(0, ICON_RESTART);
            drawButton(1, ICON_HOME);
        }
        if (gameState == STATE_LOST) {
            drawLostSign(640, 200);
            drawButton(0, ICON_RESTART);
            drawButton(1, ICON_HOME);
        }
    }
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS) {
        return;
    }

    if (key == GLFW_KEY_ESCAPE) {
        if (gameState == STATE_PLAYING) {
            gameState = STATE_PAUSED;
        }
        else if (gameState == STATE_PAUSED) {
            gameState = STATE_PLAYING;
        }
        return;
    }

    if (gameState == STATE_CUTSCENE) {
        if (key == GLFW_KEY_SPACE) {
            skipCutscene();
            gameState = STATE_PLAYING;
        }
        return;
    }

    if (gameState != STATE_PLAYING) {
        return;
    }
    if (key >= GLFW_KEY_1 && key <= GLFW_KEY_4) {
        int lane = key - GLFW_KEY_1;
        laneSelected[lane] = !laneSelected[lane];
    }
    if (key == GLFW_KEY_W || key == GLFW_KEY_UP) {
        jumpPlayers();
    }
    if (key == GLFW_KEY_S || key == GLFW_KEY_DOWN) {
        duckPlayers();
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT ||
        action != GLFW_PRESS) {
        return;
    }

    if (gameState == STATE_MENU) {
        int b = buttonUnderMouse(2);
        if (b == 0) {
            startNewGame();
            startCutscene();
            gameState = STATE_CUTSCENE;
        }
        if (b == 1) {
            glfwSetWindowShouldClose(window, true);
        }
    }
    else if (gameState == STATE_PAUSED) {
        int b = buttonUnderMouse(3);
        if (b == 0) {
            gameState = STATE_PLAYING;
        }
        if (b == 1) {
            startNewGame();
            gameState = STATE_PLAYING;
        }
        if (b == 2) {
            gameState = STATE_MENU;
        }
    }
    else if (gameState == STATE_WON || gameState == STATE_LOST) {
        int b = buttonUnderMouse(2);
        if (b == 0) {
            startNewGame();
            gameState = STATE_PLAYING;
        }
        if (b == 1) {
            gameState = STATE_MENU;
        }
    }
    else if (gameState == STATE_PLAYING) {
        int card = cardUnderMouse();
        if (card != -1) {
            placePlayers(card);
        }
        else if (hoveredLane != -1) {
            laneSelected[hoveredLane] = !laneSelected[hoveredLane];
        }
    }
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    mouseX = (float)xpos;
    mouseY = (float)ypos;
    hoveredLane = laneAt(mouseX, mouseY);
}

int main()
{
    if (!glfwInit()) {
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCREEN_W, SCREEN_H, "Minecraft Bedwars 2D", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to init GLAD\n";
        return -1;
    }

    glViewport(0, 0, SCREEN_W, SCREEN_H);
    glfwSwapInterval(1);

    createShaderProgram();
    createShapes();

    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);

    for (int i = 0;i < NUM_LANES;i++) {
        laneSelected[i] = false;
    }

    float lastFrame = (float)glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (deltaTime > 0.05f) {
            deltaTime = 0.05f;
        }

        updateGame(deltaTime);
        drawGame();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    activeEnemies.clear();
    activePlayers.clear();
    playerArrows.clear();
    hardEnemyAttackTimers.clear();

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}