#include "Enemy.h"
#include <glad/glad.h>



void drawRect(float x, float y, float w, float h, int r, int g, int b);
void drawRotatedRect(float cx, float cy, float w, float h, float angle, int r, int g, int b);
void drawCircle(float cx, float cy, float radius, int r, int g, int b);
void drawTriangle(float cx, float cy, float w, float h, float angle, int r, int g, int b);
void drawNumber(int n, float x, float y, float s, int r, int g, int b);


Enemy::Enemy(EnemyType enemyType, int lane, glm::vec2 startPsoition)
	:Person(lane,
		(enemyType == EnemyType::HARD) ? 7 : 5,
		(enemyType == EnemyType::HARD) ? 40.0f : 30.0f,
		(enemyType == EnemyType::HARD) ? 1.0f : 2.0f,
		startPsoition),
	enemyType(enemyType),
	enemyState(EnemyState::MOVING) {

	if (enemyType == EnemyType::EASY) {
		collectibleReward = 10;
	}
	else if (enemyType == EnemyType::MEDIUM) {
		collectibleReward = 20;
	}
	else {
		collectibleReward = 50;
	}

	
}

Enemy::~Enemy() {
	
	activeProjectiles.clear();

}

void Enemy::update(float deltaTime) {

	if (!isAlive()) {
		enemyState = EnemyState::DYING;
		return;
	}

	if (!isColliding) {
		enemyState = EnemyState::MOVING;
		position.x -= speed * deltaTime; // here we are reducing the x coordinates because the enemies move to the right to left
	}
	else {
		enemyState = EnemyState::ATTACKING;
	}

	attackTimer += deltaTime;

	if (attackTimer >= attackCooldown) {
		if (enemyType == EnemyType::HARD) {
			if (isColliding) {
				attack();
				attackTimer = 0.0f;
			}
		}
		else {
			attack();
			attackTimer = 0.0f;
		}
	}

	for (auto& proj : activeProjectiles) {
		if (proj.Active) {
			proj.position.x -= proj.speed * deltaTime;

			if (proj.position.x < 130.0f) {
				proj.Active = false;
			}
		}
	}
}

static void applyDamageColor(int baseR, int baseG, int baseB, float rTint, int& outR, int& outG, int& outB) {
	outR = static_cast<int>(baseR + (255 - baseR) * rTint);
	outG = static_cast<int>(baseG * (1.0f - rTint));
	outB = static_cast<int>(baseB * (1.0f - rTint));
}

void Enemy::drawPhantom(float cx, float cy, float rTint) const {
	int r, g, b;
	int t = glfwGetTime() * 12.0f;
	float bob = (enemyState == EnemyState::MOVING) ? sin(t) * 4.0f : 0.0f;
	float flapAngle = (enemyState == EnemyState::MOVING) ? -sin(t) * 35.0f : 0.0f;

	cy += bob;
	// Wings
	applyDamageColor(40, 50, 95, rTint, r, g, b);
	drawRotatedRect(cx - 24, cy - 8, 30, 10, -20.0f - flapAngle, r, g, b);
	drawRotatedRect(cx + 24, cy - 8, 30, 10, 20.0f + flapAngle, r, g, b);
	// Main Body
	applyDamageColor(60, 75, 130, rTint, r, g, b);
	drawRect(cx - 15, cy - 12, 30, 24, r, g, b);
	// Head & Tail
	applyDamageColor(35, 40, 70, rTint, r, g, b);
	drawRect(cx - 25, cy - 8, 12, 16, r, g, b);
	drawRect(cx + 15, cy - 4, 18, 8, r, g, b);
	// Glowing Eyes
	drawRect(cx - 22, cy - 4, 4, 4, 180, 255, 60);
}

