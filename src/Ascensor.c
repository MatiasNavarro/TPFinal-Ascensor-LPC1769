/*
 ===============================================================================
 Name        : Proyecto-Final.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
 ===============================================================================
 */

/*PROYECTO FINAL
 * Implementar un ascensor que cumpla con los siguientes requerimientos:
 * 	-Debe tener 3 pisos
 * 	-Debe tener un tablero de llamada y muestra del piso en el que se encuentra
 * 	 junto con un indicador de direccion del movimiento
 * 	-Debe tener una cabina, en la cual tenga el llamado a cualquiera de los 3 pisos,
 * 	 ademas de una parada de emergencia sonora.
 * 	-Debe enviar informacion a una estacion que se encuentra fuerade la cabina.
 *
 * 	Utilizar un motor con control o motor paso a paso.
 * 	Un potenciometro multivueltas conectado al ADC para dar presicion a la posicion.
 * 	La logica de funcionamiento debe ser acorde a uno real.
 */

/*CONFIGURACION DE PUERTOS
 *	Puerto 0:
 *		P0.0 Tx
 *		P0.1 Rx
 *		
 *		P0.4 - Dirección motor
 *		P0.6 - P0.9 Entrada del teclado
 *		P0.23 - P0.25 Salida del teclado
 *		-------------------------------------------
 *		Teclado 0 - 1 - 2 Pisos
 *		Teclado *(Alarma) #(Parada de emergencia)
 *		-------------------------------------------
 *		P2.0 PWM1
 *		P0.23 ADC0.0
 *
 *	Puerto 2:
 *		P2.1 - P2.7 Salida de display 7 segmentos (sin punto)
 *		P2.8 Salida (Display encendido o apagado)
 *		P2.10 - P0.12 Salidas led's
 *			- ROJO: Parada de Emergencia
 *			- AZUL: Ascensor subiendo
 *			- VERDE: Ascensor bajando
 *		P2.13 Salida ALARMA
 */

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>
#define TOLERANCIA 10

//Funciones de configuracion
void configPuertos(void);
void configUART(void);
void configTimer(void);
void configADC(void);
void configPWM(void);
void configNVIC(void);

//Funciones especificas
void encenderMotor(int, int);
void mostrarPiso(int piso);
void cambioPiso(int resultado);
void pisoUART(int piso);
void enviar(char);
void delay(int d);
int teclado(void);
void calibracion(void);			//Utilizada para calibrar los valores de ADC correspondientes a los pisos
void paradaEmergencia(void);
void alarma(void);
void apagarMotor(void);

//Variables
uint8_t pisoActual = 0;
//uint16_t ADC_Values = [Valor0,Valor1,Valor2]; 	//Contiene los valores del ADC, con referencia a cada piso
int stop = 0;
int dir = 0;
int moviendo=0;
int destino=0;
int pisoProx=-1;
int pisoDestino=-1;

int PISO0 = 0;
int PISO1 = 0;
int PISO2 = 0;


int main(void) {

	configPuertos();
	configUART();
	configTimer();
	configPWM();
	configADC();
	calibracion();
	configNVIC();

	while (1) {
	}

	return 0;
}

