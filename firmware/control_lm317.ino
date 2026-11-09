/*
 * ============================================================================
 *  control_lm317.ino  -  Control digital del nodo ADJ del LM317 por PWM
 * ============================================================================
 *
 *  Proyecto:  Fuente de Alimentacion Lineal Regulable v2.0
 *  Placa:     Arduino Uno / Nano estandar (microcontrolador ATmega328P, 5 V)
 *  Autor:     Firmware didactico para practica de Electronica de Potencia
 *
 *  ---------------------------------------------------------------------------
 *  QUE HACE ESTE SKETCH
 *  ---------------------------------------------------------------------------
 *  Sustituye al potenciometro manual (RV1) de la v1.0 por un control digital.
 *  El usuario escribe por el Monitor Serie el VOLTAJE DE SALIDA que desea
 *  (en voltios) y el Arduino genera una senal PWM cuyo ciclo de trabajo (duty)
 *  produce, al final de la cadena analogica, ese voltaje en la salida.
 *
 *  ---------------------------------------------------------------------------
 *  LA CADENA DE CONTROL COMPLETA  (duty -> tension RC -> tension ADJ -> Vout)
 *  ---------------------------------------------------------------------------
 *
 *    [Arduino]        [Filtro RC]        [Opamp no inversor]        [LM317]
 *    pin PWM  ----->  R-C pasa-bajos ---> ganancia G = 2.15   ----> nodo ADJ
 *    0..5 V             0..5 V DC           0..10.75 V               (ajuste)
 *    ~490 Hz            (valor medio)       (aisla el pin de 5 V)
 *
 *  1) PWM del Arduino: es una onda cuadrada de 0 a 5 V. Su VALOR MEDIO es
 *     proporcional al ciclo de trabajo (duty):   V_pwm_medio = 5 V * D
 *     donde D = duty / 255  (D va de 0.0 a 1.0).
 *
 *  2) Filtro RC pasa-bajos: "alisa" la onda cuadrada y entrega una tension
 *     DC estable igual a ese valor medio (0..5 V). La frecuencia de corte del
 *     RC se eligio muy por debajo de los ~490 Hz del PWM (ver documento de
 *     diseno), por eso el rizado residual es pequeno.
 *
 *  3) Opamp no inversor: amplifica esos 0..5 V hasta 0..10.75 V con una
 *     ganancia G = 1 + Rf/Rg = 2.15. Ademas AISLA el pin del Arduino (que solo
 *     tolera 5 V) del nodo ADJ, que puede superar los 10 V. El opamp tambien
 *     actua como buffer de baja corriente para el nodo ADJ (I_ADJ ~ 50-100 uA).
 *
 *  4) Nodo ADJ del LM317: el regulador mantiene SIEMPRE 1.25 V entre su pin
 *     de salida (VO) y su pin de ajuste (ADJ). Por tanto, respecto a masa:
 *
 *            Vout = V_ADJ + 1.25 V
 *
 *     Esto es equivalente a la formula clasica del LM317, en la que R_ajuste
 *     era la posicion del potenciometro:
 *
 *            Vout = 1.25 V * (1 + R_ajuste / R2)      con R2 = 240 ohm
 *
 *     En la v2.0 no variamos una resistencia: inyectamos directamente la
 *     tension V_ADJ con el opamp, lo que controla Vout de forma directa.
 *
 *  ---------------------------------------------------------------------------
 *  MAPEO CONSIGNA -> DUTY   (coherente con docs/DISENO_V2_CONTROL_DIGITAL.md)
 *  ---------------------------------------------------------------------------
 *  Juntando los pasos anteriores, con D = duty/255 (0..1):
 *
 *        V_ADJ = G * 5 V * D = 2.15 * 5 * D = 10.75 * D
 *        Vout  = V_ADJ + 1.25 = 10.75 * D + 1.25            (voltios)
 *
 *  Despejando el duty necesario para un Vout objetivo:
 *
 *        D        = (Vout - 1.25) / 10.75
 *        duty(8b) = round(D * 255)
 *
 *  Rango util:  Vout = 1.25 V  (D = 0,   duty = 0)
 *               Vout = 12.0 V  (D ~ 1,   duty = 255)
 *
 *  ---------------------------------------------------------------------------
 *  RESOLUCION: LIMITE DE PRECISION (PWM de 8 bits)
 *  ---------------------------------------------------------------------------
 *  analogWrite() usa PWM de 8 bits: solo 256 niveles de duty (0..255). Como la
 *  excursion de salida es de 10.75 V repartida en 255 pasos, el escalon minimo
 *  (1 LSB) en la salida vale:
 *
 *        dVout = 10.75 V / 255 ~ 42 mV por paso
 *
 *  Es decir, la salida NO es continua: solo puede fijarse en multiplos de
 *  ~42 mV. Frente al potenciometro de la v1.0 (ajuste continuo), este es el
 *  precio de la automatizacion. Por eso dutyAVoltaje() informa del voltaje
 *  REALMENTE aplicado tras la cuantizacion, que puede diferir unos mV del
 *  pedido. Para mayor resolucion: subir el PWM a 10 bits (~10.5 mV/LSB) o usar
 *  un DAC externo (MCP4725, 12 bits ~2.6 mV/LSB). Ver documento de diseno §4.1.
 *
 *  ---------------------------------------------------------------------------
 *  MEJORA OPCIONAL (no implementada aqui a proposito)
 *  ---------------------------------------------------------------------------
 *  El PWM por defecto del Arduino Uno/Nano trabaja a ~490 Hz en la mayoria de
 *  pines. Se podria SUBIR esa frecuencia (p. ej. a ~31 kHz) tocando los
 *  registros del Timer (TCCRnB), lo que permitiria un filtro RC mas pequeno y
 *  mas rapido. No se hace aqui para mantener el sketch portatil y usando solo
 *  las APIs del core de Arduino (analogWrite / Serial).
 * ============================================================================
 */