void Enemy::drawCreeper(float cx, float cy, float rTint) const {
	int r, g, b;
	float t = glfwGetTime() * 8.0f; // Animation speed
	float bob = (enemyState == EnemyState::MOVING) ? sin(t) * 2.0f : 0.0f;
	float stride = (enemyState == EnemyState::MOVING) ? sin(t) * 2.0f : 0.0f;

	cy += bob; // Apply vertical bounce to the upper body

	applyDamageColor(40, 180, 45, rTint, r, g, b);
	drawRect(cx - 16, cy - 36, 32, 28, r, g, b);
	drawRect(cx - 10, cy - 30, 6, 6, 20, 20, 20);
	drawRect(cx + 4, cy - 30, 6, 6, 20, 20, 20);
	drawRect(cx - 4, cy - 22, 8, 12, 20, 20, 20);

	applyDamageColor(35, 150, 40, rTint, r, g, b);
	drawRect(cx - 12, cy - 8, 24, 30, r, g, b);

	// Legs offset in opposite directions for a walking cycle
	applyDamageColor(25, 100, 30, rTint, r, g, b);
	drawRect(cx - 16 + stride, cy + 22 - bob, 12, 14, r, g, b);
	drawRect(cx + 4 - stride, cy + 22 - bob, 12, 14, r, g, b);
}


void Enemy::drawArmoredRusher(float cx, float cy, float rTint) const {
	int r, g, b;
	// Helmet
	applyDamageColor(80, 210, 225, rTint, r, g, b); // Diamond color
	drawRect(cx - 16, cy - 38, 32, 26, r, g, b);
	drawRect(cx - 10, cy - 28, 5, 4, 230, 190, 150); // Face visor slit
	drawRect(cx + 5, cy - 28, 5, 4, 230, 190, 150);
	// Armored Chestplate
	applyDamageColor(60, 190, 205, rTint, r, g, b);
	drawRect(cx - 15, cy - 12, 30, 32, r, g, b);
	// Legs
	applyDamageColor(40, 150, 170, rTint, r, g, b);
	drawRect(cx - 13, cy + 20, 10, 16, r, g, b);
	drawRect(cx + 3, cy + 20, 10, 16, r, g, b);
	// Iron/Diamond Sword in hand
	drawRect(cx - 24, cy - 22, 6, 26, 230, 230, 240);
	drawRect(cx - 27, cy + 4, 12, 4, 120, 80, 40);
}


void Enemy::attack() {
	if (enemyType == EnemyType::EASY) {
		// High attack (above the lane -> smaller Y in top-left coordinate system)
		activeProjectiles.push_back({
			260.0f, // Much faster than the enemy's 45.0f speed!
			true,   // isHighAttack = true (Player must duck)
			true  ,
			glm::vec2(position.x - 25.0f, position.y - 22.0f),// active
			});
	}
	else if (enemyType == EnemyType::MEDIUM) {
		// Low attack (below the lane -> larger Y in top-left coordinate system)
		activeProjectiles.push_back({

			260.0f, // Much faster than the enemy's 45.0f speed!
			false,  // isHighAttack = false (Player must jump)
			true  ,
			glm::vec2(position.x - 25.0f, position.y + 22.0f),// active
			});
	}
}


void Enemy::render() const {
	float rTint = 1.0f - getHealthRatio(); // 0.0f at full health, 1.0f at zero health

	// Render composite enemy model
	if (enemyType == EnemyType::EASY) {
		drawPhantom(position.x, position.y, rTint);
	}
	else if (enemyType == EnemyType::MEDIUM) {
		drawCreeper(position.x, position.y, rTint);
	}
	else {
		drawArmoredRusher(position.x, position.y, rTint);
	}

	// Always-visible health indicator above character
	float barW = 36.0f;
	float barH = 5.0f;
	float barX = position.x - barW / 2.0f;
	float barY = position.y - 48.0f;

	// Background bar (Dark Gray)
	drawRect(barX - 1, barY - 1, barW + 2, barH + 2, 20, 20, 20);
	// Active Health Fill (Green to Red)
	drawRect(barX, barY, barW * getHealthRatio(), barH, 220, 40, 40);

	// Number of remaining lives directly beside the bar
	drawNumber(currentHealth, barX + barW + 4, barY - 4, 2, 255, 255, 255);

	// Render active projectiles
	for (const auto& proj : activeProjectiles) {
		if (!proj.Active) continue;
		if (proj.isHighAtatck) {
			// High attack (TNT block)
			drawRect(proj.position.x - 7, proj.position.y - 7, 14, 14, 210, 50, 40);
			drawRect(proj.position.x - 5, proj.position.y - 2, 10, 4, 240, 240, 240);
		}
		else {
			// Low attack (Fireball)
			drawCircle(proj.position.x, proj.position.y, 7, 240, 120, 30);
			drawCircle(proj.position.x, proj.position.y, 4, 255, 220, 40);
		}
	}
}
