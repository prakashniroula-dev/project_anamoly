#include "message_screen.hpp"
#include "ui_manager.hpp"
#include <debug/logs.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

MessageScreen::MessageScreen(const std::string& message,
                             const std::string& description,
                             const std::vector<std::string>& buttonLabels,
                             std::function<void(int)> onResult)
    : m_message(message)
    , m_description(description)
    , m_buttonLabels(buttonLabels)
    , m_onResult(std::move(onResult))
    , m_messageText(m_font)
    , m_descriptionText(m_font)
{
    m_font = UIManager::get().getFont();

    m_messageText.setFont(m_font);
    m_messageText.setString(m_message);
    m_messageText.setCharacterSize(32);
    m_messageText.setStyle(sf::Text::Bold);
    m_messageText.setFillColor(sf::Color::White);

    m_descriptionText.setFont(m_font);
    m_descriptionText.setString(m_description);
    m_descriptionText.setCharacterSize(20);
    m_descriptionText.setFillColor(sf::Color(200, 200, 200));

    // Create button texts from labels
    for (const auto& label : m_buttonLabels) {
        sf::Text txt(m_font, label, 24);
        txt.setFillColor(sf::Color::White);
        m_buttonTexts.push_back(txt);
    }

    // Full‑screen overlay
    m_background.setFillColor(sf::Color(0, 0, 0, 210));
    m_background.setOutlineThickness(0.f);
}

void MessageScreen::onEnter() {
    m_selectedIndex = 0;
}

void MessageScreen::onExit() {
    // If the screen is popped without a selection (e.g., via Escape),
    // we do not call the callback, leaving it to the caller to handle.
}

void MessageScreen::updateLayout(sf::RenderWindow& window) {
    sf::Vector2f winSize = sf::Vector2f(window.getSize());

    // Background
    m_background.setSize(winSize);
    m_background.setPosition({0.f, 0.f});

    // ---- Message ----
    sf::FloatRect msgBounds = m_messageText.getLocalBounds();
    m_messageText.setPosition({
        (winSize.x - msgBounds.size.x) / 2.f,
        winSize.y * 0.25f
    });

    // ---- Description (if not empty) ----
    if (!m_description.empty()) {
        sf::FloatRect descBounds = m_descriptionText.getLocalBounds();
        m_descriptionText.setPosition({
            (winSize.x - descBounds.size.x) / 2.f,
            m_messageText.getPosition().y + msgBounds.size.y + 20.f
        });
    }

    // ---- Buttons ----
    if (m_buttonLabels.empty()) {
        // Fallback: add a dummy "OK" button if none provided
        m_buttonLabels = {"OK"};
        m_buttonTexts.clear();
        sf::Text txt(m_font, "OK", 24);
        txt.setFillColor(sf::Color::White);
        m_buttonTexts.push_back(txt);
    }

    const float BUTTON_WIDTH = 160.f;
    const float BUTTON_HEIGHT = 50.f;
    const float BUTTON_SPACING = 20.f;

    // Compute total width needed
    float totalWidth = m_buttonLabels.size() * BUTTON_WIDTH + (m_buttonLabels.size() - 1) * BUTTON_SPACING;

    // Y position: below description or message
    float descBottom = m_description.empty()
                         ? m_messageText.getPosition().y + msgBounds.size.y
                         : m_descriptionText.getPosition().y + m_descriptionText.getLocalBounds().size.y;
    float startY = descBottom + 60.f;

    // Center the row of buttons horizontally
    float startX = (winSize.x - totalWidth) / 2.f;

    m_buttonRects.clear();
    for (size_t i = 0; i < m_buttonTexts.size(); ++i) {
        float x = startX + i * (BUTTON_WIDTH + BUTTON_SPACING);
        float y = startY;
        sf::FloatRect rect(sf::Vector2f(x, y), sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT));
        m_buttonRects.push_back(rect);

        // Center text inside button
        sf::Text& txt = m_buttonTexts[i];
        sf::FloatRect textBounds = txt.getLocalBounds();
        float textX = rect.position.x + (rect.size.x - textBounds.size.x) / 2.f - textBounds.position.x;
        float textY = rect.position.y + (rect.size.y - textBounds.size.y) / 2.f - textBounds.position.y;
        txt.setPosition({textX, textY});
    }
}

