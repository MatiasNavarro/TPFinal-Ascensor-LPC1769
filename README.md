# Ascensor con LPC1769 

### Trabajo Final de la materia Electrónica Digital III de la carrera Ingeniería en Computación (UNC)

En el siguiente proyecto, se lleva a cabo el “Modelado de un Ascensor” implementado con la LPC1769 y programado en C utilizando la librería de CMSIS.

El comportamiento del mismo es similar al de un ascensor real que cuenta con 3 pisos (PB (0), 1, 2), con alarma y parada de emergencia. Está implementado con un motor paso a paso (para el funcionamiento del mismo se utilizó el driver A4988 para controlarlo) que es el encargado de posicionar la cabina en el piso adecuado en función de un valor obtenido mediante ADC previamente calibrado. El tablero del ascensor es simulado mediante un teclado matricial 4x4 los cuales se utilizan las teclas numéricas para indicar el piso deseado, y las teclas (*) para el encendido de la alarma y (#) para la parada de emergencia. 

También cuenta con LEDs que indican si el ascensor está subiendo, bajando o si ocurrió una parada de emergencia y un display 7 segmentos para indicar en que piso se encuentra actualmente. Se simula una cabina de control (mediante un PC) donde se transmiten datos por puerto serie (UART) del estado del ascensor constantemente.

### Configuración de puertos
#### Puerto 0:
 * P0.0 Tx
 * P0.1 Rx	
 * P0.4 - Dirección motor
 * P0.6 - P0.9 Entrada del teclado
 * P0.23 - P0.25 Salida del teclado
 -------------------------------------------
 * Teclado 0 - 1 - 2 Pisos
 * Teclado *(Alarma) #(Parada de emergencia)
 -------------------------------------------
 * P2.0 PWM1
 * P0.23 ADC0.0
 
#### Puerto 2:
 * P2.1 - P2.7 Salida de display 7 segmentos (sin punto)
 * P2.8 Salida (Display encendido o apagado)
 * P2.10 - P0.12 Salidas led's
  	- ROJO: Parada de Emergencia
  	- AZUL: Ascensor subiendo
  	- VERDE: Ascensor bajando
 * P2.13 Salida ALARMA
 

