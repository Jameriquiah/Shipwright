#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"

extern "C" {
#include "functions.h"
#include "variables.h"
#include "z64camera.h"
#include "z64player.h"

void Player_SetParallel(Player* player);
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

static bool IsAimingBowOrHookshot(Player* player) {
    if (player == nullptr) {
        return false;
    }

    if (!(player->stateFlags1 & PLAYER_STATE1_ITEM_IN_HAND)) {
        return false;
    }

    return IsBowOrHookshotAimAction(static_cast<PlayerItemAction>(player->heldItemAction));
}

static bool IsAimingBoomerang(Player* player) {
    if (player == nullptr) {
        return false;
    }

    if (player->stateFlags1 & PLAYER_STATE1_BOOMERANG_THROWN) {
        return false;
    }

    if (!(player->stateFlags1 & PLAYER_STATE1_USING_BOOMERANG)) {
        return false;
    }

    return player->unk_834 != 0;
}

static bool IsThirdPersonAimActive(Player* player) {
    return IsAimingBoomerang(player) || IsAimingBowOrHookshot(player);
}

static void ThirdPersonAiming_ApplyPlayerState(Player* player) {
    if (player == nullptr) {
        return;
    }

    if (!IsThirdPersonAimActive(player)) {
        return;
    }

    if (player->stateFlags1 & PLAYER_STATE1_FIRST_PERSON) {
        player->stateFlags1 &= ~PLAYER_STATE1_FIRST_PERSON;
    }

    if (player->focusActor == nullptr) {
        Player_SetParallel(player);
    }
}

static void ThirdPersonAiming_ApplyCameraOverride(PlayState* play) {
    if (play == nullptr) {
        return;
    }

    if (!CVAR_THIRD_PERSON_AIM_VALUE) {
        return;
    }

    Player* player = GET_PLAYER(play);

    if (CVarGetInteger(CVAR_ENHANCEMENT("BoomerangFirstPerson"), 0)) {
        CVarSetInteger(CVAR_ENHANCEMENT("BoomerangFirstPerson"), 0);
        CVarSetInteger(CVAR_ENHANCEMENT("BoomerangReticle"), 0);
    }

    if (IsAimingBoomerang(player)) {
        Camera_ChangeMode(Play_GetCamera(play, 0), CAM_MODE_BOOMERANG);
        return;
    }

    if (IsAimingBowOrHookshot(player)) {
        Camera_ChangeMode(Play_GetCamera(play, 0), CAM_MODE_BOWARROWZ);
    }
}

static void RegisterThirdPersonAiming() {
    COND_HOOK(OnPlayerUpdate, CVAR_THIRD_PERSON_AIM_VALUE,
              []() { ThirdPersonAiming_ApplyPlayerState(GET_PLAYER(gPlayState)); });
    COND_HOOK(OnCameraState, CVAR_THIRD_PERSON_AIM_VALUE,
              [](PlayState* play) { ThirdPersonAiming_ApplyCameraOverride(play); });
}

static RegisterShipInitFunc initFunc(RegisterThirdPersonAiming, { CVAR_THIRD_PERSON_AIM_NAME });
