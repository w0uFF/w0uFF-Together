#pragma once
// w0uFF Together - standalone solo race timer alpha (UI / no chain changes).
// GPL-3.0. Retain the original UnchainedTogether author credits.
// Uses steady_clock so elapsed time remains accurate while INSERT is closed.
// No height or finish verification yet: do not submit results as ranked runs.
#include <imgui.h>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <algorithm>

namespace wouff::duel
{
class solo_race_panel
{
    using clock = std::chrono::steady_clock;
    struct practice_entry
    {
        std::uint64_t elapsed_seconds;
        bool timed;
        bool completed;
    };
public:
    bool running() const noexcept { return running_; }
    void draw()
    {
        const auto seconds=elapsed();
        if (running_ && timed_ && seconds >= duration_seconds_)
            stop(true);
        ImGui::SeparatorText("Solo Race / Time Trial - alpha");
        ImGui::TextWrapped("Solo timer runs without an opponent. This is a local practice tool, NOT a verified race, height leaderboard or game-finish detector.");
        if (!running_)
        {
            ImGui::RadioButton("First to finish (manual stop)", &mode_, 0);
            ImGui::RadioButton("Highest within time (timer only)", &mode_, 1);
            if (mode_ == 1)
            {
                ImGui::InputInt("Days", &days_);
                ImGui::InputInt("Hours", &hours_);
                ImGui::InputInt("Minutes", &minutes_);
                ImGui::InputInt("Seconds", &seconds_);
                days_ = std::clamp(days_,0,30);
                hours_ = std::clamp(hours_,0,23);
                minutes_ = std::clamp(minutes_,0,59);
                seconds_ = std::clamp(seconds_,0,59);
                ImGui::TextDisabled("Custom duration: 1 second to 30 days.");
            }
            const auto requested = static_cast<std::uint64_t>(days_)*86400ULL+
                static_cast<std::uint64_t>(hours_)*3600ULL+
                static_cast<std::uint64_t>(minutes_)*60ULL+
                static_cast<std::uint64_t>(seconds_);
            if (ImGui::Button("Start Solo Race") && (mode_==0 || requested>0))
            {
                timed_ = mode_==1;
                duration_seconds_=timed_?requested:0;
                start_=clock::now();
                running_=true;
            }
            if (mode_==1 && requested==0)
                ImGui::TextDisabled("Enter a duration greater than zero.");
        }
        else
        {
            char buffer[80];
            format_time(buffer,sizeof(buffer),seconds);
            ImGui::Text("Elapsed: %s",buffer);
            if (timed_)
            {
                const auto remaining=duration_seconds_>seconds?duration_seconds_-seconds:0;
                format_time(buffer,sizeof(buffer),remaining);
                ImGui::Text("Remaining: %s",buffer);
                const float progress=duration_seconds_?
                    static_cast<float>(seconds)/static_cast<float>(duration_seconds_):0.f;
                ImGui::ProgressBar(std::clamp(progress,0.f,1.f),ImVec2(-1,0));
            }
            if (ImGui::Button("Stop and save unranked timer"))
                stop(false);
        }
        ImGui::SeparatorText("Local session timer history (not persistent)");
        ImGui::TextDisabled("Height, map progress, opponent bars and verified finish detection: pending SDK integration.");
        for (auto it=entries_.rbegin();it!=entries_.rend();++it)
        {
            char time_text[80];
            format_time(time_text,sizeof(time_text),it->elapsed_seconds);
            ImGui::BulletText("%s | %s | %s", it->timed?"Timed":"Finish",
                              time_text,it->completed?"timer expired":"manual stop");
        }
        if (entries_.empty())
            ImGui::TextDisabled("No recorded attempts this session.");
    }
private:
    static void format_time(char* out,std::size_t capacity,std::uint64_t sec)
    {
        std::snprintf(out,capacity,"%llud %02llu:%02llu:%02llu",
            static_cast<unsigned long long>(sec/86400),
            static_cast<unsigned long long>((sec/3600)%24),
            static_cast<unsigned long long>((sec/60)%60),
            static_cast<unsigned long long>(sec%60));
    }
    std::uint64_t elapsed() const noexcept
    {
        if (!running_) return 0;
        const auto duration=std::chrono::duration_cast<std::chrono::seconds>(
            clock::now()-start_).count();
        return duration>0?static_cast<std::uint64_t>(duration):0;
    }
    void stop(bool completed)
    {
        if (!running_) return;
        const auto seconds=elapsed();
        entries_.push_back({seconds,timed_,completed});
        if (entries_.size()>20) entries_.erase(entries_.begin());
        running_=false;
    }
    clock::time_point start_{};
    bool running_=false,timed_=false;
    int mode_=1,days_=0,hours_=0,minutes_=20,seconds_=0;
    std::uint64_t duration_seconds_=0;
    std::vector<practice_entry> entries_;
};
}
