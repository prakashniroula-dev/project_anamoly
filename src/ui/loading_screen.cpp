#include "loading_screen.hpp"
#include <ui/ui_manager.hpp>
#include <debug/logs.hpp>
#include <SFML/Window/Event.hpp>
#include <cmath>
#include <algorithm>

// Smooth interpolation speed (units per second)
static const float PROGRESS_SPEED = 4.0f;

LoadingScreen::LoadingScreen(int totalSteps)
    : m_totalSteps(totalSteps)
    , m_currentStep(0)
    , m_displayProgress(0.f)
    , m_targetProgress(0.f)
    , m_titleText(m_font)
    , m_percentageText(m_font)
    , m_statusTextDisplay(m_font)
{
    

    m_titleText.setFont(m_font);
    m_titleText.setString("Loading...");
    m_titleText.setCharacterSize(36);
    m_titleText.setStyle(sf::Text::Bold);
    m_titleText.setFillColor(sf::Color::White);

    m_percentageText.setFont(m_font);
    m_percentageText.setCharacterSize(28);
    m_percentageText.setFillColor(sf::Color::White);

    m_statusTextDisplay.setFont(m_font);
    m_statusTextDisplay.setCharacterSize(20);
    m_statusTextDisplay.setFillColor(sf::Color(200, 200, 200));

    m_background.setFillColor(sf::Color(0, 0, 0, 220));
    m_background.setOutlineThickness(0.f);

    m_progressBarBg.setFillColor(sf::Color(40, 40, 40));
    m_progressBarBg.setOutlineColor(sf::Color::White);
    m_progressBarBg.setOutlineThickness(2.f);

    m_progressBarFill.setFillColor(sf::Color(70, 130, 200));
    m_progressBarFill.setOutlineThickness(0.f);
}

void LoadingScreen::setTotalSteps(int steps) {
    m_totalSteps = std::max(1, steps);
    m_currentStep = 0;
    m_targetProgress = 0.f;
    m_displayProgress = 0.f;
}

void LoadingScreen::advance() {
    if (m_currentStep < m_totalSteps) {
        ++m_currentStep;
        m_targetProgress = static_cast<float>(m_currentStep) / static_cast<float>(m_totalSteps);
        // Clamp to 1.0
        if (m_targetProgress > 1.f) m_targetProgress = 1.f;
    }
}

void LoadingScreen::setStep(int step) {
    m_currentStep = std::clamp(step, 0, m_totalSteps);
    m_targetProgress = static_cast<float>(m_currentStep) / static_cast<float>(m_totalSteps);
}

void LoadingScreen::setStatus(const std::string& msg) {
    m_statusText = msg;
    m_statusTextDisplay.setString(m_statusText);
}

void LoadingScreen::onEnter() {
    // Reset animation state
    m_displayProgress = 0.f;
    m_targetProgress = static_cast<float>(m_currentStep) / static_cast<float>(m_totalSteps);
}

void LoadingScreen::onExit() {}

void LoadingScreen::update(float dt) {
    updateProgressAnimation(dt);
}

void LoadingScreen::updateProgressAnimation(float dt) {
    // Smoothly move displayProgress towards targetProgress
    float diff = m_targetProgress - m_displayProgress;
    if (std::abs(diff) < 0.0001f) {
        m_displayProgress = m_targetProgress;
    } else {
        float step = PROGRESS_SPEED * dt;
        if (std::abs(diff) < step) {
            m_displayProgress = m_targetProgress;
        } else {
            m_displayProgress += (diff > 0.f ? step : -step);
        }
    }
    // Clamp
    if (m_displayProgress < 0.f) m_displayProgress = 0.f;
    if (m_displayProgress > 1.f) m_displayProgress = 1.f;

    // Update percentage text
    int percent = static_cast<int>(std::round(m_displayProgress * 100.f));
    if (percent > 100) percent = 100;
    m_percentageText.setString(std::to_string(percent) + "%");
}

bool LoadingScreen::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    // Consume all events – no interaction allowed
    return true;
}

void LoadingScreen::updateLayout(const sf::RenderWindow& window) {
    if (!m_fontLoaded) {
      m_font = UIManager::get().getFont();
      m_titleText.setFont(m_font);
      m_percentageText.setFont(m_font);
      m_statusTextDisplay.setFont(m_font);
      m_fontLoaded = true;
    }
    sf::Vector2f winSize = sf::Vector2f(window.getSize());

    // Background
    m_background.setSize(winSize);
    m_background.setPosition({0.f, 0.f});

    // Title (centered, 25% from top)
    sf::FloatRect titleBounds = m_titleText.getLocalBounds();
    m_titleText.setPosition({
        (winSize.x - titleBounds.size.x) / 2.f,
        winSize.y * 0.25f
    });

    // Progress bar dimensions
    const float BAR_WIDTH = std::min(winSize.x * 0.6f, 600.f);
    const float BAR_HEIGHT = 30.f;
    const float BAR_Y = winSize.y * 0.45f;
    float barX = (winSize.x - BAR_WIDTH) / 2.f;

    // Background of progress bar
    m_progressBarBg.setSize({BAR_WIDTH, BAR_HEIGHT});
    m_progressBarBg.setPosition({barX, BAR_Y});

    // Fill – width will be updated each frame in draw
    m_progressBarFill.setSize({BAR_WIDTH, BAR_HEIGHT});
    m_progressBarFill.setPosition({barX, BAR_Y});

    // Percentage text (centered over the bar, slightly above)
    sf::FloatRect percBounds = m_percentageText.getLocalBounds();
    m_percentageText.setPosition({
        (winSize.x - percBounds.size.x) / 2.f,
        BAR_Y - percBounds.size.y - 30.f
    });

    // Status text (below the bar)
    if (!m_statusText.empty()) {
        sf::FloatRect statusBounds = m_statusTextDisplay.getLocalBounds();
        m_statusTextDisplay.setPosition({
            (winSize.x - statusBounds.size.x) / 2.f,
            BAR_Y + BAR_HEIGHT + 25.f
        });
    }
}

void LoadingScreen::draw(sf::RenderWindow& window) {
    // Update layout first (in case of resize)
    updateLayout(window);

    // Draw background
    window.draw(m_background);

    // Draw title
    window.draw(m_titleText);

    // Draw progress bar background
    window.draw(m_progressBarBg);

    // Draw progress bar fill (clipped width)
    sf::FloatRect bgBounds = m_progressBarBg.getLocalBounds();
    float fillWidth = bgBounds.size.x * m_displayProgress;
    if (fillWidth > 0.f) {
        m_progressBarFill.setSize({fillWidth, bgBounds.size.y});
        window.draw(m_progressBarFill);
    }

    // Draw percentage
    window.draw(m_percentageText);

    // Draw status (if any)
    if (!m_statusText.empty()) {
        window.draw(m_statusTextDisplay);
    }
}