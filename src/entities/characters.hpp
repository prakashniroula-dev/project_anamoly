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
#include <core/scale.hpp>
#include <graphics/tiles.hpp>


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
    enum CharacterState { None, Jumping, Shooting, Recharging };
    bool m_grounded = false;

    // ---------- PHYSICS CONSTANTS ----------
    // ---------- PHYSICS CONSTANTS ----------
    public:
    static constexpr float GRAVITY = 1200.f;        // was 600
    static constexpr float MAX_FALL_SPEED = 600.f;  // was 350
    static constexpr float JUMP_SPEED = -600.f;     // was -300
    static constexpr float WALK_SPEED = 200.f;      // was 100
    static constexpr float RUN_SPEED = 350.f;       // was 200

    static constexpr float ACCELERATION = 900.f;    // how fast we reach full speed
    static constexpr float FRICTION = 700.f;        // deceleration when no keys pressed

    // ---------- COLLISION BOX ----------
    static constexpr float BASE_WIDTH = 28.f;
    static constexpr float BASE_HEIGHT = 64.f;
    static constexpr float OFFSET_X = 44.f;
    static constexpr float OFFSET_Y = 58.f;
    static constexpr float FEET_WIDTH = 20.f;
    static constexpr float FEET_HEIGHT = 1.f;

    inline sf::Vector2f SPAWN_POS() const { return Tiles::getTilePosition(2, 5); } // Starting position
    private:

    float timer = 0;
    CharacterState state = CharacterState::None;
    CharacterMoving moving = CharacterMoving::Idle;
    sf::Vector2f vel = sf::Vector2f(0.f, 0.f);
    sf::Vector2f pos = sf::Vector2f(0.f, 0.f);
    int direction = 1;
    std::string character;
    float m_screenHeight = 0.f;

    // ---------- BOUNDS FUNCTIONS ----------
    bool onGround();
    
    // ---------- PHYSICS ----------
    void physics(float dt);
    void resolveCollision();
    
    // ---------- ANIMATION ----------
    void animate(sf::RenderWindow& win, float dt);

    // ---------- DEBUG ----------
    void drawDebugBounds(sf::RenderWindow& win);

public:
    sf::FloatRect getBounds();
    sf::FloatRect getFeetBounds();
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