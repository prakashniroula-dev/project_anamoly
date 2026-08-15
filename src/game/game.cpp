#include <game/Game.hpp>
#include <game/fps_display.hpp>
#include <game/infobox.hpp>
#include <debug/logs.hpp>
#include <map/terrain.hpp>
#include <entities/characters.hpp>
#include <entities/player.hpp>
#include <entities/npc_manager.hpp>
#include <graphics/background.hpp>
#include <graphics/overlay.hpp>
#include <graphics/tiles.hpp>
#include <graphics/textures.hpp>
#include <editor/level_editor.hpp>
#include <ui/ui_manager.hpp>
#include <ui/main_menu.hpp>
#include <core/scale.hpp>
#include <core/constants.hpp>
#include <story/script_registry.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <map/map_manager.hpp>
#include <optional>

namespace Game
{

    Game::Game(const std::string &title, unsigned int width, unsigned int height, const std::string &initialMap = "default")
        : m_width(width), m_height(height), m_title(title), m_initialMap(initialMap)
    {
        // Create window
        m_window.create(sf::VideoMode({width, height}), title);
        m_window.setVerticalSyncEnabled(false);
        m_window.setFramerateLimit(240);

        // Setup view
        m_view.setSize(sf::Vector2f(width, height));
        m_view.setViewport(sf::FloatRect(sf::Vector2f(0.f, 0.f), sf::Vector2f(1.f, 1.f)));
        m_view.setCenter(sf::Vector2f(width / 2.f, height / 2.f));

        // Camera parameters
        m_viewY = height / 2.f;
        m_targetY = m_viewY;
        m_smoothSpeed = Character::GRAVITY * 0.7f;
        m_topDeadZone = 80.f;
        m_bottomDeadZone = 200.f;

        // Create FPS display
        m_fpsDisplay = new FpsDisplay();

        // Create game objects
        m_background = new Background();
        m_overlay = new Overlay();
        m_editor = new LevelEditor();

        // Initialize resources
        initResources();
    }

    Game::~Game()
    {
        delete m_fpsDisplay;
        delete m_background;
        delete m_overlay;
        delete m_editor;
    }

    void Game::initResources()
    {
        updateScale();
        Tiles::load();
        Objects::load();
        Background::load(m_window);
        Overlay::load(m_window);

        MapManager::get().setGame(this);
        MapManager::get().loadMap(m_initialMap);
        // Terrain::loadSpawnsFromFile("assets/spawns.txt");
        // Terrain::loadFromFile("assets/map.txt");
        // Terrain::loadObjectsFromFile("assets/objects.txt");
        // Terrain::loadSolidFromFile("assets/solid_tiles.txt");

        Characters::load();
        Player::get().init();
        UIManager::get().init();
        NPCManager::get().loadDefinitions(); // register NPC types
        ScriptRegistry::init();              // register scripts
        m_editor->init();
        Player::get().setCharacter(Characters::Fighter_Detective);
        NPCManager::get().setPlayer(Player::get().getPlayer());
        // Push the main menu – spawning will happen after it's closed
        UIManager::get().pushScreen(std::make_unique<MainMenu>(this));
        MapManager::get().setPlayer(Player::get().getPlayer());
    }

    void Game::snapCameraToPlayer()
    {
        Character *player = Player::get().getPlayer();
        if (!player)
            return;

        sf::Vector2f pos = player->getPosition();
        float halfWidth = m_width / 2.f;
        float viewX = std::max(pos.x, halfWidth);

        sf::FloatRect bounds = player->getBounds();
        float charCentreY = bounds.position.y + bounds.size.y * 0.5f;

        // Clamp to world bounds
        float minY = m_height / 2.f;
        float maxY = Constants::WORLD_HEIGHT_PIXELS * Scale::get() - m_height / 2.f;
        float targetY = std::clamp(charCentreY, minY, maxY);

        m_viewY = targetY;
        m_targetY = targetY;
        m_view.setCenter(sf::Vector2f(viewX, m_viewY));
    }

