

// src/entities/Character.cpp
#include <graphics/textures.hpp>
#include <graphics/tiles.hpp>
#include <graphics/animation.hpp>
#include <entities/characters.hpp>
#include <debug/logs.hpp>
#include <core/scale.hpp>
#include <core/collision.hpp>    // Fixed spelling!
#include <map/terrain.hpp>
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


    std::string Player = std::string(Fighter_Detective); // Added for player character

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
            {Anim::Shoot, {"shot.png", 4, 80}},
            {Anim::Recharge, {"recharge.png", 17, 80}}
        });
    }

    
}



// ---------- CHARACTER IMPLEMENTATION ----------
Character::Character(std::string c, bool playerControls) : character(c), m_playerControls(playerControls) {
    vel = sf::Vector2f(0.f, 0.f);
}

void Character::init() {
    pos = Terrain::getPlayerSpawnPosition();
}

void Character::setCharacter(std::string c) { character = c; }

// ---------- BOUNDS ----------
sf::FloatRect Character::getBounds() {
    float s = Scale::get();
    sf::Vector2f scaledPos = pos * s; // Scale the base position
    sf::Vector2f offset(OFFSET_X * s, OFFSET_Y * s);
    
    return sf::FloatRect(
        scaledPos + offset,
        sf::Vector2f(BASE_WIDTH * s, BASE_HEIGHT * s)
    );
}

sf::FloatRect Character::getFeetBounds() {
    float s = Scale::get();
    sf::FloatRect full = getBounds();
    float x = full.position.x + (full.size.x - FEET_WIDTH * s) / 2.f;
    float y = full.position.y + full.size.y - FEET_HEIGHT * s;
    return sf::FloatRect(
        sf::Vector2f(x, y),
        sf::Vector2f(FEET_WIDTH * s, FEET_HEIGHT * s)
    );
}

bool Character::onGround() {
    sf::FloatRect feet = getFeetBounds();
    float s = Scale::get();
    float tileSize = 32.f * s;

    // (Optional) Extend feet downward by 1 pixel to catch rounding errors
    feet.size.y += 1.f;

    // Get the tile range that the feet rectangle covers
    sf::Vector2i topLeft = Tiles::getTileGridPosition(feet.position);
    sf::Vector2i bottomRightTile = Tiles::getTileGridPosition(feet.position + feet.size);

    int minTileX = std::min(topLeft.x, bottomRightTile.x);
    int maxTileX = std::max(topLeft.x, bottomRightTile.x);
    int minTileY = std::min(topLeft.y, bottomRightTile.y);
    int maxTileY = std::max(topLeft.y, bottomRightTile.y);

    for (int tx = minTileX; tx <= maxTileX; ++tx) {
        for (int ty = minTileY; ty <= maxTileY; ++ty) {
            if (!Terrain::isSolidTile(tx, ty)) continue;

            sf::FloatRect tileRect(
                Tiles::getTilePosition(tx, ty),
                sf::Vector2f(tileSize, tileSize)
            );

            // SFML 3.x way: check if intersection exists
            if (feet.findIntersection(tileRect).has_value()) {
                return true;
            }
        }
    }
    return false;
}


// ---------- PHYSICS ----------
void Character::physics(float dt) {
    // Apply gravity
    vel.y += GRAVITY * dt;
    vel.y = std::min(vel.y, MAX_FALL_SPEED);

    // Horizontal movement
    pos.x += vel.x * dt;
    pos.y += vel.y * dt;
    resolveCollision(); // testing - one does both

    // Vertical movement
    // resolveY();                 // handles floors, ceilings, and slopes

    // Update grounded state
    m_grounded = onGround();
    if ( m_grounded && vel.y > 0.f ) {
        if ( state == CharacterState::Jumping ) {
            state = CharacterState::None;
        }
        vel.y = 0.f; // Reset vertical velocity when grounded
    }
}

void Character::resolveCollision() {
    float s = Scale::get();
    float tileSize = 32.f * s;

    // We'll iterate a few times to handle cases where resolving one tile
    // pushes the character into another tile.
    const int MAX_ITERATIONS = 5;
    for (int iter = 0; iter < MAX_ITERATIONS; ++iter) {
        sf::FloatRect charBox = getBounds();
        bool anyCollision = false;

        // Get the tile range covering the character's bounding box
        sf::Vector2i topLeft = Tiles::getTileGridPosition(charBox.position);
        sf::Vector2i bottomRightTile = Tiles::getTileGridPosition(charBox.position + charBox.size);

        int minTileX = std::min(topLeft.x, bottomRightTile.x) - 1;
        int maxTileX = std::max(topLeft.x, bottomRightTile.x) + 1;
        int minTileY = std::min(topLeft.y, bottomRightTile.y) - 1;
        int maxTileY = std::max(topLeft.y, bottomRightTile.y) + 1;

        for (int tx = minTileX; tx <= maxTileX; ++tx) {
            for (int ty = minTileY; ty <= maxTileY; ++ty) {
                if (!Terrain::isSolidTile(tx, ty)) continue;

                sf::FloatRect tileRect(
                    Tiles::getTilePosition(tx, ty),
                    sf::Vector2f(tileSize, tileSize)
                );

                // Check collision
                auto result = Collision::getCollision(charBox, tileRect);
                if (!result.collided) continue;

                anyCollision = true;

                sf::Vector2f scaledPos = pos * s;
                
                // Adjust the scaled position
                Collision::resolveCollision(charBox, tileRect, scaledPos);
                
                // Save it back to our unscaled character position!
                pos = scaledPos / s;

                // Update charBox to reflect new position for subsequent tile checks
                charBox.position = pos + sf::Vector2f(OFFSET_X * s, OFFSET_Y * s);
            }
        }

        // If no collisions were found this iteration, we're done
        if (!anyCollision) break;
    }
}

