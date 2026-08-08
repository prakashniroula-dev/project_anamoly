#include "palette.hpp"
#include <core/scale.hpp>
#include <algorithm>
#include <cmath>

Palette::Palette(const sf::Font& f) : font(f), prevText(f), nextText(f) {
    prevBtn.setFillColor(sf::Color(60, 60, 60));
    prevBtn.setOutlineColor(sf::Color::White);
    prevBtn.setOutlineThickness(1.f);
    nextBtn = prevBtn;

    prevText.setFont(font);
    prevText.setString("<");
    prevText.setFillColor(sf::Color::White);
    nextText.setFont(font);
    nextText.setString(">");
    nextText.setFillColor(sf::Color::White);

    background.setFillColor(sf::Color(40, 40, 40, 220));
    background.setOutlineColor(sf::Color::White);
    background.setOutlineThickness(1.f);

    highlight.setFillColor(sf::Color::Transparent);
    highlight.setOutlineColor(sf::Color::Yellow);
    highlight.setOutlineThickness(2.f);
}

void Palette::setSprites(const std::vector<sf::Sprite>& s) {
    sprites = s;
}

void Palette::setLayout(int cols, int rows, float cSize, float sp) {
    columns = cols;
    rowsPerPage = rows;
    cellSize = cSize;
    spacing = sp;
}

void Palette::updateLayout(const sf::Vector2u& viewSize) {
    if (sprites.empty()) return;

    sf::Vector2f scale = Scale::getVec();
    float uniformScale = std::min(scale.x, scale.y);

    float spacingRatio = (cellSize > 0.f) ? (spacing / cellSize) : 0.1f;

    int total = static_cast<int>(sprites.size());
    int startIdx = getStartIdx();
    int endIdx = getEndIdx();
    int numDisplayed = endIdx - startIdx;
    if (numDisplayed <= 0) {
        if (page > 0) { --page; updateLayout(viewSize); }
        return;
    }

    int rows = (numDisplayed + columns - 1) / columns;

    // ---- Fixed UI element sizes (scaled) ----
    float topMargin = 10.f * uniformScale;   // from 20 to 10
    float pad          = 10.f * uniformScale;
    float gapToButtons = 10.f * uniformScale;
    float btnSize      = 30.f * uniformScale;
    float bottomMargin = 10.f * uniformScale;

    // ---- Overall limits (fractions of the view) ----
    
    float maxTotalWidth  = 0.9f * static_cast<float>(viewSize.x);   // 90% of window width
    float maxTotalHeight = 0.65f * static_cast<float>(viewSize.y);  // 65% of window height

    // Height reserved for grid background after subtracting UI elements
    float availableForBgHeight = maxTotalHeight - topMargin - gapToButtons - btnSize - bottomMargin;
    if (availableForBgHeight < 0) availableForBgHeight = 0;

    // Background itself has padding, so grid must be smaller
    float maxGridWidth  = maxTotalWidth  - 2.f * pad;
    float maxGridHeight = availableForBgHeight - 2.f * pad;
    if (maxGridWidth < 0)  maxGridWidth = 0;
    if (maxGridHeight < 0) maxGridHeight = 0;

    // Compute cell size from both constraints
    float cellByWidth  = (columns > 0) ? maxGridWidth / (columns + (columns - 1) * spacingRatio) : 0;
    float cellByHeight = (rows > 0)    ? maxGridHeight / (rows    + (rows    - 1) * spacingRatio) : 0;
    float cellSizeFinal = std::min(cellByWidth, cellByHeight);
    if (cellSizeFinal < 0) cellSizeFinal = 0;
    float spacingFinal = spacingRatio * cellSizeFinal;

    // Actual grid dimensions (without padding)
    float totalWidth  = columns * cellSizeFinal + (columns - 1) * spacingFinal;
    float totalHeight = rows    * cellSizeFinal + (rows    - 1) * spacingFinal;

    // Background dimensions (with padding)
    float bgWidth  = totalWidth  + 2.f * pad;
    float bgHeight = totalHeight + 2.f * pad;

    // Center the background in the view
    float viewW = static_cast<float>(viewSize.x);
    float viewH = static_cast<float>(viewSize.y);
    float startX = (viewW - bgWidth)  / 2.f;
    float startY = topMargin;   // fixed top margin

    // (Optional) Clamp to ensure it never goes off-screen
    // But with the constraints above, it should already be inside.

    // Place sprites inside the padded area
    float gridStartX = startX + pad;
    float gridStartY = startY + pad;
    for (int i = startIdx; i < endIdx; ++i) {
        int local = i - startIdx;
        int row = local / columns;
        int col = local % columns;
        float x = gridStartX + col * (cellSizeFinal + spacingFinal);
        float y = gridStartY + row * (cellSizeFinal + spacingFinal);
        sprites[i].setPosition({x, y});

        sf::FloatRect bounds = sprites[i].getLocalBounds();
        float maxDim = std::max(bounds.size.x, bounds.size.y);
        float s = (maxDim > 0) ? cellSizeFinal / maxDim : 1.f;
        sprites[i].setScale({s, s});
    }

    // Background
    background.setSize({bgWidth, bgHeight});
    background.setPosition({startX, startY});

    // Highlight
    if (selected >= startIdx && selected < endIdx) {
        const auto& spr = sprites[selected];
        highlight.setSize({cellSizeFinal, cellSizeFinal});
        highlight.setPosition(spr.getPosition());
        highlight.setFillColor(sf::Color::Transparent);
        highlight.setOutlineColor(sf::Color::Yellow);
        highlight.setOutlineThickness(2.f * uniformScale);
    } else {
        highlight.setFillColor(sf::Color::Transparent);
        highlight.setOutlineColor(sf::Color::Transparent);
        highlight.setOutlineThickness(0.f);
    }

    // Navigation buttons (centered below background)
    float btnSpacing = 10.f * uniformScale;
    float totalBtnWidth = btnSize * 2 + btnSpacing;
    float btnY = startY + bgHeight + gapToButtons;

    prevBtn.setSize({btnSize, btnSize});
    nextBtn.setSize({btnSize, btnSize});
    float startBtnX = startX + (bgWidth - totalBtnWidth) / 2.f;
    prevBtn.setPosition({startBtnX, btnY});
    nextBtn.setPosition({startBtnX + btnSize + btnSpacing, btnY});

    // Center text in buttons
    float charSize = btnSize * 0.6f;
    prevText.setCharacterSize(static_cast<unsigned>(charSize));
    nextText.setCharacterSize(static_cast<unsigned>(charSize));
    auto centerText = [&](sf::Text& text, const sf::RectangleShape& btn) {
        sf::FloatRect b = text.getLocalBounds();
        text.setPosition({
            btn.getPosition().x + (btn.getSize().x - b.size.x) / 2.f,
            btn.getPosition().y + (btn.getSize().y - b.size.y) / 2.f - 2.f
        });
    };
    centerText(prevText, prevBtn);
    centerText(nextText, nextBtn);
}

