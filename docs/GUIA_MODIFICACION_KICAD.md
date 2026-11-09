# Guía de Modificación en KiCad — v1.0 → v2.0 (Control Digital)

> Guía paso a paso, **didáctica**, para transformar el esquemático y el PCB de la
> **Fuente de Alimentación Lineal Regulable** de la v1.0 (control manual con potenciómetro)
> a la v2.0 (control digital **PWM → RC → opamp → nodo ADJ**).
>
> Esta guía acompaña al documento de diseño [`DISENO_V2_CONTROL_DIGITAL.md`](DISENO_V2_CONTROL_DIGITAL.md)
> (fórmulas, cálculos y justificación) y al firmware [`../firmware/control_lm317.ino`](../firmware/control_lm317.ino).

---

## ⚠️ Advertencia inicial (léela antes de empezar)

**Todo este trabajo se hace DENTRO de KiCad**, usando las herramientas gráficas:

- **Eeschema** → editor de esquemático (archivo `Variable_Voltage_Source.kicad_sch`).
- **Pcbnew** → editor de placa (archivo `Variable_Voltage_Source.kicad_pcb`).

> 🚫 **NO edites a mano** los archivos `.kicad_sch` ni `.kicad_pcb` con un editor de texto.
> Son archivos en formato S-expression con geometría, posiciones de cables (*wires*),
> uniones (*junctions*), UUIDs y referencias cruzadas. Tocarlos manualmente **corrompe el proyecto**
> y KiCad puede negarse a abrirlo. Abre el proyecto en KiCad y realiza cada cambio con el ratón
> y los menús, tal y como se describe aquí.

### Datos del proyecto que debes conocer

- **Versión de KiCad:** 10 (formato de archivo `version 20260306`). Abre el proyecto con esa versión o superior.
- **Estilo del esquemático:** **no usa etiquetas locales (*labels*)**. La conectividad se hace con
  **símbolos de alimentación (*power symbols*) globales** y **cables (*wires*)** con uniones:
  - `VCC` = riel de entrada ya rectificado y filtrado (entra al LM317 por `VI`, pin 3). ≈ 18 V.
  - `VDD` = salida regulada (sale del LM317 por `VO`, pin 2).
  - `GNDREF` = masa común del circuito (plano de masa del PCB).
- **Componentes de referencia (v1.0):**
  - `U1` = **LM317** en TO-220. Pines: **`ADJ` = pin 1**, **`VO` = pin 2 (= VDD)**, **`VI` = pin 3 (= VCC)**.
  - `R2` = resistencia de programación (en v1.0 su *Value* es un `R` genérico; la fijaremos a **240 Ω**).
  - `RV1` = **potenciómetro 5 kΩ** conectado entre el **nodo ADJ** y `GNDREF`. **Es el componente a eliminar.**

> 🧭 **Idea de fondo:** describimos las conexiones en términos de **nodos existentes**
> (`ADJ` = pin 1 del `U1`, `GNDREF`, `VCC`), **nunca** con coordenadas de cables.
> En KiCad tú dibujas el cable; la posición exacta la decide el editor.

### Recomendación previa

1. Haz una **copia de seguridad** de la carpeta del proyecto (o confirma que está versionada en git y limpia).
2. Abre KiCad → `Variable_Voltage_Source.kicad_pro` → entra a **Eeschema**.

---

## Parte A — Esquemático (Eeschema)

Realiza los pasos en orden. Guarda (`Ctrl+S`) al terminar cada bloque.

### A.1 — Eliminar el potenciómetro `RV1`

El potenciómetro `RV1` (5 kΩ) está conectado entre el **nodo ADJ** (pin 1 del `U1`) y `GNDREF`.
En la v2.0 su función la asume el opamp, así que se retira por completo.

1. Haz clic sobre el símbolo de `RV1` para seleccionarlo.
2. Pulsa `Supr` (Delete) para eliminarlo.
3. Elimina también los **cables (*wires*)** que quedaran "en el aire" tras borrar `RV1`
   (selecciónalos y `Supr`). El objetivo es dejar **limpio** el nodo ADJ, porque ahí
   conectaremos la salida del opamp en el paso A.5.

> 💡 No borres el símbolo de `GNDREF` ni el pin `ADJ` del `U1`: solo el potenciómetro y sus cables sueltos.

### A.2 — Fijar el valor de `R2` a 240 Ω

1. Haz doble clic sobre `R2` para abrir **Symbol Properties** (Propiedades del símbolo).
2. En el campo **Value**, sustituye el `R` genérico por **`240`** (o `240R` / `240Ω`, según el estilo que prefieras; usa el mismo formato que el resto del proyecto).
3. Acepta con **OK**.

