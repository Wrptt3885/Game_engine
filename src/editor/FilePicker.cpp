#include "editor/FilePicker.h"

#include <imgui.h>
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

namespace FilePicker {

static std::string ToLower(std::string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

Result Draw(const char*                     title,
            bool&                           open,
            std::string&                    browsePath,
            std::string&                    selectedFile,
            const std::vector<std::string>& extsLower,
            const char*                     applyLabel) {
    Result out;
    if (!open) return out;

    ImGui::SetNextWindowSize({520, 400}, ImGuiCond_FirstUseEver);
    ImVec2 ds = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos({ds.x * 0.5f, ds.y * 0.5f}, ImGuiCond_FirstUseEver, {0.5f, 0.5f});

    if (!ImGui::Begin(title, &open)) { ImGui::End(); return out; }

    ImGui::TextDisabled("Path:");
    ImGui::SameLine();
    ImGui::TextUnformatted(browsePath.c_str());
    ImGui::Separator();

    ImGui::BeginChild("##fpfiles", {0, -60}, true);

    fs::path cur(browsePath);
    if (cur.has_parent_path()) {
        if (ImGui::Selectable("[..] Go up")) {
            browsePath = cur.parent_path().string();
            selectedFile.clear();
        }
    }

    std::vector<fs::directory_entry> dirs, files;
    try {
        for (auto& e : fs::directory_iterator(browsePath)) {
            if (e.is_directory()) {
                dirs.push_back(e);
            } else if (e.is_regular_file()) {
                std::string ext = ToLower(e.path().extension().string());
                bool match = extsLower.empty();
                for (auto& want : extsLower) if (ext == want) { match = true; break; }
                if (match) files.push_back(e);
            }
        }
    } catch (...) {}

    auto byName = [](const fs::directory_entry& a, const fs::directory_entry& b) {
        return a.path().filename() < b.path().filename();
    };
    std::sort(dirs.begin(),  dirs.end(),  byName);
    std::sort(files.begin(), files.end(), byName);

    for (auto& d : dirs) {
        std::string label = "[+] " + d.path().filename().string();
        if (ImGui::Selectable(label.c_str(), false)) {
            browsePath = d.path().string();
            selectedFile.clear();
        }
    }
    for (auto& f : files) {
        std::string path = f.path().string();
        bool sel = (selectedFile == path);
        std::string label = "    " + f.path().filename().string();
        if (ImGui::Selectable(label.c_str(), sel))
            selectedFile = path;
    }

    ImGui::EndChild();
    ImGui::Separator();

    if (selectedFile.empty())
        ImGui::TextDisabled("No file selected");
    else
        ImGui::TextUnformatted(fs::path(selectedFile).filename().string().c_str());

    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 130.0f);

    bool canApply = !selectedFile.empty();
    if (!canApply) ImGui::BeginDisabled();
    if (ImGui::Button(applyLabel, {60, 0})) {
        out.applied = true;
        out.path    = selectedFile;
        selectedFile.clear();
        open = false;
    }
    if (!canApply) ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel", {60, 0})) {
        selectedFile.clear();
        open = false;
    }

    ImGui::End();
    return out;
}

} // namespace FilePicker
