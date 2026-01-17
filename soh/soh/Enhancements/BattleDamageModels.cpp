#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/Enhancements/BattleDamageModels.h"
#include "soh/ShipInit.hpp"
#include "soh/cvar_prefixes.h"
#include "soh/ResourceManagerHelpers.h"
#include <soh_assets.h>

#include <algorithm>
#include <cmath>
#include <string>

extern "C" {
#include "macros.h"
#include "variables.h"
#include "z64.h"
#include "z64player.h"
extern PlayState* gPlayState;
}

static int32_t sBattleDamageTier = 0;

static bool SkeletonPathExists(const std::string& path) {
    return ResourceMgr_FileAltExists(path.c_str()) || ResourceMgr_FileExists(path.c_str());
}

static std::string SelectBattleDamagePath(int32_t tier, const std::string& tier1, const std::string& tier2,
                                          const std::string& tier3, const std::string& fallback) {
    if (tier >= 3 && !tier3.empty() && SkeletonPathExists(tier3)) {
        return tier3;
    }
    if (tier >= 2 && !tier2.empty() && SkeletonPathExists(tier2)) {
        return tier2;
    }
    if (tier >= 1 && !tier1.empty() && SkeletonPathExists(tier1)) {
        return tier1;
    }
    return fallback;
}

static std::string StripOtrPrefix(const char* path) {
    std::string result = path ? path : "";
    static constexpr const char* kOtrPrefix = "__OTR__";
    if (result.rfind(kOtrPrefix, 0) == 0) {
        result = result.substr(std::char_traits<char>::length(kOtrPrefix));
    }
    return result;
}

static std::string GetFallbackTunicSkeletonPath(bool isAdult, int32_t tunic) {
    if (isAdult) {
        switch (tunic) {
            case PLAYER_TUNIC_KOKIRI:
                return StripOtrPrefix(gLinkAdultKokiriTunicSkel);
            case PLAYER_TUNIC_GORON:
                return StripOtrPrefix(gLinkAdultGoronTunicSkel);
            case PLAYER_TUNIC_ZORA:
                return StripOtrPrefix(gLinkAdultZoraTunicSkel);
            default:
                return StripOtrPrefix(gLinkAdultKokiriTunicSkel);
        }
    }

    switch (tunic) {
        case PLAYER_TUNIC_KOKIRI:
            return StripOtrPrefix(gLinkChildKokiriTunicSkel);
        case PLAYER_TUNIC_GORON:
            return StripOtrPrefix(gLinkChildGoronTunicSkel);
        case PLAYER_TUNIC_ZORA:
            return StripOtrPrefix(gLinkChildZoraTunicSkel);
        default:
            return StripOtrPrefix(gLinkChildKokiriTunicSkel);
    }
}

static std::string SelectSkeletonPath(bool isAdult, int32_t tunic, const std::string& fallbackPath) {
    const int32_t tier = sBattleDamageTier;
    if (tier <= 0) {
        return fallbackPath;
    }

    if (isAdult) {
        switch (tunic) {
            case PLAYER_TUNIC_KOKIRI: {
                const std::string tier1 =
                    "objects/object_link_boy_kokiri_damaged1/gLinkAdultKokiriTunicDamagedSkel";
                const std::string tier2 =
                    "objects/object_link_boy_kokiri_damaged2/gLinkAdultKokiriTunicDamagedSkel";
                const std::string tier3 =
                    "objects/object_link_boy_kokiri_damaged3/gLinkAdultKokiriTunicDamagedSkel";
                return SelectBattleDamagePath(tier, tier1, tier2, tier3, fallbackPath);
            }
            case PLAYER_TUNIC_GORON: {
                const std::string tier1 = "objects/object_link_boy_goron_damaged1/gLinkAdultGoronTunicDamagedSkel";
                const std::string tier2 = "objects/object_link_boy_goron_damaged2/gLinkAdultGoronTunicDamagedSkel";
                const std::string tier3 = "objects/object_link_boy_goron_damaged3/gLinkAdultGoronTunicDamagedSkel";
                return SelectBattleDamagePath(tier, tier1, tier2, tier3, fallbackPath);
            }
            case PLAYER_TUNIC_ZORA: {
                const std::string tier1 = "objects/object_link_boy_zora_damaged1/gLinkAdultZoraTunicDamagedSkel";
                const std::string tier2 = "objects/object_link_boy_zora_damaged2/gLinkAdultZoraTunicDamagedSkel";
                const std::string tier3 = "objects/object_link_boy_zora_damaged3/gLinkAdultZoraTunicDamagedSkel";
                return SelectBattleDamagePath(tier, tier1, tier2, tier3, fallbackPath);
            }
            default:
                return fallbackPath;
        }
    }

    switch (tunic) {
        case PLAYER_TUNIC_KOKIRI: {
            const std::string tier1 = "objects/object_link_child_kokiri_damaged1/gLinkChildKokiriTunicDamagedSkel";
            const std::string tier2 = "objects/object_link_child_kokiri_damaged2/gLinkChildKokiriTunicDamagedSkel";
            const std::string tier3 = "objects/object_link_child_kokiri_damaged3/gLinkChildKokiriTunicDamagedSkel";
            return SelectBattleDamagePath(tier, tier1, tier2, tier3, fallbackPath);
        }
        case PLAYER_TUNIC_GORON: {
            const std::string tier1 = "objects/object_link_child_goron_damaged1/gLinkChildGoronTunicDamagedSkel";
            const std::string tier2 = "objects/object_link_child_goron_damaged2/gLinkChildGoronTunicDamagedSkel";
            const std::string tier3 = "objects/object_link_child_goron_damaged3/gLinkChildGoronTunicDamagedSkel";
            return SelectBattleDamagePath(tier, tier1, tier2, tier3, fallbackPath);
        }
        case PLAYER_TUNIC_ZORA: {
            const std::string tier1 = "objects/object_link_child_zora_damaged1/gLinkChildZoraTunicDamagedSkel";
            const std::string tier2 = "objects/object_link_child_zora_damaged2/gLinkChildZoraTunicDamagedSkel";
            const std::string tier3 = "objects/object_link_child_zora_damaged3/gLinkChildZoraTunicDamagedSkel";
            return SelectBattleDamagePath(tier, tier1, tier2, tier3, fallbackPath);
        }
        default:
            return fallbackPath;
    }
}

