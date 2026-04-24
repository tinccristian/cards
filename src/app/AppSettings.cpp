#include "AppSettings.h"

#include "config/Defines.h"
#include "raylib.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>

using nlohmann::json;

namespace {
std::filesystem::path settingsDirectoryPath() {
    const char* appData = std::getenv("APPDATA");
    if (appData != nullptr && appData[0] != '\0') {
        return std::filesystem::path(appData) / SettingsConfig::AppFolderName;
    }
    return std::filesystem::current_path() / SettingsConfig::AppFolderName;
}

int defaultResolutionIndex() {
    const auto& options = SettingsManager::resolutionOptions();
    for (int index = 0; index < static_cast<int>(options.size()); ++index) {
        if (options[index].width == WindowConfig::Width
            && options[index].height == WindowConfig::Height) {
            return index;
        }
    }
    return 0;
}

int clampedMonitorIndex(int requestedMonitor) {
    const int monitorCount = std::max(1, GetMonitorCount());
    return std::clamp(requestedMonitor, 0, monitorCount - 1);
}

void centerWindowOnMonitor(int monitorIndex, int width, int height) {
    const Vector2 monitorPosition = GetMonitorPosition(monitorIndex);
    const int monitorWidth = GetMonitorWidth(monitorIndex);
    const int monitorHeight = GetMonitorHeight(monitorIndex);

    const int windowX = static_cast<int>(monitorPosition.x) + (monitorWidth - width) / 2;
    const int windowY = static_cast<int>(monitorPosition.y) + (monitorHeight - height) / 2;

    SetWindowPosition(windowX, windowY);
}
} // namespace

bool operator==(const AppSettings& lhs, const AppSettings& rhs) {
    return lhs.resolutionIndex == rhs.resolutionIndex
        && lhs.windowMode == rhs.windowMode
        && lhs.monitorIndex == rhs.monitorIndex
        && lhs.vsyncEnabled == rhs.vsyncEnabled
        && lhs.showFps == rhs.showFps
        && lhs.masterVolume == rhs.masterVolume;
}

bool operator!=(const AppSettings& lhs, const AppSettings& rhs) {
    return !(lhs == rhs);
}

AppSettings SettingsManager::defaultSettings() {
    AppSettings settings;
    settings.resolutionIndex = defaultResolutionIndex();
    settings.windowMode = WindowMode::Windowed;
    settings.monitorIndex = 0;
    settings.vsyncEnabled = true;
    settings.showFps = false;
    settings.masterVolume = static_cast<int>(AudioConfig::MasterVolume);
    return settings;
}

AppSettings SettingsManager::loadOrCreate(std::string& warning) {
    warning.clear();

    AppSettings settings = defaultSettings();
    const std::filesystem::path directoryPath = settingsDirectoryPath();
    const std::filesystem::path filePath = directoryPath / SettingsConfig::SettingsFileName;

    std::error_code directoryError;
    std::filesystem::create_directories(directoryPath, directoryError);
    if (directoryError) {
        warning = "Could not create settings directory: " + directoryPath.string();
        clamp(settings);
        return settings;
    }

    if (!std::filesystem::exists(filePath)) {
        std::string saveError;
        if (!save(settings, saveError) && warning.empty()) {
            warning = saveError;
        }
        return settings;
    }

    std::ifstream input(filePath);
    if (!input.is_open()) {
        warning = "Could not open settings file: " + filePath.string();
        return settings;
    }

    try {
        json document;
        input >> document;

        settings.resolutionIndex = document.value("resolutionIndex", settings.resolutionIndex);
        settings.windowMode = static_cast<WindowMode>(
            document.value("windowMode", static_cast<int>(settings.windowMode))
        );
        settings.monitorIndex = document.value("monitorIndex", settings.monitorIndex);
        settings.vsyncEnabled = document.value("vsyncEnabled", settings.vsyncEnabled);
        settings.showFps = document.value("showFps", settings.showFps);
        settings.masterVolume = document.value("masterVolume", settings.masterVolume);
    } catch (const std::exception&) {
        warning = "Settings file was invalid and has been reset: " + filePath.string();
        settings = defaultSettings();
        std::string saveError;
        save(settings, saveError);
    }

    clamp(settings);
    return settings;
}

bool SettingsManager::save(const AppSettings& rawSettings, std::string& error) {
    error.clear();

    AppSettings settings = rawSettings;
    clamp(settings);

    const std::filesystem::path directoryPath = settingsDirectoryPath();
    const std::filesystem::path filePath = directoryPath / SettingsConfig::SettingsFileName;

    std::error_code directoryError;
    std::filesystem::create_directories(directoryPath, directoryError);
    if (directoryError) {
        error = "Could not create settings directory: " + directoryPath.string();
        return false;
    }

    json document = {
        { "resolutionIndex", settings.resolutionIndex },
        { "windowMode", static_cast<int>(settings.windowMode) },
        { "monitorIndex", settings.monitorIndex },
        { "vsyncEnabled", settings.vsyncEnabled },
        { "showFps", settings.showFps },
        { "masterVolume", settings.masterVolume }
    };

    std::ofstream output(filePath);
    if (!output.is_open()) {
        error = "Could not write settings file: " + filePath.string();
        return false;
    }

    output << document.dump(4);
    return true;
}

