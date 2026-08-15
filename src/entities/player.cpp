#include <entities/player.hpp>
#include <ui/ui_manager.hpp>

void Player::update(sf::RenderWindow &win, float dt)
{
  if (!player)
  {
    Log::error << "Player::update called but player is null (possibly not initialized)" << std::endl;
    return;
  }
  player->update(win, dt);
}

void Player::draw(sf::RenderWindow &win, float dt)
{
  if (!player)
  {
    Log::error << "Player::draw called but player is null (possibly not initialized)" << std::endl;
    return;
  }
  player->draw(win, dt);
  drawTooltip(win);
}

void Player::init()
{
  if (!player)
  {
    static Character defaultPlayer;
    player = &defaultPlayer;
  }
  player->init();
}

#include <entities/npc_manager.hpp>   // to disable NPC AI

void Player::swapTo(Character* newChar) {
    if (!newChar) {
        Log::error << "Player::swapTo: null character\n";
        return;
    }

    // 1. If we already have a player, push it onto the stack and disable its controls.
    if (player) {
        // If the current player is an NPC, we might want to re‑enable its AI later,
        // but for now we just disable its controls.
        player->lockControls();

        // Also, if it's an NPC, we need to stop its autonomous behavior.
        if (NPC* npc = dynamic_cast<NPC*>(player)) {
            // We'll add a method to NPC to pause its AI.
            npc->pauseAI(true);
        }

        m_characterStack.push_back(player);
    }

    // 2. Set new player.
    player = newChar;
    player->unlockControls();

    // 3. Disable NPC AI for the new character (if it's an NPC).
    if (NPC* npc = dynamic_cast<NPC*>(player)) {
        npc->pauseAI(true);     // prevent automatic patrol/follow
    }

    // 4. (Optional) Notify NPCManager that this NPC is now player-controlled.
    //    NPCManager might need to skip updating this NPC.
    //    We'll handle that inside NPC::update by checking a flag.
}

void Player::swapBack() {
    if (m_characterStack.empty()) {
        Log::warn << "Player::swapBack: no previous character to swap to\n";
        return;
    }

    // 1. Disable current player's controls and re‑enable its AI (if it's an NPC)
    if (player) {
        player->lockControls();
        if (NPC* npc = dynamic_cast<NPC*>(player)) {
            npc->pauseAI(false);   // resume AI
        }
    }

    // 2. Pop the previous character from stack and set as player.
    Character* prev = m_characterStack.back();
    m_characterStack.pop_back();

    player = prev;
    player->unlockControls();

    // 3. Disable AI for the new player (prev was originally an NPC? Might need to pause it again)
    //    Actually, when we first swapped away from an NPC, we paused it; now we are swapping back
    //    to another character (could be original player or an NPC). If the new player is an NPC,
    //    we should pause its AI so it doesn't move autonomously.
    if (NPC* npc = dynamic_cast<NPC*>(player)) {
        npc->pauseAI(true);
    }
}

void Player::setTooltip(const std::string& text) {
    m_tooltipText = text;
}

void Player::clearTooltip() {
    m_tooltipText.clear();
}

void Player::drawTooltip(sf::RenderWindow& win) const {
    if (m_tooltipText.empty() || !player) return;

    // 1. Player head world position -> screen coordinates
    sf::FloatRect bounds = player->getBounds();
    sf::Vector2f headWorld = bounds.position + sf::Vector2f(bounds.size.x / 2.f, 0.f);
    sf::Vector2f headScreen = sf::Vector2f(win.mapCoordsToPixel(headWorld));
    headScreen.y -= 20.f;   // offset above head

    sf::Font& font = UIManager::get().getFont();

    // 2. UI constants (pixel sizes, no scaling)
    const float fontSizeKey   = 28.f;     // larger "E"
    const float fontSizeLabel = 18.f;
    const float padding       = 12.f;     // horizontal padding
    const float verticalPad   = 14.f;     // slightly more vertical padding (was 12)
    const float gap           = 10.f;     // space between "E" and label
    const float pointerSize   = 10.f;     // triangle height
    const float borderThick   = 2.f;

    // 3. Build "E" text (white, bold)
    sf::Text keyText(font, "E", fontSizeKey);
    keyText.setFillColor(sf::Color(180, 200, 255));
    keyText.setStyle(sf::Text::Bold);
    sf::FloatRect keyBounds = keyText.getLocalBounds();

    // 4. Build label text (white)
    sf::Text labelText(font, m_tooltipText, fontSizeLabel);
    labelText.setFillColor(sf::Color::White);
    sf::FloatRect labelBounds = labelText.getLocalBounds();

    // 5. Layout: rectangle with extra vertical padding for a bit more height
    float totalWidth  = keyBounds.size.x + gap + labelBounds.size.x + padding * 2;
    float totalHeight = std::max(keyBounds.size.y, labelBounds.size.y) + verticalPad * 2;

    // 6. Position the bubble so its bottom‑center aligns with headScreen
    float bubbleX = headScreen.x - totalWidth / 2.f;
    float bubbleY = headScreen.y - totalHeight - pointerSize - 5.f; // gap above pointer

    // 7. Background rectangle (dark with light blue border)
    sf::RectangleShape background({totalWidth, totalHeight});
    background.setPosition({bubbleX, bubbleY});
    background.setFillColor(sf::Color(15, 15, 25, 220));
    background.setOutlineColor(sf::Color(180, 200, 255));
    background.setOutlineThickness(borderThick);

    // 8. Pointer triangle (points down)
    sf::ConvexShape pointer;
    pointer.setPointCount(3);
    float pointerX = headScreen.x;
    float pointerY = bubbleY + totalHeight;
    pointer.setPoint(0, {pointerX - pointerSize, pointerY});
    pointer.setPoint(1, {pointerX, pointerY + pointerSize});
    pointer.setPoint(2, {pointerX + pointerSize, pointerY});
    pointer.setFillColor(sf::Color(15, 15, 25, 220));
    pointer.setOutlineColor(sf::Color(180, 200, 255));
    pointer.setOutlineThickness(borderThick);

    // 9. Position texts – both vertically centered inside the box
    // "E" on the left
    float keyX = bubbleX + padding;
    float keyY = bubbleY + (totalHeight - keyBounds.size.y) / 2.f - 4.f;
    keyText.setPosition({keyX, keyY});

    // Label to the right, also vertically centered
    float labelX = keyX + keyBounds.size.x + gap;
    float labelY = bubbleY + (totalHeight - labelBounds.size.y) / 2.f - 2.f;
    labelText.setPosition({labelX, labelY});

    // 10. Draw in UI view
    sf::View originalView = win.getView();
    win.setView(UIManager::get().getUIView(win)); // switch to UI view

    win.draw(background);
    win.draw(pointer);
    win.draw(keyText);
    win.draw(labelText);

    // Restore view
    win.setView(originalView);
}