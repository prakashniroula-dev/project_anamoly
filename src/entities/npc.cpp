#include "npc.hpp"
#include "npc_manager.hpp"
#include <ui/ui_manager.hpp>
#include <ui/dialog_screen.hpp>
#include <core/scale.hpp>
#include <debug/logs.hpp>
#include <cmath>
#include <ui/ui_screen.hpp>
#include <entities/player.hpp>
#include <story/story_manager.hpp>
#include <story/story_helpers.hpp>

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
    if (!isAlive()) return;
    if (!m_cutsceneTriggered && !m_scriptRunning && !type.cutsceneScriptName.empty()) {
        Character* player = Player::get().getPlayer();
        if (player) {
            sf::Vector2f diff = player->getPosition() - getPosition();
            float dist = std::hypot(diff.x, diff.y);
            if (dist <= type.cutsceneRadius * Scale::get()) {
                auto it = ScriptRegistry::scripts.find(type.cutsceneScriptName);
                if (it != ScriptRegistry::scripts.end()) {
                    m_sequenceTruthful = runSequence(it->second);
                    m_cutsceneTriggered = true;
                } else {
                    Log::warn << "NPC cutscene script not found: " << type.cutsceneScriptName << "\n";
                }
            }
        }
    }
    if (m_scriptRunning) {
        if (m_actionTimer > 0.f) {
            m_actionTimer -= dt;
            if (m_actionTimer <= 0.f) {
                advanceScript();
            }
            return; // wait for timer to finish
        }
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
    } else {
        if (!m_aiPaused) {
            updateBehavior(dt);
            if (updateCB) updateCB(*this, dt);
        }
    }
}

bool NPC::runSequence(const std::vector<Action::Action>& actions) {
    // Save current AI state before script takes over
    m_originalBehaviorType = behaviorState.type;
    m_originalWaypointIndex = nextWaypoint;
    m_script = actions;
    m_waiting = false;
    // for (auto& action : m_script) {
    //     if (action.type == ActionType::ShowDialogue || action.type == ActionType::SwapPlayer) {
    //         action.npc = this;
    //     }
    // }
    m_currentActionIndex = 0;
    m_actionTimer = 0.f;
    m_scriptRunning = true;
    m_sequenceTruthful = true; // assume sequence is truthful
    flag_conditions = true; // reset flag conditions
    pauseAI(true);
    executeCurrentAction();
    bool result = m_sequenceTruthful;
    Log::info << "Action sequence started for NPC " << uniqueID << " with " << m_script.size() << " actions.\n";
    return result;
}

void NPC::restoreAIState() {
    behaviorState.type = m_originalBehaviorType;
    // For patrol, reset to the original waypoint index
    if (behaviorState.type == BehaviorState::Type::Patrol) {
        const auto& wps = getWaypoints();
        if (!wps.empty()) {
            nextWaypoint = m_originalWaypointIndex % wps.size();
            behaviorState.targetPos = wps[nextWaypoint];
        }
    }
    // For follow or idle, you may need additional restoration logic
    m_scriptRunning = false;
    pauseAI(false);
    idle();          // stop any scripted movement
}

