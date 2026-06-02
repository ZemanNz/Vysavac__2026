# VYSAVAČ 2026 – Uživatelská Příručka & Checklist

Tento dokument slouží jako kompletní návod k obsluze, checklist před startem a popis chování autonomního robota **VYSAVAČ 2026**. Pro spolehlivý chod, přesné třídění a bezpečnost hardwaru na soutěži důsledně dodržujte následující pokyny.

---

## 📋 Checklist před každou jízdou (Předstartovní příprava)

Před každým spuštěním robota na dráze je nutné provést následující kroky. Zanedbání kteréhokoli bodu může vést k chybě v navigaci, nepřesnému třídění puků nebo fyzickému poškození robota.

### 1. 🎯 Srovnání a kalibrace třídiče
* **Postup:** Srovnání třídiče uděláme rukou před každým startem.

### 2. 🌀 Kalibrace gyroskopu (MPU)
* **Požadavek:** Při zapnutí robota nebo při stisku tlačítka **RST** (Reset) na desce RBCX musí robot stát **naprosto v klidu**.
* **Signalizace:** Během kalibrace gyroskopu žlutá LED dioda pětkrát rychle zabliká. 
* **Princip:** Program změří 100 vzorků úhlové rychlosti v klidu, vypočte zbytkový drift (`g_gyro_z_offset`) a následně ho za jízdy průběžně odečítá pro eliminaci akumulované chyby při zatáčení. Pokud se robotem během kalibrace pohne, gyroskop se zkalibruje špatně a robot bude zatáčet nepřesně!

### 3. 🔋 Baterie
* **Pracovní rozsah:** Baterie by se měla pohybovat v rozmezí **7.0 V až 8.8 V (7000 mV – 8800 mV)**.
* **Kontrola:** Při spuštění a následně každou vteřinu vypisuje robot stav baterie na sériový monitor. Nenechávejte baterii klesnout pod 7.0 V, jinak hrozí nestabilita procesoru ESP32 při zátěži motorů.

### 4. 🧼 Očista kol
* **Pravidlo:** Před **každou** soutěžní jízdou očistěte obě hnací kola jen mokrým hadrem.

---

## 🎮 Ovládání robota (Význam tlačítek na RBCX)

Po inicializaci a úspěšné kalibraci gyroskopu svítí **žlutá LED** (stav `READY`). Robot čeká na volbu režimu pomocí tlačítek na RBCX desce:

| Tlačítko | Akce | Popis |
| :--- | :--- | :--- |
| **`BTN_LEFT`** | **Start - MODRÁ strategie** | Nastaví naši barvu puků na **Modrou ('B')**, rozsvítí modrou LED (pozn. *modrá LED fyzicky nefunguje*), zhasne žlutou a po 500 ms odstartuje asynchronní třídicí vlákno (`tridici_vlakno`). |
| **`BTN_RIGHT`**| **Start - ČERVENÁ strategie** | Nastaví naši barvu puků na **Červenou ('R')**, rozsvítí červenou LED, zhasne žlutou a po 500 ms odstartuje asynchronní třídicí vlákno (`tridici_vlakno`). |
| **`BTN_DOWN`** | **Mechanická kalibrace** | **Pouze ve stavu READY:** Spustí výše popsanou kalibraci a srovnání třídicí klapky proti mechanické stopce. |
| **`BTN_ON`**   | **Test otočení PO směru** | Spustí testovací otočení kontinuálního serva `S1` o **120° po směru** s využitím brzdné rampy. Slouží k ověření přesnosti koeficientu `12.5f` (ON). |
| **`BTN_UP`**   | **Test otočení PROTI směru** | Spustí testovací otočení kontinuálního serva `S1` o **120° proti směru** s využitím brzdné rampy. Slouží k ověření přesnosti koeficientu `13.4f` (UP). |

---

## 📡 Popis LiDARu a UART komunikace

Robot využívá architekturu **dvou spolupracujících procesorů**:
1. **RBCX deska (Arduino/PlatformIO)** – Řídí pohony, serva, bzučák, LED diody a čte ultrazvukové a barevné senzory.
2. **Hlavní ESP32 (ESP32-detekce / "mozek")** – Zpracovává data z **LiDARu** a provádí pokročilé rozhodování.

### Jak funguje komunikace:
* **Směr RBCX ➡️ ESP32 (Stav):** RBCX periodicky každých **200 ms** sestavuje strukturu `RbcxStatus` a posílá ji po UARTu do hlavního ESP32. Status obsahuje:
  * Aktuální úhel z gyroskopu (`MPU`).
  * Hodnoty z levého a pravého enkodéru motorů.
  * Naměřené vzdálenosti z ultrazvuků (`uz1` a `uz3`).
  * Napětí baterie v mV.
  * Počet úspěšně vytříděných našich puků.
* **Směr ESP32 ➡️ RBCX (Příkazy):** Hlavní ESP32 posílá příkazy `EspCommand`, které RBCX vykonává (jízda, zatáčení, otevření/zavření zásobníků).
* **Korekce z LiDARu:** ESP32 porovnává naměřená data z LiDARu s mapou a posílá příkaz `CMD_LIDAR_ERROR` s parametrem úhlové odchylky. RBCX tuto hodnotu průběžně aplikuje jako korekci směru jízdy (`g_lidar_error`), což zajišťuje stabilní jízdu bez driftování.

---

## ⚙️ Třídicí mechanismus s brzdnou rampou

Třídič využívá **kontinuální servo S1** a **dorazové (stupňové) servo S2**.
* **Brzdná rampa:** Aby se zabránilo setrvačnému přetáčení kontinuálního serva při rychlém pohybu, kód implementuje lineární zpomalovací profil:
  * Do úhlu **45°** do cíle běží motor na maximální rychlost `v_max` (`35.0f` pro třídění).
  * V posledních 45° se rychlost lineárně snižuje až na minimum `v_min` (`6.0f`), což zajistí přesné a hladké zastavení na cílovém úhlu bez překmitu.
* **Rozdílné koeficienty rychlosti:** Fyzické asymetrie serva jsou kompenzovány směrovými koeficienty v `otoc_motorem`:
  * **Po směru (ON):** `12.5f` ms/stupeň.
  * **Proti směru (UP):** `13.4f` ms/stupeň.

---

## ⏱️ Řízení času zápasu a Nouzové chování

Program na RBCX automaticky hlídá čas zápasu od stisku startovacího tlačítka (LEFT/RIGHT):

1. **🚨 Nouzový návrat (Emergency Return) – 162. vteřina (2:42)**
   * Jakmile čas zápasu dosáhne **162 vteřin**, robot jednorázově dlouze zapíská bzučákem a aktivuje příznak nouzového návratu.
   * Hlavní ESP32 přeruší sběr a naviguje robota nejkratší cestou zpět do startovního pole (home zone), aby robot stihl zaparkovat před koncem limitu.
   
2. **🛑 Konec zápasu (Match End) – 180. vteřina (3:00)**
   * Přesně po **180 vteřinách** se aktivuje nouzové zastavení.
   * Okamžitě se zastaví oba motory pohonu (`rkMotorsSetPower(0, 0)`).
   * Třídicí task je ukončen (`vTaskDelete`), kontinuální servo `S1` a stopka `S2` jsou vypnuty (`rkServosDisable`), aby se serva nepřehřívala.
   * Bzučák dvakrát dlouze zapíská.
   * Všechny LED diody začnou blikat v intervalu 250 ms.
   * Robot zůstane uzamčen v nekonečné smyčce a ignoruje veškeré další příkazy až do fyzického resetu tlačítkem **RST**.