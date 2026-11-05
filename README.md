# Fuente de Alimentación Lineal Regulable - v1.0

## 📌 Descripción General
Diseño de hardware y PCB para una **Fuente de Alimentación Lineal Regulable v1.0** basada en el regulador lineal ajustable **LM317**. El circuito convierte una entrada de voltaje continuo/alterno en una salida regulada ajustable ($V_{DD}$) mediante potenciómetro manual.

---

## 🛠️ Especificaciones y Funcionamiento
- **Entrada y Rectificación:** Bornera $J_1$ a la entrada con puente rectificador de onda completa (4x diodos 1N4004 $D_1-D_4$).
- **Filtrado Principal:** Capacitores electrolíticos en paralelo ($C_1, C_2$ de $2200\,\mu\text{F} / 50\,\text{V}$) para minimizar el rizado en $V_{CC}$.
- **Regulación y Ajuste:** Regulador lineal LM317 ($U_1$) en encapsulado TO-220, ajustado mediante potenciómetro de $5\,\text{k}\Omega$ ($RV_1$) y resistencia $R_2$.
- **Protección y Estabilidad:** Diodos de protección $D_6, D_7$ (1N4004), capacitor de desacople de ajuste $C_3$ ($33\,\mu\text{F}$) y capacitor de salida $C_4$ ($1\,\mu\text{F}$ Tántalo).
- **Salidas y Monitor:** Bornera principal $J_4$, salidas auxiliares $J_2, J_3$ y LED indicador $D_5$.
- **Diseño PCB:** Placa de 2 capas con plano de masa continuo ($GNDREF$).

---

## 📋 Lista de Componentes (BOM)

| Designador | Cantidad | Componente / Valor | Descripción |
| :--- | :--- | :--- | :--- |
| **U1** | 1 | LM317 | Regulador lineal ajustable (TO-220) |
| **D1, D2, D3, D4, D6, D7** | 6 | 1N4004 | Diodos rectificadores |
| **D5** | 1 | LED | Diodo emisor de luz 5mm |
| **C1, C2** | 2 | $2200\,\mu\text{F} / 50\,\text{V}$ | Condensadores electrolíticos radiales |
| **C3** | 1 | $33\,\mu\text{F} / 25\,\text{V}$ | Condensador electrolítico radial |
| **C4** | 1 | $1\,\mu\text{F} / 25\,\text{V}$ | Condensador de tántalo |
| **R1, R2** | 2 | Resistencia (R) | Resistencias fijas Through-Hole |
| **RV1** | 1 | Potenciómetro $5\,\text{k}\Omega$ | Control de voltaje de salida |
| **J1, J4** | 2 | Bornera 2 pines | Entradas/Salidas principales |
| **J2, J3** | 2 | Pin Header 1x02 | Salidas auxiliares $V_{CC}$ y $V_{DD}$ |

---

## 🚀 Roadmap (Versión 2.0 - Digitalización)
- [ ] Reemplazar potenciómetro analógico ($RV_1$) por potenciómetro digital o DAC.
- [ ] Control por microcontrolador (ESP32 / STM32 / ATmega328P).
- [ ] Lectura de voltaje y corriente en tiempo real con sensor INA219.
- [ ] Integración de pantalla OLED / LCD I2C.
- [ ] Protección por sobrecorriente implementada por software/hardware.