// ----------------------------------------------------------------------------
//  CONSTANTES DE DISENO
// ----------------------------------------------------------------------------

// Pin de salida PWM. En Arduino Uno/Nano el pin ~9 usa el Timer1 y da PWM a
// ~490 Hz, igual que la mayoria de pines PWM del core.
const int PIN_PWM = 9;

// Frecuencia PWM ASUMIDA (valor por defecto del Timer en Uno/Nano). Solo es
// informativa para el estudiante; no se reconfigura ningun Timer en este sketch.
const float FREC_PWM_HZ = 490.0;

// Resolucion del PWM de analogWrite(): 8 bits -> valores de 0 a 255.
const int PWM_MIN = 0;
const int PWM_MAX = 255;

// Rango de voltaje de salida alcanzable por el diseno analogico.
const float VOUT_MIN = 1.25;   // voltios (duty = 0)
const float VOUT_MAX = 12.0;   // voltios (duty ~ 255)

// Constantes del mapeo lineal Vout = PENDIENTE * D + OFFSET.
//   OFFSET   = 1.25 V  -> tension minima del LM317 (VO - ADJ fijos en 1.25 V).
//   PENDIENTE= 10.75 V -> excursion del nodo ADJ (G = 2.15 sobre 5 V de PWM).
const float VOUT_OFFSET   = 1.25;    // voltios
const float VOUT_PENDIENTE = 10.75;  // voltios por unidad de duty (D de 0 a 1)

// Velocidad del puerto serie (baudios). Valor estandar del Monitor Serie.
const long SERIAL_BAUD = 9600;

// ----------------------------------------------------------------------------
//  FUNCION: voltaje objetivo (V) -> valor de duty (0..255)
// ----------------------------------------------------------------------------
//  Aplica el mapeo lineal documentado y SATURA (clamp) la consigna al rango
//  fisico [VOUT_MIN, VOUT_MAX] antes de calcular el duty, para no pedir duties
//  fuera de 0..255.
//
//      D        = (Vout - 1.25) / 10.75
//      duty(8b) = round(D * 255)
// ----------------------------------------------------------------------------
int voltajeADuty(float voltajeObjetivo) {
  // 1) Clamp del voltaje al rango permitido.
  if (voltajeObjetivo < VOUT_MIN) {
    voltajeObjetivo = VOUT_MIN;
  } else if (voltajeObjetivo > VOUT_MAX) {
    voltajeObjetivo = VOUT_MAX;
  }

  // 2) Ciclo de trabajo normalizado D en [0.0, 1.0].
  float dutyNormalizado = (voltajeObjetivo - VOUT_OFFSET) / VOUT_PENDIENTE;

  // 3) Conversion a 8 bits (0..255) con redondeo.
  int duty = (int)(dutyNormalizado * PWM_MAX + 0.5);

  // 4) Clamp final de seguridad sobre el entero (por errores de redondeo).
  if (duty < PWM_MIN) {
    duty = PWM_MIN;
  } else if (duty > PWM_MAX) {
    duty = PWM_MAX;
  }

  return duty;
}

// ----------------------------------------------------------------------------
//  FUNCION AUXILIAR: duty (0..255) -> voltaje real estimado (V)
// ----------------------------------------------------------------------------
//  Invierte el mapeo para informar al usuario del voltaje REALMENTE fijado
//  (tras la cuantizacion a 8 bits, puede diferir unos mV de lo pedido).
// ----------------------------------------------------------------------------
float dutyAVoltaje(int duty) {
  float dutyNormalizado = (float)duty / PWM_MAX;
  return VOUT_PENDIENTE * dutyNormalizado + VOUT_OFFSET;
}

