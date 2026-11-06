#pragma once
#include <string>

enum SoundEffect {
    SFX_SELECT = 0,
    SFX_PLACE,
    SFX_COLLECT,
    SFX_HIT,
    SFX_DEATH,
    SFX_WIN,
    SFX_LOSE
};

class SoundManager {
public:
    static SoundManager& getInstance();

    bool init();
    void cleanup();

    void playBGM();
    void pauseBGM();
    void stopBGM();
    bool isBGMPlaying() const;

    void playSFX(SoundEffect sfx);

    void setMasterVolume(float vol);
    float getMasterVolume() const;
    int getVolumePercent() const;

    void volumeUp(float step = 0.1f);
    void volumeDown(float step = 0.1f);

    void toggleMute();
    bool isMuted() const;

private:
    SoundManager();
    ~SoundManager();
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    std::string findFile(const std::string& filename) const;

    bool initialized;
    bool muted;
    float currentVolume;
    float volumeBeforeMute;

    void* pEngine; // internal pointer to ma_engine
    void* pBgm;    // internal pointer to ma_sound
};
