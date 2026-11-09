# Diseño v2.0 — Control Digital del LM317 (PWM → RC → Opamp → ADJ)

> Documento de diseño para la **Fuente de Alimentación Lineal Regulable v2.0**.
> Objetivo: sustituir el potenciómetro manual $RV_1$ por un control digital con **Arduino estándar** (Uno/Nano), **sin perder la naturaleza de electrónica de potencia** (regulación lineal con el LM317 y control analógico del nodo de ajuste).

---

## 1. 📌 Problema y objetivo

En la v1.0 el voltaje de salida se fija con un potenciómetro de $5\,\text{k}\Omega$ ($RV_1$) conectado entre el **nodo ADJ** del LM317 y la masa ($GNDREF$). El requisito de la práctica es **eliminar ese potenciómetro** y controlar la salida con un microcontrolador, manteniendo el enfoque de electrónica de potencia (rectificación con diodos + regulación lineal).

Recordemos la ecuación de salida del LM317:

$$V_{out} = 1.25\,\text{V}\cdot\left(1 + \dfrac{R_{aj}}{R_2}\right) + I_{ADJ}\cdot R_{aj}$$

El término $I_{ADJ}\cdot R_{aj}$ es despreciable porque $I_{ADJ}\approx 50\text{–}100\,\mu\text{A}$, de modo que trabajamos con la forma clásica:

$$\boxed{\,V_{out} = 1.25\,\text{V}\cdot\left(1 + \dfrac{R_{aj}}{R_2}\right)\,}\qquad R_2 = 240\,\Omega \text{ (fijo)}$$

En la v1.0, $R_{aj}$ era la posición del potenciómetro $RV_1$. La idea de la v2.0 es **no variar una resistencia**, sino **inyectar directamente una tensión controlada en el nodo ADJ**. Esto es equivalente y más elegante, porque el LM317 mantiene siempre $1.25\,\text{V}$ entre su pin de salida ($V_O$) y su pin ADJ. Por tanto, respecto a masa:

$$\boxed{\,V_{out} = V_{ADJ} + 1.25\,\text{V}\,}$$

Es decir: **si controlamos el potencial del nodo ADJ respecto a $GNDREF$, controlamos la salida directamente.**

| $V_{ADJ}$ (respecto a GND) | $V_{out}$ |
| :---: | :---: |
| $0\,\text{V}$ | $1.25\,\text{V}$ |
| $5\,\text{V}$ | $6.25\,\text{V}$ |
| $10.75\,\text{V}$ | $12.0\,\text{V}$ |

Nuestro rango objetivo es **$1.25\,\text{V}$ a $\approx 12\,\text{V}$**, lo que exige barrer el nodo ADJ de **$0\,\text{V}$ a $10.75\,\text{V}$**.

> ⚠️ **Punto clave de seguridad:** el nodo ADJ puede estar a más de $10\,\text{V}$. Un pin de Arduino (lógica $5\,\text{V}$) **no puede conectarse directamente** a ese nodo: lo dañaría. Por eso usamos un **opamp como amplificador/buffer de aislamiento**.

---

## 2. 🔧 Topología de reemplazo

El potenciómetro $RV_1$ se retira y en su lugar un **opamp** fija la tensión del nodo ADJ. La señal de mando viene del Arduino como un **PWM filtrado a tensión DC**:

```
 ARDUINO (alim. aparte, 5V)             ETAPA ANALÓGICA DE POTENCIA
 +-------------------+
 |        PWM (D9/D10)|  0–5 V,            Vcc_op ≈ 18 V
 |   ~490 / 980 Hz    |  ~490 Hz             |
 |   duty 0–100%      |                      |          LM317 (U1)
 +----------+---------+                 +----+----+     +--------+
            |                           |    +    |     | VI (3) |<-- VCC ~18 V
            |   Rf = 10k                 \   OPAMP |     |        |
            v  (filtro RC)               |\        |     | VO (2) |--> VDD = Vout
     Rf 10k          Vrc (0–5 V DC)      | \_______|     |        |----+--> salida
   o---/\/\/---+-------------------------|+ (no inv)     | ADJ(1) |    |
   PWM         |                         |              /+--------+    |
              ===  C = 10 µF             |  Vadj=G·Vrc  |              |
               |                         |  (0–10.75 V) |          R2 = 240 Ω
              GND                        +------+-------+              |
                                   Rg 10k |      |  Rf' 11.5k          |
                                          +-/\/\-+--/\/\---+-----------+ (nodo ADJ)
                                          |                |
                                         GND         (realimentación G)
```

