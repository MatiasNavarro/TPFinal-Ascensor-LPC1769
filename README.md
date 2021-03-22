# Ascensor con LPC1769 

### Trabajo Final de la materia Electrónica Digital III de la carrera Ingeniería en Computación (UNC)

En el siguiente proyecto, se lleva a cabo el “Modelado de un Ascensor” implementado con la LPC1769 y programado en C utilizando la librería de CMSIS.
El comportamiento del mismo es similar al de un ascensor real que cuenta con 3 pisos (PB (0), 1, 2), con alarma y parada de emergencia. Está implementado con un motor paso a paso (para el funcionamiento del mismo se utilizó el driver A4988 para controlarlo) que es el encargado de posicionar la cabina en el piso adecuado en función de un valor obtenido mediante ADC previamente calibrado. El tablero del ascensor es simulado mediante un teclado matricial 4x4 los cuales se utilizan las teclas numéricas para indicar el piso deseado, y las teclas (*) para el encendido de la alarma y (#) para la parada de emergencia. 
También cuenta con LEDs que indican si el ascensor está subiendo, bajando o si ocurrió una parada de emergencia y un display 7 segmentos para indicar en que piso se encuentra actualmente. Se simula una cabina de control (mediante un PC) donde se transmiten datos por puerto serie (UART) del estado del ascensor constantemente.

