
#include "Player.h"

void drawRect(float x, float y, float w, float h, int r, int g, int b);
void drawNumber(int n, float x, float y, float s, int r, int g, int b);

Player::Player(PlayerType type, int lane, glm::vec2 startPosition)
    : Person(lane,
        (type == PlayerType::DEFENSE) ? 7 : 5,
        (type == PlayerType::DEFENSE) ? 0.0f : 45.0f,
        (type == PlayerType::RANGED) ? 1.5f : 0.8f,
        startPosition),
    playerType(type) {
}

void Player::jump() {
    if (playerType == PlayerType::DEFENSE || !isAlive()) {
        return;
    }

    jumping = true;
    ducking = false;
    actionTimer = 1.0f;
}


void Player::duck() {
    if (playerType == PlayerType::DEFENSE || !isAlive()) {
        return;
    }

    ducking = true;
    jumping = false;
    actionTimer = 1.0f;
}

void Player::update(float deltaTime) {
    if (!isAlive()) {
        return;
    }
    if (actionTimer > 0.0f) {
    actionTimer -= deltaTime;

    if (actionTimer <= 0.0f) {
        jumping = false;
        ducking = false;
        actionTimer = 0.0f;
    }
}

    if (playerType != PlayerType::DEFENSE && !isColliding) {
        if (position.x < 1240.0f) {
            position.x += speed * deltaTime;
        }
    }

    if (playerType == PlayerType::MELEE) {
        if (isColliding) {
            attackTimer += deltaTime;
        }
        else {
            attackTimer = 0.0f;
        }
    }
    else {
        attackTimer += deltaTime;
    }
}

void Player::render() const {
    if (!isAlive()) {
        return;
    }



float x = position.x;
float y = position.y;

if (jumping) {
    y -= 60.0f;
}
else if (ducking) {
    y += 20.0f;
}

    if (playerType == PlayerType::MELEE) {
        // Blue shirt and dark pants
        drawRect(x - 12, y - 10, 24, 28, 40, 140, 220);
        drawRect(x - 11, y + 18, 9, 18, 40, 50, 110);
        drawRect(x + 2, y + 18, 9, 18, 40, 50, 110);

        // Head and hair
        drawRect(x - 12, y - 36, 24, 26, 210, 165, 125);
        drawRect(x - 12, y - 36, 24, 7, 75, 45, 25);

        // Sword
        drawRect(x + 18, y - 23, 6, 28, 170, 225, 235);
        drawRect(x + 12, y + 4, 18, 5, 110, 70, 35);
    }
    else if (playerType == PlayerType::RANGED) {
        // Green archer outfit
        drawRect(x - 12, y - 12, 24, 30, 45, 130, 65);
        drawRect(x - 11, y + 18, 9, 18, 65, 60, 45);
        drawRect(x + 2, y + 18, 9, 18, 65, 60, 45);

        // Hood and face
        drawRect(x - 14, y - 38, 28, 28, 30, 100, 45);
        drawRect(x - 9, y - 30, 18, 17, 210, 165, 125);

        // Bow
        drawRect(x + 20, y - 27, 5, 40, 130, 85, 45);
        drawRect(x + 17, y - 24, 2, 34, 220, 220, 220);
    }
    else {
        // Armored defense character
        drawRect(x - 15, y - 14, 30, 32, 130, 140, 155);
        drawRect(x - 13, y + 18, 11, 18, 75, 85, 100);
        drawRect(x + 2, y + 18, 11, 18, 75, 85, 100);

        // Helmet
        drawRect(x - 15, y - 38, 30, 24, 170, 180, 190);
        drawRect(x - 10, y - 28, 20, 7, 50, 60, 70);

        // Large shield
        drawRect(x - 30, y - 12, 17, 39, 85, 95, 115);
        drawRect(x - 25, y - 6, 7, 26, 170, 180, 195);
    }

    // Health bar
    float barX = x - 18;
    float barY = y - 49;

    drawRect(barX - 1, barY - 1, 38, 7, 20, 20, 20);
    drawRect(barX, barY, 36 * getHealthRatio(), 5, 50, 210, 70);

    drawNumber(currentHealth, barX + 42, barY - 4, 2, 255, 255, 255);
}
