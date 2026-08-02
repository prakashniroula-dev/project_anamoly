// src/entities/Character.cpp
#include <graphics/textures.hpp>
#include <graphics/tiles.hpp>
#include <graphics/animation.hpp>
#include <entities/characters.hpp>
#include <debug/logs.hpp>
#include <core/scale.hpp>
#include <core/collision.hpp>    // Fixed spelling!
#include <entities/Terrain.hpp>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <algorithm>
#include <core/constants.hpp>

// ---------- ANIMATION KEYS (single copy) ----------
namespace Anim {
    const std::string Walk = "walk";
    const std::string Run = "run";
    const std::string Jump = "jump";
    const std::string Idle = "idle";
    const std::string Shoot = "shot";
    const std::string Recharge = "recharge";
}

// ---------- CHARACTER TYPES (single copy) ----------
namespace Characters {
    const std::string Fighter_Detective = "fighter_detective";
    const std::string Fighter_Boxer = "fighter_boxer";
    const std::string Fighter_Boss = "fighter_boss";

    void load() {
        static Log::Scope scope("Characters::Load()");
        
        scope.info << "Adding Fighter_Boss animations...\n";
        Animations::addList(Fighter_Boss, "characters/fighter_boss/", {
            {Anim::Walk, {"walk.png", 10, 80}},
            {Anim::Run, {"run.png", 10, 75}},
            {Anim::Jump, {"jump.png", 10, 80}},
            {Anim::Idle, {"idle.png", 6, 80}},
            {Anim::Shoot, {"shot.png", 11, 80}},
            {Anim::Recharge, {"recharge.png", 6, 80}}
        });
        
        scope.info << "Adding Fighter_Detective animations...\n";
        Animations::addList(Fighter_Detective, "characters/fighter_detective/", {
            {Anim::Walk, {"walk.png", 10, 80}},
            {Anim::Run, {"run.png", 10, 75}},
            {Anim::Jump, {"jump.png", 10, 80}},
            {Anim::Idle, {"idle.png", 6, 80}},
            {Anim::Shoot, {"shoot.png", 11, 80}},
            {Anim::Recharge, {"recharge.png", 6, 80}}
        });
    }

    
}



// ---------- CHARACTER IMPLEMENTATION ----------
Character::Character(std::string c) : character(c) {
    pos = sf::Vector2f(100.f, 32.f * 6 * Scale::get()); // Starting position
}

void Character::setCharacter(std::string c) { character = c; }

// ---------- BOUNDS ----------
sf::FloatRect Character::getBounds() const {
    float s = Scale::get();
    sf::Vector2f offset(OFFSET_X * s, OFFSET_Y * s);
    return sf::FloatRect(
        pos + offset,
        sf::Vector2f(BASE_WIDTH * s, BASE_HEIGHT * s)
    );
}

sf::FloatRect Character::getFeetBounds() const {
    float s = Scale::get();
    sf::FloatRect full = getBounds();
    float x = full.position.x + (full.size.x - FEET_WIDTH * s) / 2.f;
    float y = full.position.y + full.size.y - FEET_HEIGHT * s;
    return sf::FloatRect(
        sf::Vector2f(x, y),
        sf::Vector2f(FEET_WIDTH * s, FEET_HEIGHT * s)
    );
}

bool Character::onGround() const {
    sf::FloatRect feet = getFeetBounds();
    float s = Scale::get();
    float tileSize = 32.f * s;

    sf::Vector2f center = feet.position + feet.size / 2.f;
    sf::Vector2i tile = Tiles::getTileGridPosition(center);

    const TileMap& map = Terrain::getMap();
    return Tiles::isSolidTile(map, tile.x, tile.y);
}


// ---------- PHYSICS ----------
void Character::physics(float dt) {
    // Apply gravity
    vel.y += GRAVITY * dt;
    vel.y = std::min(vel.y, MAX_FALL_SPEED);

    // Horizontal movement
    pos.x += vel.x * dt;
    resolveX();

    // Vertical movement
    pos.y += vel.y * dt;
    resolveY();                 // handles floors, ceilings, and slopes

    // Update grounded state
    m_grounded = onGround();
}

