#pragma once

#include <memory>
#include <string>
#include <vector>

class UIElement;
class UILabel;
class UIImage;

class UILayer {
public:
    std::shared_ptr<UILabel> CreateLabel(const std::string& name = "UILabel");
    std::shared_ptr<UIImage> CreateImage(const std::string& name = "UIImage");

    void Remove(const std::string& name);
    void Remove(std::shared_ptr<UIElement> el);
    std::shared_ptr<UIElement> Find(const std::string& name) const;

    const std::vector<std::shared_ptr<UIElement>>& GetElements() const { return m_Elements; }
    size_t GetCount() const { return m_Elements.size(); }

    void Render();

private:
    std::vector<std::shared_ptr<UIElement>> m_Elements;
};
