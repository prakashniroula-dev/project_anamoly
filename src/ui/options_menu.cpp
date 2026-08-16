#include <ui/options_menu.hpp>
#include <ui/ui_manager.hpp>
#include <debug/logs.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <algorithm>
#include <cmath>
#include <sound/sound_manager.hpp>

// -----------------------------------------------------------------------------
// Layout constants
namespace {
    const float LABEL_WIDTH     = 150.f;
    const float VALUE_WIDTH     = 120.f;
    const float SLIDER_PADDING  = 10.f;
    const float SLIDER_HEIGHT   = 8.f;
    const float HANDLE_RADIUS   = 10.f;
    const float CONTROL_HEIGHT  = 50.f;
    const float CONTROL_SPACING = 20.f;
    const float TOGGLE_WIDTH    = 80.f;
    const float TOGGLE_HEIGHT   = 36.f;
    const float BUTTON_WIDTH    = 160.f;
    const float BUTTON_HEIGHT   = 44.f;
    const float EXTRA_SPACING   = 30.f;
}

// -----------------------------------------------------------------------------
// Helper: convert slider value to FPS string
static std::string fpsDisplayString(float value) {
    const int steps[] = {30, 60, 120, 240, 0};
    const int numSteps = sizeof(steps) / sizeof(steps[0]);
    int idx = static_cast<int>(std::round(value * (numSteps - 1)));
    idx = std::clamp(idx, 0, numSteps - 1);
    return (steps[idx] == 0) ? "Unlimited" : std::to_string(steps[idx]);
}

// -----------------------------------------------------------------------------
OptionsMenu::OptionsMenu() : m_titleText(m_font) {
    m_font = UIManager::get().getFont();

    m_titleText.setFont(m_font);
    m_titleText.setString("Options");
    m_titleText.setCharacterSize(48);
    m_titleText.setStyle(sf::Text::Bold);
    m_titleText.setFillColor(sf::Color::White);

    m_background.setFillColor(sf::Color(0, 0, 0, 220));

    // ---- Build controls (using enum IDs) ----
    Control vol;
    vol.id = ControlID::Volume;
    vol.value = m_volume;
    vol.minVal = 0.f;
    vol.maxVal = 1.f;
    vol.step = 0.01f;
    m_controls.push_back(vol);

    Control fps;
    fps.id = ControlID::MaxFps;
    // map current maxFps to 0..1 (5 steps)
    int steps[] = {30, 60, 120, 240, 0};
    int idx = 0;
    for (int i = 0; i < 5; ++i) {
        if (steps[i] == m_maxFps) { idx = i; break; }
    }
    fps.value = idx / 4.0f;
    fps.minVal = 0.f;
    fps.maxVal = 1.f;
    fps.step = 1.f / 4.f;
    m_controls.push_back(fps);

    Control music;
    music.id = ControlID::Music;
    music.value = m_musicOn ? 1.f : 0.f;
    music.minVal = 0.f;
    music.maxVal = 1.f;
    music.step = 1.f;
    m_controls.push_back(music);

    Control sfx;
    sfx.id = ControlID::Sfx;
    sfx.value = m_sfxOn ? 1.f : 0.f;
    sfx.minVal = 0.f;
    sfx.maxVal = 1.f;
    sfx.step = 1.f;
    m_controls.push_back(sfx);

    Control back;
    back.id = ControlID::Back;
    back.value = 0.f;
    m_controls.push_back(back);

    m_selectedIndex = 0;
}

// -----------------------------------------------------------------------------
void OptionsMenu::onEnter() {
    Log::info << "OptionsMenu entered.\n";
    m_needLayoutUpdate = true;
}

void OptionsMenu::onExit() {
    Log::info << "OptionsMenu exited.\n";
    applySettings();
}

// -----------------------------------------------------------------------------
void OptionsMenu::update(float dt) {}

