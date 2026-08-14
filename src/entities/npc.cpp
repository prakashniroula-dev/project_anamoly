#include "npc.hpp"
#include "npc_manager.hpp"
#include <ui/ui_manager.hpp>
#include <ui/dialog_screen.hpp>
#include <core/scale.hpp>
#include <debug/logs.hpp>
#include <cmath>
#include <ui/ui_screen.hpp>
#include <entities/player.hpp>

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
    if (m_scriptRunning) {
        // Handle scripted movement (MoveTo)
        if (behaviorState.type == BehaviorState::Type::Scripted) {
            sf::Vector2f currentPos = getPosition();
            sf::Vector2f target = behaviorState.targetPos * Scale::get();
            sf::Vector2f diff = target - currentPos;
            float distance = std::hypot(diff.x, diff.y);
            if (distance < 5.f) {
                // Reached target, advance script
                advanceScript();
            } else {
                // Walk towards target
                int dir = (diff.x > 0) ? 1 : -1;
                walk(dir);
                // Optional: jump if needed?
            }
        }
        // Handle Wait timer
        if (m_actionTimer > 0.f) {
            m_actionTimer -= dt;
            if (m_actionTimer <= 0.f) {
                advanceScript();
            }
        }
    } else {
        if (!m_aiPaused) {
            updateBehavior(dt);
            if (updateCB) updateCB(*this, dt);
        }
    }
    sf::Vector2f target = behaviorState.targetPos;
    //draw a big circle around the target position for debugging
    sf::CircleShape circle(10.f);
    circle.setFillColor(sf::Color(255, 0, 0, 100));
    circle.setPosition(target - sf::Vector2f(10.f, 10.f));
    circle.setOrigin(sf::Vector2f(10.f, 10.f));
    win.draw(circle);
    Log::info << "Target: " << target.x << ", " << target.y << " | Position: " << pos.x << ", " << pos.y << std::endl;
}

void NPC::runSequence(const std::vector<Action>& actions) {
    m_script = actions;
    m_waiting = false;
    for (auto& action : m_script) {
        if (action.type == ActionType::ShowDialogue || action.type == ActionType::SwapPlayer) {
            action.npc = this; // set the NPC pointer
        }
    }
    m_script = actions;
    m_currentActionIndex = 0;
    m_actionTimer = 0.f;
    m_scriptRunning = true;
    // Pause AI while script runs
    pauseAI(true);
    executeCurrentAction();
}

void NPC::executeCurrentAction() {
    if (m_currentActionIndex >= m_script.size()) {
        // script finished
        m_scriptRunning = false;
        pauseAI(false);
        return;
    }

    const Action& action = m_script[m_currentActionIndex];
    switch (action.type) {
        case ActionType::MoveTo:
            // Set target for movement (will be handled in update)
            behaviorState.type = BehaviorState::Type::Scripted;
            behaviorState.targetPos = action.targetPos;
            break;

        case ActionType::Wait:
            m_actionTimer = action.duration;
            break;

        case ActionType::LockPlayer:
            Player::get().lockControls();
            advanceScript();
            break;

        case ActionType::UnlockPlayer:
            Player::get().unlockControls();
            advanceScript();
            break;

        case ActionType::PlayAnimation:
            // Set a scripted animation (we'll need a new state)
            // For simplicity, we can override the moving/state enums.
            // We'll add a new member m_scriptedAnim and use it in animate().
            m_scriptedAnim = action.animKey;
            advanceScript();
            break;

        case ActionType::ShowDialogue:
            // Push dialog screen (assume we have a way to get NPC pointer)
            // The action should have the dialogueId, but we need to know which NPC.
            // We can pass the NPC pointer to the screen.
            // We'll implement a helper in NPCManager to push dialog for this NPC.
            if (action.npc) {
                UIManager::get().pushScreen(std::make_unique<DialogScreen>(action.npc));
            } else {
                // fallback: use this NPC
                UIManager::get().pushScreen(std::make_unique<DialogScreen>(this));
            }
            advanceScript();
            break;

        case ActionType::SwapPlayer:
            if (action.npc) {
                Player::get().swapTo(action.npc);
            } else {
                // If no NPC specified, swap back?
                Player::get().swapBack();
            }
            advanceScript();
            break;

        case ActionType::CallFunction:
            // Look up function in a registry (we can store std::function in a map)
            // For simplicity, we'll just log.
            Log::info << "CallFunction: " << action.functionName << std::endl;
            // Could call a global function registry.
            advanceScript();
            break;

        case ActionType::EndSequence:
            m_scriptRunning = false;
            pauseAI(false);
            // Optionally switch back to patrol/idle
            behaviorState.type = BehaviorState::Type::Idle;
            break;
    }
}

