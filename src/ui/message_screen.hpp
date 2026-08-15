#pragma once
#include "ui_screen.hpp"
#include <SFML/Graphics.hpp>
#include <functional>
#include <string>
#include <vector>

//
//  * A modal screen that displays a message with optional description
//  * and a configurable set of buttons. Each button has a custom label.
//  *
//  * Usage:
//  *   MessageScreen::show("Are you sure?", "This action cannot be undone.", {"Yes", "No"},
//  *       [](int choice) { if (choice == 0) /* Yes */; });
//  *
//  *   MessageScreen::show("Saved successfully!", "", {"OK"});
// 


class MessageScreen : public UIScreen {
public:
    /**
     * @param message      Main text (large, bold)
     * @param description  Extra smaller text below the message (optional)
     * @param buttonLabels List of button labels (e.g., {"OK"}, {"Yes", "No"}, {"Retry", "Cancel"})
     * @param onResult     Callback when a button is clicked. Receives the index of the selected button.
     *                     If the screen is closed via Escape without selection, callback is not called.
     */
    MessageScreen(const std::string& message,
                  const std::string& description = "",
                  const std::vector<std::string>& buttonLabels = {"OK"},
                  std::function<void(int)> onResult = nullptr);

    // UIScreen interface
    void onEnter() override;
    void onExit() override;
    bool handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;

    bool blocksGameUpdate() const override { return true; }
    bool blocksInput() const override { return true; }
    bool displayBelow() const override { return false; }

    // Static convenience: create and push the screen onto UIManager
    static void show(const std::string& message,
                     const std::string& description = "",
                     const std::vector<std::string>& buttonLabels = {"OK"},
                     std::function<void(int)> onResult = nullptr);

private:
    std::function<void(int)> m_onResult;
    std::string m_message;
    std::string m_description;
    std::vector<std::string> m_buttonLabels;

    int m_selectedIndex = 0;

    sf::Font m_font;
    sf::Text m_messageText;
    sf::Text m_descriptionText;
    std::vector<sf::Text> m_buttonTexts;
    std::vector<sf::FloatRect> m_buttonRects;  // for hit-testing
    sf::RectangleShape m_background;

    void updateLayout(sf::RenderWindow& window);
    void executeSelection();  // calls m_onResult with the selected index and pops the screen
};