// -----------------------------------------------------------------------------
void OptionsMenu::updateLayout(const sf::RenderWindow& window) {
    sf::Vector2f winSize = sf::Vector2f(window.getSize());
    m_uiView = UIManager::getUIView(window);
    m_background.setSize(winSize);
    m_background.setPosition({0.f, 0.f});

    // ---- Title: match main menu position and size ----
    // Main menu title block starts at winSize.y * 0.20f
    m_titleText.setCharacterSize(36);      // same as main menu
    m_titleText.setStyle(sf::Text::Bold);
    m_titleText.setFillColor(sf::Color::White);
    sf::FloatRect titleBounds = m_titleText.getLocalBounds();
    m_titleText.setPosition({
        (winSize.x - titleBounds.size.x) / 2.f,
        winSize.y * 0.20f   // exactly the same Y as main menu's first line
    });

    // Layout: all controls except the last (Back) get normal spacing;
    // Back gets extra spacing above.
    const size_t numNormal = m_controls.size() - 1;
    float totalNormalHeight = numNormal * (CONTROL_HEIGHT + CONTROL_SPACING) - CONTROL_SPACING;
    float totalHeight = totalNormalHeight + EXTRA_SPACING + CONTROL_HEIGHT;
    float startY = (winSize.y - totalHeight) / 2.f + 80.f;

    const float CONTROL_WIDTH = LABEL_WIDTH + VALUE_WIDTH + 2 * SLIDER_PADDING + 200.f;

    for (size_t i = 0; i < m_controls.size(); ++i) {
        Control& ctrl = m_controls[i];
        float x = (winSize.x - CONTROL_WIDTH) / 2.f;
        float y = startY + i * (CONTROL_HEIGHT + CONTROL_SPACING);
        if (i == m_controls.size() - 1) { // Back button
            y = startY + (i * (CONTROL_HEIGHT + CONTROL_SPACING)) + EXTRA_SPACING;
        }
        ctrl.bounds = sf::FloatRect(sf::Vector2f(x, y), sf::Vector2f(CONTROL_WIDTH, CONTROL_HEIGHT));
    }

    m_needLayoutUpdate = false;
}

// -----------------------------------------------------------------------------
std::string OptionsMenu::getDisplayString(const Control& ctrl) const {
    switch (ctrl.id) {
        case ControlID::Volume:
            return std::to_string(static_cast<int>(std::round(ctrl.value * 100))) + "%";
        case ControlID::MaxFps:
            return fpsDisplayString(ctrl.value);
        case ControlID::Music:
        case ControlID::Sfx:
            return (ctrl.value > 0.5f) ? "On" : "Off";
        case ControlID::Back:
            return "Back";
        default:
            return "";
    }
}

