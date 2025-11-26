Arquitetura de Hardware
⚙️ Resumo do Sistema

Microcontrolador: ATmega328P
Linguagem: C puro (sem Arduino)
Alimentação: Bateria recarregável de 9V
Comunicação: Rádio NRF24L01 para controle remoto

Atuadores:

Motores controlados via PWM

MOSFETs IRLZ44N para chaveamento de potência

Optoacopladores para isolamento

Sistema Laser:

Emissor laser disparando a cada 1 segundo (Timer1)

LDR de 20 mm para detecção de acertos

Feedback:

3 LEDs indicando “vida”

Função de Jogo:

Cada acerto detectado reduz 1 LED

Quando todos se apagam, o carrinho “fica fora”

1. 🖥 Unidade de Processamento

ATmega328P operando com Timer1 para gerar o intervalo de disparo de 1 segundo

Controle de PWM para motores, sem uso de bibliotecas Arduino

Comunicação SPI com o módulo NRF24L01

2. 🔋 Alimentação

Bateria recarregável de 9V

Reguladores e capacitores de filtragem para estabilidade

Optoacopladores para separar parte lógica da potência

3. 🚀 Sistema de Movimento

PWM gerado pelo ATmega328P

MOSFETs IRLZ44N acionam os motores com baixa perda

Optoacoplamento evita que ruídos dos motores afetem a eletrônica

4. 🔫 Sistema Laser

Laser disparado automaticamente a cada 1s (via interrupção do Timer1)

LDR de 20 mm usado como sensor para detectar acertos

A cada acerto, o sistema decrementa a vida (LEDs)

5. 🛰 Comunicação via Rádio

Controle remoto usando módulo NRF24L01

O transmissor envia comandos de movimento e ações

O carrinho interpreta o comando e converte em ação real via PWM

6. 🔆 LEDs de Vida

3 LEDs representam a quantidade de vida restante

Cada disparo detectado apaga 1 LED

Com 0 LEDs → movimento do carrinho é desabilitado
