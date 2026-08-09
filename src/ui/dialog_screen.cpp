#include "dialog_screen.hpp"
#include <ui/ui_manager.hpp>
#include <debug/logs.hpp>
#include <story/story_helpers.hpp>

DialogScreen::DialogScreen(NPC *npc) : npc(npc), speakerText(font), dialogueText(font)
{
  if (!npc)
  {
    Log::error << "DialogScreen created with null NPC" << std::endl;
    return;
  }
  font = UIManager::get().getFont();
  speakerText.setFont(font);
  dialogueText.setFont(font);
  speakerText.setCharacterSize(28);
  dialogueText.setCharacterSize(20);
  background.setFillColor(sf::Color(0, 0, 0, 200));
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
  const DialogueLine *line = &dialogue[index];
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
  if (!npc->getType().dialogue.empty())
  {
    moveToLine(0); // start from index 0, but it will skip if invalid
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
}

bool DialogScreen::isLineValid(const DialogueLine *line) const
{
  if (!line)
    return false;
  return StoryHelpers::evaluateCondition(line->condition);
}

void DialogScreen::selectOption(size_t index)
{
  if (index >= visibleOptions.size())
    return;
  const DialogueLine *chosen = visibleOptions[index];
  // Execute action before moving
  StoryHelpers::executeAction(chosen->action);
  // Record the choice (using its ID if provided, else fallback)
  std::string choiceId = chosen->id.empty() ? chosen->text : chosen->id;
  StoryManager::get().addChoice(choiceId);
  // Move to the chosen line (which is the child)
  currentLine = chosen;
  refreshDisplay(); // this will show the chosen line's text and its options
}
bool DialogScreen::handleEvent(const sf::Event &event, sf::RenderWindow &window)
{
  // Ensure option rectangles are up‑to‑date for hit testing
  if (!visibleOptions.empty()) {
      updateOptionRects(window);
  }
  // --- Keyboard navigation ---
  if (const auto *key = event.getIf<sf::Event::KeyPressed>())
  {
    // ESC to close
    if (key->code == sf::Keyboard::Key::Escape)
    {
      UIManager::get().popScreen();
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
        return true;
      }
    }
    else
    {
      // No options – Enter/Space advances to next line
      if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space)
      {
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

void DialogScreen::updateOptionRects(const sf::RenderWindow& window)
{
    optionRects.clear();
    if (optionTexts.empty()) return;

    sf::Vector2f winSize = static_cast<sf::Vector2f>(window.getSize());
    const float PADDINGX = std::max(0.3f * winSize.x, 40.f);
    const float PADDINGY = std::max(0.05f * winSize.y, 20.f);
    const float LINE_HEIGHT = 60.f;
    const float SPEAKER_HEIGHT = 40.f;
    const float DIALOGUE_HEIGHT = 80.f;

    // Compute boxHeight and textY (same as in draw)
    float boxHeight = PADDINGY * 2 + SPEAKER_HEIGHT + 5.f + DIALOGUE_HEIGHT + 5.f;
    if (!optionTexts.empty()) {
        boxHeight += 5.f + optionTexts.size() * LINE_HEIGHT;
    } else {
        boxHeight += 5.f;
    }
    boxHeight = std::max(boxHeight, 120.f);
    boxHeight = std::min(boxHeight, winSize.y * 0.6f);
    float boxY = winSize.y - boxHeight;
    float textY = boxY + PADDINGY + SPEAKER_HEIGHT + 5.f + DIALOGUE_HEIGHT + 5.f;

    // Uniform button width (based on longest text)
    float maxTextWidth = 0.f;
    for (const auto& opt : optionTexts) {
        float w = opt.getLocalBounds().size.x;
        if (w > maxTextWidth) maxTextWidth = w;
    }
    const float BTN_PADDING_X = 30.f;
    const float BTN_PADDING_Y = 12.f;
    float btnWidth = maxTextWidth + BTN_PADDING_X * 2 + 60.f; // extra for outline (as you set)

    for (size_t i = 0; i < optionTexts.size(); ++i) {
        const sf::Text& opt = optionTexts[i];
        float textHeight = opt.getLocalBounds().size.y;
        float btnHeight = textHeight + BTN_PADDING_Y * 2;
        float btnX = winSize.x - PADDINGX - btnWidth;
        float btnY = textY + i * LINE_HEIGHT;
        optionRects.push_back(sf::FloatRect(sf::Vector2f(btnX, btnY), sf::Vector2f(btnWidth, btnHeight)));
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
  updateOptionRects(window);
  uiView = window.getDefaultView();
  uiView.setSize(sf::Vector2f(window.getSize()));
  uiView.setCenter(sf::Vector2f(window.getSize().x / 2.f, window.getSize().y / 2.f));
  window.setView(uiView);
  const sf::View& defaultView = uiView;

  sf::Vector2f winSize = defaultView.getSize();

  // ----- Layout constants (tweaked for bigger buttons) -----
  const float PADDINGX = std::max(0.3f * winSize.x, 40.f);
  const float PADDINGY = std::max(0.05f * winSize.y, 20.f);
  const float LINE_HEIGHT = 60.f;          // increased from 30
  const float SPEAKER_HEIGHT = 40.f;
  const float DIALOGUE_HEIGHT = 80.f;

  // ----- Compute box height (includes options) -----
  float boxHeight = PADDINGY * 2;
  boxHeight += SPEAKER_HEIGHT + 5.f;
  boxHeight += DIALOGUE_HEIGHT + 5.f;
  if (!optionTexts.empty())
  {
    boxHeight += 5.f;
    boxHeight += optionTexts.size() * LINE_HEIGHT;   // each option uses LINE_HEIGHT as vertical step
  }
  else
  {
    boxHeight += 5.f;
  }
  boxHeight = std::max(boxHeight, 120.f);
  boxHeight = std::min(boxHeight, winSize.y * 0.6f);

  float boxX = 0.f;
  float boxY = winSize.y - boxHeight;

  drawBlackGradient(window, boxX, boxY, winSize.x, boxHeight);

  // ----- Speaker & Dialogue (left‑aligned) -----
  float textX = boxX + PADDINGX;
  float textY = boxY + PADDINGY;

  speakerText.setPosition({textX, textY});
  window.draw(speakerText);

  textY += SPEAKER_HEIGHT + 5.f;
  dialogueText.setPosition({textX, textY});
  window.draw(dialogueText);

  // ----- Options (right‑aligned, uniform width) -----
  optionRects.clear();
  if (!optionTexts.empty())
  {
    // 1) Find the maximum text width among all options
    float maxTextWidth = 0.f;
    for (const auto& opt : optionTexts)
    {
      float w = opt.getLocalBounds().size.x;
      if (w > maxTextWidth) maxTextWidth = w;
    }

    // 2) Button padding (make it generous)
    const float BTN_PADDING_X = 30.f;   // increased from 20
    const float BTN_PADDING_Y = 12.f;   // increased from 10
    float btnWidth = maxTextWidth + BTN_PADDING_X * 2 + 60.f; // extra for outline
    float btnHeight = 0.f;  // will be computed per button (text height + padding)

    textY += DIALOGUE_HEIGHT + 5.f;   // gap before first option

    for (size_t i = 0; i < optionTexts.size(); ++i)
    {
      sf::Text &opt = optionTexts[i];
      sf::FloatRect textBounds = opt.getLocalBounds();
      float textHeight = textBounds.size.y;

      // Button height based on this option's text height (all will be similar)
      float btnHeight_i = textHeight + BTN_PADDING_Y * 2;

      // Right‑align: button X = window width - padding - button width
      float btnX = winSize.x - PADDINGX - btnWidth;
      float btnY = textY + i * LINE_HEIGHT;   // spacing = LINE_HEIGHT

      // Store rectangle for hit testing
      optionRects.push_back(sf::FloatRect(sf::Vector2f(btnX, btnY), sf::Vector2f(btnWidth, btnHeight_i)));

      // Draw button background
      sf::RectangleShape btnShape({btnWidth, btnHeight_i});
      btnShape.setPosition({btnX, btnY});
      btnShape.setOutlineThickness(2.f);

      // Selected option: different style
      if (i == highlightedOption)
      {
        btnShape.setFillColor(sf::Color::White); // blue highlight
        btnShape.setOutlineColor(sf::Color::Black);
        opt.setFillColor(sf::Color::Black);
      }
      else
      {
        btnShape.setFillColor(sf::Color(40, 40, 40, 220)); // dark gray
        btnShape.setOutlineColor(sf::Color(120, 120, 120));
        opt.setFillColor(sf::Color::White);
        opt.setScale({1.f, 1.f});
      }
      window.draw(btnShape);

      // Position text centered inside button
      float textPosX = btnX + (btnWidth - textBounds.size.x) / 2.f;
      float textPosY = btnY + (btnHeight_i - textHeight) / 2.f;
      opt.setPosition({textPosX, textPosY});
      window.draw(opt);
    }
  }
}

void DialogScreen::onExit()
{
  // optional cleanup
}