//##################################################--CONFIGURACIONES--###############################################
//Funcion que configura los puertos en general (para uso de display, motor, leds y buzzer)
void configPuertos(void) {
	//Puerto 0
	//TECLADO

	LPC_GPIO0->FIODIR &= ~(0xF << 6); 		//P0.6 a P0.9 como entradas (Teclado)
	LPC_GPIO0->FIODIR |= (0x7 << 23); 		//P0.23 a P0.25 como salidas (Teclado)
	LPC_GPIO0->FIOSET |= (0x7 << 23);		//Ponemos las salidas en 1
	LPC_PINCON->PINMODE0 |= (0xFF << 12);  	//Pull-down enable P0.6 - P0.9
	LPC_GPIOINT->IO0IntEnR |= (0x9 << 6); 	//Activar interrupciones del P0.6 a P0.9 por flanco ascendente
	LPC_GPIOINT->IO0IntClr = 0xFFFF;	 	//Limpiar interrupciones del puerto 0

	//MOTOR
	LPC_GPIO0->FIODIR|=(1<<4);				//P0.4 maneja la dirección del motor 

	//Puerto2
	//DISPLAY 7 SEGMENTOS
	LPC_GPIO2->FIODIR |= (0x3F << 1); 		//P2.1-7 como salidas (Segmentos de los display)
	LPC_GPIO2->FIODIR |= (1 << 8); 			//P2.8 como salida (alimentacion del Display)
	//LEDs INDICADORES
	LPC_GPIO2->FIODIR |= (0x7 << 10); 		//P2.10 y P2.12 como salidas (Leds de indicacion)
	//BUZZER (Alarma)
	LPC_GPIO2->FIODIR |= (1 << 13);			//P2.13 como salida (Alarma)

	LPC_GPIO2->FIOSET |= (0x7F << 1);		//Leds de display apagado
	LPC_GPIO2->FIOCLR |= (0x3F << 1); 		//Display empieza en 0
	LPC_GPIO2->FIOCLR |= (1 << 8);			//Display encendido
	LPC_GPIO2->FIOCLR |= (0xF << 10);		//Leds apagados, alarma apagada
}

//Funcion que configura los timers (en este caso se utilizaron 2, TMR0 y TMR1)
void configTimer(void) {
	//Timer 0 -> Utilizado para la alarma
	LPC_SC->PCONP |= (1 << 1);		//Enciendo el timer 0
	LPC_SC->PCLKSEL0 &= ~(3 << 2);	//Pclk_TIM0 = 25 Mhz
	LPC_TIM0->TCR = 0x02;			//Apago y reseteo el timer
	LPC_TIM0->MR0 = 75000000;		//Cada 3 seg
	LPC_TIM0->MCR |= (3 << 0);		//Interrumpe y reseteo timer0
}

//Funcion que configura el UART para la transmision de datos por puerto serie
void configUART(void) {
	LPC_SC->PCONP |= (1 << 25);          //Energizo Uart3
	LPC_SC->PCLKSEL1 &= ~(3 << 18);      //CCLK_UART2=CCLK/4
	LPC_UART3->LCR |= 3;              	//Palabra 8 bit
	LPC_UART3->LCR |= (1 << 7);	      	//DLAB=1
	LPC_UART3->DLL = 163;			    //Baudrate 9600
	LPC_UART3->DLM = 0;
	LPC_UART3->FCR |= (1 << 0);		    //Fifo Activada
	LPC_PINCON->PINSEL0 |= (2 << 0); 	//TXD3 P0.0
	LPC_PINCON->PINSEL0 |= (2 << 2); 	//RXD3 P0.1
	LPC_UART3->LCR &= ~(1 << 7);		//DLAB=0
}

//Funcion que configura el PWM, para hacer funcionar el motor
void configPWM(void) {
	LPC_SC->PCONP |= (1 << 6); 			//Energizo PWM
	LPC_SC->PCLKSEL0 &= ~(3 << 12); 	//Pclk_PWM=25MHz
	LPC_PINCON->PINSEL4 |= (1 << 0); 	// P2.0 funcion PWM1.1
	LPC_PWM1->PR = 0x00; 				//Prescaler = 0
	LPC_PWM1->MCR |= (1 << 1);			//Match0 Resetea
	LPC_PWM1->MR0 = 500000; 			//10ms
	LPC_PWM1->MR1 = 250000;				//Apetura del pulso
	LPC_PWM1->LER |= (1 << 0);
	LPC_PWM1->LER |= (1 << 1);
	LPC_PWM1->PCR |= (1 << 9); 			//Habilito salida PWM P2.0
}

//Configuracion del ADC

