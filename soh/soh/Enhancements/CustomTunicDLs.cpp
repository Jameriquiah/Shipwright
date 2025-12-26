#include "CustomTunicDLs.h"

#include "soh/ResourceManagerHelpers.h"

#include "variables.h"
#include "z64item.h"

#include <string>
#include <string_view>

namespace {
constexpr std::string_view kOtrPrefix = "__OTR__";
constexpr std::string_view kBoyPrefix = "objects/object_link_boy/";
constexpr std::string_view kChildPrefix = "objects/object_link_child/";
constexpr std::string_view kRemapNames[] = {
    "gLinkAdultBottleDL",
    "gLinkAdultBowStringDL",
    "gLinkAdultHandHoldingBrokenGiantsKnifeDL",
    "gLinkAdultHylianShieldAndSheathNearDL",
    "gLinkAdultHylianShieldSwordAndSheathNearDL",
    "gLinkAdultLeftArmOutNearDL",
    "gLinkAdultLeftGauntletPlate1DL",
    "gLinkAdultLeftGauntletPlate2DL",
    "gLinkAdultLeftGauntletPlate3DL",
    "gLinkAdultLeftHandClosedNearDL",
    "gLinkAdultLeftHandHoldingBgsNearDL",
    "gLinkAdultLeftHandHoldingHammerNearDL",
    "gLinkAdultLeftHandHoldingMasterSwordNearDL",
    "gLinkAdultLeftHandNearDL",
    "gLinkAdultLeftHandOutNearDL",
    "gLinkAdultLeftHoverBootDL",
    "gLinkAdultLeftIronBootDL",
    "gLinkAdultMasterSwordAndSheathNearDL",
    "gLinkAdultMirrorShieldAndSheathNearDL",
    "gLinkAdultMirrorShieldSwordAndSheathNearDL",
    "gLinkAdultRightArmOutNearDL",
    "gLinkAdultRightGauntletPlate1DL",
    "gLinkAdultRightGauntletPlate2DL",
    "gLinkAdultRightGauntletPlate3DL",
    "gLinkAdultRightHandClosedNearDL",
    "gLinkAdultRightHandHoldingBowFirstPersonDL",
    "gLinkAdultRightHandHoldingBowNearDL",
    "gLinkAdultRightHandHoldingHookshotNearDL",
    "gLinkAdultRightHandHoldingHylianShieldNearDL",
    "gLinkAdultRightHandHoldingMirrorShieldNearDL",
    "gLinkAdultRightHandHoldingOotNearDL",
    "gLinkAdultRightHandNearDL",
    "gLinkAdultRightHandOutNearDL",
    "gLinkAdultRightHoverBootDL",
    "gLinkAdultRightIronBootDL",
    "gLinkAdultRightShoulderNearDL",
    "gLinkAdultSheathNearDL",
    "gLinkChildBottleDL",
    "gLinkChildDekuShieldAndSheathNearDL",
    "gLinkChildDekuShieldSwordAndSheathNearDL",
    "gLinkChildDekuShieldWithMatrixDL",
    "gLinkChildGoronBraceletDL",
    "gLinkChildHylianShieldAndSheathNearDL",
    "gLinkChildHylianShieldSwordAndSheathNearDL",
    "gLinkChildLeftFistAndBoomerangNearDL",
    "gLinkChildLeftFistAndKokiriSwordNearDL",
    "gLinkChildLeftFistNearDL",
    "gLinkChildLeftHandHoldingMasterSwordDL",
    "gLinkChildLeftHandNearDL",
    "gLinkChildLeftHandUpNearDL",
    "gLinkChildLinkDekuStickDL",
    "gLinkChildRightArmStretchedSlingshotDL",
    "gLinkChildRightFistAndDekuShieldNearDL",
    "gLinkChildRightHandAndOotNearDL",
    "gLinkChildRightHandClosedNearDL",
    "gLinkChildRightHandHoldingFairyOcarinaNearDL",
    "gLinkChildRightHandHoldingSlingshotNearDL",
    "gLinkChildRightHandNearDL",
    "gLinkChildRightShoulderNearDL",
    "gLinkChildSheathNearDL",
    "gLinkChildSlingshotStringDL",
    "gLinkChildSwordAndSheathNearDL",
};

std::string_view GetTunicSuffix() {
    s32 tunicEquip = (gSaveContext.equips.equipment & gEquipMasks[EQUIP_TYPE_TUNIC]) >> gEquipShifts[EQUIP_TYPE_TUNIC];
    switch (tunicEquip) {
        case EQUIP_VALUE_TUNIC_KOKIRI:
            return "_kokiri";
        case EQUIP_VALUE_TUNIC_GORON:
            return "_goron";
        case EQUIP_VALUE_TUNIC_ZORA:
            return "_zora";
        default:
            return "";
    }
}

bool ShouldRemapDisplayList(std::string_view remainder) {
    for (std::string_view name : kRemapNames) {
        if (remainder == name) {
            return true;
        }
    }
    return false;
}
} // namespace

const char* CustomTunicDLs_RemapPath(const char* path) {
    if (path == nullptr) {
        return path;
    }

    std::string_view view(path);
    bool hasOtrPrefix = view.starts_with(kOtrPrefix);
    if (hasOtrPrefix) {
        view = view.substr(kOtrPrefix.size());
    }

    std::string_view basePrefix;
    if (view.starts_with(kBoyPrefix)) {
        basePrefix = kBoyPrefix;
    } else if (view.starts_with(kChildPrefix)) {
        basePrefix = kChildPrefix;
    } else {
        return path;
    }

    std::string_view tunicSuffix = GetTunicSuffix();
    if (tunicSuffix.empty()) {
        return path;
    }

    std::string_view baseName = basePrefix.substr(0, basePrefix.size() - 1);
    std::string_view remainder = view.substr(basePrefix.size());
    if (!ShouldRemapDisplayList(remainder)) {
        return path;
    }

    static thread_local std::string remapped;
    remapped.clear();
    if (hasOtrPrefix) {
        remapped.append(kOtrPrefix);
    }
    remapped.append(baseName);
    remapped.append(tunicSuffix);
    remapped.push_back('/');
    remapped.append(remainder);

    if (ResourceMgr_FileExists(remapped.c_str()) ||
        (ResourceMgr_IsAltAssetsEnabled() && ResourceMgr_FileAltExists(remapped.c_str()))) {
        return remapped.c_str();
    }

    return path;
}
