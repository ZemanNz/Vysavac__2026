#pragma once

#include <Arduino.h>
#include "robotka.h"
#include "stepper_motor.h"


void otevri_nas(){
    rkServosSetPosition(4, 80); // Servo 1 nastaví na 90°
}

void zavri_nas(){
    rkServosSetPosition(4, 16); // Servo 1 nastaví na 0°
}

void otevri_souper(){
    rkServosSetPosition(3, -90);
}

void zavri_souper(){
    rkServosSetPosition(3, 0);
}

void srovnej_trididlo() {

    // Točí motorem proti směru hodinových ručiček, dokud senzor nevrátí 0 
    // (resp. pro absolutní bezpečí raději kontroluji aby tam nebyl ani šum > 10)
    while (rkIrRight() > 10) {
        rotacePoSmeru();
    }
    
    // Zastavíme a vrátíme původní stav
    vypni_civky(); // Povolit cívky, aby motor přestal krokovat


    rkServosSetPosition(2, 75);

    delay(400);


    int pocet_kroku = (60 * 64) / 45;
  
    for(int i=0;i<pocet_kroku;i++){
      rotaceProtiSmeru();
    }

    vypni_civky(); // Povolit cívky, aby motor přestal krokovat

    delay(300);

    rkServosSetPosition(2, 0);

    delay(10);
}

// Proměnná "nase_barva", kterou definujeme v main.cpp
extern char nase_barva;
extern volatile int pocet_nasich_puku;
extern volatile bool g_puk_roztrizen;
extern volatile bool g_zadost_o_srovnani;
extern volatile int g_lajny_bez_puku;
extern volatile int pocet_roz_p;
extern volatile bool g_match_ended;

char urci_barvu_puku(float &r, float &g, float &b) {
    // 1) Ochrana před falešnou detekcí na prázdno (hodnoty si bývají velmi blízké, např R:114, G:114, B:107)
    // Pokud je rozdíl mezi nejvyšší a nejnižší barvou malý, bereme to hned jako prázdno.

    Serial.print("R: "); Serial.print(r, 3);
    Serial.print(" G: "); Serial.print(g, 3);
    Serial.print(" B: "); Serial.println(b, 3);



    if ((abs(r - g) < 20 && abs(r - b) < 20 && abs(b - g) < 20) || (r > 110 && g > 110 && b > 110)) {
        return 'N';
    }

    // Detekce červeného puku (vysoká R složka a R musí výrazně převyšovat ostatní)
    if (r > 185 && r > g + 30 && r > b + 30) {
        return 'R';
    }
    
    // Detekce modrého puku (B složka musí být dominantní, nebo se jedná o specifický tmavě modrý puk)
    if ((b > 95 && b > r + 20 && b > g + 12) || 
        (r > 100 && r < 120 && g > 85 && g < 105 && b > 70 && b < 90)) {
        return 'B';
    }
    
    return 'N'; // Neznámá barva / prázdno (jakýkoliv okolní šum)
}

void roztrid_puk(char barva) {
    if (barva == nase_barva) {
        // Pokud je barva shodná, krokovej motor se otočí o 120°
        otoc_motorem(120, false);
    } else {
        // V opačném případě se otočí o 120° opačně
        otoc_motorem(120, true);
    }
}

// Funkce běžící v samostatném vlákně
void tridici_vlakno(void *pvParameters) {
    int pocitadlo_puku = 0;
    float r = 0, g = 0, b = 0;

    // První naměřené hodnoty senzoru bezprostředně po jeho zapnutí
    // bývají občas totální nesmysly (tzv. "garbage readings", např. 255/255/255 nebo 0.6).
    // Proto ho nejprve necháme 3x změřit barvu jen "na prázdno" a data zahodíme.
    for (int i = 0; i < 3; i++) {
        rkColorSensorGetRGB("front", &r, &g, &b);
        vTaskDelay(pdMS_TO_TICKS(150)); // Malé zpoždění, ať má senzor čas na reakci
    }

    while (true) {
        if (g_match_ended) {
            vypni_civky();
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }
        // Přečtení hodnot ze senzoru přímo ve funkci
        if (rkColorSensorGetRGB("front", &r, &g, &b)) {
            // ROZHODOVÁNÍ O BARVĚ PROBÍHÁ POUZE JEDNOU:
            char barva = urci_barvu_puku(r, g, b);
            
            // Pokud zachytíme reálný puk (barva NENÍ neznámá)
            if (barva != 'N') {
                if (barva == nase_barva) {
                    pocet_nasich_puku++;
                    roztrid_puk(barva);
                    
                    // Rychlá kalibrace po každém 2. našem puku
                    if (pocet_nasich_puku % 2 == 0) {
                        Serial.println(">> Rychla kalibrace tridice po 2. nasem puku...");
                        
                        // 1) Přejedeme pozici o 40° po směru, aby uvolnila prostor pro servo-stopku
                        otoc_motorem(40, false);
                        vTaskDelay(pdMS_TO_TICKS(50));
                        
                        // 2) Sklopíme dorazové servo dolů
                        rkServosSetPosition(2, 75);
                        vTaskDelay(pdMS_TO_TICKS(150));
                        
                        // 3) Otočíme se zpět proti směru o 170°, čímž narazíme na doraz a srovnáme se
                        otoc_motorem(170, true);
                        vTaskDelay(pdMS_TO_TICKS(50));
                        
                        // 4) Servo zvedneme nahoru
                        rkServosSetPosition(2, 0);
                        vTaskDelay(pdMS_TO_TICKS(100));
                        
                        Serial.println(">> Rychla kalibrace dokoncena.");
                    }
                } else {
                    roztrid_puk(barva);
                }
                
                g_puk_roztrizen = true;
                pocet_roz_p++;       // Počítadlo celkově roztříděných puků
                g_lajny_bez_puku = 0; // Reset počítadla lajn bez puků
                
                // Počkáme chvilku po roztřídění, ať si třídič "oddechne"
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
        
        // Asynchronní požadavek na rychlou kalibraci třídiče (např. po 3 lajnách bez puku)
        if (g_zadost_o_srovnani) {
            Serial.println(">> Rychla kalibrace na zadost (dlouho bez puku)...");
            
            // 1) Otočení o 160° po směru (ze základní pozice do přetočené)
            otoc_motorem(160, false);
            vTaskDelay(pdMS_TO_TICKS(50));
            
            // 2) Sklopíme dorazové servo dolů
            rkServosSetPosition(2, 75);
            vTaskDelay(pdMS_TO_TICKS(150));
            
            // 3) Otočení se zpět proti směru o 170° nadoraz
            otoc_motorem(170, true);
            vTaskDelay(pdMS_TO_TICKS(50));
            
            // 4) Servo zvedneme nahoru
            rkServosSetPosition(2, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
            
            g_zadost_o_srovnani = false;
            Serial.println(">> Rychla kalibrace na zadost dokoncena.");
        }
        
        // Zpoždění aby vlákno neběželo na 100% zátěži CPU (FreeRTOS)
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