> Justificación del valor en [`DISENO_V2_CONTROL_DIGITAL.md`](DISENO_V2_CONTROL_DIGITAL.md) §6:
> $I_{prog}=1.25/240\approx 5.2\,\text{mA}>3.5\,\text{mA}$ y $P_{R_2}\approx 6.5\,\text{mW}$ (basta ¼ W).

### A.3 — Añadir el header de control del Arduino (`J5`)

Añadimos un conector para traer la señal de PWM del Arduino y compartir la masa.

1. Pulsa `A` (Place Symbol / Colocar símbolo) y busca en la librería **`Connector`**:
   - **`Conn_01x03`** (header 1x03) si quieres 3 pines: `PWM_IN`, `GND`, `VCC_OPAMP` (opcional), **recomendado**.
   - **`Conn_01x02`** (header 1x02) si solo necesitas `PWM_IN` y `GND`.
2. Colócalo en una zona despejada del esquemático. Asigna la referencia **`J5`**.
3. Pon un *Value* descriptivo (p. ej. `CTRL_ARDUINO`).
4. Define el criterio de pines (lo cablearemos en A.5):
   - **Pin 1 → `PWM_IN`** (entrada de la señal del Arduino, 0–5 V).
   - **Pin 2 → `GND`** (se conectará a `GNDREF`: masa común con el Arduino).
   - **Pin 3 → `VCC_OPAMP`** (opcional, por si alimentas el opamp desde fuera en vez de desde `VCC`).

> 🔗 **Masa común obligatoria:** la masa del Arduino debe unirse a `GNDREF` a través del pin 2 de `J5`.
> Sin esa referencia común, el PWM no comparte el 0 V con el nodo ADJ y el control no funciona
> (ver [`DISENO_V2_CONTROL_DIGITAL.md`](DISENO_V2_CONTROL_DIGITAL.md) §5).

### A.4 — Colocar el opamp y la red RC + ganancia

Coloca los componentes nuevos (sus valores salen del **BOM delta** del documento de diseño, §8).

1. **Opamp `U2`** — pulsa `A` y coloca el símbolo genérico de opamp:
   - Librería **`Amplifier_Operational`**, símbolo **`Opamp_Single_Generic`** (o el símbolo del encapsulado concreto,
     p. ej. `MCP6002` / `LM358` si prefieres la unidad con sus pines de alimentación explícitos).
   - Referencia **`U2`**, *Value* **`MCP6002`** (alternativa económica: `LM358`).
2. **Filtro RC** (convierte el PWM en DC):
   - **`R3` = 10 kΩ** (resistencia serie del filtro).
   - **`C5` = 10 µF / 16 V** (condensador del filtro a masa).
