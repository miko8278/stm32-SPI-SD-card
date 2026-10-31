---
title: "Vergleich verschiedener Dateisysteme für
den SD-Karten-Einsatz in eingebetteten
Systemen"
author: "Michael Kolorz (miko8278@bht-berlin.de)"
institute: "Erstprüfer:Prof. Dr. Peter Gober \\
            Zweitprüfer:Prof. Dr. David Dietrich"
date: 4. November 2026
---


# Themen des Vortrags
\footnotesize
1. Überblick und Motivation
2. Hardwaredesign
3. Software
    1. SPI-SD-Treiber
    2. FatFs und littlefs
6. Dateisystemtests
    1. Analyse der Schreibfehlerraten mittels bekannter Zeichenfolge
    2. Analyse der Schreibfehlerraten mittels bekannter Zeichenfolge und manipulierter Schreibfunktion
    3. Analyse von pseudo-randomisierter Zeichenfolge bei Anhang an Datei
    4. Verhalten der Dateisysteme bei Stromunterbrechungen
    5. Direkte Blockadressierung (NoFs)
7. Fazit
\normalsize

---

# Überblick


---

# Motivation persönlich

-test 1
-test 2
---

# Motivation technisch

-test 3
-test 4

---
# Hardware: Schaltplan

\centering
![](Bilder/Schaltplan_main_highres_grey.png){ width=100% }


# Hardware: Platine unbestückt

\centering
![](Bilder/PreviewPCB.png){ width=100% }

# Hardware: Platine bestückt

\centering
![](Bilder/Platine_bestueckt.jpg){ width=100% }

# Software: SPI-SD-Treiber

# Software: FatFs

# Software: littlefs



# Bildertest 2 Nebeneinander

::: columns
::: column
![](Bilder/MISO_dip.png){ width=100% }
:::

::: column
![](Bilder/MISO_dip.png){ width=100% }
:::
:::

