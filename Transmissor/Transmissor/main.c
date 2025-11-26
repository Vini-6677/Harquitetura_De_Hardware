/**
 * @file main.c
 * @brief Código do transmissor NRF24L01 para joystick no ATmega328P.
 * 
 * Este código lê o estado de um joystick e botões, converte as leituras
 * em comandos de movimento e envia via rádio para o receptor utilizando
 * o módulo NRF24L01.
 */
/**
 * @mainpage Projeto Transmissor NRF24L01
 * 
 * @section descricao Descrição Geral
 * Este projeto implementa um sistema de transmissão de dados via rádio
 * utilizando o módulo NRF24L01, controlado por um joystick analógico e
 * botões físicos conectados ao ATmega328P.
 * 
 * - MCU: ATmega328P @ 16 MHz  
 * - Comunicação: SPI  
 * - Módulo RF: NRF24L01  
 * - Linguagem: C (AVR-GCC)
 * 
 * @section autor Autor
 * Desenvolvido por Juana Isabella Lopez Portillo, Naomi Fernandes, Vinícius Scarpitti de Oliveira, 2025.
 */

#define F_CPU 16000000UL /**< Frequência do clock do ATmega328P */
#include <avr/io.h>
#include <util/delay.h>
#include "nrf24_avr.h" /**< Biblioteca para comunicação com o módulo NRF24L01 */

#define CE_PIN 9   /**< Pino CE do módulo NRF24L01 */
#define CSN_PIN 10 /**< Pino CSN do módulo NRF24L01 */

/** @brief Endereço do transmissor (5 bytes) */
uint8_t txaddr[5] = {'N','O','D','E','1'};

/**
 * @brief Inicializa o ADC (Conversor Analógico-Digital).
 * 
 * Configura o ADC para utilizar AVCC (5V) como referência e prescaler de 128.
 */
void adc_frente_tras_y(void) {
    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

/**
 * @brief Lê o valor analógico de um canal específico.
 * 
 * @param canal Canal do ADC a ser lido (0–7).
 * @return Valor convertido (0–1023).
 */
uint16_t adc_valor(uint8_t canal) {
    ADMUX = (ADMUX & 0xF0) | (canal & 0x0F);
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

/**
 * @brief Retorna o comando correspondente ao movimento do joystick.
 * 
 * A função converte as posições X e Y do joystick em um código de comando único.
 * 
 * @param x Valor do eixo X (0–1023).
 * @param y Valor do eixo Y (0–1023).
 * @return Código do comando (0xA5–0xAE).
 */
uint8_t get_cmd(uint16_t x, uint16_t y) {
    int8_t x_dir = 0, y_dir = 0;

    if (x < 200) x_dir = -1;
    else if (x > 800) x_dir = 1;

    if (y < 200) y_dir = -1;
    else if (y > 800) y_dir = 1;

    uint8_t estado = (x_dir + 1) * 3 + (y_dir + 1);

    switch (estado) {
        case 4: return 0xA6; // Parado
        case 5: return 0xA7; // Frente
        case 3: return 0xA5; // Trás
        case 1: return 0xA8; // Esquerda
        case 7: return 0xAA; // Direita
        case 2: return 0xAB; // Frente + Esquerda
        case 8: return 0xAC; // Frente + Direita
        case 0: return 0xAD; // Trás + Esquerda
        case 6: return 0xAE; // Trás + Direita
        default: return 0xA6;
    }
}

/**
 * @brief Função principal.
 * 
 * Inicializa periféricos, lê o joystick e os botões, calcula velocidade e
 * envia pacotes via NRF24L01 a cada 50 ms.
 */
int main(void) {
    DDRB |= (1 << PB5); // LED onboard

    adc_frente_tras_y();

    DDRD &= ~((1 << PD0) | (1 << PD1) | (1 << PD2) | (1 << PD3));
    PORTD |= (1 << PD0) | (1 << PD1) | (1 << PD2) | (1 << PD3);

    nrf24_begin(CE_PIN, CSN_PIN, RF24_SPI_SPEED);
    nrf24_setChannel(76);
    nrf24_setPayloadSize(2);
    nrf24_openWritingPipe(txaddr);

    uint8_t data_packet[2];

    while (1) {
        uint16_t y = adc_valor(2);
        uint16_t x = adc_valor(3);

        uint8_t velocidade_final;

        if (y > 800) {
            long temp_vel = (y - 801) * 255L;
            temp_vel = 255 - (temp_vel / 222);
            if (temp_vel < 0) temp_vel = 0;
            velocidade_final = (uint8_t)temp_vel;
        } else if (y < 200) {
            long temp_vel = (199 - y) * 255L;
            temp_vel = 255 - (temp_vel / 199);
            if (temp_vel < 0) temp_vel = 0;
            velocidade_final = (uint8_t)temp_vel;
        } else {
            velocidade_final = 255;
        }

        data_packet[1] = velocidade_final;

        switch (~PIND & 0x0F) {
            case (1 << PD0): data_packet[0] = 0xA1; break;
            case (1 << PD1): data_packet[0] = 0xA2; break;
            case (1 << PD2): data_packet[0] = 0xA3; break;
            case (1 << PD3): data_packet[0] = 0xA4; break;
            default:
                data_packet[0] = get_cmd(x, y);
                break;
        }

        nrf24_write(&data_packet, 2);
        _delay_ms(50);
    }
}
