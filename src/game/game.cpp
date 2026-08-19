#include <game/game.hpp>
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
#include <game/save_game.hpp>
#include <story/story_manager.hpp>
#include <ui/pause_menu.hpp>
#include <ui/message_screen.hpp>
#include <sound/sound_manager.hpp>
#include <ui/loading_screen.hpp>
#include <settings/settings.hpp>
#include <cstdlib>
#include <game/interaction_manager.hpp>
#include <clue/clue_manager.hpp>
#include <story/story_manager.hpp>
#include <ui/notification.hpp>

namespace Game
{

    Game::Game(const std::string &title, unsigned int width, unsigned int height, const std::string &initialMap)
    : m_width(width), m_height(height), m_title(title), m_initialMap(initialMap)
    {
        // 1. Create window
        m_window.create(sf::VideoMode({width, height}), title);
        m_window.setVerticalSyncEnabled(false);
        m_window.setFramerateLimit(240);

        Log::info << "Window created." << "\n";

        // 2. Setup view
        m_view.setSize(sf::Vector2f(width, height));
        m_view.setViewport(sf::FloatRect(sf::Vector2f(0.f, 0.f), sf::Vector2f(1.f, 1.f)));
        m_view.setCenter(sf::Vector2f(width / 2.f, height / 2.f));

        Log::info << "View initialized." << "\n";

        m_viewY = height / 2.f;
        m_targetY = m_viewY;
        m_smoothSpeed = Character::GRAVITY * 0.7f;
        m_topDeadZone = 80.f;
        m_bottomDeadZone = 200.f;

        // 3. Create game objects
        m_fpsDisplay = new FpsDisplay();
        m_background = new Background();
        m_overlay = new Overlay();
        m_editor = new LevelEditor();

        // 4. Push loading screen
        const int TOTAL_STEPS = 14; // adjust to your actual number of steps
        auto loader = std::make_unique<LoadingScreen>(TOTAL_STEPS);
        m_loadingScreen = loader.get();
        UIManager::get().init(); // before pushing loading screen, ensure UIManager is initialized
        UIManager::get().pushScreen(std::move(loader));

        Log::info << "Loading screen initialized." << "\n";

        // ★ Immediately render the loading screen (shows 0%) ★
        renderLoadingScreen();

        Log::info << "Loading screen rendered." << "\n";

        // 5. Perform loading steps
        initResources();

        Log::info << "Resources initialized." << "\n";
        // 6. Remove loading screen and show main menu
        SoundManager::get().playMusic("main_menu", true);
        UIManager::get().gotoScreen(std::make_unique<MainMenu>(this));

        Log::info << "Main menu displayed." << "\n";
    }

    void Game::renderLoadingScreen() {
        // Process pending events (so the window stays responsive)
        while (const std::optional event = m_window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                m_window.close();
                return;
            }
            Log::info << "Game::renderLoadingScreen: processing event.\n";
            sf::View oldView = m_window.getView();
            m_window.setView(UIManager::get().getUIView(m_window));
            UIManager::get().handleEvent(*event, m_window);
            m_window.setView(oldView);
        }
        
