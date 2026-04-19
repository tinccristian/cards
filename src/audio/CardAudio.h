#pragma once

#include <string>

class CardAudio {
public:
    CardAudio() = default;

    bool initialize(std::string& warning);
    void shutdown();

    void playHover();
    void playShuffle();

private:
    void applyConfiguredVolumes() const;

    bool m_hoverLoaded   = false;
    bool m_shuffleLoaded = false;
    int  m_hoverPitchIndex = 0;
    int  m_hoverVoiceIndex = 0;
};
