
#include "Player.h"

extern const int SCREEN_W;

void drawRect(float x, float y, float w, float h, int r, int g, int b);
void drawNumber(int n, float x, float y, float s, int r, int g, int b);

void drawDamagedRect(float x, float y, float w, float h,
                     int r, int g, int b, float damage)
{
    int red = r + (255 - r) * damage;
    int green = g * (1.0f - damage);
    int blue = b * (1.0f - damage);

    drawRect(x, y, w, h, red, green, blue);
}

Player::Player(PlayerType type, int lane, glm::vec2 startPosition)
    : Person(lane,
        (type == PlayerType::DEFENSE) ? 7 : 5,
        (type == PlayerType::DEFENSE) ? 0.0f : 45.0f,
        (type == PlayerType::RANGED) ? 1.5f : 0.8f,
        startPosition),
    playerType(type) {
}

void Player::jump() {
    if (playerType == PlayerType::DEFENSE || !active || !isAlive()) {
        return;
    }

    jumping = true;
    ducking = false;
    actionTimer = 1.0f;
}


void Player::duck() {
    if (playerType == PlayerType::DEFENSE || !active || !isAlive()) {
        return;
    }

    ducking = true;
    jumping = false;
    actionTimer = 1.0f;
}

void Player::update(float deltaTime) {
    if (!active || !isAlive()) {
        return;
    }
    if (swingTimer > 0.0f) {
    swingTimer -= deltaTime;

    if (swingTimer < 0.0f) {
        swingTimer = 0.0f;
    }
}   if (bowTimer > 0.0f) {
    bowTimer -= deltaTime;

    if (bowTimer < 0.0f) {
        bowTimer = 0.0f;
    }
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
        position.x += speed * deltaTime;
    }

    // The health-bar outline is the leftmost visual at x - 19.
    if (position.x - 19.0f >= SCREEN_W) {
        active = false;
        isColliding = false;
        jumping = false;
        ducking = false;
        return;
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
    if (!active || !isAlive()) {
        return;
    }

    float x = position.x;
    float y = position.y;

    // Smooth jump: rise and then land
    if (jumping) {
        float progress = 1.0f - actionTimer / 1.0f;
        y -= 40.0f * 4.0f * progress * (1.0f - progress);
    }

    float damage = 1.0f - getHealthRatio();

    // Draw a body part, making it shorter while ducking
    auto drawPart = [&](float px, float py, float w, float h,
                        int r, int g, int b) {

        if (ducking) {
            float feetY = y + 36.0f;
            py = feetY - (feetY - py) * 0.65f;
            h *= 0.65f;
        }

        drawDamagedRect(px, py, w, h, r, g, b, damage);
    };

    if (playerType == PlayerType::MELEE) {
        // Blue shirt and dark pants
        drawPart(x - 12, y - 10, 24, 28, 40, 140, 220);
        drawPart(x - 11, y + 18, 9, 18, 40, 50, 110);
        drawPart(x + 2, y + 18, 9, 18, 40, 50, 110);

        // Head and hair
        drawPart(x - 12, y - 36, 24, 26, 210, 165, 125);
        drawPart(x - 12, y - 36, 24, 7, 75, 45, 25);

        // Sword animation
        if (swingTimer > 0.0f) {
            // Sword swings horizontally toward the enemy
            drawPart(x + 15, y - 10, 30, 6, 170, 225, 235);
            drawPart(x + 9, y - 14, 7, 14, 110, 70, 35);
        }
        else {
            // Normal sword position
            drawPart(x + 18, y - 23, 6, 28, 170, 225, 235);
            drawPart(x + 12, y + 4, 18, 5, 110, 70, 35);
        }
    }
    else if (playerType == PlayerType::RANGED) {
        // Green archer outfit
        drawPart(x - 12, y - 12, 24, 30, 45, 130, 65);
        drawPart(x - 11, y + 18, 9, 18, 65, 60, 45);
        drawPart(x + 2, y + 18, 9, 18, 65, 60, 45);

        // Hood and face
        drawPart(x - 14, y - 38, 28, 28, 30, 100, 45);
        drawPart(x - 9, y - 30, 18, 17, 210, 165, 125);


    // Bow animation
    if (bowTimer > 0.0f) {
        // Bow while shooting
        drawPart(x + 22, y - 27, 5, 40, 130, 85, 45);
        drawPart(x + 12, y - 24, 2, 34, 220, 220, 220);
        drawPart(x + 10, y - 7, 16, 3, 170, 225, 235);
    }
    else {
        // Normal bow position
        drawPart(x + 20, y - 27, 5, 40, 130, 85, 45);
        drawPart(x + 17, y - 24, 2, 34, 220, 220, 220);
    }
    }
    else {
        // Armored Defense character
        drawPart(x - 15, y - 14, 30, 32, 130, 140, 155);
        drawPart(x - 13, y + 18, 11, 18, 75, 85, 100);
        drawPart(x + 2, y + 18, 11, 18, 75, 85, 100);

        // Helmet
        drawPart(x - 15, y - 38, 30, 24, 170, 180, 190);
        drawPart(x - 10, y - 28, 20, 7, 50, 60, 70);

        // Shield
        drawPart(x - 30, y - 12, 17, 39, 85, 95, 115);
        drawPart(x - 25, y - 6, 7, 26, 170, 180, 195);
    }

    // Health bar
    float barX = x - 18;
    float barY = ducking ? y - 27.0f : y - 49.0f;

    drawRect(barX - 1, barY - 1, 38, 7, 20, 20, 20);
    drawRect(barX, barY, 36 * getHealthRatio(), 5, 50, 210, 70);

    drawNumber(currentHealth, barX + 42, barY - 4, 2, 255, 255, 255);
}
