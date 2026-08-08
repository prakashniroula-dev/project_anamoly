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

void Palette::updateLayout(const sf::Vector2u& windowSize) {
    if (sprites.empty()) return;

    sf::Vector2f scale = Scale::getVec();
    float uniformScale = std::min(scale.x, scale.y);

    // Desired cell size and spacing, scaled by UI factor
    float baseCellSize = cellSize * uniformScale;
    float baseSpacing = spacing * uniformScale;

    int total = static_cast<int>(sprites.size());
    int startIdx = getStartIdx();
    int endIdx = getEndIdx();
    int numDisplayed = endIdx - startIdx;
    if (numDisplayed <= 0) {
        if (page > 0) { --page; updateLayout(windowSize); }
        return;
    }

    int rows = (numDisplayed + columns - 1) / columns;  // rows needed for this page

    float maxWidth  = 0.8f * static_cast<float>(windowSize.x);
    float maxHeight = 0.5f * static_cast<float>(windowSize.y);

    // Compute required grid size with base values
    float requiredWidth = columns * baseCellSize + (columns - 1) * baseSpacing;
    float requiredHeight = rows * baseCellSize + (rows - 1) * baseSpacing;

    // Fit within constraints, if needed (uniform scaling)
    float fitScale = 1.0f;
    if (requiredWidth > maxWidth)
        fitScale = std::min(fitScale, maxWidth / requiredWidth);
    if (requiredHeight > maxHeight)
        fitScale = std::min(fitScale, maxHeight / requiredHeight);

    float finalCellSize = baseCellSize * fitScale;
    float finalSpacing = baseSpacing * fitScale;

    // Actual grid dimensions
    float totalWidth = columns * finalCellSize + (columns - 1) * finalSpacing;
    float totalHeight = rows * finalCellSize + (rows - 1) * finalSpacing;

    // Centered horizontally, with a top offset
    float startX = (static_cast<float>(windowSize.x) - totalWidth) / 2.f;
    float startY = 20.f * uniformScale;

    // Place sprites and scale them to fit their cells
    for (int i = startIdx; i < endIdx; ++i) {
        int local = i - startIdx;
        int row = local / columns;
        int col = local % columns;
        float x = startX + col * (finalCellSize + finalSpacing);
        float y = startY + row * (finalCellSize + finalSpacing);
        sprites[i].setPosition({x, y});

        sf::FloatRect bounds = sprites[i].getLocalBounds();
        float maxDim = std::max(bounds.size.x, bounds.size.y);
        float s = (maxDim > 0) ? finalCellSize / maxDim : 1.f;
        sprites[i].setScale({s, s});
    }

    // Background (with padding)
    float pad = 10.f * uniformScale;
    background.setSize({totalWidth + pad * 2, totalHeight + pad * 2});
    background.setPosition({startX - pad, startY - pad});

    // Highlight selection
    if (selected >= startIdx && selected < endIdx) {
        const auto& spr = sprites[selected];
        highlight.setSize({finalCellSize, finalCellSize});
        highlight.setPosition(spr.getPosition());
        highlight.setFillColor(sf::Color::Transparent);
        highlight.setOutlineColor(sf::Color::Yellow);
        highlight.setOutlineThickness(2.f * uniformScale);
    } else {
        highlight.setFillColor(sf::Color::Transparent);
        highlight.setOutlineColor(sf::Color::Transparent);
        highlight.setOutlineThickness(0.f);
    }

    // Navigation buttons
    float btnSize = 30.f * uniformScale;
    float btnSpacing = 10.f * uniformScale;
    float totalBtnWidth = btnSize * 2 + btnSpacing;
    float btnY = background.getPosition().y + background.getSize().y + 10.f * uniformScale;

    prevBtn.setSize({btnSize, btnSize});
    nextBtn.setSize({btnSize, btnSize});
    float startBtnX = startX + (totalWidth - totalBtnWidth) / 2.f;
    prevBtn.setPosition({startBtnX, btnY});
    nextBtn.setPosition({startBtnX + btnSize + btnSpacing, btnY});

    // Center text inside buttons
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