void configADC() {
	LPC_SC->PCONP |= (1 << 12);			//Energizo ADC
	LPC_SC->PCLKSEL0 |= (3 << 0);		//Pclk_ADC = Cclk/8 = 12MHz
	LPC_PINCON->PINSEL0 |= (2 << 4);	//P0.23 -> AD0.0
	LPC_PINCON->PINMODE0 |= (2 <<4);	//P0.23 Pull-up and Pull-down desactivados
	LPC_ADC->ADCR |= (1 << 21);			//ADC activado
	LPC_ADC->ADCR |= (1 << 7);			//Canal 0
	LPC_ADC->ADCR |= (1 << 16);			//ADC modo burst
	LPC_ADC->ADINTEN |= (1 << 7);		//Interrupciones de canal 0
}


//Funcion que configura cuando encender el motor y hacia donde es el giro del mismo
void encenderMotor(int piso, int dir) {

	if(!(piso==-1 || dir==0)){					//Si no tiene un piso ni dirección
		moviendo=1;								//Flag de que el ascensor se está moviendo
		if (dir == 1) {							//Sube 1 piso
			LPC_GPIO0->FIOSET |= (1 << 4);
			LPC_GPIO2->FIOSET |= (1 << 11);
		}
		if (dir == 2) {							//Sube 2 pisos
			LPC_GPIO0->FIOSET |= (1 << 4);
			LPC_GPIO2->FIOSET |= (1 << 11);
		}
		if (dir == -1) {						//Baja 1 piso
			LPC_GPIO0->FIOCLR |= (1 << 4);
			LPC_GPIO2->FIOSET |= (1 << 12);
		}
		if (dir == -2) {						//Baja 2 pisos
			LPC_GPIO0->FIOCLR |= (1 << 4);
			LPC_GPIO2->FIOSET |= (1 << 12);
		}

		LPC_PWM1->TCR |= (1 << 0); 				//Activa timer counter del PWM
		LPC_PWM1->TCR |= (1 << 3); 				//PWM Activo
		return;
	}
}

void apagarMotor(){
	LPC_GPIO2->FIOCLR |= (3 << 11);	//Apaga los 2 leds que indican si sube o baja
	LPC_PWM1->TCR &= ~(3 << 0); 	//Desabilito y reseteo PWM
	LPC_PWM1->TCR &= ~(1 << 3); 	//PWM Desactivado
	moviendo=0;

}

//Funcion que muestra en el display en que piso se encuentra
void mostrarPiso(int piso) {
	LPC_GPIO2->FIOSET |= (0x7F << 1);

	switch (piso) {
	case 0:									//Muestra el 0
		LPC_GPIO2->FIOCLR |= (0x3F << 1);
		break;

	case 1:									//Muestra el 1
		LPC_GPIO2->FIOCLR |= (0X06 << 1);
		break;

	case 2:									//Muestra el 2
		LPC_GPIO2->FIOCLR |= (0X5B << 1);
		break;
	}
}

//Funcion que envia los datos por puerto UART
void enviar(char c) {
	while ((LPC_UART3->LSR & (1 << 5)) == 0)	
		;
	LPC_UART3->THR = c;
}

//Funcion que desarma el arreglo y lo pasa a la funcion enviar
void mensaje(char msj[]) {
	for (int i = 0; msj[i]; i++)
		enviar(msj[i]);
}

//Enviar mensaje por UART, indicando el piso donde se encuentra
void pisoUART (int piso){
	char p[] = "Piso ";
	char n[20];
	char ent[] = "\n \r";
	itoa(piso, n, 10);

	mensaje(p);
	mensaje(n);
	mensaje(ent);
}

//Funcion que apaga el motor y enciende el led de emergencia
void paradaEmergencia(){
	LPC_GPIO2->FIOSET |= (1 << 10);//prende luz parada de emergencia
	apagarMotor();
	char s[] = "Parada de Emergencia  \n\r";
	mensaje(s);
	pisoProx=-1;

}

//Funcion que activa la alarma (buzzer) y enciende el timer para que apague la misma
void alarma(){
	LPC_GPIO2->FIOSET |= (1 << 13); 	//Enciendo la alarma
	LPC_TIM0->TCR = 0x01; 				//Enciendo el timer de 3 seg que desactiva la alarma
	//Envia mensaje de Alarma
	char a[] = "Alarma Activada \n\r ";
	mensaje(a);

}

