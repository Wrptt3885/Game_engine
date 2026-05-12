#include "ui/UILabel.h"
#include "ui/UICanvas.h"

void UILabel::Draw() {
    UICanvas::DrawText(text, pos, color, fontSize, centered);
}