// ----------------------------------------------------------------------------
//  SETUP
// ----------------------------------------------------------------------------
void setup() {
  // Pin PWM como salida.
  pinMode(PIN_PWM, OUTPUT);

  // Arrancar en el voltaje minimo seguro (duty = 0 -> Vout = 1.25 V).
  analogWrite(PIN_PWM, PWM_MIN);

  // Inicializar el puerto serie.
  Serial.begin(SERIAL_BAUD);

  // Instrucciones de uso para el estudiante.
  Serial.println(F("============================================"));
  Serial.println(F(" Control digital LM317 v2.0 (PWM -> RC -> opamp -> ADJ)"));
  Serial.println(F("============================================"));
  Serial.print(F("Rango de salida: "));
  Serial.print(VOUT_MIN, 2);
  Serial.print(F(" V a "));
  Serial.print(VOUT_MAX, 2);
  Serial.println(F(" V"));
  Serial.print(F("Pin PWM: D"));
  Serial.print(PIN_PWM);
  Serial.print(F("   Frecuencia PWM asumida: ~"));
  Serial.print(FREC_PWM_HZ, 0);
  Serial.println(F(" Hz   Resolucion: 8 bits (0-255)"));
  Serial.println(F("Mapeo: Vout = 10.75 * (duty/255) + 1.25"));
  Serial.println(F("Resolucion: ~42 mV por paso (10.75 V / 255, PWM 8 bits)"));
  Serial.println(F("--------------------------------------------"));
  Serial.println(F("Escribe el voltaje deseado (p. ej. 5.0) y pulsa ENTER."));
  Serial.print(F("Minimo fisico: "));
  Serial.print(VOUT_MIN, 2);
  Serial.println(F(" V. Valores por debajo se saturan a ese minimo."));
  Serial.println(F("Valores por encima del maximo se saturan al maximo."));
  Serial.println(F("Entrada vacia / no numerica / <= 0 se descarta (ver [ERROR])."));
  Serial.println(F("--------------------------------------------"));
}

// ----------------------------------------------------------------------------
//  LOOP
// ----------------------------------------------------------------------------
//  Lee por Serial un numero (voltaje objetivo en V), lo valida, lo convierte a
//  duty con analogWrite() y responde con el voltaje fijado y el duty aplicado.
// ----------------------------------------------------------------------------
void loop() {
  // Solo actuamos cuando hay datos disponibles en el buffer serie.
  if (Serial.available() > 0) {
    // parseFloat() lee el numero (acepta decimales) hasta un separador.
    float voltajePedido = Serial.parseFloat();

    // Descartar el resto de la linea (espacios, fin de linea, etc.).
    while (Serial.available() > 0) {
      Serial.read();
    }

    // Validacion de entrada. Importante sobre parseFloat():
    //   - Devuelve 0.0 si la entrada esta VACIA o NO es un numero (p. ej. letras),
    //     o si realmente se escribio "0". No distingue esos tres casos entre si.
    //   - El minimo FISICO de la fuente es VOUT_MIN = 1.25 V (el LM317 no baja de
    //     ahi: Vout = V_ADJ + 1.25 con V_ADJ >= 0). Por tanto, cualquier valor
    //     <= 0 (incluida la entrada vacia/no numerica y un "0" intencionado) no es
    //     una consigna alcanzable: se trata como entrada no valida y se descarta.
    //   - Si lo que quieres es "lo minimo posible", escribe 1.25 (o menos: se
    //     saturara a 1.25 V por el clamp de voltajeADuty()).
    if (voltajePedido <= 0.0) {
      Serial.print(F("[ERROR] Entrada vacia, no numerica o <= 0. El minimo fisico es "));
      Serial.print(VOUT_MIN, 2);
      Serial.println(F(" V. Escribe un voltaje en ese rango (p. ej. 5.0)."));
      return;
    }

    // Avisar si la consigna estaba fuera del rango fisico (se saturara).
    if (voltajePedido < VOUT_MIN || voltajePedido > VOUT_MAX) {
      Serial.print(F("[AVISO] "));
      Serial.print(voltajePedido, 2);
      Serial.print(F(" V esta fuera de rango ["));
      Serial.print(VOUT_MIN, 2);
      Serial.print(F(", "));
      Serial.print(VOUT_MAX, 2);
      Serial.println(F("] V. Se aplicara el limite mas cercano."));
    }

    // Convertir a duty (con clamp) y aplicarlo al pin PWM.
    int duty = voltajeADuty(voltajePedido);
    analogWrite(PIN_PWM, duty);

    // Voltaje real estimado tras la cuantizacion a 8 bits.
    float voltajeReal = dutyAVoltaje(duty);

    // Responder al usuario.
    Serial.print(F("[OK] Voltaje fijado ~ "));
    Serial.print(voltajeReal, 2);
    Serial.print(F(" V   |   duty = "));
    Serial.print(duty);
    Serial.print(F(" / 255   (D = "));
    Serial.print((float)duty / PWM_MAX, 3);
    Serial.println(F(")"));
  }
}