//Funcion que cambia el piso actual dependiendo el valor obtenido por ADC
void cambioPiso( int resultado){
	if (resultado<PISO1+TOLERANCIA && resultado> PISO1-TOLERANCIA )
		pisoActual=1;
	if (resultado<PISO2+TOLERANCIA && resultado> PISO2-TOLERANCIA )
		pisoActual=2;
	if (resultado<PISO0+TOLERANCIA && resultado> PISO0-TOLERANCIA )
		pisoActual=0;
}

//Funcion que devuelve que tecla ha sido presionada
int teclado (void){
	int aux=-1;
	//FILA 0
	if (((LPC_GPIOINT->IO0IntStatR) >> 9) & 1) { //¿Se pulso la fila 0? (Pisos 1, 2, 3)

		LPC_GPIO0->FIOCLR |= (0x7 << 23); 		//Limpio todas las salidas (i.e = 0)
		LPC_GPIO0->FIOSET |= (1 << 23); 		//Ponemos un 1 en la columna 1
		if ((LPC_GPIO0->FIOPIN >> 9) & 1) {		//¿Se pulso el 1?

			LPC_GPIO2->FIOCLR |= (1 << 10); //Limpio el LED de STOP
			aux= 1;
		}

		LPC_GPIO0->FIOCLR |= (0x7 << 23);	//Limpio las salidas
		LPC_GPIO0->FIOSET |= (1 << 24);		//Ponemos un 1 en la columna 2
		if ((LPC_GPIO0->FIOPIN >> 9) & 1) {	//¿Se pulso el 2?
			//se que se apreto la tecla 2
			LPC_GPIO2->FIOCLR |= (1 << 10); //Limpio el LED de STOP
			aux= 2;
		}


		LPC_GPIO0->FIOCLR |= (0x7 << 23);	//Limpio las salidas
		LPC_GPIO0->FIOSET |= (1 << 25);		//Ponemos un 1 en la columna 3


		LPC_GPIOINT->IO0IntClr |= (1 << 9);	//Limpio el flag de la interrupcion
	}

	//FILA 1
	else if (((LPC_GPIOINT->IO0IntStatR) >> 8) & 1) { //Si se pulso la fila 1
		LPC_GPIOINT->IO0IntClr |= (1 << 8);
	}
	//FILA 2
	else if (((LPC_GPIOINT->IO0IntStatR) >> 7) & 1) { //Si se pulso la fila 2
		LPC_GPIOINT->IO0IntClr |= (1 << 7);
	}

	//FILA 3
	else if (((LPC_GPIOINT->IO0IntStatR) >> 6) & 1)	//Si se pulso la fila 3 (* Alarma - # Parada de Emergencia)
	{
		LPC_GPIO0->FIOCLR |= (0x7 << 23);	//Limpio todas las salidas (i.e = 0)
		LPC_GPIO0->FIOSET |= (1 << 23);		//Ponemos un 1 en la columna 1
		if ((LPC_GPIO0->FIOPIN >> 6) & 1) {
			alarma();
		}

		LPC_GPIO0->FIOCLR |= (0x7 << 23);	//Limpio las salidas
		LPC_GPIO0->FIOSET |= (1 << 24);		//Ponemos un 1 en la columna 2
		if (((LPC_GPIO0->FIOPIN >> 6) & 1)) {
			//se que se apreto la tecla 0
			LPC_GPIO2->FIOCLR |= (1 << 10); //Limpio el LED de STOP
			aux= 0;
		}

		LPC_GPIO0->FIOCLR |= (0x7 << 23);	//Limpio todas las salidas (i.e = 0)
		LPC_GPIO0->FIOSET |= (1 << 25);		//Ponemos un 1 en la columna 3
		if ((LPC_GPIO0->FIOPIN >> 6) & 1) {
			//se que se apreto la tecla # (Parada de emergencia)
			//Enviar mensaje de STOP por UART
			paradaEmergencia();
		}

		LPC_GPIOINT->IO0IntClr |= (1 << 6);
	}

	delay(7000000);
	LPC_GPIO0->FIOSET |= (0x7 << 23);
	return aux;
	LPC_GPIOINT->IO0IntClr = 0xFFFF;
}