void NPC::executeCurrentAction() {
    if (m_currentActionIndex >= m_script.size()) {
        m_scriptRunning = false;
        pauseAI(false);
        restoreAIState();
        return;
    }

    Log::info << "Try to execute action " << m_currentActionIndex << " of " << m_script.size() << std::endl;
    const Action::Action& action = m_script[m_currentActionIndex];
    Log::info << "Executing action " << m_currentActionIndex << ": type=" << static_cast<int>(action.type) << std::endl;

    // ---- Handle flag-related actions unconditionally ----
    if (action.type == ActionType::EvaluateState) {
        std::string evalName = std::get<std::string>(action.param);
        bool result = StoryHelpers::evaluateCondition(evalName);
        flag_conditions = result;
        Log::info << "EvaluateState: " << evalName << " => " << (result ? "true" : "false") << std::endl;
        advanceScript();
        return;
    }

    // ---- Handle regular actions only if flag conditions allow ----
    if (!flag_conditions) {
        Log::warn << "Action skipped due to failed flag conditions: type="
                  << static_cast<int>(action.type) << std::endl;
        advanceScript();
        return;
    }

     // Now that we are executing an action, set sequenceTruthful to true
    // Now flag_conditions is true; execute the action
    switch (action.type) {
        case ActionType::ExecuteState:
            StoryHelpers::executeAction(std::get<std::string>(action.param));
            Log::info << "ExecuteState: " << std::get<std::string>(action.param) << std::endl;
            advanceScript();
            break;
        case ActionType::Truthful:
            m_sequenceTruthful = true;
            Log::info << "Sequence marked as truthful.\n";
            advanceScript();
            break;
        case ActionType::Falseful:
            m_sequenceTruthful = false;
            Log::info << "Sequence marked as falseful.\n";
            advanceScript();
            break;
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
            behaviorState.targetPos = std::get<sf::Vector2f>(action.param);
            m_actionTimer = -1.f;   // no wait timer
            break;

        case ActionType::MoveRelative:
            behaviorState.type = BehaviorState::Type::Scripted;
            behaviorState.targetPos = getPosition() / Scale::get() + std::get<sf::Vector2f>(action.param);
            m_actionTimer = -1.f;
            break;

        case ActionType::MoveTowardsPlayer: {
            Character* player = Player::get().getPlayer();
            if (!player) {
                Log::warn << "MoveTowardsPlayer: Player pointer is null. Skipping.\n";
                advanceScript();
                break;
            }
            sf::Vector2f playerPos = player->getPosition() / Scale::get();
            sf::Vector2f currentPos = getPosition() / Scale::get();
            sf::Vector2f diff = playerPos - currentPos;
            int direction = diff.x < 0 ? -1 : 1;
            float amt = std::get<float>(action.param);
            if ( amt < 0.f ) {
                amt = std::abs(diff.x) - 50.f;
                amt = std::max(amt, 0.f); // ensure non-negative
            }
            behaviorState.type = BehaviorState::Type::Scripted;
            behaviorState.targetPos = currentPos + sf::Vector2f(direction * amt, 0.f);
            m_actionTimer = -1.f;
            break;
        }
        case ActionType::Wait:
            m_actionTimer = std::get<float>(action.param);
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
            m_scriptedAnim = std::get<std::string>(action.param);
            advanceScript();
            break;

        case ActionType::ShowDialogue: {
            std::string dialogueId = std::get<std::string>(action.param);
            bool allowEscape = std::get<bool>(action.param2);
            UIManager::get().pushScreen(std::make_unique<DialogScreen>(this, dialogueId, allowEscape));
            m_waitingForDialogue = true;   // block script until dialogue closes
            break;
        }

        case ActionType::SwapPlayer:
            if (std::get<NPC*>(action.param)) {
                Player::get().swapTo(std::get<NPC*>(action.param));
            } else {
                Player::get().swapBack();
            }
            advanceScript();
            break;
        case ActionType::CallFunction: {
            std::string funcName = std::get<std::string>(action.param);
            Log::info << "Calling function: " << funcName << std::endl;
            auto it = FunctionRegistry::functions.find(funcName);
            if (it != FunctionRegistry::functions.end()) {
                it->second(this);
            } else {
                Log::warn << "Unknown function: " << funcName << std::endl;
            }
            advanceScript();
            break;
        }
        case ActionType::EndSequence:
            m_scriptRunning = false;
            Log::info << "Setting sequnce complete to true!";
            pauseAI(false);
            restoreAIState();
            // No advanceScript – script ends here
            break;
        default:
            Log::warn << "Unknown action type: " << static_cast<int>(action.type) << std::endl;
            advanceScript();
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
            m_waiting = false;
            nextWaypoint = m_nextWaypointAfterWait;
            const auto& wps = getWaypoints();
            if (!wps.empty()) {
                behaviorState.targetPos = wps[nextWaypoint];
                behaviorState.type = BehaviorState::Type::Patrol;
                moving = CharacterMoving::Idle;
                vel.x = 0.f;
                Log::info << "Resuming patrol to waypoint " << nextWaypoint << std::endl;
            } else {
                behaviorState.type = BehaviorState::Type::Idle;
            }
        }
    } else if (behaviorState.type == BehaviorState::Type::Idle) {
        // If we are idle but not waiting, and we have waypoints, resume patrol
        const auto& wps = getWaypoints();
        if (!wps.empty()) {
            // Ensure nextWaypoint is valid
            if (nextWaypoint >= wps.size()) nextWaypoint = 0;
            behaviorState.targetPos = wps[nextWaypoint];
            behaviorState.type = BehaviorState::Type::Patrol;
            moving = CharacterMoving::Idle;
            vel.x = 0.f;
            Log::info << "Resuming patrol from idle state to waypoint " << nextWaypoint << std::endl;
        }
    }
}

void NPC::followUpdate(float dt) { /* implement later */ }
void NPC::scriptedUpdate(float dt) { /* implement later */ }