// -----------------------------------------------------------------------------
void OptionsMenu::drawControl(const Control& ctrl, sf::RenderWindow& window) {
    const sf::FloatRect& rect = ctrl.bounds;
    sf::Vector2f pos = rect.position;
    sf::Vector2f size = rect.size;

    // Background
    if ( ctrl.id != ControlID::Back ) {
        sf::RectangleShape bg(size);
        bg.setPosition(pos);
        bg.setFillColor(ctrl.isHovered ? sf::Color(60, 60, 60) : sf::Color(50, 50, 50));
        bg.setOutlineColor(sf::Color(100, 100, 100));
        bg.setOutlineThickness(1.f);
        window.draw(bg);
    }

    // Label
    std::string label;
    switch (ctrl.id) {
        case ControlID::Volume:   label = "Volume:"; break;
        case ControlID::MaxFps:   label = "Max FPS:"; break;
        case ControlID::Music:    label = "Music:"; break;
        case ControlID::Sfx:      label = "SFX:"; break;
        case ControlID::Back:     label = ""; break; // no label for Back button
        default: break;
    }
    if (!label.empty()) {
        sf::Text labelText(m_font, label, 20);
        labelText.setFillColor(sf::Color::White);
        labelText.setPosition(pos + sf::Vector2f(10.f, 10.f));
        window.draw(labelText);
    }

    // Value display (except for Back)
    if (ctrl.id != ControlID::Back) {
        sf::Text valueText(m_font, getDisplayString(ctrl), 20);
        valueText.setFillColor(sf::Color::Yellow);
        sf::FloatRect valBounds = valueText.getLocalBounds();
        float valueX = pos.x + size.x - 10.f;
        valueText.setPosition({
            valueX - valBounds.size.x,
            pos.y + (size.y - valBounds.size.y) / 2.f - 2.f
        });
        window.draw(valueText);
    }

    // Draw the actual control
    if (ctrl.id == ControlID::Volume || ctrl.id == ControlID::MaxFps) {
        // Slider
        float sliderX = pos.x + LABEL_WIDTH + SLIDER_PADDING;
        float sliderW = size.x - LABEL_WIDTH - VALUE_WIDTH - 2 * SLIDER_PADDING;
        float sliderY = pos.y + size.y / 2.f - SLIDER_HEIGHT / 2.f;

        sf::RectangleShape track(sf::Vector2f(sliderW, SLIDER_HEIGHT));
        track.setPosition({sliderX, sliderY});
        track.setFillColor(sf::Color(40, 40, 40));
        track.setOutlineColor(sf::Color(120, 120, 120));
        track.setOutlineThickness(1.f);
        window.draw(track);

        float fillW = ctrl.value * sliderW;
        sf::RectangleShape fill(sf::Vector2f(fillW, SLIDER_HEIGHT));
        fill.setPosition({sliderX, sliderY});
        fill.setFillColor(sf::Color(70, 130, 200));
        window.draw(fill);

        sf::CircleShape handle(HANDLE_RADIUS);
        handle.setFillColor(ctrl.isActive ? sf::Color::Yellow : sf::Color::White);
        handle.setOutlineColor(sf::Color::Black);
        handle.setOutlineThickness(1.f);
        float handleX = sliderX + ctrl.value * sliderW;
        handle.setPosition({
            handleX - HANDLE_RADIUS,
            sliderY + SLIDER_HEIGHT / 2.f - HANDLE_RADIUS
        });
        window.draw(handle);
    }
    else if (ctrl.id == ControlID::Music || ctrl.id == ControlID::Sfx) {
        // Toggle button
        sf::RectangleShape toggleBtn(sf::Vector2f(TOGGLE_WIDTH, TOGGLE_HEIGHT));
        float toggleX = pos.x + size.x - TOGGLE_WIDTH - 10.f;
        float toggleY = pos.y + (size.y - TOGGLE_HEIGHT) / 2.f;
        toggleBtn.setPosition({toggleX, toggleY});
        bool isOn = (ctrl.value > 0.5f);
        toggleBtn.setFillColor(isOn ? sf::Color(70, 130, 200) : sf::Color(80, 80, 80));
        toggleBtn.setOutlineColor(sf::Color::White);
        toggleBtn.setOutlineThickness(1.f);
        window.draw(toggleBtn);

        sf::Text toggleText(m_font, isOn ? "On" : "Off", 18);
        toggleText.setFillColor(sf::Color::White);
        sf::FloatRect tBounds = toggleText.getLocalBounds();
        toggleText.setPosition({
            toggleX + (TOGGLE_WIDTH - tBounds.size.x) / 2.f,
            toggleY + (TOGGLE_HEIGHT - tBounds.size.y) / 2.f - 2.f
        });
        window.draw(toggleText);
    }
    else if (ctrl.id == ControlID::Back) {
        // Centered button
        sf::RectangleShape btn(sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT));
        float btnX = pos.x + (size.x - BUTTON_WIDTH) / 2.f;
        float btnY = pos.y + (size.y - BUTTON_HEIGHT) / 2.f;
        btn.setPosition({btnX, btnY});
        bool isSelected = (&ctrl == &m_controls[m_selectedIndex]);
        btn.setFillColor(isSelected ? sf::Color(70, 130, 200) : sf::Color(80, 80, 80));
        btn.setOutlineColor(sf::Color::White);
        btn.setOutlineThickness(1.5f);
        window.draw(btn);

        sf::Text btnText(m_font, "Back", 22);
        btnText.setFillColor(sf::Color::White);
        sf::FloatRect bBounds = btnText.getLocalBounds();
        btnText.setPosition({
            btnX + (BUTTON_WIDTH - bBounds.size.x) / 2.f,
            btnY + (BUTTON_HEIGHT - bBounds.size.y) / 2.f - 2.f
        });
        window.draw(btnText);
    }
}

// -----------------------------------------------------------------------------
void OptionsMenu::draw(sf::RenderWindow& window) {
    if (m_needLayoutUpdate) updateLayout(window);
    window.setView(m_uiView);
    window.draw(m_background);
    window.draw(m_titleText);
    for (const auto& ctrl : m_controls) {
        drawControl(ctrl, window);
    }
}

