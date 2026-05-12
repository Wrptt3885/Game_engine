#include "ui/UIImage.h"
#include "ui/UICanvas.h"

void UIImage::Draw() {
    UICanvas::DrawImage(texture, pos, size, tint);
}