void MessageScreen::update(float dt) {
    // Nothing to update
}

bool MessageScreen::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (const auto* resized = event.getIf<sf::Event::Resized>()) {
        updateLayout(window);
    }

    // ---- Keyboard ----
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Escape) {
            // Close without calling callback (or we could call with -1)
            UIManager::get().popScreen();
            return true;
        }

        // Navigate left/right (or up/down if we want vertical, but we keep horizontal)
        if (key->code == sf::Keyboard::Key::Left) {
            if (m_buttonTexts.size() > 1) {
                m_selectedIndex = (m_selectedIndex - 1 + m_buttonTexts.size()) % m_buttonTexts.size();
            }
            return true;
        }
        if (key->code == sf::Keyboard::Key::Right) {
            if (m_buttonTexts.size() > 1) {
                m_selectedIndex = (m_selectedIndex + 1) % m_buttonTexts.size();
            }
            return true;
        }

        if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space) {
            executeSelection();
            return true;
        }
    }

    // ---- Mouse clicks ----
    if (const auto* btn = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (btn->button == sf::Mouse::Button::Left) {
            sf::Vector2f mousePos = window.mapPixelToCoords(
                sf::Vector2i(btn->position.x, btn->position.y)
            );
            for (size_t i = 0; i < m_buttonRects.size(); ++i) {
                if (m_buttonRects[i].contains(mousePos)) {
                    m_selectedIndex = i;
                    executeSelection();
                    return true;
                }
            }
        }
    }

    // ---- Mouse hover ----
    if (const auto* move = event.getIf<sf::Event::MouseMoved>()) {
        sf::Vector2f mousePos = window.mapPixelToCoords(
            sf::Vector2i(move->position.x, move->position.y)
        );
        for (size_t i = 0; i < m_buttonRects.size(); ++i) {
            if (m_buttonRects[i].contains(mousePos)) {
                m_selectedIndex = i;
                return true;
            }
        }
    }

    return true; // block all events
}

void MessageScreen::executeSelection() {
    if (m_onResult) {
        m_onResult(m_selectedIndex);
    }
    UIManager::get().popScreen();
}

void MessageScreen::draw(sf::RenderWindow& window) {
    updateLayout(window);

    window.draw(m_background);
    window.draw(m_messageText);
    if (!m_description.empty()) {
        window.draw(m_descriptionText);
    }

    // Draw buttons
    for (size_t i = 0; i < m_buttonTexts.size(); ++i) {
        sf::Text& txt = m_buttonTexts[i];
        const sf::FloatRect& rect = m_buttonRects[i];

        sf::RectangleShape btn(rect.size);
        btn.setPosition(rect.position);

        if (i == m_selectedIndex) {
            btn.setFillColor(sf::Color(70, 130, 200));
            btn.setOutlineColor(sf::Color::White);
            btn.setOutlineThickness(2.f);
            txt.setFillColor(sf::Color::White);
        } else {
            btn.setFillColor(sf::Color(60, 60, 60, 230));
            btn.setOutlineColor(sf::Color(180, 180, 180));
            btn.setOutlineThickness(1.5f);
            txt.setFillColor(sf::Color::White);
        }
        window.draw(btn);
        window.draw(txt);
    }
}

// ---- Static convenience ----
void MessageScreen::show(const std::string& message,
                         const std::string& description,
                         const std::vector<std::string>& buttonLabels,
                         std::function<void(int)> onResult) {
    auto screen = std::make_unique<MessageScreen>(message, description, buttonLabels, std::move(onResult));
    UIManager::get().pushScreen(std::move(screen));
}