#include "notification.hpp"
#include <ui/ui_manager.hpp>
#include <SFML/Graphics.hpp>
#include <memory>

namespace Notification {
    // Internal state
    struct NotificationState {
        sf::Font font;
        sf::Text title;
        sf::Text desc;
        sf::RectangleShape background;
        float duration = 3.f;
        float elapsed = 0.f;
        bool active = false;

        // Fade animation
        enum class Phase { FadeIn, Show, FadeOut, Done };
        Phase phase = Phase::Done;
        float fadeTimer = 0.f;
        const float fadeDuration = 0.3f;

        NotificationState():title(font), desc(font){
          font = UIManager::get().getFont();
        }

        void update(float dt) {
            if (!active || phase == Phase::Done) return;

            elapsed += dt;
            switch (phase) {
                case Phase::FadeIn: {
                    fadeTimer += dt;
                    float alpha = std::min(fadeTimer / fadeDuration, 1.f);
                    setAlpha(alpha);
                    if (fadeTimer >= fadeDuration) {
                        phase = Phase::Show;
                        fadeTimer = 0.f;
                    }
                    break;
                }
                case Phase::Show: {
                    if (elapsed >= duration) {
                        phase = Phase::FadeOut;
                        fadeTimer = 0.f;
                    }
                    break;
                }
                case Phase::FadeOut: {
                    fadeTimer += dt;
                    float alpha = 1.f - std::min(fadeTimer / fadeDuration, 1.f);
                    setAlpha(alpha);
                    if (fadeTimer >= fadeDuration) {
                        phase = Phase::Done;
                        active = false;
                    }
                    break;
                }
                default: break;
            }
        }

        void setAlpha(float alpha) {
            uint8_t a = static_cast<uint8_t>(alpha * 255);
            background.setFillColor(sf::Color(20, 20, 30, a));
            background.setOutlineColor(sf::Color(255, 215, 0, static_cast<uint8_t>(a * 0.6f)));
            title.setFillColor(sf::Color(255, 215, 0, a));
            desc.setFillColor(sf::Color(255, 255, 255, a));
        }

        void layout(const sf::RenderWindow& window) {
            sf::Vector2f winSize = sf::Vector2f(window.getSize());

            // Use UI view coordinates (same as default view)
            sf::FloatRect titleBounds = title.getLocalBounds();
            sf::FloatRect descBounds = desc.getLocalBounds();

            const float padding = 20.f;
            const float gap = 10.f;

            float maxTextWidth = std::max(titleBounds.size.x, descBounds.size.x);
            float bgWidth = maxTextWidth + padding * 2.f;
            float bgHeight = titleBounds.size.y + gap + descBounds.size.y + padding * 2.f;

            float margin = 20.f;
            float bgX = winSize.x - bgWidth - margin;
            float bgY = margin;

            background.setSize({bgWidth, bgHeight});
            background.setPosition({bgX, bgY});

            float titleX = bgX + (bgWidth - titleBounds.size.x) / 2.f;
            float titleY = bgY + padding;
            title.setPosition({titleX, titleY});

            float descX = bgX + (bgWidth - descBounds.size.x) / 2.f;
            float descY = titleY + titleBounds.size.y + gap;
            desc.setPosition({descX, descY});
        }

        void draw(sf::RenderWindow& window) {
            if (!active || phase == Phase::Done) return;
            layout(window);
            window.draw(background);
            window.draw(title);
            window.draw(desc);
        }

        void reset() {
            active = false;
            phase = Phase::Done;
            elapsed = 0.f;
            fadeTimer = 0.f;
        }

        void start(const std::string& titleStr, const std::string& descStr, float dur) {
            sf::Font& font = UIManager::get().getFont();
            title.setFont(font);
            title.setString(titleStr);
            title.setCharacterSize(24);
            title.setStyle(sf::Text::Bold);
            title.setFillColor(sf::Color(255, 215, 0));

            desc.setFont(font);
            desc.setString(descStr);
            desc.setCharacterSize(18);
            desc.setFillColor(sf::Color::White);

            background.setFillColor(sf::Color(20, 20, 30, 220));
            background.setOutlineColor(sf::Color(255, 215, 0, 150));
            background.setOutlineThickness(2.f);

            duration = dur;
            elapsed = 0.f;
            phase = Phase::FadeIn;
            fadeTimer = 0.f;
            active = true;
            setAlpha(0.f); // start transparent
        }
    };

    static NotificationState state;

    // ------------------------------------------------------------
    // Public API
    // ------------------------------------------------------------
    void show(const std::string& title, const std::string& description, float duration) {
        state.start(title, description, duration);
    }

    void update(float dt, const sf::RenderWindow& window) {
        state.update(dt);
    }

    void draw(sf::RenderWindow& window) {
        state.draw(window);
    }
}