        // Draw the loading screen (it's the only UI screen on top)
        m_window.clear();
        m_window.setView(UIManager::get().getUIView(m_window));
        Log::info << "Set view for loading screen rendering.\n";
        UIManager::get().draw(m_window);
        Log::info << "Loading screen drawn.\n";
        m_window.display();
        Log::info << "Loading screen displayed.\n";
    }

    void Game::advanceLoadingScreen(const std::string& status) {
        if (m_loadingScreen) {
            m_loadingScreen->setStatus(status);
            m_loadingScreen->advance();
            m_loadingScreen->update(1.f); // update immediately to reflect changes
            renderLoadingScreen();
        }
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
        advanceLoadingScreen("Initializing scale...");
        updateScale();
        
        advanceLoadingScreen("Loading tiles...");
        Tiles::load();
        
        advanceLoadingScreen("Loading objects...");
        Objects::load();

        advanceLoadingScreen("Loading background...");
        Background::load(m_window);

        advanceLoadingScreen("Loading overlay...");
        Overlay::load(m_window);

        advanceLoadingScreen("Loading map...");
        MapManager::get().setGame(this);
        MapManager::get().loadMap(m_initialMap);

        advanceLoadingScreen("Loading characters...");
        Characters::load();
        
        advanceLoadingScreen("Initializing player & UI...");
        Player::get().init();
        
        advanceLoadingScreen("Loading NPCs and scripts...");
        NPCManager::get().loadDefinitions(); // register NPC types
        ScriptRegistry::init();              // register scripts

        advanceLoadingScreen("Registering clues...");
        ClueManager::get().registerClues();

        advanceLoadingScreen("Loading sounds...");
        SoundManager::get().initAllSounds();

        advanceLoadingScreen("Loading & applying settings...");
        Settings::init(); // Load settings from file
        Settings::get().apply();

        advanceLoadingScreen("Finalizing...");
        m_editor->init();
        Player::get().setCharacter(Characters::Fighter_Detective);

        sf::Vector2f playerSpawn = Terrain::getPlayerSpawnPosition();
        std::string playerCharKey = Characters::Fighter_Detective;

        // Read character key from the player spawn point if present
        for (const auto& [pos, props] : Terrain::getSpawnMap()) {
            if (props.npcTypeId == "player") {
                if (!props.characterKey.empty())
                    playerCharKey = props.characterKey;
                break;
            }
        }

        NPC* playerNPC = NPCManager::get().getNPC("player");
        if (!playerNPC) {
            playerNPC = NPCManager::get().createNPC("player", playerSpawn, "player");
            if (playerNPC) {
                playerNPC->setCharacter(playerCharKey);
                playerNPC->snapToGround();
            }
        }
        if (playerNPC) {
            Player::get().setPlayer(*playerNPC);
            playerNPC->unlockControls();
            playerNPC->pauseAI(true); // AI disabled while player-controlled
        }

        advanceLoadingScreen("Loaded...");
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
        Game::loadAutosave(); // load autosave if it exists
        Player::get().getPlayer()->snapToGround();
        for (NPC *npc : NPCManager::get().getAllNPCs())
        {
            npc->snapToGround();
        }
        snapCameraToPlayer();
        // SoundManager::get().stopMusic();
        SoundManager::get().playMusic("main_theme", true);
        Log::info << "Game started, NPCs spawned.\n";
    }

    void Game::reset()
    {
        Log::info << "Resetting game state.\n";
        m_gameStarted = false;
        NPCManager::get().clearAll();
        Player::get().clearStack();
        MapManager::get().clearCurrentMapData();
        MapManager::get().loadMap(m_initialMap);
        Player::get().setCharacter(Characters::Fighter_Detective);
        UIManager::get().clearScreens();
        Player::get().getPlayer()->resetToSpawn();
        ClueManager::get().clearAll();
        m_view.setCenter(sf::Vector2f(m_width / 2.f, m_height / 2.f));
        m_view.setViewport(sf::FloatRect(sf::Vector2f(0.f, 0.f), sf::Vector2f(1.f, 1.f)));
        m_window.setView(m_view);
        m_gameStarted = false;
        // UIManager::get().pushScreen(std::make_unique<MainMenu>(this));
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
        updateCamera(dt);


        bool isCutscene = StoryManager::get().hasFlag("cutscene");
        if (!isCutscene) m_overlay->draw(m_window, dt);
        NPCManager::get().draw(m_window, dt);
        player.draw(m_window, dt);
        if (isCutscene) m_overlay->draw(m_window, dt);

        NPCManager::get().update(m_window, dt);
        Character* player_char = Player::get().getPlayer();
        sf::Vector2f playerPos = player_char ? player_char->getPosition() : sf::Vector2f(0.f, 0.f);
        if (player_char) {
            MapManager::get().checkCutsceneTriggers(player_char->getPosition());
        }
        InteractionManager::get().update(playerPos);
    }

    void Game::handleEvents()
    {
        while (const std::optional event = m_window.pollEvent())
        {
            bool eventConsumed = false;
            sf::View oldView = m_window.getView();                  // Save the current view
            m_window.setView(UIManager::get().getUIView(m_window)); // Set the view to UI view for event handling
            eventConsumed = UIManager::get().handleEvent(*event, m_window);
            Log::info << "EventConsumed: " << (eventConsumed ? "true" : "false") << "\n";
            m_window.setView(oldView); // Restore the original view
            if (eventConsumed) continue;
            if (event->is<sf::Event::Closed>())
            {
                quit();
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
                    UIManager::get().clearScreens();
                }
                if (key->code == sf::Keyboard::Key::S && ctrlHeld)
                {
                    MapManager::get().saveCurrentMap();
                    MessageScreen::show("Map saved !", "Saved map: " + MapManager::get().getCurrentMap(), {"OK"});
                    // showInfoBox("Saved map: " + MapManager::get().getCurrentMap());
                }

                if (!m_editor->isActive()) {
                    InteractionManager::get().handleKeyPress(key->code);
                }
                
                
                if (key->code == sf::Keyboard::Key::Escape && !m_editor->isActive())
                {
                    UIManager::get().pushScreen(std::make_unique<PauseMenu>(this));
                }

            }

            if (!m_editor->isActive()) {
                Player::get().handleEvents();
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
            bool gameShouldUpdate = true;
            UIManager::get().update(dt, gameShouldUpdate);
            if (!m_editor->isActive())
            {
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
            }
            m_window.setView(UIManager::get().getUIView(m_window));
            UIManager::get().draw(m_window);
            Notification::update(dt, m_window);    // update timer & animation
            Notification::draw(m_window);          // draw the notification
            SoundManager::get().update(dt);
            m_fpsDisplay->update(dt, m_window);
            m_window.display();
        }
    }

    void Game::quit()
    {
        MessageScreen::show("Are you sure you want to quit?", "", {"Yes", "No"}, [this](int result) {
            if (result == 0) { // Yes
                Log::info << "Game: Quitting game (confirmed).\n";
                autoSave();
                m_window.close();
            }
        });
    }

    void Game::saveCurrentState(const std::string &filepath) const
    {
        SaveGame save;
        save.mapName = MapManager::get().getCurrentMap();
        Character *player = Player::get().getPlayer();
        if (player)
        {
            save.playerPos = player->getPosition() / Scale::get(); // unscaled
            // We need a way to get the character key; you can store it in Player or Character.
            // For now, we assume Player stores a string, but it doesn't. We'll add a method later.
            // For brevity, we'll just save the current character's type if you add a getter.
            // I'll add a simple workaround: Characters::Player holds the current key.
            save.playerCharacter = Characters::Player; // defined in characters.hpp
        }
        else
        {
            save.playerCharacter = Characters::Fighter_Detective;
        }

        // Copy story state
        auto &story = StoryManager::get();
        save.flags = story.getFlags();
        save.items = story.getItems();
        save.choicesMade = story.getChoices();
        auto& npcManager = NPCManager::get();
        for (NPC* npc : npcManager.getAllNPCs()) {
            const std::string& id = npc->getUniqueID();
            if (!id.empty()) {
                NPCState state;
                state.autoTalked = npc->hasAutoTalked();
                state.talked = npc->hasTalked();
                save.npcStates[id] = state;
            }
        }

        if (!save.save(filepath))
        {
            Log::error << "Failed to save game to " << filepath << "\n";
        }
        else
        {
            Log::info << "Game saved to " << filepath << "\n";
        }
    }

    void Game::autoSave() const
    {
        Log::info << "Writing autosave...\n";
        // Create saves directory if it doesn't exist
        std::filesystem::create_directories(m_saveDir);
        saveCurrentState(m_saveDir + "autosave.dat");
    }

    bool Game::hasAutosave() const
    {
        return std::filesystem::exists(m_saveDir + "autosave.dat");
    }

    void Game::loadAutosave()
    {
        if (hasAutosave())
        {
            loadSaveFromFile(m_saveDir + "autosave.dat");
        }
        else
        {
            Log::info << "No autosave found.\n";
        }
    }

    void Game::loadSaveFromFile(const std::string &filepath)
    {
        SaveGame save;
        if (!save.load(filepath))
        {
            Log::error << "Failed to load save from " << filepath << "\n";
            return;
        }
        loadSave(save);
    }

    void Game::loadSave(const SaveGame &save)
    {
        // 1. Clear current world
        MapManager::get().clearCurrentMapData();

        // 2. Restore story state
        auto &story = StoryManager::get();
        story.setFlags(save.flags);
        story.setItems(save.items);
        story.setChoices(save.choicesMade);

        // 3. Load the map
        MapManager::get().loadMap(save.mapName);

        // 4. Spawn NPCs (they will use the restored story flags)
        MapManager::get().spawnNPCs();
        // 5. Set player character and position
        Player::get().clearStack();
        Character *player = Player::get().getPlayer();
        ClueManager::get().loadDiscovered(save.cluesDiscovered);
        if (player)
        {
            player->setCharacter(save.playerCharacter);
            player->resetToPosition(save.playerPos);
            player->snapToGround();
            // Update the global player key
            Characters::Player = save.playerCharacter;
        }

        auto& npcManager = NPCManager::get();
        for (const auto& [id, state] : save.npcStates) {
            NPC* npc = npcManager.getNPC(id);
            if (npc) {
                npc->setAutoTalked(state.autoTalked);
                npc->setTalked(state.talked);
            }
        }

        // 6. Snap camera
        snapCameraToPlayer();

        Log::info << "Game loaded from save.\n";
    }

} // namespace Game