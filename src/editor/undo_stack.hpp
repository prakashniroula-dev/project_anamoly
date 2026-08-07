#pragma once
#include <vector>
#include <algorithm>

template<typename Change, void (*ApplyFunc)(Change&, bool forward)>
class UndoStack {
public:
    using Group = std::vector<Change>;

    void beginGroup() {
        recording = true;
        currentGroup.clear();
    }

    void addChange(const Change& ch) {
        if (recording) currentGroup.push_back(ch);
    }

    void commitGroup() {
        if (!recording) return;
        recording = false;
        if (currentGroup.empty()) return;
        undoStack.push_back(std::move(currentGroup));
        if (undoStack.size() > MAX_UNDO)
            undoStack.erase(undoStack.begin());
        redoStack.clear();
        currentGroup.clear();
    }

    void undo() {
        if (undoStack.empty()) return;
        Group group = std::move(undoStack.back());
        undoStack.pop_back();
        for (auto it = group.rbegin(); it != group.rend(); ++it)
            ApplyFunc(*it, false);
        redoStack.push_back(std::move(group));
    }

    void redo() {
        if (redoStack.empty()) return;
        Group group = std::move(redoStack.back());
        redoStack.pop_back();
        for (auto& ch : group)
            ApplyFunc(ch, true);
        undoStack.push_back(std::move(group));
    }

    bool canUndo() const { return !undoStack.empty(); }
    bool canRedo() const { return !redoStack.empty(); }
    void reset() {
        undoStack.clear();
        redoStack.clear();
        currentGroup.clear();
        recording = false;
    }

private:
    std::vector<Group> undoStack, redoStack;
    Group currentGroup;
    bool recording = false;
    static constexpr size_t MAX_UNDO = 6;
};