// -----------------------------------------------------------------------------
void OptionsMenu::onControlChanged(Control& ctrl) {
    switch (ctrl.id) {
        case ControlID::Volume:   m_volume = ctrl.value; break;
        case ControlID::MaxFps: {
            const int steps[] = {30, 60, 120, 240, 0};
            int idx = static_cast<int>(std::round(ctrl.value * 4));
            idx = std::clamp(idx, 0, 4);
            m_maxFps = steps[idx];
            break;
        }
        case ControlID::Music:    m_musicOn = (ctrl.value > 0.5f); SoundManager::get().playSound("ui_click"); break;
        case ControlID::Sfx:      m_sfxOn = (ctrl.value > 0.5f); SoundManager::get().playSound("ui_click"); break;
        case ControlID::Back:     break;
    }
    applySettings();
}

void OptionsMenu::applySettings() {
    // Apply to SoundManager
    SoundManager::get().setMasterVolume(m_volume);
    SoundManager::get().setMusicVolume(m_musicOn ? 1.0f : 0.0f);
    SoundManager::get().setSFXVolume(m_sfxOn ? 1.0f : 0.0f);
    Log::info << "Options applied: Volume=" << (int)(m_volume*100)
              << "%, FPS=" << (m_maxFps==0?"Unlimited":std::to_string(m_maxFps))
              << ", Music=" << (m_musicOn?"On":"Off")
              << ", SFX=" << (m_sfxOn?"On":"Off") << "\n";
}

// -----------------------------------------------------------------------------
// Mouse & keyboard handlers (identical to previous version, but using enum)
bool OptionsMenu::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (m_needLayoutUpdate) updateLayout(window);

    if (const auto* btn = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (btn->button == sf::Mouse::Button::Left) {
            sf::Vector2f mousePos = window.mapPixelToCoords(
                sf::Vector2i(btn->position.x, btn->position.y), m_uiView);
            if (handleMousePress(mousePos)) return true;
        }
    }
    else if (const auto* move = event.getIf<sf::Event::MouseMoved>()) {
        sf::Vector2f mousePos = window.mapPixelToCoords(
            sf::Vector2i(move->position.x, move->position.y), m_uiView);
        if (handleMouseMove(mousePos)) return true;
    }
    else if (const auto* rel = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (rel->button == sf::Mouse::Button::Left) {
            sf::Vector2f mousePos = window.mapPixelToCoords(
                sf::Vector2i(rel->position.x, rel->position.y), m_uiView);
            if (handleMouseRelease(mousePos)) return true;
        }
    }

    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (handleKeyPress(*key)) return true;
    }

    return false;
}

bool OptionsMenu::handleMousePress(const sf::Vector2f& mousePos) {
    for (size_t i = 0; i < m_controls.size(); ++i) {
        Control& ctrl = m_controls[i];
        if (!ctrl.bounds.contains(mousePos)) continue;
        m_selectedIndex = i;

        if (ctrl.id == ControlID::Volume || ctrl.id == ControlID::MaxFps) {
            ctrl.isActive = true;
            float sliderX = ctrl.bounds.position.x + LABEL_WIDTH + SLIDER_PADDING;
            float sliderW = ctrl.bounds.size.x - LABEL_WIDTH - VALUE_WIDTH - 2 * SLIDER_PADDING;
            float relX = (mousePos.x - sliderX) / sliderW;
            relX = std::clamp(relX, 0.f, 1.f);
            if (ctrl.step > 0.f) {
                float steps = 1.f / ctrl.step;
                relX = std::round(relX * steps) / steps;
            }
            ctrl.value = relX;
            onControlChanged(ctrl);
            return true;
        }
        else if (ctrl.id == ControlID::Music || ctrl.id == ControlID::Sfx) {
            toggleCurrent();
            return true;
        }
        else if (ctrl.id == ControlID::Back) {
            activateCurrent();
            return true;
        }
    }
    return false;
}