void Character::resolveX() {
    float s = Scale::get();
    float tileSize = 32.f * s;
    sf::FloatRect charBox = getBounds();
    
    sf::Vector2i topLeft = Tiles::getTileGridPosition(charBox.position);
    sf::Vector2f bottomRight = charBox.position + charBox.size - sf::Vector2f(0.001f, 0.001f);
    sf::Vector2i bottomRightTile = Tiles::getTileGridPosition(bottomRight);
    
    int minTileX = std::min(topLeft.x, bottomRightTile.x) - 1;
    int maxTileX = std::max(topLeft.x, bottomRightTile.x) + 1;
    int minTileY = std::min(topLeft.y, bottomRightTile.y) - 1;
    int maxTileY = std::max(topLeft.y, bottomRightTile.y) + 1;
    
    const TileMap& terrain_map = Terrain::getMap();
    
    for (int tx = minTileX; tx <= maxTileX; ++tx) {
        for (int ty = minTileY; ty <= maxTileY; ++ty) {
            if (!Tiles::isSolidTile(terrain_map, tx, ty)) continue;
            int tileID = Tiles::getTileSafe(terrain_map, tx, ty);

            sf::FloatRect tileRect(
                Tiles::getTilePosition(tx, ty),
                sf::Vector2f(tileSize, tileSize)
            );
            auto result = Collision::getCollision(charBox, tileRect);
            if (!result.collided) continue;

            float sign = (charBox.position.x + charBox.size.x / 2.f < tileRect.position.x + tileRect.size.x / 2.f) ? 1.f : -1.f;
            pos.x -= result.overlap.x * sign;
            charBox.position.x = pos.x + OFFSET_X * s;
        }
    }
}

void Character::resolveY() {
    float s = Scale::get();
    float tileSize = 32.f * s;
    sf::FloatRect charBox = getBounds();

    sf::Vector2i topLeft = Tiles::getTileGridPosition(charBox.position);
    sf::Vector2f bottomRight = charBox.position + charBox.size - sf::Vector2f(0.001f, 0.001f);
    sf::Vector2i bottomRightTile = Tiles::getTileGridPosition(bottomRight);

    int minTileX = std::min(topLeft.x, bottomRightTile.x) - 1;
    int maxTileX = std::max(topLeft.x, bottomRightTile.x) + 1;
    int minTileY = std::min(topLeft.y, bottomRightTile.y) - 1;
    int maxTileY = std::max(topLeft.y, bottomRightTile.y) + 1;

    const TileMap& terrain_map = Terrain::getMap();

    // --- Second pass: handle non‑slope tiles (original logic) ---
    for (int tx = minTileX; tx <= maxTileX; ++tx) {
        for (int ty = minTileY; ty <= maxTileY; ++ty) {
            if (!Tiles::isSolidTile(terrain_map, tx, ty)) continue;
            int tileID = Tiles::getTileSafe(terrain_map, tx, ty);

            sf::FloatRect tileRect(
                Tiles::getTilePosition(tx, ty),
                sf::Vector2f(tileSize, tileSize)
            );

            auto result = Collision::getCollision(charBox, tileRect);
            if (!result.collided) continue;

            float sign = (charBox.position.y + charBox.size.y / 2.f < tileRect.position.y + tileRect.size.y / 2.f) ? 1.f : -1.f;
            pos.y -= result.overlap.y * sign;
            charBox.position.y = pos.y + OFFSET_Y * s;
        }
    }
}

