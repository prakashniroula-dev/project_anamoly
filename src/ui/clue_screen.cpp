#include "clue_screen.hpp"
#include <ui/ui_manager.hpp>
#include <clue/clue_manager.hpp>
#include <debug/logs.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <algorithm>
#include <cmath>
#include <regex>
#include <map>
#include <story/story_manager.hpp>
#include <sound/sound_manager.hpp>
// Helper: map color names to sf::Color
static sf::Color parseColor(const std::string& str) {
    static const std::map<std::string, sf::Color> named = {
        {"red", sf::Color::Red}, {"green", sf::Color::Green},
        {"blue", sf::Color::Blue}, {"yellow", sf::Color::Yellow},
        {"white", sf::Color::White}, {"black", sf::Color::Black},
        {"orange", sf::Color(255, 165, 0)}, {"purple", sf::Color(128, 0, 128)}
    };
    // Hex: #RRGGBB or #RGB
    if (!str.empty() && str[0] == '#') {
        unsigned int r, g, b;
        if (str.size() == 7) {
            sscanf(str.c_str(), "#%02x%02x%02x", &r, &g, &b);
            return sf::Color(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
        } else if (str.size() == 4) {
            sscanf(str.c_str(), "#%1x%1x%1x", &r, &g, &b);
            r *= 17; g *= 17; b *= 17;
            return sf::Color(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
        }
    }
    auto it = named.find(str);
    if (it != named.end()) return it->second;
    return sf::Color::White;  // fallback
}

// Simple markup parser: supports [b], [i], [u], [color=...] and their closing tags.
// Returns a vector of (text, style, color) segments.
static std::vector<std::tuple<std::string, uint32_t, sf::Color>> parseMarkup(const std::string& input) {
    std::vector<std::tuple<std::string, uint32_t, sf::Color>> segments;
    std::string currentText;
    uint32_t currentStyle = sf::Text::Regular;
    sf::Color currentColor = sf::Color::Black;   // default for parchment background
    const sf::Color defaultColor = sf::Color::Black;

    size_t pos = 0;
    while (pos < input.size()) {
        if (input[pos] == '[') {
            size_t end = input.find(']', pos);
            if (end == std::string::npos) {
                // Treat as literal text
                currentText += input.substr(pos);
                break;
            }
            std::string tag = input.substr(pos + 1, end - pos - 1);
            pos = end + 1;

            // Flush any pending text with current style/color before changing
            if (!currentText.empty()) {
                segments.emplace_back(currentText, currentStyle, currentColor);
                currentText.clear();
            }

            if (tag == "b") {
                currentStyle |= sf::Text::Bold;
            } else if (tag == "/b") {
                currentStyle &= ~sf::Text::Bold;
            } else if (tag == "i") {
                currentStyle |= sf::Text::Italic;
            } else if (tag == "/i") {
                currentStyle &= ~sf::Text::Italic;
            } else if (tag == "u") {
                currentStyle |= sf::Text::Underlined;
            } else if (tag == "/u") {
                currentStyle &= ~sf::Text::Underlined;
            } else if (tag.rfind("color=", 0) == 0) {
                std::string colorStr = tag.substr(6);
                currentColor = parseColor(colorStr);
            } else if (tag == "/color") {
                currentColor = defaultColor;   // reset to initial default
            } else {
                // Unknown tag: treat as literal text (including brackets)
                currentText += '[' + tag + ']';
            }
        } else {
            currentText += input[pos++];
        }
    }
    if (!currentText.empty()) {
        segments.emplace_back(currentText, currentStyle, currentColor);
    }
    return segments;
}

ClueScreen::ClueScreen(const std::string& initialClueId)
    : m_titleText(m_font), m_backHint(m_font),
      m_upArrow(m_font), m_downArrow(m_font),
      m_initialClueId(initialClueId)
{
    m_font = UIManager::get().getFont();

    m_backdrop.setFillColor(sf::Color(0, 0, 0, 180));
    m_parchment.setFillColor(sf::Color(245, 235, 210));
    m_parchment.setOutlineColor(sf::Color(160, 120, 80));
    m_parchment.setOutlineThickness(3.f);

    m_titleText.setFont(m_font);
    m_titleText.setString("Journal");
    m_titleText.setCharacterSize(36);
    m_titleText.setStyle(sf::Text::Bold);
    m_titleText.setFillColor(sf::Color(80, 50, 30));

    m_backHint.setFont(m_font);
    m_backHint.setString("ESC to close");
    m_backHint.setCharacterSize(16);
    m_backHint.setFillColor(sf::Color(120, 100, 80));

    // Scroll buttons
    m_upButton.setFillColor(sf::Color(160, 120, 80, 180));
    m_upButton.setOutlineColor(sf::Color(80, 50, 30));
    m_upButton.setOutlineThickness(1.f);
    m_downButton = m_upButton;

    m_upArrow.setFont(m_font);
    m_upArrow.setString("▲");
    m_upArrow.setCharacterSize(20);
    m_upArrow.setFillColor(sf::Color(80, 50, 30));
    m_downArrow.setFont(m_font);
    m_downArrow.setString("▼");
    m_downArrow.setCharacterSize(20);
    m_downArrow.setFillColor(sf::Color(80, 50, 30));

    m_highlightRect.setFillColor(sf::Color::Transparent);
    m_highlightRect.setOutlineThickness(0.f);

    loadParchmentTexture();
}

void ClueScreen::loadParchmentTexture() {
    if (m_parchmentTexture.loadFromFile("assets/ui/parchment.png")) {
        m_parchment.setTexture(&m_parchmentTexture);
        m_parchment.setFillColor(sf::Color::White);
        m_parchment.setOutlineThickness(0.f);
    }
}

void ClueScreen::onEnter() {
    refreshList();
    if (!m_initialClueId.empty())
        showDetail(m_initialClueId);
    m_scrollOffset = 0;
    m_selectedIndex = 0;
}

void ClueScreen::onExit() {
  SoundManager::get().playSound("ui_back");
}

void ClueScreen::refreshList() {
    m_clueTexts.clear();
    const auto& ids = ClueManager::get().getDiscoveredClues();
    for (size_t i = 0; i < ids.size(); ++i) {
        const ClueInfo* info = ClueManager::get().getClueInfo(ids[i]);
        if (info) {
            std::string entry = std::to_string(i + 1) + ". " + info->title;
            sf::Text txt(m_font, entry, 22);
            txt.setFillColor(sf::Color(60, 40, 20));
            m_clueTexts.push_back(txt);
        }
    }
    m_selectedIndex = (m_clueTexts.empty() ? 0 : 0);
    m_scrollOffset = 0;
}

void ClueScreen::clampScrollOffset() {
    if (m_clueTexts.empty()) return;
    int total = static_cast<int>(m_clueTexts.size());
    int maxVisible = std::max(1, m_maxVisibleItems);
    if (total <= maxVisible) {
        m_scrollOffset = 0;
    } else {
        if (m_scrollOffset < 0) m_scrollOffset = 0;
        if (m_scrollOffset > total - maxVisible)
            m_scrollOffset = total - maxVisible;
    }
    // Ensure selected index is visible
    if (m_selectedIndex < m_scrollOffset)
        m_scrollOffset = m_selectedIndex;
    if (m_selectedIndex >= m_scrollOffset + maxVisible)
        m_scrollOffset = m_selectedIndex - maxVisible + 1;
    // re-clamp
    if (m_scrollOffset < 0) m_scrollOffset = 0;
    if (m_scrollOffset > total - maxVisible && total > maxVisible)
        m_scrollOffset = total - maxVisible;
    if (total <= maxVisible) m_scrollOffset = 0;
}

void ClueScreen::updateLayout(sf::RenderWindow& window) {
    sf::Vector2f winSize = sf::Vector2f(window.getSize());

    m_backdrop.setSize(winSize);
    m_backdrop.setPosition({0.f, 0.f});

    float pw = winSize.x * 0.7f;
    float ph = winSize.y * 0.7f;
    float px = (winSize.x - pw) / 2.f;
    float py = (winSize.y - ph) / 2.f;
    m_parchment.setSize({pw, ph});
    m_parchment.setPosition({px, py});

    sf::FloatRect titleBounds = m_titleText.getLocalBounds();
    m_titleText.setPosition({px + (pw - titleBounds.size.x) / 2.f, py + 20.f});

    if (m_state == State::Detail) {
        // We no longer have a separate detail title; the main title is the clue title.
        // Paragraphs are drawn starting below the title.
        float y = py + titleBounds.size.y + 40.f;  // gap after title
        const float bulletIndent = 30.f;
        const float textX = px + 50.f;
        const float lineHeight = 35.f;
        for (auto& para : m_paragraphs) {
            float x = textX;
            if (para.type == ParagraphType::Bullet) x += bulletIndent;
            // Draw each segment of this paragraph
            for (auto& seg : para.segments) {
                seg.setPosition({x, y});
                x += seg.getLocalBounds().size.x;  // advance by segment width
            }
            y += lineHeight;
        }
        sf::FloatRect hintBounds = m_backHint.getLocalBounds();
        m_backHint.setPosition({px + pw - hintBounds.size.x - 50.f, py + ph - hintBounds.size.y - 15.f});
        m_showScrollButtons = false;
    } else { // List
        float listStartY = py + 20.f + titleBounds.size.y + 30.f;
        float listEndY   = py + ph - 40.f;
        float lineHeight = 35.f;
        float availableHeight = listEndY - listStartY;
        int totalItems = static_cast<int>(m_clueTexts.size());
        m_maxVisibleItems = std::max(1, static_cast<int>(std::floor(availableHeight / lineHeight)));
        m_showScrollButtons = (totalItems > m_maxVisibleItems);

        float listWidth = pw - 60.f;
        if (m_showScrollButtons) listWidth -= 40.f;

        clampScrollOffset();

        float y = listStartY;
        int start = m_scrollOffset;
        int end = std::min(start + m_maxVisibleItems, totalItems);
        for (int i = start; i < end; ++i) {
            m_clueTexts[i].setPosition({px + 30.f, y});
            if (i == m_selectedIndex)
                m_clueTexts[i].setFillColor(sf::Color(200, 230, 255)); // lighter blue-white
            else
                m_clueTexts[i].setFillColor(sf::Color(60, 40, 20));    // dark brown
            y += lineHeight;
        }

        // ---- Store hit‑test rectangles for visible items ----
        m_itemRects.clear();
        start = m_scrollOffset;
        end = std::min(start + m_maxVisibleItems, (int)m_clueTexts.size());
        float rectWidth = pw - 60.f;   // match the text width (adjust as needed)
        float leftMargin = px + 30.f;  // same as text's X

        for (int i = start; i < end; ++i) {
            sf::Vector2f pos = m_clueTexts[i].getPosition();
            float rectY = pos.y - 4.f;          // some padding
            float rectHeight = lineHeight + 8.f; // same as highlight
            m_itemRects.emplace_back(
                sf::FloatRect(sf::Vector2f(leftMargin, rectY),
                              sf::Vector2f(rectWidth, rectHeight))
            );
        }

        if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_clueTexts.size()) {
            // Check if selected index is within visible range
            int start = m_scrollOffset;
            int end = std::min(start + m_maxVisibleItems, (int)m_clueTexts.size());
            if (m_selectedIndex >= start && m_selectedIndex < end) {
                const sf::Text& selText = m_clueTexts[m_selectedIndex];
                sf::Vector2f pos = selText.getPosition();
                float rectX = px + 20.f;           // left margin
                float rectWidth = pw - 40.f;       // full width minus margins
                float rectY = pos.y - 4.f;
                float rectHeight = lineHeight + 2.f;
                m_highlightRect.setSize({rectWidth, rectHeight});
                m_highlightRect.setPosition({rectX, rectY});
                m_highlightRect.setFillColor(sf::Color(40, 40, 60, 200)); // dark blue-grey
                m_highlightRect.setOutlineColor(sf::Color(100, 100, 150));
                m_highlightRect.setOutlineThickness(1.f);
            } else {
                // If selected is not visible (shouldn't happen due to clamping), hide it
                m_highlightRect.setFillColor(sf::Color::Transparent);
                m_highlightRect.setOutlineThickness(0.f);
            }
        } else {
            m_highlightRect.setFillColor(sf::Color::Transparent);
            m_highlightRect.setOutlineThickness(0.f);
        }

        // Scroll buttons
        if (m_showScrollButtons) {
            float btnSize = 30.f;
            float btnX = px + pw - 30.f - btnSize;
            float btnY = py + 80.f;
            m_upButton.setSize({btnSize, btnSize});
            m_upButton.setPosition({btnX, btnY});
            m_downButton.setSize({btnSize, btnSize});
            m_downButton.setPosition({btnX, btnY + btnSize + 10.f});

            sf::FloatRect upBounds = m_upArrow.getLocalBounds();
            m_upArrow.setPosition({btnX + (btnSize - upBounds.size.x)/2.f,
                                   btnY + (btnSize - upBounds.size.y)/2.f - 2.f});
            sf::FloatRect downBounds = m_downArrow.getLocalBounds();
            m_downArrow.setPosition({btnX + (btnSize - downBounds.size.x)/2.f,
                                     btnY + btnSize + 10.f + (btnSize - downBounds.size.y)/2.f - 2.f});
        }

        sf::FloatRect hintBounds = m_backHint.getLocalBounds();
        m_backHint.setPosition({px + pw - hintBounds.size.x - 50.f, py + ph - hintBounds.size.y - 15.f});
    }
}

void ClueScreen::draw(sf::RenderWindow& window) {
    updateLayout(window);
    window.draw(m_backdrop);
    window.draw(m_parchment);
    window.draw(m_titleText);

    if (m_state == State::Detail) {
        const float bulletRadius = 5.f;
        for (const auto& para : m_paragraphs) {
            // Draw each segment
            for (const auto& seg : para.segments) {
                window.draw(seg);
            }
            // Draw bullet if needed (centered vertically on the first line)
            if (para.type == ParagraphType::Bullet) {
                sf::CircleShape bullet(bulletRadius);
                bullet.setFillColor(sf::Color(80, 60, 40));
                // Use the first segment's position for bullet placement
                if (!para.segments.empty()) {
                    sf::Vector2f pos = para.segments.front().getPosition();
                    sf::FloatRect bounds = para.segments.front().getLocalBounds();
                    float circleX = pos.x - 25.f;
                    float circleY = pos.y + bounds.size.y / 2.f - bulletRadius;
                    bullet.setPosition({circleX, circleY});
                    window.draw(bullet);
                }
            }
        }
        window.draw(m_backHint);
    } else { // List
        int start = m_scrollOffset;
        int end = std::min(start + m_maxVisibleItems, static_cast<int>(m_clueTexts.size()));
        
        if (m_highlightRect.getFillColor().a > 0) {
            window.draw(m_highlightRect);
        }

        for (int i = start; i < end; ++i)
            window.draw(m_clueTexts[i]);

        if (m_showScrollButtons) {
            window.draw(m_upButton);
            window.draw(m_downButton);
            window.draw(m_upArrow);
            window.draw(m_downArrow);
        }
        window.draw(m_backHint);
    }
}

bool ClueScreen::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Escape) {
            if (m_state == State::Detail) goBack();
            else UIManager::get().popScreen();
            return true;
        }

        if (m_state == State::List) {
            if (key->code == sf::Keyboard::Key::Up) {
                if (m_selectedIndex > 0) --m_selectedIndex;
                clampScrollOffset();
                return true;
            }
            if (key->code == sf::Keyboard::Key::Down) {
                if (m_selectedIndex + 1 < (int)m_clueTexts.size()) ++m_selectedIndex;
                clampScrollOffset();
                return true;
            }
            if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space) {
                StoryManager::get().setFlag("disable_movement");
                if (!m_clueTexts.empty()) {
                  const auto& ids = ClueManager::get().getDiscoveredClues();
                  if (m_selectedIndex < (int)ids.size())
                  showDetail(ids[m_selectedIndex]);
                }
                return true;
              }
            } else { // Detail
              if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space) {
              StoryManager::get().setFlag("disable_movement");
                goBack();
                return true;
            }
        }
    }

    // Mouse wheel scroll
    if (const auto* scroll = event.getIf<sf::Event::MouseWheelScrolled>()) {
        if (m_state == State::List && m_showScrollButtons) {
            int delta = static_cast<int>(scroll->delta);
            m_scrollOffset -= delta;
            clampScrollOffset();
            return true;
        }
    }

    // Mouse clicks on scroll buttons
    if (const auto* btn = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (btn->button == sf::Mouse::Button::Left && m_state == State::List && m_showScrollButtons) {
            sf::Vector2f mousePos = window.mapPixelToCoords(
                sf::Vector2i(btn->position.x, btn->position.y));
            if (m_upButton.getGlobalBounds().contains(mousePos)) {
                --m_scrollOffset;
                clampScrollOffset();
                return true;
            }
            if (m_downButton.getGlobalBounds().contains(mousePos)) {
                ++m_scrollOffset;
                clampScrollOffset();
                return true;
            }
        }

        // Click on a list item to open detail
        if (btn->button == sf::Mouse::Button::Left && m_state == State::List) {
            // since mouseHover already captures selectedIndex
            if (m_selectedIndex < 0) return true;
            auto& ids = ClueManager::get().getDiscoveredClues();
            showDetail(ids[m_selectedIndex]);
            return true;
        }
    }

        // ---- Mouse hover on list items ----
    if (const auto* move = event.getIf<sf::Event::MouseMoved>()) {
        if (m_state == State::List) {
            sf::Vector2f mousePos = window.mapPixelToCoords(
                sf::Vector2i(move->position.x, move->position.y)
            );
            for (size_t i = 0; i < m_itemRects.size(); ++i) {
                if (m_itemRects[i].contains(mousePos)) {
                    // Map local index to global selected index
                    int globalIdx = m_scrollOffset + static_cast<int>(i);
                    if (globalIdx < (int)m_clueTexts.size()) {
                        m_selectedIndex = globalIdx;
                        return true;
                    }
                }
            }
        }
    }

    return true;
}

