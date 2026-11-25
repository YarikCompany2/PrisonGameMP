#pragma once
#include "raylib.h"
#include <cstring>

class Outfit {
public:
    int id;
    char name[32];
    int defense;
    bool disguisesAsGuard; 
    bool isContraband; 
    Color displayColor;

    Outfit() {
        id = 0;
        strncpy(name, "Inmate Outfit", sizeof(name) - 1);
        defense = 0;
        disguisesAsGuard = false;
        isContraband = false;
        displayColor = ORANGE;
    }

    Outfit(int outfitId, const char* outfitName, int defenseRating, bool guardDisguise, bool contrabandStatus, Color outfitColor) {
        id = outfitId;
        strncpy(name, outfitName, sizeof(name) - 1);
        defense = defenseRating;
        disguisesAsGuard = guardDisguise;
        isContraband = contrabandStatus;
        displayColor = outfitColor;
    }

    static Outfit CreateInmateOutfit() {
        return Outfit(0, "Inmate Outfit", 0, false, false, ORANGE);
    }

    static Outfit CreatePaddedInmateOutfit() {
        return Outfit(1, "Padded Inmate Outfit", 10, false, true, (Color){ 230, 120, 30, 255 });
    }

    static Outfit CreatePlatedInmateOutfit() {
        return Outfit(2, "Plated Inmate Outfit", 25, false, true, (Color){ 180, 90, 20, 255 });
    }

    static Outfit CreateGuardOutfit() {
        return Outfit(3, "Guard Outfit", 15, true, true, DARKBLUE);
    }

    static Outfit CreateCushionedGuardOutfit() {
        return Outfit(4, "Cushioned Guard Outfit", 30, true, true, BLUE);
    }

    static Outfit CreateMedicOutfit() {
        return Outfit(5, "Medic Outfit", 5, false, true, RAYWHITE);
    }
};