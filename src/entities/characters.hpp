// src/entities/Character.hpp
#pragma once
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <string>
#include <entities/game_object.hpp>
#include <map>
#include <algorithm>
#include <core/constants.hpp>

// Forward declare tile map type (we only need it in the cpp)
using TileMap = std::map<std::pair<int, int>, int>;

// Animation key constants (declared as extern)
namespace Anim {
    extern const std::string Walk;
    extern const std::string Run;
    extern const std::string Jump;
    extern const std::string Idle;
    extern const std::string Shoot;
    extern const std::string Recharge;
}

// Character type constants
namespace Characters {
    extern const std::string Fighter_Detective;
    extern const std::string Fighter_Boxer;
    extern const std::string Fighter_Boss;
    
    // Declaration only
    void load();
}

class Character : public GameObject {
    enum CharacterMoving { Idle, Walking, Running };
    enum CharacterState { None, Jumping, JumpEnd, Shooting, Recharging };
    bool m_grounded = false;

    // ---------- PHYSICS CONSTANTS ----------
    static constexpr float GRAVITY = 600.f;
    static constexpr float MAX_FALL_SPEED = 350.f;
    static constexpr float JUMP_SPEED = -300.f;
    static constexpr float WALK_SPEED = 100.f;
    static constexpr float RUN_SPEED = 200.f;

    // ---------- COLLISION BOX ----------
    static constexpr float BASE_WIDTH = 28.f;
    static constexpr float BASE_HEIGHT = 60.f;
    static constexpr float OFFSET_X = 40.f;
    static constexpr float OFFSET_Y = 52.f;
    static constexpr float FEET_WIDTH = 20.f;
    static constexpr float FEET_HEIGHT = 1.f;

    float timer = 0;
    CharacterState state = CharacterState::None;
    CharacterMoving moving = CharacterMoving::Idle;
    sf::Vector2f vel = sf::Vector2f(0.f, 0.f);
    sf::Vector2f pos = sf::Vector2f(0.f, 0.f);
    int direction = 1;
    std::string character;
    float m_screenHeight = 0.f;

    // ---------- BOUNDS FUNCTIONS ----------
    bool onGround() const;
    
    // ---------- PHYSICS ----------
    void physics(float dt);
    void resolveX();
    void resolveY();
    
    // ---------- ANIMATION ----------
    void animate(sf::RenderWindow& win, float dt);

    // ---------- DEBUG ----------
    void drawDebugBounds(sf::RenderWindow& win);

public:
    sf::FloatRect getBounds() const;
    sf::FloatRect getFeetBounds() const;
    Character(std::string c = Characters::Fighter_Boss);
    
    inline sf::Vector2f getPosition() const { return pos; }
    inline sf::Vector2f getSize() const { return sf::Vector2f(128.f, 128.f); } // Placeholder size
    
    void setCharacter(std::string c);
    
    // Movement controls
    void walk(int dir = 1);
    void run();
    void shoot();
    void recharge();
    void idle();
    void jump();

    // GameObject interface
    void draw(sf::RenderWindow& win, float dt) override;
    void update(sf::RenderWindow& win, float dt) override;
};