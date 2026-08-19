#include <entities/player.hpp>
#include <ui/ui_manager.hpp>
#include <story/story_manager.hpp>

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

    if (player) {
        player->lockControls();
        if (NPC* npc = dynamic_cast<NPC*>(player)) {
            npc->pauseAI(false);   // resume AI
            npc->idle();           // reset movement state so patrol can take over
        }
    }

    Character* prev = m_characterStack.back();
    m_characterStack.pop_back();

    player = prev;
    player->unlockControls();

    if (NPC* npc = dynamic_cast<NPC*>(player)) {
        npc->pauseAI(true);
    }
}

bool Player::canSwap(NPC* npc) const {
    // Base condition: the NPC must exist.
    if (!npc) return false;

    // EXTEND HERE: add additional conditions, e.g.:

    // example -> right now evaluates to false disabling all swap !!
    if (!StoryManager::get().getFlag("swap_enabled")) return false;

    // if (npc->getType().id == "special") return false;

    // Default: swapping is allowed.
    return true;
}


void Player::drawTooltip(sf::RenderWindow& win) const {
    if (m_hints.empty() || !player) return;

    // 1. Head world -> screen coordinates
    sf::FloatRect bounds = player->getBounds();
    sf::Vector2f headWorld = bounds.position + sf::Vector2f(bounds.size.x / 2.f, 0.f);
    sf::Vector2f headScreen = sf::Vector2f(win.mapCoordsToPixel(headWorld));
    headScreen.y -= 20.f;   // offset above head

    sf::Font& font = UIManager::get().getFont();

    // 2. Build text objects for each hint
    struct HintRender {
        sf::Text keyText;
        sf::Text labelText;
        float width;   // total width of this hint (key + gap + label + padding)
    };
    std::vector<HintRender> renders;
    const float keyFontSize = 28.f;
    const float labelFontSize = 18.f;
    const float padding = 12.f;
    const float gap = 8.f;
    const float verticalPad = 14.f;
    const float pointerSize = 10.f;
    const float borderThick = 2.f;
    const float spacingBetweenHints = 20.f;

    for (const auto& hint : m_hints) {
        sf::Text keyText(font, std::string(1, hint.key), keyFontSize);
        keyText.setFillColor(sf::Color(180, 200, 255));  // subtle tint
        keyText.setStyle(sf::Text::Bold);

        sf::Text labelText(font, hint.label, labelFontSize);
        labelText.setFillColor(sf::Color::White);

        sf::FloatRect keyBounds = keyText.getLocalBounds();
        sf::FloatRect labelBounds = labelText.getLocalBounds();

        float hintWidth = keyBounds.size.x + gap + labelBounds.size.x + padding * 2;
        renders.push_back({std::move(keyText), std::move(labelText), hintWidth});
    }

    // 3. Compute total width and height
    float totalWidth = 0.f;
    for (size_t i = 0; i < renders.size(); ++i) {
        totalWidth += renders[i].width;
        if (i > 0) totalWidth += spacingBetweenHints;
    }
    float maxHeight = 0.f;
    for (auto& r : renders) {
        maxHeight = std::max(maxHeight, r.keyText.getLocalBounds().size.y);
        maxHeight = std::max(maxHeight, r.labelText.getLocalBounds().size.y);
    }
    float totalHeight = maxHeight + verticalPad * 2;

    // 4. Position the bubble
    float bubbleX = headScreen.x - totalWidth / 2.f;
    float bubbleY = headScreen.y - totalHeight - pointerSize - 5.f;

    // 5. Background
    sf::RectangleShape background({totalWidth, totalHeight});
    background.setPosition({bubbleX, bubbleY});
    background.setFillColor(sf::Color(15, 15, 25, 220));
    background.setOutlineColor(sf::Color(180, 200, 255));
    background.setOutlineThickness(borderThick);

    // 6. Pointer triangle
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

    // 7. Place each hint's text inside the box
    float currentX = bubbleX + padding;
    for (auto& r : renders) {
        // Key (aligned left, vertically centered)
        sf::FloatRect keyBounds = r.keyText.getLocalBounds();
        float keyX = currentX;
        float keyY = bubbleY + (totalHeight - keyBounds.size.y) / 2.f - 4.f;
        r.keyText.setPosition({keyX, keyY});

        // Label (to the right of key)
        float labelX = keyX + keyBounds.size.x + gap;
        sf::FloatRect labelBounds = r.labelText.getLocalBounds();
        float labelY = bubbleY + (totalHeight - labelBounds.size.y) / 2.f - 2.f;
        r.labelText.setPosition({labelX, labelY});

        currentX += r.width + spacingBetweenHints;
    }

    // 8. Draw in UI view
    sf::View originalView = win.getView();
    win.setView(UIManager::get().getUIView(win));

    win.draw(background);
    win.draw(pointer);
    for (auto& r : renders) {
        win.draw(r.keyText);
        win.draw(r.labelText);
    }

    win.setView(originalView);
}