void Character::snapToGround() {
    // Move down until we are on the ground, but limit iterations to avoid infinite loops.
    const int MAX_ITER = 100;
    for (int i = 0; i < MAX_ITER; ++i) {
        if (onGround())
            break;
        pos.y += 0.5f;   // move down by half a pixel (unscaled)
    }
    // If we still aren't on the ground, try moving up a bit (shouldn't happen).
    if (!onGround()) {
        for (int i = 0; i < MAX_ITER; ++i) {
            pos.y -= 0.5f;
            if (onGround()) break;
        }
    }
}

// ---------- ANIMATION ----------
void Character::animate(sf::RenderWindow& win, float dt) {
    std::string animKey = Anim::Idle;
    // if (!m_playerControls) {
    //     Log::info << "animate() using key: " << character << animKey << "\n";
    // }
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
    sf::Vector2f scale = Scale::getVec();
    if (direction == -1) {
        scale.x *= -1.f;          // horizontal flip
    }
    
    s.setTextureRect(rect);
    s.scale(scale);
    const float originX = OFFSET_X + BASE_WIDTH / 2.f;   // 44 + 14 = 58
    const float originY = OFFSET_Y + BASE_HEIGHT;        // 58 + 62 = 120
    s.setOrigin({0.f, 0.f});

    // 2. Place the origin at the logical position.
    sf::Vector2f drawPos = pos * Scale::get();

    if (direction == -1) {
        drawPos.x += 120.f * Scale::get();  // or whatever value matches your visual alignment
    }
    s.setPosition(drawPos);

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

    // sf::CircleShape dot(4.f);
    // dot.setPosition(pos - sf::Vector2f(4.f, 4.f));
    // dot.setFillColor(sf::Color::Cyan);
    // win.draw(dot);
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
    if ( !m_playerControls ) {
        Log::info << "NPC shooting at position: (" << pos.x << ", " << pos.y << ")\n";
        Log::info << "State: " << static_cast<int>(state) << ", Moving: " << static_cast<int>(moving) << "\n";
    }
    if ( state != CharacterState::None) return;
    if (state != CharacterState::Shooting) timer = 0;
    state = CharacterState::Shooting;
}

void Character::recharge() {
    state = CharacterState::Recharging;
    vel.x = 0;
    timer = 0;
}

void Character::idle() {
    moving = CharacterMoving::Idle;
    state = CharacterState::None;
    vel.x = 0;
    vel.y = 0;
}

void Character::jump() {
    if (state != CharacterState::None || !onGround()) return;
    state = CharacterState::Jumping;
    vel.y = JUMP_SPEED;
    timer = 0;
}

void Character::draw(sf::RenderWindow& win, float dt) {
    animate(win, dt);
    // drawDebugBounds(win);
}

void Character::update(sf::RenderWindow& win, float dt) {
    m_screenHeight = static_cast<float>(win.getSize().y);

    if (m_playerControls) {
        bool shiftKey = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift);
        vel.x = 0;
        moving = CharacterMoving::Idle;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) walk(1);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) walk(-1);

        if (moving == CharacterMoving::Walking && shiftKey) run();

        if (state != CharacterState::Jumping && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space))
            jump();

        if (state != CharacterState::Recharging && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
            shoot();

        if (state != CharacterState::Recharging && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R))
            recharge();
    }

    physics(dt);
    if (pos.y + getSize().y > Constants::WORLD_HEIGHT_PIXELS) {
        // Reset to initial position
        pos = SPAWN_POS();
        vel = sf::Vector2f(0.f, 0.f);
        state = CharacterState::None;
        moving = CharacterMoving::Idle;
        timer = 0.f;
        // (Optionally reset animation state)
    }

    timer += dt;
}

sf::Sprite Characters::getCharacterSprite(const std::string& characterKey) {
    Log::info << "Getting character sprite for key: " << characterKey << "\n";
    auto info = Animations::get(characterKey + Anim::Idle);
    sf::Sprite s(Textures::get(info.textureKey));
    s.setTextureRect(sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(128, 128)));
    s.scale(Scale::getVec());
    return s;
}

void Characters::setPlayerCharacter(const std::string& characterKey) {
    Player = characterKey;
}