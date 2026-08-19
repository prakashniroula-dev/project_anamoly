#include "death_screen.hpp"
#include "ui_manager.hpp"
#include <game/game.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <sound/sound_manager.hpp>
#include <ui/main_menu.hpp>

DeathScreen::DeathScreen(Game::Game* game) : m_game(game), m_title(m_font), m_subtitle(m_font) {
    m_font = UIManager::get().getFont();
    m_title.setFont(m_font);
    m_title.setString("YOU DIED");
    m_title.setCharacterSize(56);          // slightly smaller, more elegant
    m_title.setStyle(sf::Text::Bold);
    m_title.setFillColor(sf::Color(200, 50, 50)); // softer red

    m_subtitle.setFont(m_font);
    m_subtitle.setString("What would you like to do?");
    m_subtitle.setCharacterSize(28);
    m_subtitle.setFillColor(sf::Color::White);

    // Use slightly smaller font for options so long text fits
    const std::vector<std::string> labels = {"Retry from last save", "New Game"};
    for (const auto& label : labels) {
        sf::Text txt(m_font, label, 24);   // reduced from 28
        txt.setFillColor(sf::Color::White);
        m_options.push_back(txt);
    }

    m_backdrop.setFillColor(sf::Color(0, 0, 0, 220));
}

void DeathScreen::onEnter() {
    m_selectedIndex = 0;
}

bool DeathScreen::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Up) {
            m_selectedIndex = (m_selectedIndex - 1 + m_options.size()) % m_options.size();
            return true;
        }
        if (key->code == sf::Keyboard::Key::Down) {
            m_selectedIndex = (m_selectedIndex + 1) % m_options.size();
            return true;
        }
        if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space) {
            executeSelected();
            return true;
        }
    }

    if (const auto* btn = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (btn->button == sf::Mouse::Button::Left) {
            sf::Vector2f mousePos = window.mapPixelToCoords(sf::Vector2i(btn->position.x, btn->position.y));
            for (size_t i = 0; i < m_optionRects.size(); ++i) {
                if (m_optionRects[i].contains(mousePos)) {
                    m_selectedIndex = i;
                    executeSelected();
                    return true;
                }
            }
        }
    }

    if (const auto* move = event.getIf<sf::Event::MouseMoved>()) {
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Vector2i(move->position.x, move->position.y));
        for (size_t i = 0; i < m_optionRects.size(); ++i) {
            if (m_optionRects[i].contains(mousePos)) {
                m_selectedIndex = i;
                return true;
            }
        }
    }

    return false;
}

void DeathScreen::updateLayout(sf::RenderWindow& window) {
    sf::Vector2f winSize = sf::Vector2f(window.getSize());

    // ---- Full‑screen backdrop ----
    m_backdrop.setSize(winSize);
    m_backdrop.setPosition({0.f, 0.f});

    // ---- Title: "YOU DIED" ----
    sf::FloatRect titleBounds = m_title.getLocalBounds();
    float titleX = (winSize.x - titleBounds.size.x) / 2.f;
    float titleY = winSize.y * 0.20f;   // same as main menu
    m_title.setPosition({titleX, titleY});

    // ---- Subtitle ----
    sf::FloatRect subBounds = m_subtitle.getLocalBounds();
    float subX = (winSize.x - subBounds.size.x) / 2.f;
    float subY = titleY + titleBounds.size.y + 30.f;
    m_subtitle.setPosition({subX, subY});

    // ---- Options buttons (larger) ----
    const float BUTTON_WIDTH = 420.f;        // wider to fit long text
    const float BUTTON_HEIGHT = 60.f;        // taller, more clickable
    const float BUTTON_SPACING = BUTTON_HEIGHT + 25.f; // more breathing room

    float totalHeight = m_options.size() * BUTTON_HEIGHT + (m_options.size() - 1) * BUTTON_SPACING;
    float startY = subY + subBounds.size.y + 60.f;
    float available = winSize.y - startY - 50.f;
    if (available > totalHeight) {
        startY += (available - totalHeight) / 2.f;
    }

    m_optionRects.clear();
    for (size_t i = 0; i < m_options.size(); ++i) {
        float x = (winSize.x - BUTTON_WIDTH) / 2.f;
        float y = startY + i * BUTTON_SPACING;
        sf::FloatRect rect(sf::Vector2f(x, y), sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT));
        m_optionRects.push_back(rect);

        sf::Text& txt = m_options[i];
        sf::FloatRect textBounds = txt.getLocalBounds();
        // centre text inside button
        float textX = rect.position.x + (rect.size.x - textBounds.size.x) / 2.f - textBounds.position.x;
        float textY = rect.position.y + (rect.size.y - textBounds.size.y) / 2.f - textBounds.position.y;
        txt.setPosition({textX, textY});
    }
}

void DeathScreen::draw(sf::RenderWindow& window) {
    updateLayout(window);

    window.draw(m_backdrop);
    window.draw(m_title);
    window.draw(m_subtitle);

    for (size_t i = 0; i < m_options.size(); ++i) {
        sf::Text& txt = m_options[i];
        const sf::FloatRect& rect = m_optionRects[i];

        sf::RectangleShape btn(rect.size);
        btn.setPosition(rect.position);

        if (i == m_selectedIndex) {
            btn.setFillColor(sf::Color(70, 130, 200));   // highlighted blue
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

void DeathScreen::executeSelected() {
    SoundManager::get().playSound("ui_click");
    if (m_selectedIndex == Retry) {
        if (m_game->hasAutosave()) {
            m_game->loadAutosave();
            UIManager::get().popScreen();
        } else {
            m_game->reset();
            UIManager::get().gotoScreen(std::make_unique<MainMenu>(m_game));
        }
    } else if (m_selectedIndex == NewGame) {
        m_game->reset();
        UIManager::get().gotoScreen(std::make_unique<MainMenu>(m_game));
    }
}