3. **Red de ganancia no inversora** ($G = 1 + R_f'/R_g = 2.15$):
   - **`R4` = 10 kΩ** ($R_g$, de la entrada inversora a masa).
   - **`R5` = 11.5 kΩ** ($R_f'$, realimentación de la salida a la entrada inversora).
4. **Desacople de alimentación del opamp:**
   - **`C6` = 100 nF** (entre el pin de alimentación positiva del opamp y `GNDREF`).

> 📐 Valores y cálculos completos (filtro $f_c\approx 1.59\,\text{Hz}$, ganancia $G=2.15$):
> ver [`DISENO_V2_CONTROL_DIGITAL.md`](DISENO_V2_CONTROL_DIGITAL.md) §3 y §4.

### A.5 — Reconectar (cablear) la nueva etapa

Dibuja los cables con la herramienta **Wire** (`W`). Recuerda: describimos **nodos**, no coordenadas.
KiCad coloca el cable; tú solo defines qué pin va con qué nodo.

1. **Entrada del filtro RC ← señal del Arduino:**
   - Conecta el **pin 1 de `J5` (`PWM_IN`)** a un extremo de **`R3`**.
2. **Salida del filtro RC → entrada no inversora del opamp:**
   - Conecta el otro extremo de **`R3`** con:
     - el terminal superior de **`C5`** (el otro terminal de `C5` va a **`GNDREF`**), y
     - la **entrada no inversora `+`** del **`U2`**.
   - Ese nodo intermedio es $V_{rc}$ (la DC filtrada de 0–5 V).
3. **Red de ganancia (no inversor):**
   - Conecta la **salida** del `U2` a un extremo de **`R5` (`R_f'`)**.
   - El otro extremo de **`R5`** va al nodo de la **entrada inversora `−`** del `U2`.
   - Desde ese mismo nodo (entrada `−`), conecta **`R4` (`R_g`)** a **`GNDREF`**.
4. **Salida del opamp → nodo ADJ del LM317** (¡aquí estaba el *wiper* de `RV1`!):
   - Conecta la **salida** del `U2` al **pin 1 (`ADJ`) del `U1`**.
   - Este es el cambio central: el opamp **inyecta la tensión** del nodo ADJ en lugar del potenciómetro.
5. **Alimentación del opamp:**
   - Conecta el **pin de alimentación positiva (`V+`)** del `U2` a **`VCC`** (≈ 18 V, el riel ya rectificado),
     o al pin 3 de `J5` (`VCC_OPAMP`) si prefieres un riel auxiliar de 12–15 V.
   - Conecta el **pin de alimentación negativa (`V−`/`GND`)** del `U2` a **`GNDREF`** (alimentación simple).
   - Coloca **`C6` (100 nF)** entre `V+` del `U2` y `GNDREF`, lo más cerca posible del opamp.
6. **Masa del header:**
   - Conecta el **pin 2 de `J5` (`GND`)** a **`GNDREF`**.

> ✅ **Comprobación del nodo ADJ:** tras el cableado, al pin `ADJ` (pin 1 del `U1`) deben llegar:
> la **salida del `U2`**, el `C3` de desacople (33 µF, ya existente en v1.0) y `R2` hacia `VDD`.
> **Ya NO** debe llegar el *wiper* de `RV1` (eliminado).

Guarda el esquemático (`Ctrl+S`).

---

## Parte B — Reglas eléctricas (ERC) y footprints

### B.1 — Ejecutar el ERC

1. En Eeschema: **Inspect → Electrical Rules Checker** (o el icono de "mariquita"/ERC).
2. Pulsa **Run ERC**.
3. Revisa y resuelve las advertencias típicas al añadir componentes:
   - **Pines de alimentación del opamp sin fuente (*power input not driven*):** suele resolverse al conectar `V+` a `VCC` y `V−` a `GNDREF`. Si persiste, añade una etiqueta **PWR_FLAG** sobre el riel correspondiente.
   - **Pines sin conectar:** marca con el símbolo **"No Connect" (`Q`)** cualquier pin que deliberadamente dejes libre (p. ej. la 2ª unidad del MCP6002 si no se usa; sus entradas deben quedar en un estado definido, no al aire).
   - **Entradas de opamp sin uso:** si el encapsulado tiene un segundo amplificador no utilizado, conéctalo como seguidor (salida a `−`, `+` a masa) para que no oscile.
4. Repite hasta que el ERC quede **sin errores** (las advertencias deben estar justificadas).

### B.2 — Asignar footprints a los componentes nuevos

Abre **Tools → Assign Footprints** (o el icono correspondiente) y asigna una huella a cada componente nuevo:

| Componente | Footprint sugerido |
| :--- | :--- |
| `U2` (opamp) | `Package_DIP:DIP-8_W7.62mm` (THT) o `Package_SO:SOIC-8_3.9x4.9mm_P1.27mm` (SMD) |
| `R3`, `R4`, `R5` | `Resistor_THT:R_Axial_DIN0207_...` (THT) o `Resistor_SMD:R_0805_...` |
| `C5` (10 µF) | `Capacitor_THT:CP_Radial_...` (electrolítico radial) |
| `C6` (100 nF) | `Capacitor_THT:C_Disc_...` o `Capacitor_SMD:C_0805_...` |
| `J5` (header) | `Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical` (o `1x02`) |

> Mantén el **mismo criterio THT/SMD** que el resto de la placa para que el montaje sea homogéneo.

Guarda y actualiza los campos de footprint en el esquemático.

---

## Parte C — PCB (Pcbnew)

> 🧱 La **geometría del PCB se gestiona dentro de Pcbnew**: colocación de huellas, rutado de pistas,
> plano de masa y Gerbers. **No se edita el `.kicad_pcb` a mano.**

### C.1 — Traer los cambios del esquemático

1. Abre **Pcbnew**.
2. Ejecuta **Tools → Update PCB from Schematic** (atajo **`F8`**).
3. En el diálogo, revisa la lista de cambios:
   - Se **añadirán** las huellas nuevas: `U2`, `R3`, `R4`, `R5`, `C5`, `C6`, `J5`.
   - Se **eliminará** la huella de `RV1`.
   - `R2` conservará su huella y actualizará su valor a 240 Ω.
4. Marca la opción de **eliminar componentes extra / huellas que ya no están en el esquemático**
   para que la huella de `RV1` desaparezca de la placa.
5. Pulsa **Update PCB**. Las huellas nuevas aparecen agrupadas para que las coloques.

### C.2 — Recolocar y rutar

1. **Coloca** las huellas nuevas en una zona lógica:
   - El **opamp `U2`** y su red (`R3`, `R4`, `R5`, `C5`, `C6`) juntos, cerca del nodo ADJ del `U1`.
   - `C6` lo **más cerca posible** del pin de alimentación del `U2` (desacople efectivo).
   - El **header `J5`** cerca del borde de la placa, accesible para el cable del Arduino.
2. **Ruta** las pistas siguiendo las conexiones (*ratsnest*) que marca Pcbnew:
   - La salida del `U2` hasta el pin `ADJ` del `U1`.
   - La pista de señal del filtro RC, de baja corriente (pista fina es suficiente).
   - La alimentación del opamp desde `VCC`.
3. Conecta las masas al **plano `GNDREF`** (los pines de `GND` se unen al vertido de cobre de masa).
4. Rellena de nuevo las **zonas de cobre** (plano de masa) con **Edit → Fill All Zones** (`B`) para incluir las huellas nuevas.

> 💡 La etapa de control es de **baja corriente** (decenas de µA en ADJ). No requiere pistas anchas;
> prioriza una masa limpia y el desacople del opamp.

### C.3 — DRC y regeneración de Gerbers

1. Ejecuta **Inspect → Design Rules Checker (DRC)** y corrige cualquier error
   (pistas sin conectar, *clearance*, zonas sin rellenar).
2. Cuando el DRC esté limpio, **regenera los archivos de fabricación**:
   - **File → Fabrication Outputs → Gerbers (.gbr)** → genera las capas.
   - **File → Fabrication Outputs → Drill Files (.drl)** → genera los taladros.
   - Reempaqueta el ZIP de producción (sustituye el `Gerbers.zip`/`Produccion.zip` antiguo por los nuevos).
3. Opcional: revisa el resultado en el **visor de Gerbers (GerbView)** antes de enviar a fabricar.

---

## 📊 Tabla resumen — qué se quita / qué se añade / cómo se reconecta

| Acción | Elemento | Detalle / Reconexión |
| :--- | :--- | :--- |
| ❌ **Se quita** | `RV1` (potenciómetro 5 kΩ) | Estaba entre el nodo **ADJ** (pin 1 de `U1`) y `GNDREF`. Se elimina junto con sus cables sueltos. |
| 🔧 **Se modifica** | `R2` | *Value* pasa de `R` genérico a **240 Ω / ¼ W** (fijo). Mantiene conexión ADJ ↔ `VDD`. |
| ➕ **Se añade** | `U2` (opamp MCP6002/LM358) | Amplificador no inversor, $G=2.15$. **Salida → nodo ADJ** (reemplaza al *wiper* de `RV1`). Alimentado desde `VCC`, masa a `GNDREF`. |
| ➕ **Se añade** | `R3` = 10 kΩ + `C5` = 10 µF | **Filtro RC**: entrada desde `J5.PWM_IN`; salida ($V_{rc}$) → entrada **`+`** del `U2`. `C5` a `GNDREF`. |
| ➕ **Se añade** | `R4` = 10 kΩ ($R_g$) | De la entrada **`−`** del `U2` a `GNDREF`. |
| ➕ **Se añade** | `R5` = 11.5 kΩ ($R_f'$) | Realimentación: salida del `U2` → entrada **`−`** del `U2`. |
| ➕ **Se añade** | `C6` = 100 nF | Desacople entre `V+` del `U2` y `GNDREF`, junto al opamp. |
| ➕ **Se añade** | `J5` (header 1x03) | `PWM_IN` (pin 1), `GND` → `GNDREF` (pin 2), `VCC_OPAMP` opcional (pin 3). Masa común con el Arduino. |
| 🔗 **Se reconecta** | Nodo **ADJ** (pin 1 de `U1`) | Antes: *wiper* de `RV1`. Ahora: **salida del `U2`** (+ `C3` 33 µF y `R2`, que ya existían). |

---

## ✅ Checklist final

- [ ] `RV1` eliminado del esquemático (y del PCB tras `F8`).
- [ ] `R2` con *Value* = 240 Ω.
- [ ] `J5`, `U2`, `R3`, `R4`, `R5`, `C5`, `C6` colocados y cableados según la Parte A.
- [ ] Salida del `U2` conectada al pin **ADJ** del `U1`.
- [ ] Opamp alimentado desde `VCC` y masa a `GNDREF`; masa del Arduino unida vía `J5`.
- [ ] **ERC** sin errores en Eeschema.
- [ ] Footprints asignados a los componentes nuevos.
- [ ] **Update PCB from Schematic (F8)** aplicado; huella de `RV1` eliminada.
- [ ] **DRC** sin errores en Pcbnew; zonas de masa rellenadas.
- [ ] **Gerbers y taladros regenerados**.

---

*Para la teoría, cálculos y elección de componentes, consulta
[`DISENO_V2_CONTROL_DIGITAL.md`](DISENO_V2_CONTROL_DIGITAL.md).
Para el código del Arduino, [`../firmware/control_lm317.ino`](../firmware/control_lm317.ino).*