bool OptionsMenu::handleMouseMove(const sf::Vector2f& mousePos) {
    for (size_t i = 0; i < m_controls.size(); ++i) {
        Control& ctrl = m_controls[i];
        bool wasHovered = ctrl.isHovered;
        ctrl.isHovered = ctrl.bounds.contains(mousePos);
        if (ctrl.isHovered && !wasHovered) {
            m_selectedIndex = i;
        }
    }

    for (Control& ctrl : m_controls) {
        if ((ctrl.id == ControlID::Volume || ctrl.id == ControlID::MaxFps) && ctrl.isActive) {
            float sliderX = ctrl.bounds.position.x + LABEL_WIDTH + SLIDER_PADDING;
            float sliderW = ctrl.bounds.size.x - LABEL_WIDTH - VALUE_WIDTH - 2 * SLIDER_PADDING;
            float relX = (mousePos.x - sliderX) / sliderW;
            relX = std::clamp(relX, 0.f, 1.f);
            if (ctrl.step > 0.f) {
                float steps = 1.f / ctrl.step;
                relX = std::round(relX * steps) / steps;
            }
            ctrl.value = relX;
            onControlChanged(ctrl);
            return true;
        }
    }
    return false;
}

bool OptionsMenu::handleMouseRelease(const sf::Vector2f& mousePos) {
    for (Control& ctrl : m_controls) {
        if ((ctrl.id == ControlID::Volume || ctrl.id == ControlID::MaxFps) && ctrl.isActive) {
            SoundManager::get().playSound("ui_click");
            ctrl.isActive = false;
            return true;
        }
    }
    return false;
}

bool OptionsMenu::handleKeyPress(const sf::Event::KeyPressed& key) {
    switch (key.code) {
        case sf::Keyboard::Key::Escape:
            // Find Back button and activate it
            for (size_t i = 0; i < m_controls.size(); ++i) {
                if (m_controls[i].id == ControlID::Back) {
                    m_selectedIndex = i;
                    activateCurrent();
                    return true;
                }
            }
            UIManager::get().popScreen();
            return true;
        case sf::Keyboard::Key::Up:   selectPrevious(); return true;
        case sf::Keyboard::Key::Down: selectNext(); return true;
        case sf::Keyboard::Key::Left:
        case sf::Keyboard::Key::Right:
            adjustSlider((key.code == sf::Keyboard::Key::Right) ? 1.f : -1.f);
            return true;
        case sf::Keyboard::Key::Enter:
        case sf::Keyboard::Key::Space:
            if (m_controls[m_selectedIndex].id == ControlID::Music ||
                m_controls[m_selectedIndex].id == ControlID::Sfx) {
                toggleCurrent();
            } else if (m_controls[m_selectedIndex].id == ControlID::Back) {
                activateCurrent();
            }
            return true;
        default: break;
    }
    return false;
}

// -----------------------------------------------------------------------------
void OptionsMenu::selectPrevious() {
    if (m_controls.empty()) return;
    int newIdx = m_selectedIndex - 1;
    if (newIdx < 0) newIdx = m_controls.size() - 1;
    m_selectedIndex = newIdx;
}

void OptionsMenu::selectNext() {
    if (m_controls.empty()) return;
    int newIdx = m_selectedIndex + 1;
    if (newIdx >= (int)m_controls.size()) newIdx = 0;
    m_selectedIndex = newIdx;
}

void OptionsMenu::adjustSlider(float delta) {
    Control& ctrl = m_controls[m_selectedIndex];
    if (ctrl.id != ControlID::Volume && ctrl.id != ControlID::MaxFps) return;
    float step = ctrl.step > 0.f ? ctrl.step : 0.01f;
    float newVal = ctrl.value + delta * step;
    newVal = std::clamp(newVal, 0.f, 1.f);
    if (ctrl.step > 0.f) {
        float steps = 1.f / ctrl.step;
        newVal = std::round(newVal * steps) / steps;
    }
    ctrl.value = newVal;
    onControlChanged(ctrl);
}

void OptionsMenu::toggleCurrent() {
    Control& ctrl = m_controls[m_selectedIndex];
    if (ctrl.id != ControlID::Music && ctrl.id != ControlID::Sfx) return;
    ctrl.value = (ctrl.value > 0.5f) ? 0.f : 1.f;
    onControlChanged(ctrl);
}

void OptionsMenu::activateCurrent() {
    Control& ctrl = m_controls[m_selectedIndex];
    if (ctrl.id == ControlID::Back) {
        SoundManager::get().playSound("ui_click");
        UIManager::get().popScreen();
    }
}