#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "SoundManager.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

static bool fileExists(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

SoundManager& SoundManager::getInstance() {
    static SoundManager instance;
    return instance;
}

SoundManager::SoundManager()
    : initialized(false),
      muted(false),
      currentVolume(0.7f),
      volumeBeforeMute(0.7f),
      pEngine(nullptr),
      pBgm(nullptr)
{
}

SoundManager::~SoundManager() {
    cleanup();
}

std::string SoundManager::findFile(const std::string& filename) const {
    std::vector<std::string> searchPaths = {
        filename,
        "audio/" + filename,
        "../audio/" + filename,
        "../../audio/" + filename,
        "Bedwars/audio/" + filename,
        "../" + filename
    };

    for (const auto& path : searchPaths) {
        if (fileExists(path)) {
            return path;
        }
    }
    return "audio/" + filename;
}

bool SoundManager::init() {
    if (initialized) {
        return true;
    }

    ma_engine* engine = new ma_engine();
    ma_result result = ma_engine_init(NULL, engine);
    if (result != MA_SUCCESS) {
        std::cerr << "[SoundManager] Failed to initialize miniaudio engine: " << result << std::endl;
        delete engine;
        return false;
    }
    pEngine = engine;

    ma_engine_set_volume(engine, currentVolume);

    // Initialize BGM
    ma_sound* bgm = new ma_sound();
    std::string bgmPath = findFile("bgm.mp3");
    if (!fileExists(bgmPath)) {
        bgmPath = findFile("bgm.wav");
    }

    result = ma_sound_init_from_file(engine, bgmPath.c_str(), MA_SOUND_FLAG_STREAM, NULL, NULL, bgm);
    if (result == MA_SUCCESS) {
        ma_sound_set_looping(bgm, MA_TRUE);
        pBgm = bgm;
        std::cout << "[SoundManager] Loaded BGM from: " << bgmPath << std::endl;
    } else {
        std::cerr << "[SoundManager] Could not load BGM file (" << bgmPath << "), code: " << result << std::endl;
        delete bgm;
        pBgm = nullptr;
    }

    initialized = true;
    return true;
}

void SoundManager::cleanup() {
    if (!initialized) return;

    if (pBgm) {
        ma_sound* bgm = static_cast<ma_sound*>(pBgm);
        ma_sound_stop(bgm);
        ma_sound_uninit(bgm);
        delete bgm;
        pBgm = nullptr;
    }

    if (pEngine) {
        ma_engine* engine = static_cast<ma_engine*>(pEngine);
        ma_engine_uninit(engine);
        delete engine;
        pEngine = nullptr;
    }

    initialized = false;
}

void SoundManager::playBGM() {
    if (!initialized) init();
    if (pBgm) {
        ma_sound* bgm = static_cast<ma_sound*>(pBgm);
        if (!ma_sound_is_playing(bgm)) {
            ma_sound_start(bgm);
        }
    }
}

void SoundManager::pauseBGM() {
    if (pBgm) {
        ma_sound* bgm = static_cast<ma_sound*>(pBgm);
        if (ma_sound_is_playing(bgm)) {
            ma_sound_stop(bgm);
        }
    }
}

void SoundManager::stopBGM() {
    if (pBgm) {
        ma_sound* bgm = static_cast<ma_sound*>(pBgm);
        ma_sound_stop(bgm);
        ma_sound_seek_to_pcm_frame(bgm, 0);
    }
}

bool SoundManager::isBGMPlaying() const {
    if (!pBgm) return false;
    ma_sound* bgm = static_cast<ma_sound*>(pBgm);
    return ma_sound_is_playing(bgm);
}

void SoundManager::playSFX(SoundEffect sfx) {
    if (!initialized) init();
    if (!pEngine) return;

    std::string sfxFile;
    switch (sfx) {
        case SFX_SELECT:  sfxFile = "select.wav";  break;
        case SFX_PLACE:   sfxFile = "place.wav";   break;
        case SFX_COLLECT: sfxFile = "collect.wav"; break;
        case SFX_HIT:     sfxFile = "hit.wav";     break;
        case SFX_DEATH:   sfxFile = "death.wav";   break;
        case SFX_WIN:     sfxFile = "win.wav";     break;
        case SFX_LOSE:    sfxFile = "lose.wav";    break;
        default:          sfxFile = "select.wav";  break;
    }

    std::string path = findFile(sfxFile);
    ma_engine* engine = static_cast<ma_engine*>(pEngine);
    ma_engine_play_sound(engine, path.c_str(), NULL);
}

void SoundManager::setMasterVolume(float vol) {
    currentVolume = std::max(0.0f, std::min(1.0f, vol));
    if (muted && currentVolume > 0.0f) {
        muted = false;
    }
    if (pEngine) {
        ma_engine* engine = static_cast<ma_engine*>(pEngine);
        ma_engine_set_volume(engine, muted ? 0.0f : currentVolume);
    }
}

float SoundManager::getMasterVolume() const {
    return currentVolume;
}

int SoundManager::getVolumePercent() const {
    if (muted) return 0;
    return static_cast<int>(std::round(currentVolume * 100.0f));
}

void SoundManager::volumeUp(float step) {
    if (muted) {
        muted = false;
    }
    setMasterVolume(currentVolume + step);
    playSFX(SFX_SELECT);
}

void SoundManager::volumeDown(float step) {
    if (muted) {
        muted = false;
    }
    setMasterVolume(currentVolume - step);
    playSFX(SFX_SELECT);
}

void SoundManager::toggleMute() {
    muted = !muted;
    if (muted) {
        volumeBeforeMute = currentVolume;
        if (pEngine) {
            ma_engine* engine = static_cast<ma_engine*>(pEngine);
            ma_engine_set_volume(engine, 0.0f);
        }
    } else {
        if (pEngine) {
            ma_engine* engine = static_cast<ma_engine*>(pEngine);
            ma_engine_set_volume(engine, currentVolume);
        }
        playSFX(SFX_SELECT);
    }
}

bool SoundManager::isMuted() const {
    return muted;
}
