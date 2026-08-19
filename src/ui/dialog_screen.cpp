#include "dialog_screen.hpp"
#include <ui/ui_manager.hpp>
#include <debug/logs.hpp>
#include <story/story_helpers.hpp>
#include <sound/sound_manager.hpp>
#include <cstdlib>
#include <entities/npc_manager.hpp>

void DialogScreen::show(const DialogLine& line) {
  auto registeredType = NPCManager::get().getRegisteredType("dummy");
  if ( registeredType == nullptr ) {
    Log::error << "DialogScreen::show: NPC type 'dummy' not registered!" << std::endl;
    return;
  }
  registeredType->dialogue.clear();
  registeredType->dialogue.push_back(line);
  if (NPCManager::get().getNPC("dummy") == nullptr) {
    NPCManager::get().createNPC(
      SpawnProps{
        Characters::Fighter_Detective, 1.f, 0.f, false, false, "dummy", "dummy", "", {}
      }, sf::Vector2f(0.f, 0.f)
    );
  }
  UIManager::get().pushScreen(std::make_unique<DialogScreen>(NPCManager::get().getNPC("dummy"), line.id));
}

DialogScreen::DialogScreen(NPC *npc, bool allowEscape) : npc(npc), speakerText(font), dialogueText(font), m_allowEscape(allowEscape)
{
  if (!npc)
  {
    Log::error << "DialogScreen created with null NPC" << std::endl;
    return;
  }
  font = UIManager::get().getFont();
  speakerText.setFont(font);
  dialogueText.setFont(font);
  speakerText.setStyle(sf::Text::Bold | sf::Text::Underlined);
  speakerText.setCharacterSize(28);
  dialogueText.setCharacterSize(20);
  background.setFillColor(sf::Color(0, 0, 0, 200));
}

DialogScreen::DialogScreen(NPC *npc, const std::string &dialogueId, bool allowEscape) : DialogScreen(npc, allowEscape)
{
  if (!npc) return;
  // Find the dialogue line with the given ID
  Log::info << "DialogScreen: DialogID = " << dialogueId << "\n";
  const auto &dialogue = npc->getType().dialogue;
  for (size_t i = 0; i < dialogue.size(); ++i)
  {
    if (dialogue[i].id == dialogueId)
    {
      Log::info << "DialogScreen: Found dialogue ID '" << dialogueId << "' at index " << i << std::endl;
      moveToLine(static_cast<int>(i));
      return;
    }
  }
  Log::error << "DialogScreen: Dialogue ID '" << dialogueId << "' not found for NPC" << std::endl;
}

// Add this method
void DialogScreen::moveToLine(int index)
{
  const auto &dialogue = npc->getType().dialogue;
  if (index < 0 || index >= (int)dialogue.size())
  {
    // End dialogue
    UIManager::get().popScreen();
    return;
  }
  const DialogLine *line = &dialogue[index];
  if (!isLineValid(line))
  {
    // If the line is invalid, try to advance to its nextIndex (if any)
    if (line->nextIndex >= 0 && line->nextIndex < (int)dialogue.size())
    {
      moveToLine(line->nextIndex);
      Log::info << "Skipping invalid dialogue line at index " << index << ", moving to nextIndex " << line->nextIndex << std::endl;
    }
    else
    {
      UIManager::get().popScreen();
    }
    return;
  }
  currentLine = line;
  refreshDisplay();
}

// Update onEnter()
void DialogScreen::onEnter()
{
  Log::info << "DialogScreen onEnter() for NPC!" << std::endl;
  if (!npc->getType().dialogue.empty())
  {
    if (currentLine == nullptr) {
      moveToLine(0);
    }
  }
  else
  {
    UIManager::get().popScreen();
  }
}

// Update advanceToNextLine()
void DialogScreen::advanceToNextLine()
{
  if (!currentLine)
  {
    UIManager::get().popScreen();
    return;
  }
  if (currentLine->nextIndex >= 0)
  {
    moveToLine(currentLine->nextIndex); // validates the new line
  }
  else
  {
    UIManager::get().popScreen();
  }
}

void DialogScreen::refreshDisplay()
{
  if (!currentLine)
  {
    UIManager::get().popScreen();
    return;
  }
  speakerText.setString(currentLine->speaker);
  dialogueText.setString(currentLine->text);

  // Build visible options (filter by condition)
  visibleOptions.clear();
  for (const auto &opt : currentLine->options)
  {
    if (isLineValid(&opt))
      visibleOptions.push_back(&opt);
  }

  // Create option texts
  optionTexts.clear();
  optionTexts.reserve(visibleOptions.size());
  for (size_t i = 0; i < visibleOptions.size(); ++i)
  {
    sf::Text optText(font);
    optText.setString(std::to_string(i + 1) + ". " + visibleOptions[i]->text);
    optText.setCharacterSize(18);
    optText.setFillColor(sf::Color::White);
    optionTexts.push_back(optText);
  }

  highlightedOption = 0; // reset highlight to first option
  std::string soundKey = currentLine->soundKey;
  if (currentLine && currentLine->speaker == "Player") {
    if (soundKey.empty()) {
      soundKey = "null";
    }
  };

  if (currentLine && !soundKey.empty()) {
    if(soundKey == "null") return;
    SoundManager::get().playSound(soundKey);
  } else {
      // fallback random
      int idx = rand() % 4;
    SoundManager::get().playSound("npc_talk_continue" + std::to_string(idx));
  }
}

