#include "editor/UndoManager.h"

#include <deque>

namespace {
std::deque<UndoManager::Entry> g_Undo;
std::deque<UndoManager::Entry> g_Redo;
const std::string              g_EmptyName;
} // namespace

void UndoManager::Push(std::string name,
                       std::function<void()> redo,
                       std::function<void()> undo) {
    Entry e{std::move(name), std::move(redo), std::move(undo)};
    g_Undo.push_back(std::move(e));
    while ((int)g_Undo.size() > MaxHistory) g_Undo.pop_front();
    g_Redo.clear();
}

bool UndoManager::Undo() {
    if (g_Undo.empty()) return false;
    Entry e = std::move(g_Undo.back());
    g_Undo.pop_back();
    if (e.undo) e.undo();
    g_Redo.push_back(std::move(e));
    return true;
}

bool UndoManager::Redo() {
    if (g_Redo.empty()) return false;
    Entry e = std::move(g_Redo.back());
    g_Redo.pop_back();
    if (e.redo) e.redo();
    g_Undo.push_back(std::move(e));
    return true;
}

void UndoManager::Clear() {
    g_Undo.clear();
    g_Redo.clear();
}

bool UndoManager::CanUndo() { return !g_Undo.empty(); }
bool UndoManager::CanRedo() { return !g_Redo.empty(); }

const std::string& UndoManager::TopUndoName() {
    return g_Undo.empty() ? g_EmptyName : g_Undo.back().name;
}
const std::string& UndoManager::TopRedoName() {
    return g_Redo.empty() ? g_EmptyName : g_Redo.back().name;
}
