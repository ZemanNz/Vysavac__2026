#pragma once
#include <Arduino.h>
#include "robotka.h"

// Konfigurace pro kontinuální servo na S1 (Port 1)
const float SERVO_SPEED = 20.0f;        // Rychlost otáčení (hodnota v rozsahu 0 až 90)
const float MS_PER_DEGREE = 11.4075f;       // Koeficient převodu úhlu na čas v ms (lze doladit)

void init_stepper() {
    // Stepper motor nepoužíváme, inicializace pinů je prázdná
}

void vypni_civky() {
    // Pro kontinuální servo vypnutí znamená nastavení neutralu a vypnutí napájení serva
    rkServosSetPosition(1, 0.0f);
    delay(100); // Necháme servu čas zaregistrovat stop-pulz a zastavit se
    rkServosDisable(1);
}

void otoc_motorem(int uhel, bool proti_smeru) {
    float rychlost = proti_smeru ? -SERVO_SPEED : SERVO_SPEED;
    int duration_ms = (int)(uhel * MS_PER_DEGREE);
    
    if (duration_ms > 0) {
        rkServosSetPosition(1, rychlost);
        delay(duration_ms);
    }
    
    // Zastavení serva po dokončení otáčení
    vypni_civky();
}

// Pomocné funkce pro simulovaný krokový posun (např. při srovnávání na senzor)
void rotacePoSmeru() {
    rkServosSetPosition(1, SERVO_SPEED);
    delay(5);
}

void rotaceProtiSmeru() {
    rkServosSetPosition(1, -SERVO_SPEED);
    delay(5);
}
