#pragma once
#include <glm/glm.hpp>
#include <algorithm>
using namespace glm;


class Person {
protected:
	int lane;
	int maxHealth;
	int currentHealth;
	float speed;
	float attackCooldown;
	float attackTimer;
	bool isColliding;
	vec2 position;

public:
	Person(int lane, int maxHealth, float speed, float attackCooldown, vec2 position) {
		this->lane = lane;
		this->maxHealth = maxHealth;
		this->currentHealth = maxHealth;
		this->speed = speed;
		this->attackCooldown = attackCooldown;
		this->position = position;
		this->attackTimer = 0.0f;
		this->isColliding = false;

	}

	//virtual destructor
	virtual ~Person() = default;


	virtual void update(float deltaTime) = 0;
	virtual void render() const = 0;

	void takeDamage(int damageAmount) {
		currentHealth = std::max(0, currentHealth - damageAmount);
	}
	void takeHeal(int amount) {
		currentHealth = std::min(maxHealth, currentHealth + amount);
	}
	bool isAlive() const {
		return currentHealth > 0;
	}

	float getHealthRatio() const {
		return (maxHealth > 0) ? static_cast<float>(currentHealth) / maxHealth : 0.0f;
	}
	// Getters
	int getMaxHealth() const { return maxHealth; }
	int getCurrentHealth() const { return currentHealth; }
	int getLane() const { return lane; }
	float getSpeed() const { return speed; }
	float getAttackCooldown() const { return attackCooldown; }
	bool getIsColliding() const { return isColliding; }
	vec2 getPosition() const { return position; }

	// Setters
	void setMaxHealth(int value) {
		if (value > 0)
			maxHealth = value;
	}
	void setCurrentHealth(int value) {
		if (value >= 0 && value <= maxHealth) currentHealth = value;
	}
	void setPosition(glm::vec2 value) { position = value; }
	void setIsColliding(bool colliding) { isColliding = colliding; }
	void setAttackCooldown(float value) {
		if (value >= 0.0f) attackCooldown = value;
	}

};