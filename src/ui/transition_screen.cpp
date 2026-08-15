#include "transition_screen.hpp"
#include <ui/ui_manager.hpp>
#include <debug/logs.hpp>

TransitionScreen::TransitionScreen(Callback cb, float duration)
    : callback(std::move(cb)), fadeDuration(duration) {
    overlay.setFillColor(sf::Color::Black);
}

void TransitionScreen::onEnter() {
  Log::info << "TransitionScreen: onEnter() called. fadeDuration=" << fadeDuration << "\n";
    state = State::FadingOut;
    timer = 0.f;
    currentAlpha = 0.f;
}

void TransitionScreen::update(float dt) {
  Log::info << "TransitionScreen: update() called. dt=" << dt << ", state=" << static_cast<int>(state) << ", timer=" << timer << ", currentAlpha=" << currentAlpha << "\n";
    switch (state) {
        case State::FadingOut: {
            timer += dt;
            float half = fadeDuration / 2.f;
            float progress = (half > 0.f) ? timer / half : 1.f;
            if (progress >= 1.f) {
                currentAlpha = 255.f;
                state = State::Waiting;
                timer = 0.f;
                if (callback) {
                    callback(*this);
                } else {
                    continueTransition();
                }
            } else {
                currentAlpha = 255.f * progress;
            }
            break;
        }
        case State::Waiting:
            // wait for continueTransition()
            break;
        case State::FadingIn: {
            timer += dt;
            float half = fadeDuration / 2.f;
            float progress = (half > 0.f) ? timer / half : 1.f;
            if (progress >= 1.f) {
                currentAlpha = 0.f;
                state = State::Done;
                Log::info << "TransitionScreen: transition done, popping screen.\n";
                UIManager::get().popScreen();
            } else {
                currentAlpha = 255.f * (1.f - progress);
            }
            break;
        }
        case State::Done:
            break;
    }
}

void TransitionScreen::draw(sf::RenderWindow& window) {
    if (currentAlpha <= 0.f) return;
    auto size = window.getSize();
    overlay.setSize(sf::Vector2f(size));
    overlay.setPosition({0.f, 0.f});
    overlay.setFillColor(sf::Color(0, 0, 0, static_cast<uint8_t>(currentAlpha)));
    window.draw(overlay);
}

bool TransitionScreen::handleEvent(const sf::Event&, sf::RenderWindow&) {
    // Consume all events during transition to prevent accidental input
    return true;
}

void TransitionScreen::continueTransition() {
  Log::info << "TransitionScreen: continueTransition() called.\n";
    if (state == State::Waiting) {
        state = State::FadingIn;
        timer = 0.f;
    } else {
        Log::warn << "TransitionScreen::continueTransition() called in wrong state\n";
    }
  Log::info << "TransitionScreen: state=" << static_cast<int>(state) << ", timer=" << timer << "\n";
}

void TransitionScreen::performWithFade(std::function<void()> action, float fadeDuration) {
    auto wrapper = [action = std::move(action)](TransitionScreen& screen) {
      Log::info << "TransitionScreen: performWithFade wrapper called.\n";
        if (action) action();
        screen.continueTransition();
        Log::info << "TransitionScreen: performWithFade wrapper finished.\n";
    };
    Log::info << "TransitionScreen: pushing screen with fadeDuration=" << fadeDuration << "\n";
    UIManager::get().pushScreen(std::make_unique<TransitionScreen>(wrapper, fadeDuration));
}

void TransitionScreen::endWithFade(std::function<void()> action, float fadeDuration) {
    auto wrapper = [action = std::move(action)](TransitionScreen& screen) {
      Log::info << "TransitionScreen: endWithFade wrapper called.\n";
        if (action) action();
        screen.continueTransition();
        Log::info << "TransitionScreen: endWithFade wrapper finished.\n";
    };
    Log::info << "TransitionScreen: pushing screen with fadeDuration=" << fadeDuration << "\n";
    UIManager::get().gotoScreen(std::make_unique<TransitionScreen>(wrapper, fadeDuration));
}