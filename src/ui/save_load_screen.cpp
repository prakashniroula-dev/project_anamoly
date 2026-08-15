#include "save_load_screen.hpp"
#include "ui_manager.hpp"
#include <debug/logs.hpp>
#include <filesystem>
#include <fstream>
#include <algorithm>

SaveLoadScreen::SaveLoadScreen(Mode m, std::function<void(const std::string&)> cb)
    : mode(m), onSelect(std::move(cb)), title(font) {
    font = UIManager::get().getFont();
    title.setFont(font);
    title.setString(mode == Mode::Save ? "Save Game" : "Load Game");
    title.setCharacterSize(32);
    title.setFillColor(sf::Color::White);
    background.setFillColor(sf::Color(0, 0, 0, 220));
}

void SaveLoadScreen::scanSlots() {
    slotFiles.clear();
    std::string dir = "saves/";
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".dat") {
            slotFiles.push_back(entry.path().filename().string());
        }
    }
    // Sort by name
    std::sort(slotFiles.begin(), slotFiles.end());
    if (mode == Mode::Save) {
        // Always allow saving to a new slot: add an empty slot placeholder
        slotFiles.push_back("new_slot.dat");
    }
    selectedIndex = 0;
}

void SaveLoadScreen::onEnter() {
    scanSlots();
}

void SaveLoadScreen::updateLayout(sf::RenderWindow& window) {
    sf::Vector2f winSize = sf::Vector2f(window.getSize());
    background.setSize(winSize);
    background.setPosition({0.f, 0.f});

    // Title
    sf::FloatRect titleBounds = title.getLocalBounds();
    title.setPosition({(winSize.x - titleBounds.size.x) / 2.f, 50.f});

    // Slot labels
    slotLabels.clear();
    for (size_t i = 0; i < slotFiles.size(); ++i) {
        sf::Text txt(font);
        std::string label = slotFiles[i];
        if (label == "new_slot.dat") label = "[New Save]";
        else {
            // Optionally read first line of save to display metadata (e.g., map name)
            std::ifstream in("saves/" + slotFiles[i]);
            std::string line;
            if (std::getline(in, line)) {
                size_t eq = line.find('=');
                if (eq != std::string::npos) {
                    std::string key = line.substr(0, eq);
                    if (key == "map") label += " (" + line.substr(eq+1) + ")";
                }
            }
        }
        txt.setString(label);
        txt.setCharacterSize(24);
        txt.setFillColor(sf::Color::White);
        slotLabels.push_back(txt);
    }

    // Position slots
    float y = 120.f;
    float spacing = 50.f;
    for (auto& txt : slotLabels) {
        sf::FloatRect b = txt.getLocalBounds();
        txt.setPosition({(winSize.x - b.size.x) / 2.f, y});
        y += spacing;
    }
}

void SaveLoadScreen::draw(sf::RenderWindow& window) {
    updateLayout(window);
    window.draw(background);
    window.draw(title);
    for (size_t i = 0; i < slotLabels.size(); ++i) {
        // Highlight selected
        if (i == selectedIndex) {
            slotLabels[i].setFillColor(sf::Color::Yellow);
        } else {
            slotLabels[i].setFillColor(sf::Color::White);
        }
        window.draw(slotLabels[i]);
    }
}

bool SaveLoadScreen::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Escape) {
            UIManager::get().popScreen();
            return true;
        }
        if (key->code == sf::Keyboard::Key::Up) {
            selectedIndex = (selectedIndex - 1 + slotLabels.size()) % slotLabels.size();
            return true;
        }
        if (key->code == sf::Keyboard::Key::Down) {
            selectedIndex = (selectedIndex + 1) % slotLabels.size();
            return true;
        }
        if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space) {
            executeSelection();
            return true;
        }
    }

    // Mouse click handling (simplified – use rects if needed)
    if (const auto* btn = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (btn->button == sf::Mouse::Button::Left) {
            sf::Vector2f pos = window.mapPixelToCoords(sf::Vector2i(btn->position.x, btn->position.y));
            float y = 120.f;
            float spacing = 50.f;
            for (size_t i = 0; i < slotLabels.size(); ++i) {
                sf::FloatRect b = slotLabels[i].getGlobalBounds();
                if (b.contains(pos)) {
                    selectedIndex = i;
                    executeSelection();
                    return true;
                }
            }
        }
    }
    return false;
}

void SaveLoadScreen::executeSelection() {
    if (selectedIndex >= slotFiles.size()) return;
    std::string filename = slotFiles[selectedIndex];
    if (mode == Mode::Save && filename == "new_slot.dat") {
        // Generate a new slot name based on current time or next number
        int num = 1;
        while (std::filesystem::exists("saves/slot" + std::to_string(num) + ".dat")) ++num;
        filename = "slot" + std::to_string(num) + ".dat";
    }
    if (onSelect) onSelect(filename);
    UIManager::get().popScreen();
}