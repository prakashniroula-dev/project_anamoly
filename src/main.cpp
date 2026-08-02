// main.cpp (FULL VERSION)
#include <SFML/Graphics.hpp>
#include <iostream>
#include <debug/logs.hpp>
#include <entities/terrain.hpp>
#include <entities/characters.hpp>    // 👈 Updated path!
#include <graphics/tiles.hpp>
#include <graphics/textures.hpp>
#include <graphics/background.hpp>
#include <core/scale.hpp>
#include <editor/level_editor.hpp>

using namespace std;

class FpsDisplay {
    float fpsDisplayTimer = 0.0f;
    int frameCount = 0;
public:
    void update(float dt, sf::RenderWindow& window) {
        frameCount++;
        fpsDisplayTimer += dt;
        if (fpsDisplayTimer >= 1.0f) {
            string title = "MyApp - FPS: " + std::to_string(frameCount);
            window.setTitle(title);
            frameCount = 0;
            fpsDisplayTimer = 0.0f;
        }
    }
};

class MainWindow {
private:
    sf::RenderWindow window;
    sf::Clock clock;
    int width, height;
    string title;
    FpsDisplay fpsDisplay;
    sf::View view;
    
    Background bg;
    Character character;   // ✅ Uncommented!
    LevelEditor editor;  // Level editor instance

public:
    MainWindow(string title, int width, int height)
        : width(width), height(height), title(title) {
        window = sf::RenderWindow(sf::VideoMode({width, height}), title);
        window.setVerticalSyncEnabled(false);
        window.setFramerateLimit(240);
        
        view.setSize(sf::Vector2f(width, height));
        view.setViewport(sf::FloatRect(sf::Vector2f(0.f, 0.f), sf::Vector2f(1.f, 1.f)));
        view.setCenter(sf::Vector2f(width / 2.f, height / 2.f));

        
        // Load all resources
        Tiles::load();
        Background::load(window);
        Characters::load();   // ✅ Uncommented!
        Terrain::loadFromFile("assets/map.txt");
        editor.initPalette();
        // character.setCharacter(Characters::Fighter_Detective);  // ✅ Uncommented!
        updateScale();
    }

     void updateScale() {
        // World height is 10 tiles × 32px = 320
        float scale = static_cast<float>(window.getSize().y) / 320.f;
        Scale::set(scale);
    }

    void update(float dt) {
        const float MAX_DT = 1.f / 30.f;
        if (dt > MAX_DT) dt = MAX_DT;

        float character_pos = character.getPosition().x;
        float half_width = width / 2.f;
        float view_center_x = std::max(character_pos, half_width);
        view.setCenter(sf::Vector2f(view_center_x, height / 2.f));
        
        bg.draw(window, dt);
        Terrain::draw(window, dt);
        character.update(window, dt);
        character.draw(window, dt);
    }

    void run() {
        while (window.isOpen()) {
            while (const std::optional event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window.close();
                }
                else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                    width = resized->size.x;
                    height = resized->size.y;
                    view.setSize(sf::Vector2f(width, height));
                    view.setCenter(sf::Vector2f(width / 2.f, height / 2.f));
                    updateScale();
                }
                // Toggle editor with F1
                else if (event->is<sf::Event::KeyPressed>()) {
                    const auto& key = event->getIf<sf::Event::KeyPressed>();
                    if (key->code == sf::Keyboard::Key::F1) {
                        editor.setActive(!editor.isActive());
                    }
                    // Save with S key (only when editor active)
                    if (key->code == sf::Keyboard::Key::S && editor.isActive()) {
                        Terrain::saveToFile("assets/map.txt");
                        std::cout << "Map saved!\n";
                    }
                }

                // Pass events to editor (clicks, scroll)
                editor.handleEvent(*event, window);
            }
            window.clear();
            
            float dt = clock.restart().asSeconds();
            window.setView(view);
            
            if (editor.isActive()) {
                bg.draw(window, dt); // Draw background under editor
                Terrain::draw(window, clock.restart().asSeconds()); // Draw terrain under editor
                editor.draw(window);
                if ( sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
                    view.move(sf::Vector2f(200.f * dt, 0.f)); // Move right
                } else if ( sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
                    view.move(sf::Vector2f(-200.f * dt, 0.f)); // Move left
                } else if ( sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
                    view.move(sf::Vector2f(0.f, -200.f * dt)); // Move up
                } else if ( sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
                    view.move(sf::Vector2f(0.f, 200.f * dt)); // Move down
                }
                if ( sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
                    editor.setActive(false); // Exit editor
                }
            } else {
                update(dt);
            }
            
            fpsDisplay.update(dt, window);
            window.display();
        }
    }
};

int main() {
    // TODO: Fix logging level (depends on your logs.hpp API)
    Log::Level::set(Logging::Info);
    
    MainWindow win("Hello, SFML!", 1200, 600);
    win.run();
    return 0;
}