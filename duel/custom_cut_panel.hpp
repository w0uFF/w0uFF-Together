#pragma once
// Custom named cut checkpoints: separate from the upstream fixed cut catalog.
// GPL-3.0; no network or Unreal calls here.
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace wouff::practice
{
class custom_cut_panel
{
    struct cut { std::string name; double x=0,y=0,z=0; };
public:
    template<class Teleport>
    void draw(const std::filesystem::path& path, double x, double y, double z,
              bool position_valid, Teleport&& teleport)
    {
        if (!loaded_) { loaded_=true; path_=path; load(); }
        if (!ImGui::CollapsingHeader("My custom cut checkpoints", ImGuiTreeNodeFlags_DefaultOpen))
            return;
        if (load_error_)
        {
            ImGui::TextColored(ImVec4(1,.4f,.4f,1),
                "Invalid custom checkpoints file: edits disabled to protect existing data.");
            return;
        }
        ImGui::TextDisabled("Create, rename, delete or teleport to a saved cut. Stored locally.");
        ImGui::InputText("Checkpoint name", &name_);
        if (ImGui::Button("Create checkpoint here"))
        {
            if (position_valid && valid_name(name_) && list_.size()<100 &&
                std::isfinite(x) && std::isfinite(y) && std::isfinite(z))
            {
                list_.push_back({name_,x,y,z});
                selected_=static_cast<int>(list_.size())-1;
                persist();
            }
        }
        if (list_.empty()) { ImGui::TextDisabled("No custom checkpoints saved."); }
        else
        {
            if (selected_<0 || selected_>=static_cast<int>(list_.size())) selected_=0;
            if (ImGui::BeginCombo("Saved checkpoints",list_[selected_].name.c_str()))
            {
                for (int i=0;i<static_cast<int>(list_.size());++i)
                    if (ImGui::Selectable(list_[i].name.c_str(),i==selected_))
                    {
                        selected_=i;
                        name_=list_[i].name;
                    }
                ImGui::EndCombo();
            }
            if (ImGui::Button("Rename selected") && valid_name(name_))
            {
                list_[selected_].name=name_;
                persist();
            }
            ImGui::SameLine();
            if (ImGui::Button("Update position here") && position_valid)
            {
                list_[selected_].x=x;list_[selected_].y=y;list_[selected_].z=z;
                persist();
            }
            if (ImGui::Button("Teleport to selected cut"))
            {
                const auto& p=list_[selected_];
                teleport(p.x,p.y,p.z);
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete selected...")) ImGui::OpenPopup("Confirm delete cut");
            if (ImGui::BeginPopupModal("Confirm delete cut",nullptr,ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::TextWrapped("Delete this local checkpoint? This cannot be undone.");
                if (ImGui::Button("Delete permanently"))
                {
                    list_.erase(list_.begin()+selected_);
                    selected_=list_.empty()?-1:0;
                    persist();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
        }
        if (save_error_) ImGui::TextColored(ImVec4(1,.4f,.4f,1),"Could not save custom checkpoints.");
        if (load_error_) ImGui::TextColored(ImVec4(1,.4f,.4f,1),"Custom checkpoints JSON invalid; file preserved.");
    }
private:
    static bool valid_name(const std::string& s) noexcept
    {
        return !s.empty() && s.size()<=80 && s.find_first_not_of(" \t\r\n")!=std::string::npos;
    }
    void load()
    {
        if (!std::filesystem::exists(path_)) return;
        try
        {
            std::ifstream in(path_);
            nlohmann::json doc; in>>doc;
            if (!doc.is_object() || doc.value("schema",0)!=1 ||
                !doc.contains("cuts") || !doc["cuts"].is_array()) throw std::runtime_error("schema");
            std::vector<cut> temp;
            for (const auto& item:doc["cuts"])
            {
                if (!item.is_object()) continue;
                cut c{item.at("name").get<std::string>(),
                    item.at("x").get<double>(),item.at("y").get<double>(),item.at("z").get<double>()};
                if (valid_name(c.name) && std::isfinite(c.x) &&
                    std::isfinite(c.y) && std::isfinite(c.z) && temp.size()<100)
                    temp.push_back(std::move(c));
            }
            list_=std::move(temp);
            if (!list_.empty()) {selected_=0; name_=list_[0].name;}
        }
        catch (...) { load_error_=true; }
    }
    void persist()
    {
        save_error_=false;
        try
        {
            std::filesystem::create_directories(path_.parent_path());
            nlohmann::json doc;
            doc["schema"]=1;
            doc["cuts"]=nlohmann::json::array();
            for (const auto& c:list_)
                doc["cuts"].push_back({{"name",c.name},{"x",c.x},{"y",c.y},{"z",c.z}});
            // A separate file prevents damage to the fixed practice catalog.
            const auto tmp=std::filesystem::path(path_.string()+".tmp");
            { std::ofstream out(tmp,std::ios::trunc);out<<doc.dump(2);out.flush();
              if (!out.good()) throw std::runtime_error("write"); }
            // Preserve original data in a backup before replacement.
            const auto backup=std::filesystem::path(path_.string()+".bak");
            if (std::filesystem::exists(backup)) std::filesystem::remove(backup);
            if (std::filesystem::exists(path_)) std::filesystem::rename(path_,backup);
            try {std::filesystem::rename(tmp,path_);}
            catch (...) {if (std::filesystem::exists(backup) && !std::filesystem::exists(path_))
                std::filesystem::rename(backup,path_); throw;}
        }
        catch (...) {save_error_=true;}
    }
    std::filesystem::path path_;
    std::vector<cut> list_;
    std::string name_="My cut";
    int selected_=-1;
    bool loaded_=false,save_error_=false,load_error_=false;
};
}
