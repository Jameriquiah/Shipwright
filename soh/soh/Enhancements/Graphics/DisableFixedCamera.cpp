#include <libultraship/bridge.h>
#include <set>
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
}

#define CVAR_NAME CVAR_ENHANCEMENT("DisableFixedCamera")
#define CVAR_VALUE CVarGetInteger(CVAR_NAME, 0)

static bool sForceNormalCamera = false;

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

void RegisterDisableFixedCamera() {
    sForceNormalCamera = false;

    if (!CVarGetInteger(CVAR_ENHANCEMENT("3DSceneRender"), 0)) {
        if (CVAR_VALUE) {
            CVarSetInteger(CVAR_NAME, 0);
        }
        return;
    }

    COND_HOOK(AfterSceneCommands, CVAR_VALUE, [](int16_t sceneNum) {
        sForceNormalCamera = IsPrerenderedScene(sceneNum);
        if (sForceNormalCamera && gPlayState != NULL) {
            gPlayState->unk_1242B = 0;
        }
    });

    COND_HOOK(OnActorInit, CVAR_VALUE, [](void* actorPtr) {
        Actor* actor = static_cast<Actor*>(actorPtr);
        if (!sForceNormalCamera || actor->id != ACTOR_PLAYER) {
            return;
        }

        actor->params = (actor->params & 0xFF00) | 0x00FF;
    });

    COND_HOOK(OnPlayerUpdate, CVAR_VALUE, []() {
        if (!sForceNormalCamera || gPlayState == NULL) {
            return;
        }

        Camera* camera = GET_ACTIVE_CAM(gPlayState);
        if (camera == NULL) {
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
}

static RegisterShipInitFunc initFunc(RegisterDisableFixedCamera,
                                     { CVAR_NAME, CVAR_ENHANCEMENT("3DSceneRender") });
