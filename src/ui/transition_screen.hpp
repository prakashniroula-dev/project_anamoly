#pragma once
#include <ui/ui_screen.hpp>
#include <functional>
#include <SFML/Graphics/RectangleShape.hpp>

class TransitionScreen : public UIScreen {
public:
    using Callback = std::function<void(TransitionScreen&)>;

    TransitionScreen(Callback callback, float fadeDuration = 0.5f);

    void onEnter() override;
    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;
    bool handleEvent(const sf::Event& event, sf::RenderWindow& window) override;

    bool blocksGameUpdate() const override { return true; }
    bool blocksInput() const override { return true; }

    void continueTransition();

    // Static helper: push a transition screen that runs the given action
    static void performWithFade(std::function<void()> action, float fadeDuration = 0.5f);

private:
    enum class State { FadingOut, Waiting, FadingIn, Done };
    State state = State::FadingOut;
    Callback callback;
    float fadeDuration;
    float timer = 0.f;
    sf::RectangleShape overlay;
    float currentAlpha = 0.f; // 0–255
};