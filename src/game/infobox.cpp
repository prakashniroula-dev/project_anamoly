#include <game/infobox.hpp>
#include <SFML/Graphics.hpp>

void showInfoBox(const std::string& message) {
    sf::RenderWindow box(sf::VideoMode({800, 150}), "Info");
    sf::Font font;
    if (!font.openFromFile("assets/fonts/orbitron.ttf")) return;

    sf::Text text(font, message, 16);
    text.setFillColor(sf::Color::White);
    text.setPosition({20, 50});

    while (box.isOpen()) {
        while (const std::optional event = box.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                box.close();
        }
        box.clear(sf::Color(50, 50, 50));
        box.draw(text);
        box.display();
    }
}