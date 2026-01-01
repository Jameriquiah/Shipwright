#include <libultraship/bridge.h>
#include <set>
#include <spdlog/spdlog.h>
#include "soh/Enhancements/enhancementTypes.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"
#include "soh/cvar_prefixes.h"

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "z64.h"
extern PlayState* gPlayState;
Vec3s* Camera_GetCamBGData(Camera* camera);
}

#define CVAR_NAME CVAR_ENHANCEMENT("DisableFixedCamera")
#define CVAR_VALUE CVarGetInteger(CVAR_NAME, 0)

static bool sForceNormalCamera = false;
static bool sAppliedForSensitive = false;
static bool sSensitiveDebugPrinted = false;
static bool sEnableSensitiveUpdate = false;
static bool sEnableNonSensitiveUpdate = false;

static const std::set<SceneID> sPrerenderedScenes = {
    SCENE_MARKET_ENTRANCE_DAY,
    SCENE_MARKET_ENTRANCE_NIGHT,
    SCENE_MARKET_ENTRANCE_RUINS,
    SCENE_BACK_ALLEY_DAY,
    SCENE_BACK_ALLEY_NIGHT,
    SCENE_MARKET_DAY,
    SCENE_MARKET_NIGHT,
    SCENE_MARKET_RUINS,
    SCENE_TEMPLE_OF_TIME_EXTERIOR_DAY,
    SCENE_TEMPLE_OF_TIME_EXTERIOR_NIGHT,
    SCENE_TEMPLE_OF_TIME_EXTERIOR_RUINS,
    SCENE_KNOW_IT_ALL_BROS_HOUSE,
    SCENE_TWINS_HOUSE,
    SCENE_MIDOS_HOUSE,
    SCENE_SARIAS_HOUSE,
    SCENE_BACK_ALLEY_HOUSE,
    SCENE_BAZAAR,
    SCENE_KOKIRI_SHOP,
    SCENE_GORON_SHOP,
    SCENE_ZORA_SHOP,
    SCENE_POTION_SHOP_KAKARIKO,
    SCENE_POTION_SHOP_MARKET,
    SCENE_BOMBCHU_SHOP,
    SCENE_HAPPY_MASK_SHOP,
    SCENE_LINKS_HOUSE,
    SCENE_DOG_LADY_HOUSE,
    SCENE_STABLE,
    SCENE_IMPAS_HOUSE,
    SCENE_CARPENTERS_TENT,
    SCENE_GRAVEKEEPERS_HUT,
};

static bool IsPrerenderedScene(int16_t sceneNum) {
    return sPrerenderedScenes.contains(static_cast<SceneID>(sceneNum));
}

// These are handled separately as they force the camera to reset for some reason
static bool IsSensitiveScene(int16_t sceneNum) {
    switch (sceneNum) {
        case SCENE_TEMPLE_OF_TIME_EXTERIOR_DAY:
        case SCENE_TEMPLE_OF_TIME_EXTERIOR_NIGHT:
        case SCENE_TEMPLE_OF_TIME_EXTERIOR_RUINS:
        case SCENE_BACK_ALLEY_DAY:
        case SCENE_BACK_ALLEY_NIGHT:
            return true;
        default:
            return false;
    }
}

static bool FindFirstValidCamDataIdx(PlayState* play, s16* outIdx) {
    CollisionHeader* colHeader = BgCheck_GetCollisionHeader(&play->colCtx, BGCHECK_SCENE);
    if (colHeader == NULL || colHeader->cameraDataListLen == 0) {
        return false;
    }

    for (s16 i = 0; i < (s16)colHeader->cameraDataListLen; i++) {
        if (func_80041C10(&play->colCtx, i, BGCHECK_SCENE) != NULL) {
            *outIdx = i;
            return true;
        }
    }

    return false;
}

static void EnsureSensitiveSceneCamData(PlayState* play, Camera* camera) {
    if (!IsSensitiveScene(play->sceneNum)) {
        return;
    }

    if (camera->camDataIdx >= 0 && func_80041C10(&play->colCtx, camera->camDataIdx, BGCHECK_SCENE) != NULL) {
        return;
    }

    s16 camDataIdx = -1;
    if (FindFirstValidCamDataIdx(play, &camDataIdx)) {
        camera->camDataIdx = camDataIdx;
        camera->prevCamDataIdx = camDataIdx;
        camera->nextCamDataIdx = camDataIdx;
    }
}