    void Game::startGame()
    {
        if (m_gameStarted)
            return;
        m_gameStarted = true;
        // Spawn NPCs and run their scripts (now that the menu is gone)
        // NPCManager::spawnAllNPCs();
        // Spawn NPCs NOW – after player is ready
        MapManager::get().spawnNPCs();
        Player::get().getPlayer()->snapToGround();
        for (NPC *npc : NPCManager::get().getAllNPCs())
        {
            npc->snapToGround();
        }
        snapCameraToPlayer();
        Log::info << "Game started, NPCs spawned.\n";
    }

    void Game::updateScale()
    {
        float scale = static_cast<float>(m_window.getSize().y) / 320.f;
        Scale::set(scale);
    }

    void Game::updateCamera(float dt)
    {
        Player &player = Player::get();
        float halfWidth = m_width / 2.f;
        float viewX = std::max(player.getPlayer()->getPosition().x, halfWidth);

        sf::FloatRect bounds = player.getPlayer()->getBounds();
        float charCentreY = bounds.position.y + bounds.size.y * 0.5f;
        float screenY = charCentreY - m_viewY + (m_height / 2.f);

        float topBoundary = m_topDeadZone;
        float bottomBoundary = m_height - m_bottomDeadZone;

        if (screenY < topBoundary)
        {
            m_targetY = charCentreY;
        }
        else if (screenY > bottomBoundary)
        {
            m_targetY = charCentreY;
        }

        if (m_targetY + (m_height / 2.0f) > Constants::WORLD_HEIGHT_PIXELS * Scale::get())
        {
            m_targetY = Constants::WORLD_HEIGHT_PIXELS * Scale::get() - (m_height / 2.0f);
        }
        if (m_targetY - (m_height / 2.0f) < 0.f)
        {
            m_targetY = m_height / 2.0f;
        }

        float diff = m_targetY - m_viewY;
        float maxStep = m_smoothSpeed * dt;
        if (std::abs(diff) < maxStep)
        {
            m_viewY = m_targetY;
        }
        else
        {
            m_viewY += (diff > 0 ? 1.f : -1.f) * maxStep;
        }

        m_view.setCenter(sf::Vector2f(viewX, m_viewY));
    }

    void Game::update(float dt)
    {
        Player &player = Player::get();
        const float MAX_DT = 1.f / 30.f;
        if (dt > MAX_DT)
            dt = MAX_DT;

        // if (MapManager::get().hasPendingSpawn() && UIManager::get().isEmpty()) {
        //     MapManager::get().spawnPendingNPCs();
        // }

        m_background->draw(m_window, dt);
        Terrain::draw(m_window, dt);

        player.update(m_window, dt);
        NPCManager::get().update(m_window, dt);
        updateCamera(dt);

        m_overlay->draw(m_window, dt);
        NPCManager::get().draw(m_window, dt);
        player.draw(m_window, dt);

        sf::Vector2f playerPos = player.getPlayer()->getPosition();

        // Check for transitions using MapManager
        m_nearTransition = MapManager::get().getTransitionAt(playerPos, 100.f);
        if (!m_editor->isActive())
        {
            sf::Vector2f playerPos = Player::get().getPlayer()->getPosition();
            NPC *nearest = NPCManager::get().getNearestInteractable(playerPos);
            bool tooltip = false;
            if (m_nearTransition.has_value())
            { // ← fixed condition
                Player::get().setTooltip(m_nearTransition->label);
                tooltip = true;
            }

            if (nearest)
            {
                Player::get().setTooltip("Talk");
                tooltip = true;
            }

            if (!tooltip)
            {
                Player::get().clearTooltip();
            }
        }
    }