void NPC::advanceScript() {
    ++m_currentActionIndex;
    executeCurrentAction();
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
    const auto& wps = getWaypoints();
    if (wps.empty()) {
        behaviorState.type = BehaviorState::Type::Idle;
        return;
    }

    // ---- 1. Compute target and character centre in UNSCALED coordinates ----
    sf::Vector2f target = behaviorState.targetPos;
    sf::Vector2f center = pos + sf::Vector2f(
        Character::OFFSET_X + Character::BASE_WIDTH / 2.f,
        Character::OFFSET_Y + Character::BASE_HEIGHT / 2.f
    );

    sf::Vector2f diff = target - center;
    float distance = std::abs(diff.x);

    // ---- 2. Reached waypoint? ----
    if (distance < 5.f) {
        // Start waiting before moving to next waypoint
        m_waiting = true;
        m_waitTimer = m_waitTime;
        m_nextWaypointAfterWait = (nextWaypoint + 1) % wps.size(); // next index

        // Stop moving and switch to idle behavior
        behaviorState.type = BehaviorState::Type::Idle;
        moving = CharacterMoving::Idle;
        vel.x = 0.f;

        Log::info << "Reached waypoint, waiting " << m_waitTime << "s before moving to index " << m_nextWaypointAfterWait << std::endl;
        return;
    }

    // ---- 3. Direction (now correct) ----
    int dir = (diff.x > 0) ? 1 : -1;

    // ---- 4. Obstacle / danger checks (using scaled coordinates, as before) ----
    float s = Scale::get();
    sf::FloatRect charBox = getBounds();                 // scaled
    sf::Vector2f bottomRight = charBox.position + charBox.size;
    sf::Vector2i tilePos = Tiles::getTileGridPosition(bottomRight);
    tilePos.x += dir * 0.01f;

    bool blockAhead = (Terrain::isSolidTile(tilePos.x + dir, tilePos.y) ||
                       Terrain::isSolidTile(tilePos.x + dir, tilePos.y + 1));

    bool inDanger = false;
    if (!Terrain::isSolidTile(tilePos.x, tilePos.y) &&
        !Terrain::isSolidTile(tilePos.x, tilePos.y - 1) &&
        !Terrain::isSolidTile(tilePos.x, tilePos.y - 2) &&
        !Terrain::isSolidTile(tilePos.x, tilePos.y - 3) &&
        !Terrain::isSolidTile(tilePos.x + dir, tilePos.y) &&
        !Terrain::isSolidTile(tilePos.x + dir, tilePos.y - 1) &&
        !Terrain::isSolidTile(tilePos.x + dir, tilePos.y - 2) &&
        !Terrain::isSolidTile(tilePos.x + dir, tilePos.y - 3)) {
        inDanger = true;
    }

    // ---- 5. Blocked / danger handling ----
    if (inDanger) {
        m_blockedTimer += dt;
        if (m_blockedTimer > 0.3f && !m_directionReversed) {
            int prevIndex = (nextWaypoint == 0) ? wps.size() - 1 : nextWaypoint - 1;
            behaviorState.targetPos = wps[prevIndex];
            m_directionReversed = true;
            m_blockedTimer = 0.f;
            nextWaypoint = prevIndex;
            return;
        }

        if (moving == CharacterMoving::Idle) {
            walk(dir);
        }

        if (onGround()) {
            if (blockAhead && Terrain::isSolidTile(tilePos.x + dir, tilePos.y + 1)) {
                jump();
            } else if (inDanger) {
                jump();
            }
        }
        return;
    }

    // ---- 6. Normal movement ----
    m_blockedTimer = 0.f;
    m_directionReversed = false;

    if (distance > 100.f && moving == CharacterMoving::Idle) {
        walk(dir);
    }
}

void NPC::idleUpdate(float dt) {
    if (m_waiting) {
        m_waitTimer -= dt;
        if (m_waitTimer <= 0.f) {
            // Waiting finished: move to the next waypoint
            m_waiting = false;
            nextWaypoint = m_nextWaypointAfterWait;
            const auto& wps = getWaypoints();
            if (!wps.empty()) {
                behaviorState.targetPos = wps[nextWaypoint];
                behaviorState.type = BehaviorState::Type::Patrol;
                moving = CharacterMoving::Idle;   // allow new direction on next frame
                vel.x = 0.f;
                Log::info << "Resuming patrol to waypoint " << nextWaypoint << " at (" 
                          << wps[nextWaypoint].x << ", " << wps[nextWaypoint].y << ")" << std::endl;
            } else {
                behaviorState.type = BehaviorState::Type::Idle; // no waypoints, stay idle
            }
        }
    }
    // If not waiting, do nothing (or you can add other idle behaviours)
}

void NPC::followUpdate(float dt) { /* implement later */ }
void NPC::scriptedUpdate(float dt) { /* implement later */ }