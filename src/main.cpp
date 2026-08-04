// main.cpp (FULL VERSION)
#include <SFML/Graphics.hpp>
#include <iostream>
#include <debug/logs.hpp>
#include <entities/terrain.hpp>
#include <entities/characters.hpp>
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
    Character character;
    LevelEditor editor;

    // --- Vertical camera state (asymmetric dead‑zone) ---
    float viewY;           // current vertical centre (world coords)
    float targetY;         // target vertical centre
    float smoothSpeed;     // interpolation speed (pixels per second) – lower = more gradual
    float topDeadZone;     // how close to top edge before camera moves (pixels)
    float bottomDeadZone;  // how close to bottom edge before camera moves (pixels)

public:
    MainWindow(string title, int width, int height)
        : width(width), height(height), title(title) {
        window = sf::RenderWindow(sf::VideoMode({width, height}), title);
        window.setVerticalSyncEnabled(false);
        window.setFramerateLimit(240);
        
        view.setSize(sf::Vector2f(width, height));
        view.setViewport(sf::FloatRect(sf::Vector2f(0.f, 0.f), sf::Vector2f(1.f, 1.f)));
        view.setCenter(sf::Vector2f(width / 2.f, height / 2.f));

        // Camera parameters – more gradual movement
        viewY = height / 2.f;
        targetY = viewY;
        smoothSpeed = 350.f;      // pixels per second – slow enough to be smooth
        topDeadZone = 200.f;      // larger safe zone at the top
        bottomDeadZone = 100.f;   // smaller safe zone at the bottom

        // Load resources
        Tiles::load();
        Background::load(window);
        Characters::load();
        Terrain::loadFromFile("assets/map.txt");
        editor.initPalette();
        updateScale();
    }

    void updateScale() {
        float scale = static_cast<float>(window.getSize().y) / 320.f;
        Scale::set(scale);
    }

    // ------------------------------------------------------------------
    // Camera logic: horizontal follows, vertical dead‑zone (linear)
    // ------------------------------------------------------------------
    void updateCamera(float dt) {
        // --- Horizontal: always follow character's X ---
        float halfWidth = width / 2.f;
        float viewX = std::max(character.getPosition().x, halfWidth);

        // --- Vertical: character centre from bounds ---
        sf::FloatRect bounds = character.getBounds();
        float charCentreY = bounds.position.y + bounds.size.y * 0.5f;

        // Where is the character's centre on the screen (pixels from top)?
        float screenY = charCentreY - viewY + (height / 2.f);

        // Dead‑zone boundaries – asymmetric
        float topBoundary = topDeadZone;
        float bottomBoundary = height - bottomDeadZone;

        // Update target only when leaving the dead‑zone
        if (screenY < topBoundary) {
            // Near top → centre the character vertically
            targetY = charCentreY;
        }
        else if (screenY > bottomBoundary) {
            // Near bottom → centre the character vertically
            targetY = charCentreY;
        }
        // else: inside dead‑zone → target unchanged (camera holds)

        // Linear interpolation with capped speed (gradual movement)
        float diff = targetY - viewY;
        float maxStep = smoothSpeed * dt;   // how far we can move this frame
        if (std::abs(diff) < maxStep) {
            viewY = targetY; // snap if close enough
        } else {
            viewY += (diff > 0 ? 1.f : -1.f) * maxStep;
        }

        // Apply final view centre
        view.setCenter(sf::Vector2f(viewX, viewY));
    }

    void update(float dt) {
        const float MAX_DT = 1.f / 30.f;
        if (dt > MAX_DT) dt = MAX_DT;

        bg.draw(window, dt);
        Terrain::draw(window, dt);

        character.update(window, dt);
        updateCamera(dt);

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
                else if (event->is<sf::Event::KeyPressed>()) {
                    const auto& key = event->getIf<sf::Event::KeyPressed>();
                    if (key->code == sf::Keyboard::Key::F1) {
                        if (!editor.isActive()) {
                            editor.setActive(true);
                        } else {
                            Terrain::saveToFile("assets/map.txt");
                            std::cout << "Map saved!\n";
                        }
                    }
                }
                editor.handleEvent(*event, window);
            }

            window.clear();
            
            float dt = clock.restart().asSeconds();
            window.setView(view);
            
            if (editor.isActive()) {
                bg.draw(window, dt);
                Terrain::draw(window, clock.restart().asSeconds());
                editor.draw(window);
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
                    view.move(sf::Vector2f(200.f * dt, 0.f));
                } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
                    view.move(sf::Vector2f(-200.f * dt, 0.f));
                } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
                    view.move(sf::Vector2f(0.f, -200.f * dt));
                } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
                    view.move(sf::Vector2f(0.f, 200.f * dt));
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
                    editor.setActive(false);
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
    Log::Level::set(Logging::Info);
    MainWindow win("Hello, SFML!", 1200, 600);
    win.run();
    return 0;
}