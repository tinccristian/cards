#pragma once

#include <string>
#include <vector>

enum class WindowMode {
    Windowed = 0,
    Fullscreen = 1,
    Borderless = 2
};

enum class OptionsSection {
    Display,
    Sounds
};

struct ResolutionOption {
    int width  = 0;
    int height = 0;
};

struct AppSettings {
    int        resolutionIndex = 0;
    WindowMode windowMode      = WindowMode::Windowed;
    int        monitorIndex    = 0;
    bool       vsyncEnabled    = true;
    bool       showFps         = false;
    int        masterVolume    = 100;
};

bool operator==(const AppSettings& lhs, const AppSettings& rhs);
bool operator!=(const AppSettings& lhs, const AppSettings& rhs);

class SettingsManager {
public:
    static AppSettings loadOrCreate(std::string& warning);
    static bool save(const AppSettings& settings, std::string& error);

    static void clamp(AppSettings& settings);
    static void applyDisplaySettings(const AppSettings& settings);
    static void applyAudioSettings(const AppSettings& settings);

    static const std::vector<ResolutionOption>& resolutionOptions();
    static std::string resolutionLabel(int resolutionIndex);
    static const char* windowModeLabel(WindowMode windowMode);
    static std::string settingsFilePath();

private:
    static AppSettings defaultSettings();
};