Flujo de la señal:

1. **Arduino → PWM:** un pin genera una onda cuadrada de $0\text{–}5\,\text{V}$ con ciclo de trabajo (duty) programable. La media temporal de esa onda es $\overline{V}=D\cdot 5\,\text{V}$.
2. **Filtro RC pasa-bajos:** convierte el PWM en una tensión DC limpia $V_{rc}$ de $0$ a $5\,\text{V}$.
3. **Opamp (no inversor, ganancia $G$):** amplifica $0\text{–}5\,\text{V}$ a $0\text{–}10.75\,\text{V}$ y, sobre todo, **desacopla** el frágil mundo de $5\,\text{V}$ del Arduino del nodo ADJ (que puede superar los $10\,\text{V}$). El opamp entrega la corriente que pida el nodo ADJ sin cargar el filtro RC.
4. **Nodo ADJ del LM317:** recibe la tensión del opamp. El LM317 ajusta $V_{out}=V_{ADJ}+1.25\,\text{V}$ con regulación **lineal** y capacidad de corriente de potencia.

**¿Por qué este método conserva la electrónica de potencia?** Porque el elemento que entrega la potencia a la carga sigue siendo el **regulador lineal LM317** (etapa de disipación real, con rectificación por diodos a la entrada). El Arduino solo aporta una **consigna analógica de bajo nivel**; no conmuta la corriente de la carga. Mantenemos así el espíritu "lineal / analógico" de la asignatura, a diferencia de una fuente conmutada.

---

## 3. 🧮 Cálculo del filtro RC

El PWM del Arduino en los pines estándar tiene una frecuencia base de **$\approx 490\,\text{Hz}$** (Timer1/Timer2 en modo por defecto; los pines 5 y 6 van a $\approx 980\,\text{Hz}$). Para obtener una DC limpia, la frecuencia de corte del filtro debe quedar **muy por debajo** de $490\,\text{Hz}$.

Filtro pasa-bajos RC de primer orden:

$$f_c = \dfrac{1}{2\pi R C}$$

**Valores elegidos:** $R = 10\,\text{k}\Omega$, $C = 10\,\mu\text{F}$.

$$f_c = \dfrac{1}{2\pi\cdot 10000\cdot 10\times10^{-6}} = \dfrac{1}{0.628} \approx 1.59\,\text{Hz}$$

La relación de atenuación respecto al PWM es $f_c/f_{pwm}\approx 1.59/490 \approx 1/308$, más de dos décadas por debajo: excelente atenuación del rizado.

**Rizado residual.** El peor caso de rizado pico a pico en un filtro de primer orden se da a duty $D=0.5$ y vale aproximadamente:

$$V_{r(pp)} \approx \dfrac{V_{DD}\cdot D\,(1-D)}{f_{pwm}\cdot R\,C} = \dfrac{5\cdot 0.25}{490\cdot (10000\cdot 10\times10^{-6})} = \dfrac{1.25}{490\cdot 0.1} \approx 25.5\,\text{mV}$$

Tras la ganancia del opamp ($G=2.15$, ver sección 4), ese rizado se refleja en la salida como $\approx 25.5\,\text{mV}\times 2.15 \approx 55\,\text{mV}_{pp}$, perfectamente aceptable para una fuente de laboratorio.

**Compromiso rizado ↔ tiempo de establecimiento.** La constante de tiempo es:

$$\tau = R\,C = 10000\cdot 10\times10^{-6} = 0.1\,\text{s}\quad\Rightarrow\quad t_{est}\approx 5\tau = 0.5\,\text{s}$$