void ClueScreen::buildParagraphs(const ClueInfo& info) {
    m_paragraphs.clear();
    for (const auto& p : info.paragraphs) {
        // Parse the text into segments with markup
        auto parsed = parseMarkup(p.text);
        FormattedParagraph fp;
        fp.type = p.type;
        for (const auto& [text, style, color] : parsed) {
            sf::Text seg(m_font);
            seg.setString(text);
            seg.setCharacterSize(20);
            seg.setFillColor(color);
            seg.setStyle(style);
            fp.segments.push_back(seg);
        }
        // If no segments were produced (empty string), add an empty text to keep layout consistent
        if (fp.segments.empty()) {
            sf::Text empty(m_font);
            empty.setString("");
            fp.segments.push_back(empty);
        }
        m_paragraphs.push_back(fp);
    }
}

void ClueScreen::showDetail(const std::string& clueId) {
    SoundManager::get().playSound("inspect");
    const ClueInfo* info = ClueManager::get().getClueInfo(clueId);
    if (!info) return;
    m_state = State::Detail;
    m_currentDetail = *info;
    m_titleText.setString(info->title);   // set main title to clue title
    buildParagraphs(*info);
    m_backHint.setString("Esc : Go back");
}

void ClueScreen::goBack() {

  SoundManager::get().playSound("ui_back");
    m_state = State::List;
    refreshList();
    m_titleText.setString("Journal");     // revert main title
    m_backHint.setString("Esc : close");
    m_scrollOffset = 0;
    m_selectedIndex = 0;
}

void ClueScreen::update(float dt) {}