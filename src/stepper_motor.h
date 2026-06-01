#pragma once
#include <Arduino.h>
#include "robotka.h"

// Konfigurace pro kontinuální servo na S1 (Port 1)
const float SERVO_SPEED = 20.0f;        // Rychlost otáčení (hodnota v rozsahu 0 až 90)
const float MS_PER_DEGREE = 12.7f;       // Koeficient převodu úhlu na čas v ms (upraveno pro rychlost 35.0 a zpomalování)

void init_stepper() {
    // Stepper motor nepoužíváme, inicializace pinů je prázdná
}

void vypni_civky() {
    // Pro kontinuální servo vypnutí znamená nastavení neutralu a vypnutí napájení serva
    rkServosSetPosition(1, 0.0f);
    delay(100); // Necháme servu čas zaregistrovat stop-pulz a zastavit se
    rkServosDisable(1);
}

void otoc_motorem(int uhel, bool proti_smeru, float rychlost = 35.0f) {
    // Rozdílné koeficienty pro každý směr (kontinuální serva mívají asymetrickou rychlost)
    float ms_per_degree = proti_smeru ? 13.4f : 12.5f; // proti_smeru (UP) nedotáčelo -> prodloužit čas, po_smeru (ON) přetáčelo -> zkrátit čas
    float C = 1.0f / (SERVO_SPEED * ms_per_degree); 
    
    float theta_target = (float)uhel;
    float theta_turned = 0.0f;
    float theta_decel = 45.0f; // Začít zpomalovat 45° před cílem (více času na dobrzdění z vyšší rychlosti)
    float v_max = rychlost;    // Použijeme zadanou maximální rychlost
    float v_min = 6.0f;        // Minimální rychlost, aby se servo nezaseklo
    if (v_min > v_max) v_min = v_max;
    
    uint32_t last_time = millis();
    
    // Spustíme motor na počáteční rychlost
    float pocatecni_rychlost = proti_smeru ? -v_max : v_max;
    rkServosSetPosition(1, pocatecni_rychlost);
    
    while (theta_turned < theta_target) {
        delay(10); // Krok regulace
        
        uint32_t now = millis();
        float dt = (float)(now - last_time);
        last_time = now;
        
        float theta_rem = theta_target - theta_turned;
        if (theta_rem < 0.0f) theta_rem = 0.0f; // Ochrana: zbývající úhel nemůže jít do záporu
        
        // Výpočet rychlosti podle zbývajícího úhlu (zpomalovací rampa)
        float speed = v_max;
        if (theta_rem < theta_decel) {
            // Lineární zpomalení z v_max na v_min
            speed = v_min + (v_max - v_min) * (theta_rem / theta_decel);
        }
        
        // Bezpečnostní omezení rychlosti, aby nikdy nedošlo k obrácení chodu
        if (speed < v_min) speed = v_min;
        if (speed > v_max) speed = v_max;
        
        // Integrace otočeného úhlu
        theta_turned += C * speed * dt;
        
        // Nastavení rychlosti serva s ohledem na směr
        float output_speed = proti_smeru ? -speed : speed;
        rkServosSetPosition(1, output_speed);
    }
    
    // Zastavení serva a odpojení cívek
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
