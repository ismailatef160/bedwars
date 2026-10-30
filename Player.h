
#pragma once
#include "Person.h"

enum class PlayerType {
    MELEE,
    RANGED,
    DEFENSE
};

class Player : public Person {
private:
    PlayerType playerType;
    bool jumping = false;
    bool ducking = false;
    float actionTimer = 0.0f;

public:
    Player(PlayerType type, int lane, glm::vec2 startPosition);

    void update(float deltaTime) override;
    void render() const override;

    PlayerType getType() const { return playerType; }

    bool canAttack() const {
        return attackTimer >= attackCooldown;
    }

    void resetAttackTimer() {
        attackTimer = 0.0f;
    }
    void jump();
    void duck();

    bool isJumping() const { return jumping; }
    bool isDucking() const { return ducking; }
};
