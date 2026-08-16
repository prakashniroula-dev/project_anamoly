#pragma once
#include <ui/ui_screen.hpp>
#include <SFML/Graphics.hpp>
#include <string>

class LoadingScreen : public UIScreen {
public:
    // totalSteps: the number of steps to complete (e.g., 10).
    // The progress will go from 0 to totalSteps.
    LoadingScreen(int totalSteps = 1);

    // Set the total number of steps (resets progress to 0).
    void setTotalSteps(int steps);

    // Advance progress by one step (or set to a specific step).
    void advance();
    void setStep(int step);

    // Optionally set a status message displayed below the progress bar.
    void setStatus(const std::string& msg);

    // UIScreen interface
    void onEnter() override;
    void onExit() override;
    bool handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;

    bool blocksGameUpdate() const override { return true; }
    bool blocksInput() const override { return true; }
    bool displayBelow() const override { return false; } // opaque overlay

private:
    // Progress state
    int m_totalSteps;
    int m_currentStep;              // logical step (0..total)
    float m_displayProgress;        // 0..1 smooth animated value
    float m_targetProgress;         // target based on currentStep/totalSteps

    std::string m_statusText;

    // UI elements
    sf::Font m_font;
    sf::Text m_titleText;
    sf::Text m_percentageText;
    sf::Text m_statusTextDisplay;
    sf::RectangleShape m_background;
    sf::RectangleShape m_progressBarBg;
    sf::RectangleShape m_progressBarFill;
    bool m_fontLoaded = false;

    // Layout constants (updated on resize)
    void updateLayout(const sf::RenderWindow& window);
    void updateProgressAnimation(float dt);
};