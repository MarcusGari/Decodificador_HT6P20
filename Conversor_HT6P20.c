 ////////////////////////Conversor_HT6P20.c//////////////////////////////////////////////////////////////////////////////
// AUTOR : Marcus Vinícius Marques Costa                                                                                //
// FUNÇÃO: Decodificar mensagem gerada pelo encoder ht6p20A/B/D comumente usado em controles remotos tipo CodeLearning  //
// OBS.  : A variável "addr" (32 bits) guarda o endereço recebido;                                                      //
//         A variável "dado" (8 bits) indica o botão acionado;                                                          //
//         Microcontolador utilizado PIC18F452.                                                                         //
 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define lig_tmr   T1CON.TMR1ON         //Habilita ou desabilita TMR1
#define est_tmr   PIR1.F0              //Flag de estouro do TMR1
#define lo(param) ((short *)&param)[0] //Função p/ ler o 1º byte de um registro
#define hi(param) ((short *)&param)[1] //Função p/ ler o 2º byte de um registro

unsigned short int qtd_bits;           //Quantidade de bits a serem tratados
unsigned short int cauda;              //Últimos bits recebidos pelo protocolo
unsigned short int expoente;           //Expoente auxiliar para análise da cauda
unsigned short int falha = 0;          //Receberá o código de erro/falha
unsigned long  int addr = 0;           //Variável que armazena os addrs recebidos
unsigned short int dado = 0;           //Variável que armazena dado recebidos
unsigned       int ajuste;             //Valor para ajuste do tempo do TMR1

void init_ht6p20(void){                //Inicialização de TMR1 e INT0
   RCON.IPEN       = 0;                //Desabilita prioridades de interrupção (0 = Disable priority levels on interrupts)
   PIE1            = 0;                //Desabilita interrupções PP,AD,Rx,Tx,SSP,CCP,TMR1,TMR2
   //configurações do TMR1:
   T1CON.RD16      = 0;                //f7 - Modo 16 bits (0 = Enables register Read/Write of Timer1 in two 8-bit operations)
   T1CON.T1CKPS1   = 0;                //f5 - Prescale 1:1
   T1CON.T1CKPS0   = 0;                //f4 - Prescale 1:1
   T1CON.T1OSCEN   = 0;                //f3 - Desabilita oscilador do timer1 (0 = Timer1 Oscillator is shut-off)
   T1CON.TMR1CS    = 0;                //f1 - Clock Interno (0 = Internal clock (FOSC/4))
   T1CON.TMR1ON    = 0;                //f0 - Timer1 desligado (0 = Stops Timer1)
   CCP1CON         = 0;                //Desabilita módulo CCP (Capture/Compare/PWM) do TMR1
   TMR1L           = 0;                //Zera TMR1 (low byte)
   TMR1H           = 0;                //Zera TMR1 (high byte)
   est_tmr         = 0;                //Limpa flag de estouro do TMR1
   //configurações do INT0:
   TRISB.f0        = 1;                //Define portB.f0 como entrada
   INTCON2.RBPU    = 1;                //Desabilita pull-ups no portB (1 = All PORTB pull-ups are disabled)
   INTCON2.INTEDG0 = 1;                //INT0 em borda de subida (1 = Interrupt on rising edge)
   INTCON          = 0;                //Desabilita todas as interrupções e limpa flags
   INTCON.INT0IE   = 1;                //Habilita INT0
   INTCON.GIE      = 1;                //Habilita Global Interruptions
}

short receive_ht6p20(void){            //Decodificação do formato HT6P20A/B/D
   ////////////////////////////////////////////
   //       RECEBIMENTO DA MENSAGEM          //
   ////////////////////////////////////////////
   ajuste=65536-TMR1L-(TMR1H*256);     //Seta o ajuste para TMR1 com o pulso inicial
   est_tmr = 0;                        //Limpa flag de estouro do TMR1
   TMR1L = lo(ajuste);                 //Ajusta TMR1 (low byte)
   TMR1H = hi(ajuste);                 //Ajusta TMR1 (high byte)
   qtd_bits = 28;                      //Reseta a quantidade de bits (20|22 + 4|2 + 4)
   while(qtd_bits>0){                  //Enquanto houver bits a receber
   /////PARTE 1 de 3/////
      lig_tmr = 1;                     //Liga TMR1
      while(est_tmr==0);               //Aguarda TMR1 estourar
   /////PARTE 2 de 3/////
      lig_tmr = 0;                     //Desliga TMR1
      addr = addr << 1;                //Move addr para receber o próximo bit
      if(portB.f0==0){                 //Se RB0 for 0 indica bit 1
         addr++;                       //Incrementa addr
      }
      est_tmr = 0;                     //Limpa flag de estouro do TMR1
      TMR1L = lo(ajuste);              //Ajusta TMR1 (low byte)
      TMR1H = hi(ajuste);              //Ajusta TMR1 (high byte)
      lig_tmr = 1;                     //Liga TMR1
      while(est_tmr==0);               //Aguarda TMR1 estourar
   /////PARTE 3 de 3/////
      lig_tmr = 0;                     //Desliga TMR1
      if(portB.f0==0){                 //Se RB0 for 0 indica erro
         falha=qtd_bits;               //Seta falha para informar o erro
         return(0);                    //Encerra leitura da mensagem
      }
      qtd_bits--;                      //Decrementa a quantidade de bits
      est_tmr = 0;                     //Limpa flag de estouro do TMR1
      TMR1L = lo(ajuste);              //Ajusta TMR1 (low byte)
      TMR1H = hi(ajuste);              //Ajusta TMR1 (high byte)
      while(portB.f0==1);              //Aguarda TMR1 estourar
	 ///// FIM DO BIT /////
   }
   ////////////////////////////////////////////
   // CONFIRMAÇÃO DO FIM DO PROTOCOLO (0101) //
   ////////////////////////////////////////////
   cauda = 0;                          //Reseta a cauda
   expoente = 1;                       //Reseta o expoente
   while(qtd_bits<4){                  //Lê os 4 últimos bits da mensagem
      cauda = cauda + addr%2 * expoente; //Copia último bit na cauda
      addr = addr >> 1;                //Apaga último bit da mensagem
      expoente = expoente * 2;         //Incrementa o expoente "exponencialmente"
      qtd_bits++;                      //Decrementa a quantidade de bits
   }
   if(cauda!=0b0101){                  //Se os 4 últimos bits não estiverem corretos
      falha = 30;                      //Seta falha para informar o erro
      return(0);                       //Encerra leitura da mensagem
   }
   ////////////////////////////////////////////
   //   SEPARAÇÃO DOS 2 ÚLTIMOS BITs (DADOS) //
   ////////////////////////////////////////////
   dado = addr%2;                      //Copia último bit para dado
   addr = addr >> 1;                   //Deleta último bit da mensagem
   dado = dado + ((addr%2)*2);         //Copia penúltimo bit para dado
   addr = addr >> 1;                   //Deleta penúltimo bit da mensagem
   return(1);                          //Leitura da mensagem concluída com sucesso
}