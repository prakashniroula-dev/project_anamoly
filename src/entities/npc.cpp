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

void NPC::dialogueEnded() {
    Log::info << "NPC dialogue ended for NPC" << "\n";
    if (m_waitingForDialogue) {
        m_waitingForDialogue = false;
        advanceScript();   // continue script execution
    }
}

void NPC::update(sf::RenderWindow& win, float dt) {
    // Log::info << "NPC update: state=" << static_cast<int>(state) << ", scriptRunning=" << m_scriptRunning << "\n";
    Character::update(win, dt); // base physics & animation
    if (m_scriptRunning) {
        if (m_waitingForDialogue) {
            return;   // wait for dialogue to end
        }
        // Handle scripted movement (MoveTo)
        Log::info << "NPC update: scripted movement state=" << static_cast<int>(behaviorState.type) << "\n";
        if (behaviorState.type == BehaviorState::Type::Scripted) {
            Log::info << "NPC scripted movement: targetPos=(" << behaviorState.targetPos.x << ", " << behaviorState.targetPos.y << ")\n";
            sf::Vector2f currentPos = getPosition();
            sf::Vector2f target = behaviorState.targetPos * Scale::get();
            sf::Vector2f diff = target - currentPos;
            float distance = std::abs(diff.x);

            if (distance < 5.f) {
                // Snap to target (unscaled)
                pos = behaviorState.targetPos;
                behaviorState.type = BehaviorState::Type::Idle;
                idle();
                advanceScript();
            } else {
                // Walk horizontally towards target
                int dir = (diff.x > 0) ? 1 : -1;
                walk(dir);
                // Optional: add vertical movement if needed (e.g., jump)
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
}

void NPC::runSequence(const std::vector<Action>& actions) {
    m_script = actions;
    m_waiting = false;
    for (auto& action : m_script) {
        if (action.type == ActionType::ShowDialogue || action.type == ActionType::SwapPlayer) {
            action.npc = this; // set the NPC pointer
        }
    }
    m_currentActionIndex = 0;
    m_actionTimer = 0.f;
    m_scriptRunning = true;
    // Pause AI while script runs
    pauseAI(true);
    executeCurrentAction();
    Log::info << "Action sequence done for NPC " << uniqueID << " with " << m_script.size() << " actions.\n";    
}

void NPC::executeCurrentAction() {
    if (m_currentActionIndex >= m_script.size()) {
        Log::info << "NPC script finished." << std::endl;
        // script finished
        m_scriptRunning = false;
        pauseAI(false);
        return;
    }
    Log::info << "Try to execute action " << m_currentActionIndex << " of " << m_script.size() << std::endl;
    const Action& action = m_script[m_currentActionIndex];
    Log::info << "Executing action " << m_currentActionIndex << ": type=" << static_cast<int>(action.type) << std::endl;
    switch (action.type) {
        case ActionType::FacePlayer: {
            Character* player = Player::get().getPlayer();
            if (!player) {
                Log::warn << "FacePlayer: Player pointer is null. Skipping.\n";
                advanceScript();
                break;
            }
            int dir = (player->getPosition().x > getPosition().x) ? 1 : -1;
            direction = dir;
            advanceScript();
            break;
        }
        case ActionType::MoveTo:
            behaviorState.type = BehaviorState::Type::Scripted;
            behaviorState.targetPos = action.targetPos;
            m_actionTimer = -1.f;   // no wait timer
            break;

        case ActionType::MoveRelative:
            behaviorState.type = BehaviorState::Type::Scripted;
            behaviorState.targetPos = getPosition() / Scale::get() + action.targetPos;
            m_actionTimer = -1.f;   // no wait timer
            break;

        case ActionType::Wait:
            m_actionTimer = action.duration;
            Log::info << "Waiting for " << m_actionTimer << " seconds.\n";
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
            if (action.npc) {
                UIManager::get().pushScreen(std::make_unique<DialogScreen>(action.npc));
            } else {
                UIManager::get().pushScreen(std::make_unique<DialogScreen>(this));
            }
            m_waitingForDialogue = true;   // block script until dialogue closes
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

        case ActionType::CallFunction: {
            Log::info << "Calling function: " << action.functionName << std::endl;
            auto it = FunctionRegistry::functions.find(action.functionName);
            if (it != FunctionRegistry::functions.end()) {
                it->second(this);
            } else {
                Log::warn << "Unknown function: " << action.functionName << std::endl;
            }
            advanceScript();
            break;
        }

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


    // prevent jump to prevent unexpected issues, implement jumping characters in script or smth
    // maybe updateCB, where it checks for position and jump but only if needed

    // bool blockAhead = (Terrain::isSolidTile(tilePos.x + dir, tilePos.y) ||
    //                    Terrain::isSolidTile(tilePos.x + dir, tilePos.y + 1));

    // bool inDanger = false;
    // if (!Terrain::isSolidTile(tilePos.x, tilePos.y) &&
    //     !Terrain::isSolidTile(tilePos.x, tilePos.y - 1) &&
    //     !Terrain::isSolidTile(tilePos.x, tilePos.y - 2) &&
    //     !Terrain::isSolidTile(tilePos.x, tilePos.y - 3) &&
    //     !Terrain::isSolidTile(tilePos.x + dir, tilePos.y) &&
    //     !Terrain::isSolidTile(tilePos.x + dir, tilePos.y - 1) &&
    //     !Terrain::isSolidTile(tilePos.x + dir, tilePos.y - 2) &&
    //     !Terrain::isSolidTile(tilePos.x + dir, tilePos.y - 3)) {
    //     inDanger = true;
    // }

    // ---- 5. Blocked / danger handling ----
    // if (inDanger) {
    //     m_blockedTimer += dt;
    //     if (m_blockedTimer > 0.3f && !m_directionReversed) {
    //         int prevIndex = (nextWaypoint == 0) ? wps.size() - 1 : nextWaypoint - 1;
    //         behaviorState.targetPos = wps[prevIndex];
    //         m_directionReversed = true;
    //         m_blockedTimer = 0.f;
    //         nextWaypoint = prevIndex;
    //         return;
    //     }

    //     if (moving == CharacterMoving::Idle) {
    //         walk(dir);
    //     }

    //     if (onGround()) {
    //         if (blockAhead && Terrain::isSolidTile(tilePos.x + dir, tilePos.y + 1)) {
    //             jump();
    //         } else if (inDanger) {
    //             jump();
    //         }
    //     }
    //     return;
    // }

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