static void ApplySkeletonToSkelAnime(SkelAnime* skelAnime, const std::string& skeletonPath) {
    if (skelAnime == nullptr || skeletonPath.empty()) {
        return;
    }

    SkeletonHeader* header = ResourceMgr_LoadSkeletonByName(skeletonPath.c_str(), skelAnime);
    if (header == nullptr) {
        return;
    }

    skelAnime->skeleton = header->segment;
    skelAnime->skeletonHeader = header;
}

void BattleDamageModels_ApplyToLocalPlayer() {
    if (!GameInteractor::IsSaveLoaded(true) || gPlayState == nullptr) {
        return;
    }

    Player* player = GET_PLAYER(gPlayState);
    if (player == nullptr) {
        return;
    }

    const bool isAdult = (gSaveContext.linkAge == LINK_AGE_ADULT);
    const int32_t tunic = TUNIC_EQUIP_TO_PLAYER(CUR_EQUIP_VALUE(EQUIP_TYPE_TUNIC));
    const std::string fallbackPath = GetFallbackTunicSkeletonPath(isAdult, tunic);
    const std::string skeletonPath = SelectSkeletonPath(isAdult, tunic, fallbackPath);

    ApplySkeletonToSkelAnime(&player->skelAnime, skeletonPath);
    ApplySkeletonToSkelAnime(&player->upperSkelAnime, skeletonPath);
    ApplySkeletonToSkelAnime(&gPlayState->pauseCtx.playerSkelAnime, skeletonPath);
}

static int32_t GetBattleDamageTier() {
    if (!GameInteractor::IsSaveLoaded(true) || gPlayState == nullptr) {
        return 0;
    }

    if (gSaveContext.healthCapacity <= 0) {
        return 0;
    }

    if (gSaveContext.health <= 0) {
        return 3;
    }

    auto LogInterp = [](float start, float end, float t) {
        const float clampedT = std::clamp(t, 0.0f, 1.0f);
        return std::exp(std::log(start) + clampedT * (std::log(end) - std::log(start)));
    };

    const float maxHearts = static_cast<float>(gSaveContext.healthCapacity) / 16.0f;
    float thresholdPercent1 = 16.66f;
    float thresholdPercent2 = 8.33f;

    if (maxHearts <= 1.0f) {
        thresholdPercent1 = 50.0f;
        thresholdPercent2 = 25.0f;
    } else if (maxHearts < 3.0f) {
        const float t = (maxHearts - 1.0f) / 2.0f;
        thresholdPercent1 = LogInterp(50.0f, 16.66f, t);
        thresholdPercent2 = LogInterp(25.0f, 8.33f, t);
    } else if (maxHearts < 20.0f) {
        const float t = (maxHearts - 3.0f) / 17.0f;
        thresholdPercent1 = LogInterp(16.66f, 40.0f, t);
        thresholdPercent2 = LogInterp(8.33f, 20.0f, t);
    } else {
        thresholdPercent1 = 40.0f;
        thresholdPercent2 = 20.0f;
    }

    if (thresholdPercent2 > thresholdPercent1) {
        std::swap(thresholdPercent1, thresholdPercent2);
    }

    const float healthPercent =
        (static_cast<float>(gSaveContext.health) * 100.0f) / static_cast<float>(gSaveContext.healthCapacity);
    if (healthPercent <= thresholdPercent2) {
        return 2;
    }
    if (healthPercent <= thresholdPercent1) {
        return 1;
    }
    return 0;
}

static void ApplyBattleDamageModelState() {
    const int32_t nextTier = GetBattleDamageTier();

    if (sBattleDamageTier == nextTier) {
        return;
    }

    sBattleDamageTier = nextTier;
    BattleDamageModels_ApplyToLocalPlayer();
}

static void RefreshBattleDamageModelState() {
    sBattleDamageTier = GetBattleDamageTier();
    BattleDamageModels_ApplyToLocalPlayer();
}

static void RegisterBattleDamageModels() {
    COND_HOOK(OnLoadGame, true, [](int32_t) { RefreshBattleDamageModelState(); });
    COND_HOOK(OnExitGame, true, [](int32_t) { sBattleDamageTier = 0; });

    COND_HOOK(OnPlayerHealthChange, true, [](int16_t) { ApplyBattleDamageModelState(); });
    COND_HOOK(OnLinkSkeletonInit, true, []() { RefreshBattleDamageModelState(); });
    COND_HOOK(OnLinkEquipmentChange, true, []() { RefreshBattleDamageModelState(); });
    COND_HOOK(OnAssetAltChange, true, []() { RefreshBattleDamageModelState(); });
}

static RegisterShipInitFunc initFunc(RegisterBattleDamageModels);