bool DialogScreen::isLineValid(const DialogLine *line) const
{
  if (!line)
    return false;
  return StoryHelpers::evaluateCondition(line->condition);
}

void DialogScreen::selectOption(size_t index)
{
  if (index >= visibleOptions.size())
    return;
  const DialogLine *chosen = visibleOptions[index];
  // Execute action before moving
  StoryHelpers::executeAction(chosen->action);
  // Record the choice (using its ID if provided, else fallback)
  std::string choiceId = chosen->id.empty() ? chosen->text : chosen->id;
  StoryManager::get().addChoice(choiceId);

  StoryHelpers::executeAction(currentLine->action); // execute current line's action as well
  // Move to the chosen line (which is the child)
  currentLine = chosen;
  refreshDisplay(); // this will show the chosen line's text and its options
}
bool DialogScreen::handleEvent(const sf::Event &event, sf::RenderWindow &window)
{
  // Ensure option rectangles are up‑to‑date for hit testing
  if (!visibleOptions.empty()) {
      updateLayout(window);
  }
  // --- Keyboard navigation ---
  if (const auto *key = event.getIf<sf::Event::KeyPressed>())
  {
    // ESC to close
    if (key->code == sf::Keyboard::Key::Escape)
    {
      if (m_allowEscape) {
        UIManager::get().popScreen();
      }
      return true;
    }

    if (!visibleOptions.empty())
    {
      // Arrow keys to change highlight
      if (key->code == sf::Keyboard::Key::Up)
      {
        highlightedOption = (highlightedOption - 1 + visibleOptions.size()) % visibleOptions.size();
        return true;
      }
      if (key->code == sf::Keyboard::Key::Down)
      {
        highlightedOption = (highlightedOption + 1) % visibleOptions.size();
        return true;
      }
      // Enter or Space to select highlighted option
      if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space)
      {
        selectOption(highlightedOption);
        StoryManager::get().setFlag("disable_movement");
        return true;
      }
    }
    else
    {
      // No options – Enter/Space advances to next line
      if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space)
      {
        StoryManager::get().setFlag("disable_movement");
        advanceToNextLine();
        return true;
      }
    }
  }

  // --- Mouse click on options (using stored rectangles) ---
  if (const auto *btn = event.getIf<sf::Event::MouseButtonPressed>())
  {
    if (btn->button == sf::Mouse::Button::Left && !visibleOptions.empty())
    {
      sf::Vector2f mousePos = window.mapPixelToCoords(
          sf::Vector2i(btn->position.x, btn->position.y),
          uiView);
      for (size_t i = 0; i < optionRects.size(); ++i)
      {
        if (optionRects[i].contains(mousePos))
        {
          selectOption(i);
          return true;
        }
      }
    }
  }

    // --- Mouse hover to highlight option ---
  if (const auto* move = event.getIf<sf::Event::MouseMoved>()) {
      if (!visibleOptions.empty()) {
          sf::Vector2f mousePos = window.mapPixelToCoords(
              sf::Vector2i(move->position.x, move->position.y),
              uiView
          );
          for (size_t i = 0; i < optionRects.size(); ++i) {
              if (optionRects[i].contains(mousePos)) {
                  highlightedOption = i;
                  return true;   // event consumed
              }
          }
          // Optionally, if mouse leaves all buttons, do nothing (keep last highlight)
          // or reset to 0? We'll keep the last to avoid flickering.
      }
  }

  return false;
}

