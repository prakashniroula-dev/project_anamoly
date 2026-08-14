#include <ui/main_menu.hpp>
#include <ui/ui_manager.hpp>
#include <debug/logs.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

MainMenu::MainMenu(): titleLine1(font), titleLine2(font) {
    labels = {"New Game", "Continue", "Options", "Quit"};
    font = UIManager::get().getFont();

    // ---- Title: two lines ----
    titleLine1.setFont(font);
    titleLine1.setString("Project");
    titleLine1.setCharacterSize(36);
    titleLine1.setStyle(sf::Text::Bold);
    titleLine1.setFillColor(sf::Color::White);

    titleLine2.setFont(font);
    titleLine2.setString("A.N.A.M.O.L.Y");
    titleLine2.setCharacterSize(36);
    titleLine2.setStyle(sf::Text::Bold);
    titleLine2.setFillColor(sf::Color(200, 200, 255)); // subtle tint

    // ---- Options ----
    for (const auto& label : labels) {
        sf::Text txt(font, label, 24);
        txt.setFillColor(sf::Color::White);
        optionTexts.push_back(txt);
    }

    // Full‑screen overlay
    background.setFillColor(sf::Color(0, 0, 0, 210));
    background.setOutlineThickness(0.f);
}

void MainMenu::onEnter() {
    selectedIndex = 0;
}

void MainMenu::onExit() {}

void MainMenu::updateLayout(sf::RenderWindow& window) {
    sf::Vector2f winSize = sf::Vector2f(window.getSize());

    // ---- 1. Full‑screen background ----
    background.setSize(winSize);
    background.setPosition({0.f, 0.f});

    // ---- 2. Title block (two lines, centered as a unit) ----
    sf::FloatRect bounds1 = titleLine1.getLocalBounds();
    sf::FloatRect bounds2 = titleLine2.getLocalBounds();

    // Total block height = line1 height + gap + line2 height
    const float LINE_GAP = 16.f;
    float blockWidth = std::max(bounds1.size.x, bounds2.size.x);
    float blockHeight = bounds1.size.y + LINE_GAP + bounds2.size.y;

    // Top‑left of the block
    float blockX = (winSize.x - blockWidth) / 2.f;
    float blockY = winSize.y * 0.20f; // 20% from top

    // Place line1
    float line1X = blockX + (blockWidth - bounds1.size.x) / 2.f;
    float line1Y = blockY;
    titleLine1.setPosition({line1X, line1Y});

    // Place line2
    float line2X = blockX + (blockWidth - bounds2.size.x) / 2.f;
    float line2Y = blockY + bounds1.size.y + LINE_GAP;
    titleLine2.setPosition({line2X, line2Y});

    // ---- 3. Buttons: centered below the title block ----
    const float OPTION_WIDTH = 360.f;
    const float OPTION_HEIGHT = 50.f;
    const float OPTION_SPACING = OPTION_HEIGHT + 20.f;

    float optionStartY = blockY + blockHeight + 60.f; // gap after title

    optionRects.clear();
    for (size_t i = 0; i < optionTexts.size(); ++i) {
        sf::Text& txt = optionTexts[i];

        float rectX = (winSize.x - OPTION_WIDTH) / 2.f;
        float rectY = optionStartY + i * OPTION_SPACING;
        sf::FloatRect rect(sf::Vector2f(rectX, rectY), sf::Vector2f(OPTION_WIDTH, OPTION_HEIGHT));
        optionRects.push_back(rect);

        // Center text inside button
        sf::FloatRect textBounds = txt.getLocalBounds();
        float textX = rect.position.x + (rect.size.x - textBounds.size.x) / 2.f - textBounds.position.x;
        float textY = rect.position.y + (rect.size.y - textBounds.size.y) / 2.f - textBounds.position.y;
        txt.setPosition({textX, textY});
    }
}

void MainMenu::update(float dt) {}

bool MainMenu::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (const auto* resized = event.getIf<sf::Event::Resized>()) {
        updateLayout(window);
    }

    // ---- Keyboard ----
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Escape) {
            UIManager::get().popScreen();
            return true;
        }
        if (key->code == sf::Keyboard::Key::Up) {
            selectedIndex = (selectedIndex - 1 + optionTexts.size()) % optionTexts.size();
            return true;
        }
        if (key->code == sf::Keyboard::Key::Down) {
            selectedIndex = (selectedIndex + 1) % optionTexts.size();
            return true;
        }
        if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space) {
            executeSelected();
            return true;
        }
    }

    // ---- Mouse clicks ----
    if (const auto* btn = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (btn->button == sf::Mouse::Button::Left) {
            sf::Vector2f mousePos = window.mapPixelToCoords(
                sf::Vector2i(btn->position.x, btn->position.y)
            );
            for (size_t i = 0; i < optionRects.size(); ++i) {
                if (optionRects[i].contains(mousePos)) {
                    selectedIndex = i;
                    executeSelected();
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
        for (size_t i = 0; i < optionRects.size(); ++i) {
            if (optionRects[i].contains(mousePos)) {
                selectedIndex = i;
                return true;
            }
        }
    }

    return false;
}

void MainMenu::executeSelected() {
    Option opt = static_cast<Option>(selectedIndex);
    switch (opt) {
        case NewGame:
            Log::info << "MainMenu: Starting new game.\n";
            UIManager::get().popScreen();
            break;
        case Continue:
            Log::info << "MainMenu: Continue selected (not implemented).\n";
            UIManager::get().popScreen();
            break;
        case Options:
            Log::info << "MainMenu: Options selected (not implemented).\n";
            break;
        case Quit:
            Log::info << "MainMenu: Quit selected.\n";
            UIManager::get().popScreen();
            break;
    }
}

void MainMenu::draw(sf::RenderWindow& window) {
    updateLayout(window);

    window.draw(background);

    // Draw title lines
    window.draw(titleLine1);
    window.draw(titleLine2);

    // Draw buttons
    for (size_t i = 0; i < optionTexts.size(); ++i) {
        sf::Text& txt = optionTexts[i];
        const sf::FloatRect& rect = optionRects[i];

        sf::RectangleShape btn(rect.size);
        btn.setPosition(rect.position);

        if (i == selectedIndex) {
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