#pragma once

namespace offsets {
    constexpr auto dwLocalPlayer = 0x171C6E8;
    constexpr auto dwEntityList = 0x162C020;
    constexpr auto dwViewMatrix = 0x171D1E0;

    constexpr auto dwPawnHealth = 0x808;
    constexpr auto dwPlayerPawn = 0x5DC;
    constexpr auto dwSanitizedName = 0x720;
    constexpr auto m_iTeamNum = 0x3BF;
    constexpr auto m_vecOrigin = 0x1204;

    constexpr auto m_hGameSceneNode = 0x310;
    constexpr auto m_hModelState = 0x160;
    constexpr auto m_hBoneArray = 0x80;

    constexpr auto m_pWeaponServices = 0x1140;
    constexpr auto m_hMyWeapons = 0x48;
    constexpr auto m_hActiveWeapon = 0x60;
    constexpr auto m_AttributeManager = 0x10D8;

    constexpr auto m_hPlayerPawn = 0x7fc;
    constexpr auto m_pClippingWeapon = 0x1330;
    constexpr auto m_Item = 0x50;
    constexpr auto m_iItemDefinitionIndex = 0x1BA;
}