    void Game::handleEvents()
    {
        while (const std::optional event = m_window.pollEvent())
        {
            bool uiConsumed = UIManager::get().handleEvent(*event, m_window);
            if (event->is<sf::Event::Closed>())
            {
                m_window.close();
            }
            else if (const auto *resized = event->getIf<sf::Event::Resized>())
            {
                m_width = resized->size.x;
                m_height = resized->size.y;
                m_view.setSize(sf::Vector2f(m_width, m_height));
                m_view.setCenter(sf::Vector2f(m_width / 2.f, m_height / 2.f));
                updateScale();
            }
            else if (event->is<sf::Event::KeyPressed>())
            {
                const auto &key = event->getIf<sf::Event::KeyPressed>();
                const bool ctrlHeld = key->control;
                if (key->code == sf::Keyboard::Key::F1 ||
                    key->code == sf::Keyboard::Key::F2 ||
                    key->code == sf::Keyboard::Key::F3 ||
                    key->code == sf::Keyboard::Key::F4 ||
                    key->code == sf::Keyboard::Key::Num1 ||
                    key->code == sf::Keyboard::Key::Num2 ||
                    key->code == sf::Keyboard::Key::Num3 ||
                    key->code == sf::Keyboard::Key::Num4)
                {
                    m_editor->setActive(true);
                }
                if (key->code == sf::Keyboard::Key::S && ctrlHeld)
                {
                    MapManager::get().saveCurrentMap();
                    showInfoBox("Saved map: " + MapManager::get().getCurrentMap());
                }

                if (key->code == sf::Keyboard::Key::E && !m_editor->isActive())
                {
                    sf::Vector2f playerPos = Player::get().getPlayer()->getPosition();
                    NPC *nearest = NPCManager::get().getNearestInteractable(playerPos);
                    if (nearest)
                    {
                        NPCManager::get().interact();
                    }
                    else if (m_nearTransition.has_value())
                    { // ← fixed condition
                        const Transition &tr = *m_nearTransition;
                        MapManager::get().switchToMap(tr.targetMap, tr.spawnPosition);
                        m_nearTransition.reset();
                    }
                }
            }

            if (m_editor->isActive())
            {
                m_editor->handleEvent(*event, m_window);
            }

            if (!m_gameStarted && UIManager::get().isEmpty())
            {
                startGame();
            }
        }
    }

    void Game::run()
    {
        while (m_window.isOpen())
        {
            m_window.setView(m_view);
            handleEvents();

            m_window.clear();

            float dt = m_clock.restart().asSeconds();

            if (m_editor->isActive())
            {
                // Editor mode: draw world and editor overlay
                m_background->draw(m_window, dt);
                Terrain::draw(m_window, dt);
                m_editor->draw(m_window);

                // Editor camera movement
                float speed = 200.f;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift))
                    speed *= 2.f;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
                    m_view.move(sf::Vector2f(speed * dt, 0.f));
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
                    m_view.move(sf::Vector2f(-speed * dt, 0.f));
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
                    m_view.move(sf::Vector2f(0.f, -speed * dt));
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
                    m_view.move(sf::Vector2f(0.f, speed * dt));
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape))
                {
                    m_editor->setActive(false);
                }
            }
            else
            {
                bool gameShouldUpdate = true;
                UIManager::get().update(dt, gameShouldUpdate);
                if (gameShouldUpdate)
                {
                    update(dt);
                }
                else
                {
                    // UI blocks game update, but draw world paused
                    m_window.setView(m_view);
                    m_background->draw(m_window, dt);
                    Terrain::draw(m_window, dt);
                    NPCManager::get().draw(m_window, dt);
                    Player::get().draw(m_window, dt);
                    m_overlay->draw(m_window, dt);
                }
                m_window.setView(UIManager::get().getUIView(m_window));
                UIManager::get().draw(m_window);
            }

            m_fpsDisplay->update(dt, m_window);
            m_window.display();
        }
    }

    void Game::quit()
    {
        m_window.close();
    }

} // namespace Game