static void UpdateSceneHooks() {
    COND_HOOK(OnPlayerUpdate, CVAR_VALUE && sEnableSensitiveUpdate, []() {
        if (!sForceNormalCamera || gPlayState == NULL) {
            return;
        }

        Camera* camera = GET_ACTIVE_CAM(gPlayState);
        if (camera == NULL) {
            return;
        }

        if (!IsSensitiveScene(gPlayState->sceneNum)) {
            return;
        }

        if (!sSensitiveDebugPrinted) {
            SPDLOG_INFO(
                "[DisableFixedCamera] scene={} frame={} setting={} mode={} camIdx={} unk14A={:04X} unk14C={:04X} anim={}",
                gPlayState->sceneNum, gPlayState->state.frames, camera->setting, camera->mode, camera->camDataIdx,
                camera->unk_14A, camera->unk_14C, camera->animState);
            sSensitiveDebugPrinted = true;
        }
        if (sAppliedForSensitive) {
            bool isLockOnMode = (camera->mode == CAM_MODE_TARGET) || (camera->mode == CAM_MODE_FOLLOWTARGET) ||
                                (camera->mode == CAM_MODE_BATTLE);
            if (camera->setting != CAM_SET_NORMAL0) {
                Camera_ChangeSetting(camera, CAM_SET_NORMAL0);
            }
            if (!isLockOnMode && camera->mode != CAM_MODE_NORMAL) {
                Camera_ChangeMode(camera, CAM_MODE_NORMAL);
            }
            camera->nextCamDataIdx = -1;
            camera->unk_14C &= ~(0x1 | 0x4);
            return;
        }

        Player* player = GET_PLAYER(gPlayState);
        if (player == NULL) {
            return;
        }

        if (camera->camDataIdx < 0 || Camera_GetCamBGData(camera) == NULL) {
            return;
        }

        Camera_ResetAnim(camera);
        if (camera->setting != CAM_SET_NORMAL0) {
            Camera_ChangeSetting(camera, CAM_SET_NORMAL0);
        }

        Camera_ChangeMode(camera, CAM_MODE_NORMAL);
        Camera_InitPlayerSettings(camera, player);
        camera->nextCamDataIdx = -1;
        camera->unk_14C &= ~(0x1 | 0x4);
        gPlayState->unk_1242B = 0;
        sAppliedForSensitive = true;
    });
}

void RegisterDisableFixedCamera() {
    sForceNormalCamera = false;
    sEnableSensitiveUpdate = false;
    sEnableNonSensitiveUpdate = false;
    UpdateSceneHooks();

    if (!CVarGetInteger(CVAR_ENHANCEMENT("3DSceneRender"), 0)) {
        if (CVAR_VALUE) {
            CVarSetInteger(CVAR_NAME, 0);
        }
        return;
    }

    COND_HOOK(AfterSceneCommands, CVAR_VALUE, [](int16_t sceneNum) {
        sForceNormalCamera = IsPrerenderedScene(sceneNum);
        sAppliedForSensitive = false;
        sSensitiveDebugPrinted = false;
        sEnableSensitiveUpdate = sForceNormalCamera && IsSensitiveScene(sceneNum);
        sEnableNonSensitiveUpdate = sForceNormalCamera && !IsSensitiveScene(sceneNum);
        UpdateSceneHooks();
        if (sForceNormalCamera && gPlayState != NULL) {
            if (!IsSensitiveScene(sceneNum)) {
                gPlayState->unk_1242B = 0;
            }
        }
    });

    COND_HOOK(OnPlayDrawBegin, CVAR_VALUE, []() {
        if (!sForceNormalCamera || gPlayState == NULL) {
            return;
        }

        Camera* camera = GET_ACTIVE_CAM(gPlayState);
        if (camera == NULL) {
            return;
        }

        EnsureSensitiveSceneCamData(gPlayState, camera);
        if (!sEnableNonSensitiveUpdate || IsSensitiveScene(gPlayState->sceneNum)) {
            return;
        }

        gPlayState->unk_1242B = 0;
        if (camera->setting != CAM_SET_NORMAL0) {
            Camera_ChangeSetting(camera, CAM_SET_NORMAL0);
        }

        camera->camDataIdx = -1;
        camera->nextCamDataIdx = -1;
        camera->prevCamDataIdx = -1;
        camera->unk_14A |= 0x40;
    });

    COND_HOOK(OnActorInit, CVAR_VALUE, [](void* actorPtr) {
        Actor* actor = static_cast<Actor*>(actorPtr);
        if (!sForceNormalCamera || actor->id != ACTOR_PLAYER) {
            return;
        }

        if (gPlayState != NULL && IsSensitiveScene(gPlayState->sceneNum)) {
            return;
        }

        actor->params = (actor->params & 0xFF00) | 0x00FF;
    });

    COND_HOOK(OnPlayDestroy, true, []() {
        sEnableSensitiveUpdate = false;
        sEnableNonSensitiveUpdate = false;
        UpdateSceneHooks();
    });
}

static RegisterShipInitFunc initFunc(RegisterDisableFixedCamera,
                                     { CVAR_NAME, CVAR_ENHANCEMENT("3DSceneRender") });
