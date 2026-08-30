#pragma once

#include "CoreMinimal.h"
#include "SoundID.generated.h"

UENUM(BlueprintType)
enum class ESoundID : uint8
{
    None,

    // Player
    Player_Footstep,
    Player_Jump,
    Player_Land,
    Player_Hurt,
    Player_Death,

    // Weapon
    Weapon_Rifle_Fire,
    Weapon_Rifle_Reload,
    Weapon_Rifle_Empty,
    Weapon_Shotgun_Fire,
    Weapon_Shotgun_Reload,

    // Throwable
    Grenade_Pin,
    Grenade_Throw,
    Grenade_Explosion,

    // Item
    Item_Pickup,
    Item_Drop,
    Item_Use,

    // Enemy
    Enemy_Hurt,
    Enemy_Death,
    Enemy_Footstep,
    Enemy_Alert,

    // Environment
    Door_Open,
    Door_Close,
    Light_Break,
    Generator_Start,
    Generator_Stop,
    Explosion,

    // UI
    UI_Click,
    UI_Hover,
    UI_Open,
    UI_Close,

    // Music
    BGM_MainMenu,
    BGM_Gameplay,
    BGM_Combat
};