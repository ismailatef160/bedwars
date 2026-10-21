#pragma once
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>
#include <glm/glm.hpp>
#include "Person.h"
#include <vector>
enum class EnemyType {
	EASY,
	MEDIUM,
	HARD
};

enum class EnemyState {
	MOVING,
	ATTACKING,
	DYING
};

struct Projectile {
	float speed;
	bool isHighAtatck;
	bool Active;
	glm::vec2 position;
};

class Enemy: public Person {
protected:
	EnemyType enemyType;
	EnemyState enemyState;
	int collectibleReward;
	std::vector<Projectile> activeProjectiles;

	void drawPhantom(float cx, float cy, float rTint) const;
	void drawCreeper(float cx, float cy, float rTint) const;
	void drawArmoredRusher(float cx, float cy, float rTint) const;

public:
	Enemy(EnemyType enemyType, int lane, glm::vec2 startPosition);

	~Enemy() override;


	void update(float deltaTime) override;
	void render() const override;

 	void attack();
	void DropCollictible() const;

	std::vector<Projectile>& getProjectiles() { return activeProjectiles; }
	EnemyType getType() const { return enemyType; }
	int getReward() const { return collectibleReward; }
};