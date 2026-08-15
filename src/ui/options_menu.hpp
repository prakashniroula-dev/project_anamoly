#pragma once
#include <ui/ui_screen.hpp>
#include <SFML/Graphics.hpp>
#include <vector>
#include <functional>

class OptionsMenu : public UIScreen {
public:
    OptionsMenu();

    void onEnter() override;
    void onExit() override;
    bool handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;

    bool blocksGameUpdate() const override { return true; }
    bool blocksInput() const override { return true; }
    bool displayBelow() const override { return false; } // Don't display screens below

private:
    enum class ControlID {
        Volume,
        MaxFps,
        Music,
        Sfx,
        Back
    };

    struct Control {
        ControlID id;
        sf::FloatRect bounds;
        float value;            // 0..1 for sliders, 0/1 for toggles
        float minVal;
        float maxVal;
        float step;
        bool isHovered = false;
        bool isActive = false;  // for slider dragging
    };

    // Settings (dummy – replace with your actual settings manager)
    float   m_volume = 1.0f;
    int     m_maxFps = 60;
    bool    m_musicOn = true;
    bool    m_sfxOn = true;

    std::vector<Control> m_controls;
    int m_selectedIndex = 0;
    bool m_needLayoutUpdate = true;

    sf::Font m_font;
    sf::View m_uiView;
    sf::RectangleShape m_background;
    sf::Text m_titleText;

    // Helper functions
    void updateLayout(const sf::RenderWindow& window);
    void drawControl(const Control& ctrl, sf::RenderWindow& window);
    void applySettings();  // dummy – logs the values
    void onControlChanged(Control& ctrl);

    // Mouse/Keyboard handling
    bool handleMousePress(const sf::Vector2f& mousePos);
    bool handleMouseMove(const sf::Vector2f& mousePos);
    bool handleMouseRelease(const sf::Vector2f& mousePos);
    bool handleKeyPress(const sf::Event::KeyPressed& key);

    void selectPrevious();
    void selectNext();
    void adjustSlider(float delta);
    void toggleCurrent();
    void activateCurrent();

    // Helper to get display string for a control
    std::string getDisplayString(const Control& ctrl) const;
};