/**
 * @file receptor.c
 * @brief Código do receptor NRF24L01 para controle de robô com sistema de penalidade e vidas.
 *
 * Este programa recebe comandos via rádio do transmissor, controla os motores,
 * o laser e o sistema de vidas do robô. Quando o sensor LDR detecta um acerto,
 * o robô entra em modo de penalidade, desativando temporariamente o laser e os motores.
 *
 * - MCU: ATmega328P @ 16 MHz  
 * - Comunicação: SPI via módulo NRF24L01  
 * - Periféricos: Laser (PC1), motores (PWM em PD3 e PD6), LDR (ADC0)
 * - Linguagem: C (AVR-GCC)
 */

/**
 * @mainpage Projeto Receptor NRF24L01
 * 
 * @section descricao Descrição Geral
 * Este projeto implementa o receptor de um sistema de combate entre robôs,
 * utilizando comunicação sem fio via módulo NRF24L01. O receptor interpreta
 * comandos enviados pelo transmissor e executa movimentos, controle de laser
 * e gestão de penalidades com base em leituras de um sensor LDR.
 * 
 * @section autor Autor
 * Desenvolvido por Juana Isabella Lopez Portillo, Naomi Fernandes, Vinícius Scarpitti de Oliveira, 2025.
 */

#define F_CPU 16000000UL /**< Frequência do clock do microcontrolador */
#include <avr/io.h>
#include <avr/interrupt.h>
#include "nrf24_avr.h" /**< Biblioteca do módulo NRF24L01 */

/** Pino CE do módulo NRF24L01 */
#define CE_PIN 8
/** Pino CSN do módulo NRF24L01 */
#define CSN_PIN 10

// === Variáveis globais e de controle ===
volatile uint8_t tempo_rotacao = 0;      /**< Flag usada para temporização do laser */
volatile uint8_t modo_penalidade = 0;    /**< Indica se o modo penalidade está ativo */
volatile uint8_t contador_penalidade = 0;/**< Conta os segundos durante a penalidade */
volatile uint8_t ldr_estado = 0;         /**< Flag para controle da leitura do LDR */

/** Endereço de recepção do módulo NRF24L01 */
uint8_t rxaddr[5] = {'N','O','D','E','1'};

/**
 * @brief Rotina de interrupção do Timer1.
 *
 * Controla o piscar do laser e o tempo de penalidade. A cada 1 segundo,
 * verifica se o modo penalidade está ativo e, caso não esteja, alterna o estado do laser.
 */
ISR(TIMER1_COMPA_vect) {
	if (modo_penalidade == 1) {
		contador_penalidade++;
		if (contador_penalidade >= 5) { // Penalidade de 5 segundos
			modo_penalidade = 0;
			contador_penalidade = 0;
			PORTC &= ~(1 << PC1); // Desliga laser
		}
	}

	if (modo_penalidade == 0) {
		PORTC ^= (1 << PC1); // Pisca laser
		tempo_rotacao = 1;
	} else {
		PORTC &= ~(1 << PC1); // Laser desligado
	}
}

/**
 * @brief Configura o Timer1 para controle do laser.
 */
void timer1_laser() {
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1 = 0;
	OCR1A = 62499; // 1s com prescaler 256
	TCCR1B |= (1 << WGM12) | (1 << CS12);
	TIMSK1 |= (1 << OCIE1A);
}

/**
 * @brief Inicializa o laser e o Timer1.
 */
void liga_laser() {
	DDRC |= (1 << PC1);
	PORTC &= ~(1 << PC1);
	timer1_laser();
	sei();
}

/**
 * @brief Desliga os motores.
 */
void motor_desligado() {
	DDRD |= (1<<PD3) | (1<<PD6);
	OCR0A = 0;
	OCR2B = 0;
	TCCR0A = (1<<COM0A1)|(1<<COM0A0)|(1<<WGM01)|(1<<WGM00);
	TCCR2A = (1<<COM2B1)|(1<<COM2B0)|(1<<WGM21)|(1<<WGM20);
	TCCR0B = (1<<CS01)|(1<<CS00);
	TCCR2B = (1<<CS22);
}

/**
 * @brief Liga os motores com PWM máximo.
 */
void motor_ligado() {
	DDRD |= (1<<PD3) | (1<<PD6);
	OCR0A = 255;
	OCR2B = 255;
	TCCR0A = (1<<COM0A1)|(1<<COM0A0)|(1<<WGM01)|(1<<WGM00);
	TCCR2A = (1<<COM2B1)|(1<<COM2B0)|(1<<WGM21)|(1<<WGM20);
	TCCR0B = (1<<CS01)|(1<<CS00);
	TCCR2B = (1<<CS22);
}

/**
 * @brief Configura o ADC para leitura do sensor LDR.
 */
void ldr() {
	ADMUX = (1 << REFS0);
	ADCSRA = (1 << ADEN)|(1 << ADPS2)|(1 << ADPS1)|(1 << ADPS0);
}

