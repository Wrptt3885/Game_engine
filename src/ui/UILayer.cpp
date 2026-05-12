#include "ui/UILayer.h"
#include "ui/UIElement.h"
#include "ui/UILabel.h"
#include "ui/UIImage.h"
#include "ui/UICanvas.h"
#include <algorithm>

static glm::vec2 ScreenCenter() {
    float w = UICanvas::GetScreenW();
    float h = UICanvas::GetScreenH();
    if (w <= 0.0f) w = 1280.0f;
    if (h <= 0.0f) h = 720.0f;
    return {w * 0.5f, h * 0.5f};
}

std::shared_ptr<UILabel> UILayer::CreateLabel(const std::string& name) {
    auto el  = std::make_shared<UILabel>();
    el->name = name;
    el->pos  = ScreenCenter();
    m_Elements.push_back(el);
    return el;
}

std::shared_ptr<UIImage> UILayer::CreateImage(const std::string& name) {
    auto el  = std::make_shared<UIImage>();
    el->name = name;
    el->pos  = ScreenCenter();
    m_Elements.push_back(el);
    return el;
}

void UILayer::Remove(const std::string& name) {
    m_Elements.erase(
        std::remove_if(m_Elements.begin(), m_Elements.end(),
            [&](const auto& e){ return e && e->name == name; }),
        m_Elements.end());
}

void UILayer::Remove(std::shared_ptr<UIElement> el) {
    m_Elements.erase(
        std::remove(m_Elements.begin(), m_Elements.end(), el),
        m_Elements.end());
}

std::shared_ptr<UIElement> UILayer::Find(const std::string& name) const {
    for (auto& e : m_Elements)
        if (e && e->name == name) return e;
    return nullptr;
}

void UILayer::Render() {
    for (auto& e : m_Elements)
        if (e && e->visible) e->Draw();
}
