#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"

extern "C" {
#include "functions.h"
#include "variables.h"
#include "z64camera.h"
#include "z64player.h"

}

#define CVAR_THIRD_PERSON_AIM_NAME CVAR_ENHANCEMENT("ThirdPersonAiming")
#define CVAR_THIRD_PERSON_AIM_VALUE CVarGetInteger(CVAR_THIRD_PERSON_AIM_NAME, 0)

static bool IsBowOrHookshotAimAction(PlayerItemAction itemAction) {
    switch (itemAction) {
        case PLAYER_IA_BOW:
        case PLAYER_IA_BOW_FIRE:
        case PLAYER_IA_BOW_ICE:
        case PLAYER_IA_BOW_LIGHT:
        case PLAYER_IA_SLINGSHOT:
        case PLAYER_IA_HOOKSHOT:
        case PLAYER_IA_LONGSHOT:
            return true;
        default:
            return false;
    }
}

static bool ShouldOverrideAimCamera(Player* player) {
    if (player == nullptr) {
        return false;
    }

    if (!CVAR_THIRD_PERSON_AIM_VALUE) {
        return false;
    }

    return IsBowOrHookshotAimAction(static_cast<PlayerItemAction>(player->heldItemAction));
}

static void RegisterThirdPersonAiming() {
    COND_HOOK(OnPlayerAimCameraMode, CVAR_THIRD_PERSON_AIM_VALUE, [](Player* player, s16* camMode) {
        if (!ShouldOverrideAimCamera(player)) {
            return;
        }

        if (CVarGetInteger(CVAR_ENHANCEMENT("BoomerangFirstPerson"), 0)) {
            CVarSetInteger(CVAR_ENHANCEMENT("BoomerangFirstPerson"), 0);
            CVarSetInteger(CVAR_ENHANCEMENT("BoomerangReticle"), 0);
        }

        *camMode = CAM_MODE_TARGET;
    });
}

static RegisterShipInitFunc initFunc(RegisterThirdPersonAiming, { CVAR_THIRD_PERSON_AIM_NAME });
