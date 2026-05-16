#pragma once

#include <string>
#include <vector>

// Shared file-browser dialog used by the editor's pickers (model import,
// texture, script, audio, animation clip). Caller owns the open/browse/selected
// state; this function draws the window and reports when the user clicks Apply.
namespace FilePicker {

struct Result {
    bool        applied = false;  // user clicked Apply this frame
    std::string path;             // absolute path of the chosen file (valid when applied)
};

// Draws a modal-style file picker window. The dialog auto-closes on Apply or
// Cancel by setting `open = false`. Pass extensions as lowercase including the
// leading dot (e.g. ".png").
Result Draw(const char*                     title,
            bool&                           open,
            std::string&                    browsePath,
            std::string&                    selectedFile,
            const std::vector<std::string>& extsLower,
            const char*                     applyLabel = "Apply");

} // namespace FilePicker
