#include <entities/npc.hpp>
#include <debug/logs.hpp>
#include <graphics/tiles.hpp>
#include <entities/terrain.hpp>

void NPC::patrolTo(sf::Vector2f targetPos) {
  patrolTarget = targetPos;
  // Additional logic for patrolling can be added here
}

void NPC::update(sf::RenderWindow& win, float dt) {
  
  float distanceToTarget = std::abs(patrolTarget.x - pos.x);
  float distanceYToTarget = std::abs(patrolTarget.y - pos.y);
  if ( distanceToTarget > Constants::TILE_SIZE * 2 ) {
    patrolling = true;
  }
  
  // Simple patrol logic: move towards patrolTarget
  if ( patrolling && distanceToTarget > 3.f) { // small threshold to avoid jitter
    int direction = patrolTarget.x > pos.x ? 1 : -1;
    sf::FloatRect charBox = getBounds();
    auto [tileX, tileY] = Tiles::getTileGridPosition(charBox.position + charBox.size); // Bottom-right corner of the character's bounding box
    tileX += direction * 0.01f;
    bool isSolidTileAhead = (
      Terrain::isSolidTile(tileX, tileY + 1) ||
      Terrain::isSolidTile(tileX, tileY + 2)
    );
    bool inDanger = (
      !Terrain::isSolidTile(tileX, tileY) &&
      !Terrain::isSolidTile(tileX, tileY - 1) &&
      !Terrain::isSolidTile(tileX, tileY - 2) &&
      !Terrain::isSolidTile(tileX, tileY - 3) &&
      !Terrain::isSolidTile(tileX + direction, tileY) &&
      !Terrain::isSolidTile(tileX + direction, tileY - 1) &&
      !Terrain::isSolidTile(tileX + direction, tileY - 2) &&
      !Terrain::isSolidTile(tileX + direction, tileY - 3)
    );
    float nearestSolidTileInPath = 0.f;
    for (int i = 1; i <= 5; ++i) {
      bool isSolid = (
        Terrain::isSolidTile(tileX, tileY) ||
        Terrain::isSolidTile(tileX + direction * i, tileY) ||
        Terrain::isSolidTile(tileX + direction * i, tileY + 1) ||
        Terrain::isSolidTile(tileX + direction * i, tileY + 2) ||
        Terrain::isSolidTile(tileX + direction * i, tileY - 1) ||
        Terrain::isSolidTile(tileX + direction * i, tileY - 2)
      );
      if (isSolid) {
        nearestSolidTileInPath = i;
        break;
      }
    }
    if ( nearestSolidTileInPath == 0.f ) {
      patrolling = false;
      idle();
    }
    
    if ( nearestSolidTileInPath > 2 || (moving == CharacterMoving::Running && !onGround())) {
      run();
    } else {
      walk(direction);
    }
    
    if (((isSolidTileAhead && onGround()) || inDanger) ) {
      jump();
    }

  } else if ( patrolling ) {
    pos.x = patrolTarget.x; // Snap to target position
    patrolling = false;
    idle();
  }
  
  Character::update(win, dt); // Call base class update for physics and animation
}