//Funcion que establece si el motor sube, baja o está detenido
void setDir(int orig, int dest){
	if (dest>orig)
		dir=1;
	else if(dest < orig)
		dir=-1;
	else
		dir=0;
}

//Funcion que establece el piso destino
void setDestino(int piso){
	switch (piso){
		case (0):
				destino=PISO0;
				break;
		case(1):
				destino= PISO1;
				break;
		case(2):
				destino=PISO2;
				break;
		default:
			;
	}
}

//Calibracion de los pisos apenas enciende el programa
void calibracion(void){
	int val[6];
	for (int i=0; i<5;i=i+1){
		val[i]= (LPC_ADC->ADDR7 >> 4) & 0xFFF;
		delay(2000);
	}
	//Se realiza un promedio de 5 valores para obtener un valor inicial del piso 0 y así calibrar el mismo
	val[5]=(val[0]+val[1]+val[2]+val[3]+val[4])/5;

	PISO0 = val[5];
	PISO1 = PISO0 + 288;
	PISO2 = PISO1 + 288;
}


//Retardo usado para algunas funciones
void delay(int d) {
	int i;
	for (i = 0; i < d; i++)
		;
}

void configNVIC(void) {
	NVIC_EnableIRQ(EINT3_IRQn);			//Interrupciones puertos 0 y 2
	NVIC_EnableIRQ(TIMER0_IRQn);		//Interrupciones del Timer0		
	NVIC_EnableIRQ(ADC_IRQn);			//Interrupciones del ADC
}



//######################################--INTERRUPCIONES--################################################
//Funcion encargada de hacer el barrido del teclado, para saber que tecla se pulso
void EINT3_IRQHandler(void) {

	NVIC_DisableIRQ(EINT3_IRQn);		//Deshabilito interrupciones de EINT0
	if (!moviendo){						//Si no se está moviendo, establece a que piso ir
		pisoDestino=teclado();
		setDir(pisoActual,pisoDestino);
		setDestino(pisoDestino);
		encenderMotor(pisoDestino, dir);
			}
	else{								//Si se está moviendo, guarda el piso proximo
		if (pisoProx==-1){
			pisoProx=teclado();
			if (pisoProx==pisoDestino)
				pisoProx=-1;
		}
	}

	//NVIC_ClearPendingIRQ(EINT3_IRQn);
	LPC_GPIOINT->IO0IntClr = 0xFFFF;
	NVIC_EnableIRQ(EINT3_IRQn);
}

//Interrupcion de Timer 0, el mismo apaga la alarma 
void TIMER0_IRQHandler(void) {
	LPC_GPIO2->FIOCLR |= (1 << 13);		//Apago la alarma
	LPC_TIM0->TCR = 0x02;				//Apago el timer
	LPC_TIM0->IR |= (1 << 0);			//Bajo el flag de la interrupcion
}

//Interrupciones del ADC
void ADC_IRQHandler() {
	int resultado = (LPC_ADC->ADDR7 >> 4) & 0xFFF; //Obtiene el resultado de la conversion
	if (moviendo) {	
		if (resultado<(destino+TOLERANCIA) && resultado>(destino-TOLERANCIA)) { //Si llego al piso destino
			cambioPiso(resultado);
			apagarMotor();
			mostrarPiso(pisoActual);
			delay(10000000);

			if(pisoProx!=-1){	//Si tiene en memoria otro piso
				setDir(pisoActual, pisoProx);
				setDestino(pisoProx);
				encenderMotor(pisoProx,dir);
				pisoProx=-1;
			}
		}

		if(resultado!=destino){
			cambioPiso(resultado);
			mostrarPiso(pisoActual);
			pisoUART(pisoActual);
		}

	}
	else {}
}
