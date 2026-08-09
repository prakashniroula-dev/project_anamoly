// #include <entities/npc.hpp>
// #include <debug/logs.hpp>
// #include <graphics/tiles.hpp>
// #include <entities/terrain.hpp>

// void NPC::patrolTo(sf::Vector2f targetPos) {
//   patrolTarget = targetPos;
//   // Additional logic for patrolling can be added here
// }

// void NPC::update(sf::RenderWindow& win, float dt) {
  
//   float distanceToTarget = std::abs(patrolTarget.x - pos.x);
//   float distanceYToTarget = std::abs(patrolTarget.y - pos.y);
//   if ( distanceToTarget > Constants::TILE_SIZE * 2 ) {
//     patrolling = true;
//   }
  
//   // Simple patrol logic: move towards patrolTarget
//   if ( patrolling && distanceToTarget > 3.f) { // small threshold to avoid jitter
//     int direction = patrolTarget.x > pos.x ? 1 : -1;
//     sf::FloatRect charBox = getBounds();
//     auto [tileX, tileY] = Tiles::getTileGridPosition(charBox.position + charBox.size); // Bottom-right corner of the character's bounding box
//     tileX += direction * 0.01f;
//     bool isSolidTileAhead = (
//       Terrain::isSolidTile(tileX, tileY + 1) ||
//       Terrain::isSolidTile(tileX, tileY + 2)
//     );
//     bool inDanger = (
//       !Terrain::isSolidTile(tileX, tileY) &&
//       !Terrain::isSolidTile(tileX, tileY - 1) &&
//       !Terrain::isSolidTile(tileX, tileY - 2) &&
//       !Terrain::isSolidTile(tileX, tileY - 3) &&
//       !Terrain::isSolidTile(tileX + direction, tileY) &&
//       !Terrain::isSolidTile(tileX + direction, tileY - 1) &&
//       !Terrain::isSolidTile(tileX + direction, tileY - 2) &&
//       !Terrain::isSolidTile(tileX + direction, tileY - 3)
//     );
//     float nearestSolidTileInPath = 0.f;
//     for (int i = 1; i <= 5; ++i) {
//       bool isSolid = (
//         Terrain::isSolidTile(tileX, tileY) ||
//         Terrain::isSolidTile(tileX + direction * i, tileY) ||
//         Terrain::isSolidTile(tileX + direction * i, tileY + 1) ||
//         Terrain::isSolidTile(tileX + direction * i, tileY + 2) ||
//         Terrain::isSolidTile(tileX + direction * i, tileY - 1) ||
//         Terrain::isSolidTile(tileX + direction * i, tileY - 2)
//       );
//       if (isSolid) {
//         nearestSolidTileInPath = i;
//         break;
//       }
//     }
//     if ( nearestSolidTileInPath == 0.f ) {
//       patrolling = false;
//       idle();
//     }
    
//     if ( nearestSolidTileInPath > 2 || (moving == CharacterMoving::Running && !onGround())) {
//       run();
//     } else {
//       walk(direction);
//     }
    
//     if (((isSolidTileAhead && onGround()) || inDanger) ) {
//       jump();
//     }

//   } else if ( patrolling ) {
//     pos.x = patrolTarget.x; // Snap to target position
//     patrolling = false;
//     idle();
//   }
  
//   Character::update(win, dt); // Call base class update for physics and animation
// }

#include "npc.hpp"
#include "npc_manager.hpp"
#include <ui/ui_manager.hpp>
#include <ui/dialog_screen.hpp>
#include <core/scale.hpp>
#include <debug/logs.hpp>
#include <cmath>
#include <ui/ui_screen.hpp>

NPC::NPC(const NPCType& type, const sf::Vector2f& spawnPos, const std::string& uniqueID)
    : Character(type.characterKey, false), type(type), uniqueID(uniqueID) {
    pos = spawnPos;
    if (type.behaviorType == "patrol") {
        behaviorState.type = BehaviorState::Type::Patrol;
        if (!type.waypoints.empty()) {
            behaviorState.targetPos = type.waypoints[0];
        }
    }
}

void NPC::update(sf::RenderWindow& win, float dt) {
    Character::update(win, dt); // base physics & animation
    updateBehavior(dt);
    if (updateCB) updateCB(*this, dt);
}

void NPC::talk() {
    if (!UIManager::get().isEmpty()) return;
    if (talkStartCB) talkStartCB(*this);
    auto& ui = UIManager::get();
    auto screen = std::make_unique<DialogScreen>(this);
    ui.pushScreen(std::move(screen));
}

void NPC::updateBehavior(float dt) {
    switch (behaviorState.type) {
        case BehaviorState::Type::Patrol: patrolUpdate(dt); break;
        case BehaviorState::Type::Idle:   idleUpdate(dt); break;
        case BehaviorState::Type::Follow: followUpdate(dt); break;
        case BehaviorState::Type::Scripted: scriptedUpdate(dt); break;
        default: break;
    }
}

void NPC::patrolUpdate(float dt) {
    if (type.waypoints.empty()) return;
    sf::Vector2f pos = getPosition();
    sf::Vector2f target = behaviorState.targetPos * Scale::get();
    sf::Vector2f diff = target - pos;
    float len = std::sqrt(diff.x*diff.x + diff.y*diff.y);
    if (len < 5.f) {
        nextWaypoint = (nextWaypoint + 1) % type.waypoints.size();
        behaviorState.targetPos = type.waypoints[nextWaypoint];
    } else {
        // Choose 4-directional movement (match your Character::walk direction)
        float ax = std::abs(diff.x);
        float ay = std::abs(diff.y);
        if (ax > ay) {
            walk(diff.x > 0 ? 1 : 3); // 1=right, 3=left
        } else {
            walk(diff.y > 0 ? 2 : 0); // 2=down, 0=up (adjust to your enum)
        }
    }
}

void NPC::idleUpdate(float dt) { /* do nothing */ }
void NPC::followUpdate(float dt) { /* implement later */ }
void NPC::scriptedUpdate(float dt) { /* implement later */ }