void DialogScreen::updateLayout(sf::RenderWindow& window)
{
    // Use the exact same constants and logic as in your fixed draw()
    sf::Vector2f winSize = sf::Vector2f(window.getSize());
    uiView = UIManager::getUIView(window); // ensure uiView is set for event mapping
    window.setView(uiView); // set the view for layout calculations

    const float PADDINGX = std::max(0.3f * winSize.x, 40.f);
    const float PADDINGY = std::max(0.05f * winSize.y, 20.f);
    const float LINE_HEIGHT = 30.f;          // as in draw
    const float SPEAKER_HEIGHT = 50.f;       // as in draw
    const float DIALOGUE_HEIGHT = 60.f;      // as in draw

    // ----- Compute box dimensions (matches draw) -----
    float boxHeight = PADDINGY * 2;
    boxHeight += SPEAKER_HEIGHT + 5.f;
    boxHeight += DIALOGUE_HEIGHT + 5.f;
    boxHeight = std::max(boxHeight, 60.f);
    boxHeight = std::min(boxHeight, winSize.y * 0.6f);

    float boxX = 0.f;
    float boxY = winSize.y - boxHeight;

    // Store for drawing
    boxY = std::floor(boxY); // optional: align to pixel grid
    m_boxRect = sf::FloatRect(sf::Vector2f(boxX, boxY), sf::Vector2f(winSize.x, boxHeight));

    // ----- Speaker & dialogue positions -----
    float textX = boxX + PADDINGX;
    float textY = boxY + PADDINGY;
    textX = std::floor(textX); // optional: align to pixel grid
    textY = std::floor(textY);
    m_speakerPos = sf::Vector2f(textX, textY);
    m_dialoguePos = sf::Vector2f(textX, std::floor(textY + SPEAKER_HEIGHT + 5.f));

    // ----- Options (right‑aligned, uniform width) -----
    optionRects.clear();
    if (!optionTexts.empty())
    {
        // 1) Find max text width
        float maxTextWidth = 0.f;
        for (const auto& opt : optionTexts) {
            float w = opt.getLocalBounds().size.x;
            if (w > maxTextWidth) maxTextWidth = w;
        }

        const float BTN_PADDING_X = 30.f;
        const float BTN_PADDING_Y = 12.f;
        const float BTN_HEIGHT = LINE_HEIGHT + 40.f;   // spacing between options (as in draw)
        float btnWidth = maxTextWidth + BTN_PADDING_X * 2 + 60.f; // extra for outline

        // Options start above the box (matches draw's `textY = boxY - optionTexts.size() * 60.f;`)
        float optStartY = boxY - optionTexts.size() * 60.f;   // 60 is the fixed gap used in draw

        for (size_t i = 0; i < optionTexts.size(); ++i)
        {
            const sf::Text& opt = optionTexts[i];
            sf::FloatRect textBounds = opt.getLocalBounds();
            float textHeight = textBounds.size.y;
            float btnHeight = textHeight + BTN_PADDING_Y * 2;

            float btnX = winSize.x / 2.f - btnWidth / 2.f;
            float btnY = optStartY + i * BTN_HEIGHT;
            btnX = std::floor(btnX);
            btnY = std::floor(btnY);

            optionRects.push_back(sf::FloatRect(sf::Vector2f(btnX, btnY),
                                                sf::Vector2f(btnWidth, btnHeight)));
        }
    }
}

void DialogScreen::update(float dt)
{
  // nothing
}

void drawBlackGradient(sf::RenderWindow& win, float x, float y, float width, float height) {
    int heightInt = static_cast<int>(std::ceil(height));
    if (heightInt <= 0) return;  // safety guard
    sf::Image gradientImage;
    gradientImage.resize({1, static_cast<unsigned int>(heightInt)});
    for (int py = 0; py < heightInt; ++py) {
        float t = static_cast<float>(py) / heightInt;   // 0 at top, 1 at bottom
        unsigned alpha = static_cast<unsigned>(255.f * t);
        gradientImage.setPixel({0, static_cast<unsigned int>(py)}, sf::Color(0, 0, 0, alpha));
    }
    sf::Texture tex;
    tex.loadFromImage(gradientImage);
    sf::Sprite gradient(tex);
    gradient.setPosition({x, y});
    gradient.setTextureRect({{0, 0}, {static_cast<int>(width), heightInt}});
    win.draw(gradient);
}

void DialogScreen::draw(sf::RenderWindow &window)
{
  updateLayout(window);

 drawBlackGradient(window, m_boxRect.position.x, m_boxRect.position.y,
                      m_boxRect.size.x, m_boxRect.size.y);

  speakerText.setPosition(m_speakerPos);
  window.draw(speakerText);
  dialogueText.setPosition(m_dialoguePos);
  window.draw(dialogueText);

  // 5. Draw options using the precomputed optionRects
  for (size_t i = 0; i < optionTexts.size(); ++i)
  {
      const sf::FloatRect& rect = optionRects[i];
      sf::Text& opt = optionTexts[i];

      // Button background
      sf::RectangleShape btnShape(rect.size);
      btnShape.setPosition(rect.position);
      btnShape.setOutlineThickness(2.f);

      if (i == highlightedOption)
      {
          btnShape.setFillColor(sf::Color::White);
          btnShape.setOutlineColor(sf::Color::Black);
          opt.setFillColor(sf::Color::Black);
      }
      else
      {
          btnShape.setFillColor(sf::Color(40, 40, 40, 220));
          btnShape.setOutlineColor(sf::Color(120, 120, 120));
          opt.setFillColor(sf::Color::White);
      }
      window.draw(btnShape);

      // Center text inside the button
      sf::FloatRect textBounds = opt.getLocalBounds();
      float textX = rect.position.x + (rect.size.x - textBounds.size.x) / 2.f;
      float textY = rect.position.y + (rect.size.y - textBounds.size.y) / 2.f;
      textX = std::floor(textX); // optional: align to pixel grid
      textY = std::floor(textY);
      opt.setPosition({textX, textY});
      window.draw(opt);
  }
}

void DialogScreen::onExit() {
  if (npc) {
      Log::info << "Exiting DialogScreen for NPC!" << std::endl;
        npc->dialogueEnded();
    } else {
      Log::info << "Exiting DialogScreen with null NPC!" << std::endl;
    }
}