/**
 * @brief Lê o valor analógico do LDR.
 *
 * @param canal Canal ADC a ser lido (0–7)
 * @return Valor de 0–1023 representando a intensidade luminosa.
 */
uint16_t valor_ldr(uint8_t canal) {
	ADMUX = (ADMUX & 0xF0) | (canal & 0x0F);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADC;
}

/**
 * @brief Executa as ações quando o LDR detecta um acerto.
 *
 * Liga motores brevemente e ativa o modo penalidade por 5 segundos.
 */
void vida() {
	PORTD |= (1 << PD0);
	motor_ligado();
	cli();
	tempo_rotacao = 0;
	sei();
	while (!tempo_rotacao);
	tempo_rotacao = 0;
	modo_penalidade = 1;
	contador_penalidade = 0;
	PORTC &= ~(1<< PC1);
}

/**
 * @brief Função principal do receptor NRF24L01.
 *
 * Configura o laser, ADC, motores e o módulo NRF24L01.
 * Gerencia o sistema de vidas e penalidades com base nas leituras do LDR
 * e nos comandos recebidos via rádio.
 */
int main(void) {
	liga_laser();
	motor_desligado();
	ldr();
	sei();

	// LEDs de status
	DDRC |= (1 << PC4) | (1 << PC3);
	DDRD |= (1 << PD0) | (1 << PD1) | (1 << PD2);
	PORTC ^= (1 << PC4) | (1 << PC3);
	PORTD ^= (1 << PD2);

	// Configuração do NRF24L01
	nrf24_begin(CE_PIN, CSN_PIN, RF24_SPI_SPEED);
	nrf24_setChannel(76);
	nrf24_setPayloadSize(2);
	nrf24_openReadingPipe(0, rxaddr);
	nrf24_startListening();

	uint8_t estado_rele = 0;
	uint8_t vidas  = 3;
	uint8_t velocidade_F_R;

	while(1) {
		if(modo_penalidade == 1){
			motor_desligado();
			continue;
		}

		uint16_t ldr_val = valor_ldr(0);

		if (nrf24_available()) {
			uint8_t dados_recebidos[2];
			nrf24_read(&dados_recebidos, 2);
			uint8_t cmd = dados_recebidos[0];
			uint8_t velocidade = dados_recebidos[1];
			velocidade_F_R = 255 - velocidade;

			switch(cmd) {
				case 0xA1: // Reset vidas
					if(vidas == 0){
						vidas = 3;
						PORTC ^= (1 << PC4) | (1 << PC3);
						PORTD ^= (1 << PD2);
					}
					break;

				case 0xA2: PORTC ^= (1 << PC4); break;
				case 0xA3: PORTD ^= (1 << PD0); break;
				case 0xA4: PORTD ^= (1 << PD1); break;

				case 0xA5: // Ré
					if(estado_rele == 0){
						motor_ligado();
						PORTD &= ~((1 << PD0) | (1 << PD1));
						OCR0A = velocidade_F_R;
						OCR2B = velocidade_F_R;
					}
					break;

				case 0xA6: // Parar
					motor_desligado();
					PORTD &= ~((1 << PD0) | (1 << PD1));
					estado_rele = 0;
					break;

				case 0xA7: // Frente
					PORTD |= (1 << PD0) | (1 << PD1);
					motor_ligado();
					estado_rele = 1;
					OCR0A = velocidade_F_R;
					OCR2B = velocidade_F_R;
					break;

				case 0xA8: // Curva esquerda
					motor_ligado();
					OCR0A = 0;
					OCR2B = velocidade;
					break;

				case 0xAA: // Curva direita
					motor_ligado();
					OCR0A = velocidade;
					OCR2B = 0;
					break;

				case 0xAB: // Frente + Esquerda
					motor_ligado();
					OCR0A = velocidade_F_R;
					OCR2B = velocidade >> 1;
					break;

				case 0xAC: // Frente + Direita
					motor_ligado();
					OCR0A = velocidade >> 1;
					OCR2B = velocidade_F_R;
					break;

				case 0xAD: // Ré + Esquerda
					motor_ligado();
					OCR0A = velocidade_F_R;
					OCR2B = velocidade >> 1;
					break;

				case 0xAE: // Ré + Direita
					motor_ligado();
					OCR0A = velocidade >> 1;
					OCR2B = velocidade_F_R;
					break;
			}
		}

		// Controle de vidas pelo LDR
		if(ldr_val < 30 && vidas == 3 && ldr_estado == 0){
			vidas--;
			PORTC &= ~(1 << PC3);
			ldr_estado = 1;
			vida();
		}
		else if (ldr_val < 30 && vidas == 2 && ldr_estado == 0){
			vidas--;
			PORTC &= ~(1 << PC4);
			ldr_estado = 1;
			vida();
		}
		else if(ldr_val < 30 && vidas == 1 && ldr_estado == 0){
			vidas--;
			PORTD &= ~(1 << PD2);
			ldr_estado = 1;
			vida();
		}
		else if(ldr_val > 30){
			ldr_estado = 0;
		}
	}
}