// ---------- ANIMATION ----------
void Character::animate(sf::RenderWindow& win, float dt) {
    std::string animKey = Anim::Idle;
    if (moving == CharacterMoving::Walking) animKey = Anim::Walk;
    else if (moving == CharacterMoving::Running) animKey = Anim::Run;

    if (state == CharacterState::Jumping) animKey = Anim::Jump;
    else if (state == CharacterState::Shooting) animKey = Anim::Shoot;
    else if (state == CharacterState::Recharging) animKey = Anim::Recharge;

    auto info = Animations::get(character + animKey);
    sf::Sprite s(Textures::get(info.textureKey));
    float total_dur = info.totalFrames * info.duration / 1000.0f;
    float time_cycle = std::fmod(timer, total_dur);
    unsigned int currentFrame = static_cast<unsigned int>(time_cycle / info.duration * 1000.0f) % info.totalFrames;

    sf::IntRect rect(sf::Vector2i(currentFrame * 128, 0), sf::Vector2i(128, 128));
    if (direction == -1) {
        rect.position.x += rect.size.x;
        rect.size.x = -rect.size.x;
    }
    
    s.setTextureRect(rect);
    s.setScale(sf::Vector2f(0.9f, 0.9f));
    s.scale(Scale::getVec());
    s.setPosition(pos);

    if (currentFrame == info.totalFrames - 1) {
        state = CharacterState::None;
    }

    win.draw(s);
}

// ---------- DEBUG ----------
void Character::drawDebugBounds(sf::RenderWindow& win) {
    sf::FloatRect col = getBounds();
    sf::RectangleShape debugCol(sf::Vector2f(col.size.x, col.size.y));
    debugCol.setPosition(col.position);
    debugCol.setFillColor(sf::Color::Transparent);
    debugCol.setOutlineColor(sf::Color::Red);
    debugCol.setOutlineThickness(2.f);
    win.draw(debugCol);

    sf::FloatRect feet = getFeetBounds();
    sf::RectangleShape debugFeet(sf::Vector2f(feet.size.x, feet.size.y));
    debugFeet.setPosition(feet.position);
    debugFeet.setFillColor(sf::Color::Transparent);
    debugFeet.setOutlineColor(sf::Color::Green);
    debugFeet.setOutlineThickness(2.f);
    win.draw(debugFeet);

    sf::CircleShape dot(4.f);
    dot.setPosition(pos - sf::Vector2f(4.f, 4.f));
    dot.setFillColor(sf::Color::Cyan);
    win.draw(dot);
}

// ---------- PUBLIC INTERFACE ----------
void Character::walk(int dir) {
    if (state != CharacterState::None && state != CharacterState::Jumping) return;
    moving = CharacterMoving::Walking;
    vel.x = WALK_SPEED * dir;
    direction = dir;
}

void Character::run() {
    if (state != CharacterState::None && state != CharacterState::Jumping) return;
    moving = CharacterMoving::Running;
    vel.x = RUN_SPEED * direction;
}

void Character::shoot() {
    if (state != CharacterState::Shooting) timer = 0;
    vel.x = 0;
    vel.y = 0;
    state = CharacterState::Shooting;
}

void Character::recharge() {
    state = CharacterState::Recharging;
    vel.x = 0;
    vel.y = 0;
    timer = 0;
}

void Character::idle() {
    moving = CharacterMoving::Idle;
    state = CharacterState::None;
    vel.x = 0;
    vel.y = 0;
}

void Character::jump() {
    if (state == CharacterState::Jumping) return;
    state = CharacterState::Jumping;
    vel.y = JUMP_SPEED;
    timer = 0;
}

void Character::draw(sf::RenderWindow& win, float dt) {
    animate(win, dt);
    // drawDebugBounds(win);  // Comment out when happy with bounds
}

void Character::update(sf::RenderWindow& win, float dt) {
    m_screenHeight = static_cast<float>(win.getSize().y);

    drawDebugBounds(win);

    bool shiftKey = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift);
    vel.x = 0;
    moving = CharacterMoving::Idle;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) walk();
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) walk(-1);

    if (moving == CharacterMoving::Walking && shiftKey) run();

    if (state != CharacterState::Jumping && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space))
        jump();

    if (state != CharacterState::Recharging && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
        shoot();

    if (state != CharacterState::Recharging && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R))
        recharge();

    physics(dt);
    timer += dt;
}