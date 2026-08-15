#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <optional>
#include <map/map_manager.hpp>

// Forward declarations
class Background;
class Overlay;
class LevelEditor;
class FpsDisplay;
// struct Transition;

namespace Game {
    class Game {
    public:
        Game(const std::string& title, unsigned int width, unsigned int height, const std::string& initialMap);
        void run();
        ~Game();
        void snapCameraToPlayer();
        void quit();

    private:
        // Window & loop
        sf::RenderWindow m_window;
        sf::Clock        m_clock;
        sf::View         m_view;
        unsigned int     m_width;
        unsigned int     m_height;
        std::string      m_title;

        // FPS display (included as a member, but definition in separate file)
        FpsDisplay*      m_fpsDisplay = nullptr; // or unique_ptr

        // World objects
        Background*      m_background = nullptr;
        Overlay*         m_overlay = nullptr;
        LevelEditor*     m_editor = nullptr;

        // Camera state
        float m_viewY;
        float m_targetY;
        float m_smoothSpeed;
        float m_topDeadZone;
        float m_bottomDeadZone;

        // map transition near detection
        std::optional<Transition> m_nearTransition;
        std::string m_initialMap;

        // Private methods
        void updateScale();
        void updateCamera(float dt);
        void update(float dt);
        void handleEvents();
        void initResources();   // load textures, spawns, etc.
        bool m_gameStarted = false;
        void startGame();

        // Disable copying
        Game(const Game&) = delete;
        Game& operator=(const Game&) = delete;
    };
} // namespace Game