Bajar $f_c$ reduce el rizado pero hace la respuesta más lenta. Con $C=10\,\mu\text{F}$ el ajuste de salida tarda $\approx 0.5\,\text{s}$ en estabilizarse: un buen equilibrio para una fuente de alimentación (no necesitamos cambios rápidos de consigna).

> 💡 **Opción de menor rizado:** si se quisiera menos de $10\,\text{mV}_{pp}$ sin ralentizar tanto, puede usarse un **filtro RC de dos etapas** ($R=10\,\text{k}\Omega + C=4.7\,\mu\text{F}$ seguido de $R=10\,\text{k}\Omega + C=4.7\,\mu\text{F}$), que atenúa a $-40\,\text{dB/década}$. Para esta v2.0 el filtro simple $10\,\text{k}\Omega/10\,\mu\text{F}$ es suficiente.

---

## 4. 📐 Mapeo $0\text{–}5\,\text{V} \rightarrow$ rango de ADJ y ganancia del opamp

El filtro RC entrega $V_{rc}\in[0,\,5]\,\text{V}$. Necesitamos que el nodo ADJ barra $[0,\,10.75]\,\text{V}$ para cubrir $V_{out}\in[1.25,\,12]\,\text{V}$ (ver sección 1, pues $V_{out}=V_{ADJ}+1.25\,\text{V}$).

Como ambos rangos **empiezan en 0 V**, basta un **amplificador no inversor** (sin offset), cuya ganancia es:

$$G = 1 + \dfrac{R_f'}{R_g}$$

Ganancia requerida:

$$G = \dfrac{V_{ADJ,\max}}{V_{rc,\max}} = \dfrac{10.75\,\text{V}}{5\,\text{V}} = 2.15$$

**Resistencias elegidas (serie E96):** $R_g = 10\,\text{k}\Omega$, $R_f' = 11.5\,\text{k}\Omega$.

$$G = 1 + \dfrac{11.5\,\text{k}\Omega}{10\,\text{k}\Omega} = 1 + 1.15 = 2.15 \quad\checkmark$$

Comprobación de los extremos del barrido:

| $V_{rc}$ (RC) | $V_{ADJ}=G\cdot V_{rc}$ | $V_{out}=V_{ADJ}+1.25$ |
| :---: | :---: | :---: |
| $0\,\text{V}$ | $0\,\text{V}$ | $1.25\,\text{V}$ |
| $2.5\,\text{V}$ | $5.38\,\text{V}$ | $6.63\,\text{V}$ |
| $5.0\,\text{V}$ | $10.75\,\text{V}$ | $12.0\,\text{V}$ $\checkmark$ |

Relación consigna → salida (útil para el firmware):

$$V_{out} = 2.15\cdot(D\cdot 5\,\text{V}) + 1.25\,\text{V} = 10.75\cdot D + 1.25\,\text{V}$$

donde $D\in[0,1]$ es el ciclo de trabajo del PWM.

> 💡 Si solo se dispone de resistencias E24, usar $R_f'=11\,\text{k}\Omega$ ($G=2.1$) da un máximo de $V_{out}\approx 11.75\,\text{V}$, diferencia despreciable para la práctica.

### 4.1 🎚️ Resolución: límite de precisión del PWM de 8 bits

El `analogWrite()` estándar del Arduino Uno/Nano usa **PWM de 8 bits**, es decir, solo **256 niveles de duty** ($0$ a $255$). Como la excursión de salida es de $10.75\,\text{V}$ sobre esos $255$ pasos, el **escalón mínimo (1 LSB)** en la salida es:

$$\Delta V_{out} = \dfrac{10.75\,\text{V}}{255} \approx 42\,\text{mV/LSB}$$

Esto significa que la salida **no es continua**: solo puede fijarse en múltiplos de $\approx 42\,\text{mV}$. Frente al **potenciómetro de la v1.0**, que era un ajuste continuo (resolución "infinita" limitada solo por la mano del usuario), el control digital introduce esta **cuantización** como precio de la automatización. Nótese que el escalón de cuantización ($\approx 42\,\text{mV}$) es del mismo orden que el rizado residual ($\approx 55\,\text{mV}_{pp}$, §3): ambos son los dos límites de precisión del diseño y conviene presentarlos juntos. El firmware (`dutyAVoltaje()`) **informa del voltaje realmente aplicado** tras esta cuantización, para que el estudiante vea la diferencia con lo pedido.

> 💡 **Mejora opcional (mayor resolución):** si se necesita un ajuste más fino, puede subirse la resolución del PWM reconfigurando el Timer a modo de más bits (p. ej. 10 bits → $\approx 10.5\,\text{mV/LSB}$), usar un DAC real (MCP4725, 12 bits → $\approx 2.6\,\text{mV/LSB}$) o un micro con DAC integrado. Para esta v2.0 los 8 bits son suficientes para una fuente de laboratorio didáctica.

---

## 5. ⚙️ Elección y alimentación del opamp

**Requisitos del opamp:**

- Debe poder **entregar en su salida tensiones por encima de $5\,\text{V}$** (hasta $\approx 10.75\,\text{V}$ en el nodo ADJ). Por tanto **no puede alimentarse a $5\,\text{V}$**.
- Debe funcionar bien cerca de $0\,\text{V}$ en la salida (para alcanzar $V_{out}=1.25\,\text{V}$, es decir $V_{ADJ}=0\,\text{V}$).
- La corriente exigida es mínima: el nodo ADJ del LM317 toma solo $I_{ADJ}\approx 50\text{–}100\,\mu\text{A}$.

**Alimentación recomendada:** alimentar el opamp desde el riel **$V_{CC}\approx 18\,\text{V}$** (la tensión ya rectificada y filtrada de entrada al LM317), o desde un riel auxiliar de $12\text{–}15\,\text{V}$. Con $18\,\text{V}$ de alimentación, un opamp alcanza holgadamente los $10.75\,\text{V}$ de salida necesarios.

> 🚫 **El opamp NUNCA se alimenta desde el Arduino.** El riel de $5\,\text{V}$ del Arduino **no serviría** (el opamp debe entregar hasta $\approx 10.75\,\text{V}$ en su salida, imposible con $5\,\text{V}$ de alimentación) y, además, mezclaría la electrónica de potencia con la placa de control. La alimentación positiva del opamp (`VCC_OPAMP`, pin 3 del header `J5`) proviene **siempre de la PLACA** ($V_{CC}\approx 18\,\text{V}$) **o de un riel auxiliar externo** de $12\text{–}15\,\text{V}$, **nunca del Arduino**. El Arduino se alimenta **aparte** (por su USB o su jack) y **solo comparte GND** con la placa (pin 2 de `J5`). Así se respeta la premisa de "Arduino alimentado aparte": la única conexión eléctrica Arduino↔placa son la señal PWM (pin 1 de `J5`) y la masa común (pin 2).

> 📌 **Masa común obligatoria:** el $GNDREF$ de la placa, la masa del filtro RC y la masa del Arduino **deben estar unidas**. Si no, la referencia de $0\,\text{V}$ del PWM no coincide con la del nodo ADJ y el control no funciona. Esta masa común **no** implica alimentar el opamp desde el Arduino: solo comparten el $0\,\text{V}$ de referencia.

**Opamp sugerido:**

| Opamp | Ventaja | Limitación a tener en cuenta |
| :--- | :--- | :--- |
| **MCP6002 / MCP6022** (rail-to-rail) | Salida casi a $0\,\text{V}$ y cerca del riel; ideal para llegar a $V_{out}=1.25\,\text{V}$ | — |
| **LM358** (clásico, barato) | Muy disponible; su salida llega casi a $0\,\text{V}$ con alimentación simple | No es rail-to-rail por arriba; con $V_{CC}=18\,\text{V}$ sobra margen, así que no es problema |
| **TL071** (FET, bajo ruido) | Bajo ruido, alta impedancia de entrada | Su salida **no** baja hasta $0\,\text{V}$ (se queda a $\sim 1.5\,\text{V}$ del riel negativo); dificulta alcanzar $V_{out}=1.25\,\text{V}$ con alimentación simple |

**Recomendación:** **MCP6002** (rail-to-rail, alimentación simple), porque garantiza alcanzar ambos extremos del rango ($0\,\text{V}$ y $10.75\,\text{V}$ en ADJ) con un solo riel de $18\,\text{V}$. El **LM358** es una alternativa económica perfectamente válida.

**Por qué el opamp actúa como buffer de baja corriente:** el nodo ADJ pide solo decenas de microamperios. El opamp los entrega sin esfuerzo y, gracias a su alta impedancia de entrada, **no carga el filtro RC** (que es de alta impedancia, $R=10\,\text{k}\Omega$). Así la tensión DC del filtro no se degrada y el nodo ADJ queda alimentado por una fuente de baja impedancia y estable.

---

## 6. 🔩 $R_2$: valor y potencia

Se fija $R_2 = 240\,\Omega$ (valor típico recomendado por el datasheet del LM317; antes era genérico "R").

Corriente de programación (la mínima que debe circular para que el LM317 regule, $> 3.5\,\text{mA}$):

$$I_{prog} = \dfrac{V_{ref}}{R_2} = \dfrac{1.25\,\text{V}}{240\,\Omega} \approx 5.2\,\text{mA}\quad(> 3.5\,\text{mA}\ \checkmark)$$

Potencia disipada en $R_2$:

$$P_{R_2} = \dfrac{V_{ref}^2}{R_2} = \dfrac{(1.25\,\text{V})^2}{240\,\Omega} \approx 6.5\,\text{mW}$$

**Recomendación:** una resistencia de **$240\,\Omega$, $\tfrac{1}{4}\,\text{W}$** es más que suficiente (margen $\times 38$).

---

## 7. 🌡️ Disipación térmica del LM317

El LM317 es un regulador **lineal**: disipa como calor la diferencia entre la tensión de entrada y la de salida, multiplicada por la corriente de carga.

$$P_{disip} = (V_{in} - V_{out})\cdot I_{load}$$

Con $V_{CC}=V_{in}=18\,\text{V}$ y una corriente de carga asumida de **$I_{load}=0.5\,\text{A}$** (valor razonable para esta fuente de laboratorio):

| Caso | $V_{out}$ | $V_{in}-V_{out}$ | $P_{disip}$ ($I=0.5\,\text{A}$) |
| :--- | :---: | :---: | :---: |
| **Peor caso (salida mínima)** | $1.25\,\text{V}$ | $16.75\,\text{V}$ | $\approx 8.4\,\text{W}$ |
| Salida máxima | $12\,\text{V}$ | $6\,\text{V}$ | $\approx 3.0\,\text{W}$ |

> El **peor caso térmico es con $V_{out}$ mínima**, porque es cuando mayor tensión cae sobre el regulador.

**Temperatura de unión** ($T_a = 25\,^\circ\text{C}$ ambiente):

$$T_j = T_a + P_{disip}\cdot R_{th}$$

**Sin disipador** (TO-220 al aire, $R_{th(JA)}\approx 50\,^\circ\text{C/W}$):

$$T_j = 25 + 8.4\cdot 50 \approx 444\,^\circ\text{C}\;\; \text{(imposible: el límite es } 125\,^\circ\text{C)}$$

**Con disipador** ($R_{th(JC)}=3\,^\circ\text{C/W}$, interfaz $R_{th(CS)}=1\,^\circ\text{C/W}$, disipador $R_{th(SA)}=11\,^\circ\text{C/W}$ ⇒ $R_{th}\approx 15\,^\circ\text{C/W}$):

$$T_j = 25 + 8.4\cdot 15 \approx 151\,^\circ\text{C}$$

Todavía **excede** los $125\,^\circ\text{C}$ en el peor caso a $0.5\,\text{A}$. Para el caso de $V_{out}=12\,\text{V}$ ($P=3\,\text{W}$): $T_j = 25 + 3\cdot 15 = 70\,^\circ\text{C}$ (correcto).

**Conclusiones:**

1. **Es obligatorio un disipador.** Sin él, el LM317 se destruye casi en cualquier condición.
2. Para operar con salidas bajas a $0.5\,\text{A}$ de forma segura, usar un **disipador más grande** ($R_{th(SA)}\le 5\,^\circ\text{C/W}$) **o limitar la corriente**. Con $R_{th}\approx 9\,^\circ\text{C/W}$ total: $T_j = 25 + 8.4\cdot 9 \approx 100\,^\circ\text{C}$ (dentro de margen).
3. Reducir $V_{CC}$ (p. ej. un transformador de menor tensión) también baja drásticamente la disipación en el peor caso. Es la mejora térmica más eficaz si no se necesitan $12\,\text{V}$ de salida con cabecera holgada.
4. Si se quisiera operar a $I_{load}=1\,\text{A}$, el peor caso sube a $\approx 16.75\,\text{W}$, inviable con disipación pasiva razonable: habría que bajar $V_{CC}$ o limitar la corriente.

### 7.1 🔒 Decisión de diseño por defecto (configuración segura)

Para que el montaje sea seguro **sin depender de una advertencia**, la v2.0 **fija una configuración por defecto** que cierra el caso térmico: se dimensiona el disipador para que el **peor caso** ($V_{out}=1.25\,\text{V}$, $I_{load}=0.5\,\text{A}$, $P_{disip}\approx 8.4\,\text{W}$) quede dentro de especificación.

**Requisito:** resistencia térmica **total** $R_{th(JA)}\le 9\,^\circ\text{C/W}$. Con $R_{th(JC)}=3\,^\circ\text{C/W}$ e interfaz $R_{th(CS)}=1\,^\circ\text{C/W}$, el disipador debe aportar:

$$R_{th(SA)} \le 9 - 3 - 1 = 5\,^\circ\text{C/W}$$

Con ese disipador, la temperatura de unión en el peor caso queda:

$$T_j = 25 + 8.4\cdot 9 \approx 100\,^\circ\text{C}\quad(< 125\,^\circ\text{C}\ \checkmark)$$

**Componente fijado en el BOM (ver §8):** disipador TO-220 con **$R_{th(SA)}\le 5\,^\circ\text{C/W}$** (p. ej. Aavid/Fischer tipo clip o atornillable de aletas, $\sim 25\text{–}35\,\text{mm}$ de alto), **más pasta térmica o almohadilla** ($R_{th(CS)}\approx 1\,^\circ\text{C/W}$). Este es el disipador **por defecto** del diseño, no una opción.

> ⚠️ **Límite de corriente por defecto.** Si solo se dispone de un disipador modesto ($R_{th(SA)}\approx 11\,^\circ\text{C/W}$, $R_{th}\approx 15\,^\circ\text{C/W}$), **limita la corriente de carga a $I_{load}\le 0.3\,\text{A}$** en el peor caso: $T_j = 25 + (16.75\cdot 0.3)\cdot 15 \approx 100\,^\circ\text{C}$. Es decir: **o** disipador de $R_{th(SA)}\le 5\,^\circ\text{C/W}$ a $0.5\,\text{A}$, **o** disipador pequeño con $I_{load}\le 0.3\,\text{A}$. Ambas configuraciones son seguras; elige una explícitamente.

---

## 8. 📦 BOM delta (componentes nuevos)

Componentes **añadidos** para la etapa de control digital (además de retirar $RV_1$ y fijar $R_2$):

| Designador (sugerido) | Cantidad | Componente / Valor | Función |
| :--- | :---: | :--- | :--- |
| **U2** | 1 | Opamp MCP6002 (o LM358) | Buffer/amplificador no inversor, $G=2.15$ |
| **R3** | 1 | $10\,\text{k}\Omega$ ¼ W | Resistencia del filtro RC |
| **C5** | 1 | $10\,\mu\text{F}$ / 16 V | Condensador del filtro RC |
| **R4** | 1 | $10\,\text{k}\Omega$ ¼ W ($R_g$) | Resistencia a masa de la red de ganancia |
| **R5** | 1 | $11.5\,\text{k}\Omega$ ¼ W ($R_f'$) | Resistencia de realimentación de ganancia |
| **C6** | 1 | $100\,\text{nF}$ | Desacople de alimentación del opamp |
| **J5** | 1 | Pin Header 1x03 | Conector de control: `PWM_IN`, `GND`, `VCC_OPAMP` (desde la placa, **no** del Arduino) |
| **HS1** | 1 | Disipador TO-220, $R_{th(SA)}\le 5\,^\circ\text{C/W}$ + pasta/almohadilla térmica | **Disipador por defecto** del LM317 (cierra el peor caso térmico a $0.5\,\text{A}$; ver §7.1) |

**Cambios sobre la v1.0:**

- ❌ **Se retira $RV_1$** (potenciómetro $5\,\text{k}\Omega$). Su función la asume el opamp inyectando tensión en el nodo ADJ.
- 🔧 **$R_2$ pasa a valor fijo $240\,\Omega$ / ¼ W** (en v1.0 estaba como "R" genérico).
- 🔌 El opamp ($U_2$) se alimenta desde **$V_{CC}\approx 18\,\text{V}$ de la placa** (o riel auxiliar externo $12\text{–}15\,\text{V}$), **nunca desde el Arduino**. El pin `VCC_OPAMP` de `J5` trae esa alimentación de placa/riel auxiliar, no del Arduino.
- 🌡️ **Disipador por defecto $HS_1$** con $R_{th(SA)}\le 5\,^\circ\text{C/W}$ (configuración segura a $0.5\,\text{A}$; alternativa: disipador pequeño con $I_{load}\le 0.3\,\text{A}$, ver §7.1).
- 🪨 El Arduino se alimenta **aparte**; solo comparte **GND** (y recibe/envía la señal PWM) con la placa a través de `J5`.

---

## 9. ✅ Resumen de valores de diseño

| Parámetro | Valor |
| :--- | :--- |
| Entrada rectificada/filtrada $V_{CC}$ | $\approx 18\,\text{V}$ DC |
| Rango de salida $V_{out}$ | $1.25\,\text{V}$ a $\approx 12\,\text{V}$ |
| $R_2$ (programación) | $240\,\Omega$ / ¼ W |
| Filtro RC | $R=10\,\text{k}\Omega$, $C=10\,\mu\text{F}$ → $f_c\approx 1.59\,\text{Hz}$ |
| Rizado tras filtro | $\approx 25.5\,\text{mV}_{pp}$ ($\approx 55\,\text{mV}_{pp}$ en salida) |
| Ganancia opamp | $G=2.15$ ($R_g=10\,\text{k}\Omega$, $R_f'=11.5\,\text{k}\Omega$) |
| Nodo ADJ | $0\,\text{V}$ a $10.75\,\text{V}$ |
| Relación consigna | $V_{out} = 10.75\cdot D + 1.25\,\text{V}$ |
| Resolución (PWM 8 bits) | $\approx 42\,\text{mV/LSB}$ en la salida ($10.75\,\text{V}/255$) |
| Disipación LM317 (peor caso, $0.5\,\text{A}$) | $\approx 8.4\,\text{W}$ |
| Disipador por defecto ($HS_1$) | $R_{th(SA)}\le 5\,^\circ\text{C/W}$ ⇒ $T_j\approx 100\,^\circ\text{C}$ a $0.5\,\text{A}$ (seguro) |
| Alternativa segura | disipador pequeño ($R_{th(SA)}\approx 11\,^\circ\text{C/W}$) con $I_{load}\le 0.3\,\text{A}$ |

---

*Documento de la v2.0. La modificación física del esquemático/PCB se describe en la guía de modificación de KiCad; el firmware de control está en `firmware/control_lm317.ino`.*
