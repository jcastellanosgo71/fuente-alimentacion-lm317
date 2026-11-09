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
| **R1** | 1 | Resistencia (R) | Resistencia fija Through-Hole (LED indicador) |
| **R2** | 1 | $240\,\Omega$ / ¼ W | Resistencia de programación del LM317 (fija en v2.0) |
| **~~RV1~~** | — | ~~Potenciómetro $5\,\text{k}\Omega$~~ | **Retirado en v2.0** (sustituido por control digital) |
| **J1, J4** | 2 | Bornera 2 pines | Entradas/Salidas principales |
| **J2, J3** | 2 | Pin Header 1x02 | Salidas auxiliares $V_{CC}$ y $V_{DD}$ |
| **U2** | 1 | Opamp MCP6002 (o LM358) | Buffer/amplificador no inversor $G=2.15$ **(v2.0)** |
| **R3** | 1 | $10\,\text{k}\Omega$ / ¼ W | Resistencia del filtro RC **(v2.0)** |
| **C5** | 1 | $10\,\mu\text{F}$ / 16 V | Condensador del filtro RC **(v2.0)** |
| **R4** | 1 | $10\,\text{k}\Omega$ / ¼ W ($R_g$) | Resistencia de la red de ganancia **(v2.0)** |
| **R5** | 1 | $11.5\,\text{k}\Omega$ / ¼ W ($R_f'$) | Resistencia de realimentación de ganancia **(v2.0)** |
| **C6** | 1 | $100\,\text{nF}$ | Desacople de alimentación del opamp **(v2.0)** |
| **J5** | 1 | Pin Header 1x03 | Header de control Arduino: `PWM_IN`, `GND`, `VCC_OPAMP` **(v2.0)** |

> 📝 **Nota v2.0:** el potenciómetro $RV_1$ **se retira** y $R_2$ **pasa a valor fijo $240\,\Omega$**. Los componentes marcados **(v2.0)** son la etapa de control digital añadida. Ver [Control Digital v2.0](#-control-digital-v20).

---

## 🤖 Control Digital v2.0
En la **v2.0** se elimina el ajuste manual con potenciómetro ($RV_1$) y el voltaje de salida pasa a controlarse con un **Arduino estándar** (Uno/Nano), **sin perder la naturaleza de electrónica de potencia**: el elemento que entrega la potencia a la carga sigue siendo el regulador **lineal LM317** con rectificación por diodos a la entrada. El Arduino solo aporta una **consigna analógica de bajo nivel**.

El lazo de control es **PWM → RC → opamp → nodo ADJ**:

$$\text{PWM (0–5 V)} \;\rightarrow\; \text{Filtro RC} \;\rightarrow\; \text{Opamp no inversor } (G=2.15) \;\rightarrow\; \text{nodo ADJ del LM317}$$

1. **PWM:** un pin del Arduino genera una onda de $0\text{–}5\,\text{V}$ con ciclo de trabajo $D$ programable.
2. **Filtro RC** ($R_3=10\,\text{k}\Omega$, $C_5=10\,\mu\text{F}$, $f_c\approx 1.59\,\text{Hz}$): convierte el PWM en una tensión DC limpia $V_{rc}\in[0,5]\,\text{V}$.
3. **Opamp no inversor** ($R_g=10\,\text{k}\Omega$, $R_f'=11.5\,\text{k}\Omega$): amplifica a $0\text{–}10.75\,\text{V}$ y **aísla** el Arduino (lógica $5\,\text{V}$) del nodo ADJ, que puede superar los $10\,\text{V}$.
4. **Nodo ADJ:** como el LM317 mantiene $1.25\,\text{V}$ entre $V_O$ y ADJ, controlar el nodo ADJ controla la salida: $V_{out}=V_{ADJ}+1.25\,\text{V}$.

Relación consigna → salida (rango $1.25\,\text{V}$ a $\approx 12\,\text{V}$):

$$V_{out} = 10.75\cdot D + 1.25\,\text{V}\qquad D\in[0,1]$$

> ⚠️ **Masa común:** el $GNDREF$ de la placa y la masa del Arduino deben unirse (vía $J_5$). El Arduino se alimenta **aparte**.
>
> 🌡️ El LM317 es lineal: **requiere disipador** (el peor caso térmico es a $V_{out}$ mínima). Ver cálculo en el documento de diseño.

**Documentación de la v2.0:**
- 📐 Diseño y cálculos (filtro RC, ganancia, $R_2$, térmica): [`docs/DISENO_V2_CONTROL_DIGITAL.md`](docs/DISENO_V2_CONTROL_DIGITAL.md)
- 🛠️ Guía de modificación en KiCad (esquemático + PCB): [`docs/GUIA_MODIFICACION_KICAD.md`](docs/GUIA_MODIFICACION_KICAD.md)
- 💻 Firmware del Arduino: [`firmware/control_lm317.ino`](firmware/control_lm317.ino)

---

## 🚀 Roadmap (Versión 2.0 - Digitalización)
- [x] Reemplazar potenciómetro analógico ($RV_1$) por control digital (PWM → RC → opamp → nodo ADJ). **Implementado en v2.0.**
- [ ] Control por microcontrolador (ESP32 / STM32 / ATmega328P).
- [ ] Lectura de voltaje y corriente en tiempo real con sensor INA219.
- [ ] Integración de pantalla OLED / LCD I2C.
- [ ] Protección por sobrecorriente implementada por software/hardware.
