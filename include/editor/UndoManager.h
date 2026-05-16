#pragma once

#include <functional>
#include <string>
#include <vector>

// Simple history-based undo/redo. Caller mutates state, then pushes a pair
// of (redo, undo) closures. Redo re-applies, undo reverts. Stacks are
// trimmed to MaxHistory.
class UndoManager {
public:
    struct Entry {
        std::string           name;
        std::function<void()> redo;
        std::function<void()> undo;
    };

    static void Push(std::string name,
                     std::function<void()> redo,
                     std::function<void()> undo);

    static bool Undo();
    static bool Redo();
    static void Clear();

    static bool CanUndo();
    static bool CanRedo();
    static const std::string& TopUndoName();
    static const std::string& TopRedoName();

    static constexpr int MaxHistory = 100;
};