void SettingsManager::clamp(AppSettings& settings) {
    const int maxResolutionIndex = static_cast<int>(resolutionOptions().size()) - 1;
    settings.resolutionIndex = std::clamp(settings.resolutionIndex, 0, maxResolutionIndex);
    settings.monitorIndex = std::max(0, settings.monitorIndex);
    settings.masterVolume = std::clamp(settings.masterVolume, 0, 100);

    if (settings.windowMode != WindowMode::Windowed
        && settings.windowMode != WindowMode::Fullscreen
        && settings.windowMode != WindowMode::Borderless) {
        settings.windowMode = WindowMode::Windowed;
    }
}

void SettingsManager::applyDisplaySettings(const AppSettings& rawSettings) {
    AppSettings settings = rawSettings;
    clamp(settings);

    const int monitorIndex = clampedMonitorIndex(settings.monitorIndex);
    const ResolutionOption resolution = resolutionOptions()[settings.resolutionIndex];
    const Vector2 monitorPosition = GetMonitorPosition(monitorIndex);
    const int monitorWidth = GetMonitorWidth(monitorIndex);
    const int monitorHeight = GetMonitorHeight(monitorIndex);

    // Avoid exclusive fullscreen. It traps focus on some Windows setups and
    // prevents normal Alt-Tab / second-monitor taskbar behavior.
    if (IsWindowState(FLAG_FULLSCREEN_MODE)) {
        ToggleFullscreen();
    }

    if (settings.vsyncEnabled) {
        SetWindowState(FLAG_VSYNC_HINT);
    } else {
        ClearWindowState(FLAG_VSYNC_HINT);
    }

    if (settings.windowMode != WindowMode::Borderless) {
        ClearWindowState(FLAG_WINDOW_UNDECORATED);
        ClearWindowState(FLAG_WINDOW_MAXIMIZED);
    }

    switch (settings.windowMode) {
    case WindowMode::Windowed:
        ClearWindowState(FLAG_WINDOW_UNDECORATED);
        ClearWindowState(FLAG_WINDOW_MAXIMIZED);
        SetWindowSize(resolution.width, resolution.height);
        centerWindowOnMonitor(monitorIndex, resolution.width, resolution.height);
        break;

    case WindowMode::Fullscreen:
        SetWindowState(FLAG_WINDOW_UNDECORATED);
        ClearWindowState(FLAG_WINDOW_MAXIMIZED);
        SetWindowSize(monitorWidth, monitorHeight);
        SetWindowPosition(static_cast<int>(monitorPosition.x), static_cast<int>(monitorPosition.y));
        break;

    case WindowMode::Borderless:
        SetWindowState(FLAG_WINDOW_UNDECORATED);
        SetWindowSize(monitorWidth, monitorHeight);
        SetWindowPosition(static_cast<int>(monitorPosition.x), static_cast<int>(monitorPosition.y));
        SetWindowState(FLAG_WINDOW_MAXIMIZED);
        break;
    }
}

void SettingsManager::applyAudioSettings(const AppSettings& rawSettings) {
    AppSettings settings = rawSettings;
    clamp(settings);
    SetMasterVolume(static_cast<float>(settings.masterVolume) / 100.0f);
}

const std::vector<ResolutionOption>& SettingsManager::resolutionOptions() {
    static const std::vector<ResolutionOption> options = {
        { 1280, 720 },
        { 1366, 768 },
        { 1600, 900 },
        { 1920, 1080 },
        { 2560, 1440 }
    };
    return options;
}

std::string SettingsManager::resolutionLabel(int resolutionIndex) {
    const auto& options = resolutionOptions();
    const int clampedIndex = std::clamp(resolutionIndex, 0, static_cast<int>(options.size()) - 1);
    const ResolutionOption option = options[clampedIndex];
    return std::to_string(option.width) + " x " + std::to_string(option.height);
}

const char* SettingsManager::windowModeLabel(WindowMode windowMode) {
    switch (windowMode) {
    case WindowMode::Windowed:   return "Windowed";
    case WindowMode::Fullscreen: return "Fullscreen";
    case WindowMode::Borderless: return "Borderless";
    default:                     return "Windowed";
    }
}

std::string SettingsManager::settingsFilePath() {
    return (settingsDirectoryPath() / SettingsConfig::SettingsFileName).string();
}