void Palette::draw(sf::RenderWindow& window) const {
    if (!visible || sprites.empty()) return;

    window.draw(background);
    int start = getStartIdx(), end = getEndIdx();
    for (int i = start; i < end; ++i)
        window.draw(sprites[i]);
    window.draw(highlight);
    window.draw(prevBtn);
    window.draw(nextBtn);
    window.draw(prevText);
    window.draw(nextText);
}

bool Palette::handleMousePress(const sf::Vector2f& mousePos) {
    if (!visible || sprites.empty()) return false;

    // Navigation buttons
    if (prevBtn.getGlobalBounds().contains(mousePos)) {
        prevPage();
        return true;
    }
    if (nextBtn.getGlobalBounds().contains(mousePos)) {
        nextPage();
        return true;
    }

    // Sprite selection
    int start = getStartIdx(), end = getEndIdx();
    for (int i = start; i < end; ++i) {
        if (sprites[i].getGlobalBounds().contains(mousePos)) {
            setSelected(i);
            return true;
        }
    }
    return false;
}

bool Palette::handleMouseScroll(float delta) {
    if (!visible || sprites.empty()) return false;
    int newSel = selected + static_cast<int>(delta);
    if (newSel >= 0 && newSel < static_cast<int>(sprites.size())) {
        setSelected(newSel);
        return true;
    }
    return false;
}

void Palette::setSelected(int idx) {
    if (idx < 0 || idx >= static_cast<int>(sprites.size())) return;
    selected = idx;
    // Ensure page shows this selection
    int perPage = columns * rowsPerPage;
    int newPage = idx / perPage;
    if (newPage != page) page = newPage;
}

int Palette::getSelected() const { return selected; }

void Palette::setVisible(bool v) { visible = v; }
bool Palette::isVisible() const { return visible; }

void Palette::nextPage() {
    int total = sprites.size();
    int perPage = columns * rowsPerPage;
    if ((page + 1) * perPage < total) {
        ++page;
        // ensure selected is on page
        int start = page * perPage;
        if (selected < start || selected >= start + perPage) {
            selected = start;
            if (selected >= total) selected = total - 1;
        }
    }
}

void Palette::prevPage() {
    if (page > 0) {
        --page;
        int perPage = columns * rowsPerPage;
        int start = page * perPage;
        if (selected < start || selected >= start + perPage) {
            selected = start;
        }
    }
}

int Palette::getStartIdx() const {
    int perPage = columns * rowsPerPage;
    return page * perPage;
}

int Palette::getEndIdx() const {
    int perPage = columns * rowsPerPage;
    return std::min(getStartIdx() + perPage, static_cast<int>(sprites.size()));
}