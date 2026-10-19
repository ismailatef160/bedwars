#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<iostream>
#include<vector>
#include<cmath>

#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>

const int SCREEN_W = 1280;
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
#version 430 core

layout (location = 0) in vec3 aPos;

uniform mat4 u_Transform;

void main()
{
    gl_Position = u_Transform * vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 430 core

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
int cardUnlockAt[3] = { 10,60,90 };

float laneFlashTime = 0;
float coinFlashTime = 0;

void drawEnvironment()
{
}

void startCutscene()
{
}

void updateCutscene(float dt)
{
}

bool cutsceneFinished()
{
    return true;
}

void drawCutscene()
{
}

void resetPlayers()
{
}

void spawnPlayer(int type, int lane)
{
}

void jumpPlayers()
{
}

void duckPlayers()
{
}

void updatePlayers(float dt)
{
}

void drawPlayers()
{
}

int wave = 1;
bool enemyAtBase = false;
bool wavesDone = false;

void resetEnemies() {

    wave = 1;
    enemyAtBase = false;
    wavesDone = false;
}

void updateEnemies(float dt)
{
}

void drawEnemies()
{
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
    for (int i = 0;i < NUM_LANES;i++) {
        laneSelected[i] = false;
    }
    laneFlashTime = 0;
    coinFlashTime = 0;
    resetPlayers();
    resetEnemies();
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
            coins -= cardPrice[type];
            spawnPlayer(type, lane);
            placed++;
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
        drawPlayers();
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

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}