// Rotina de interrupção do Timer TC5.
// É chamada na frequência de amostragem configurada (FREQUENCIA_AMOSTRAGEM)
// e é responsável por enviar cada amostra do buffer para o DAC.
void TC5_Handler() {
  // Se o DAC não estiver ativo, força a saída para o valor médio (512)
  // e limpa o flag de interrupção.
  if (!AudioDACAtivo) {
    DAC->DATA.reg = 512;
    while (DAC->STATUS.bit.SYNCBUSY);
    TC5->COUNT16.INTFLAG.bit.MC0 = 1;
    return;
  }
  // Se há dados no buffer (head != tail), envia próxima amostra.
  if (head != tail) {
    uint16_t amostra = bufferAudio[tail];
    // Se está em modo fade-out, aplica redução progressiva à amostra.
    if (emFadeOut) {
      if (contadorFadeOut < AMOSTRAS_FADE_OUT) {
        // fatorInt vai de AMOSTRAS_FADE_OUT até 1.
        int32_t fatorInt = AMOSTRAS_FADE_OUT - contadorFadeOut;
        // amostraCentro é a amostra centrada em 0 (subtrai 512).
        int32_t amostraCentro = (int32_t)amostra - 512;
        // Aplica fator de atenuação com shift (>> 9 ≈ dividir por 512)
        // e volta a deslocar para o centro (somando 512).
        amostra = (int16_t)((amostraCentro * fatorInt) >> 9) + 512;
        contadorFadeOut++;
      } else {
        AudioDACAtivo = false;
        emFadeOut = false;
        contadorFadeOut = 0;
        FechoFicheiroPendente = true;
        amostra = 512;
      }
    }
    // Envia a amostra (já possivelmente atenuada) para o DAC.
    DAC->DATA.reg = amostra;
    while (DAC->STATUS.bit.SYNCBUSY);
    // Avança o índice de leitura no buffer circular.
    tail = (tail + 1) % BUFFER_SIZE;
  } else {
    // Se não há dados no buffer, força saída ao valor médio.
    DAC->DATA.reg = 512;
    while (DAC->STATUS.bit.SYNCBUSY);
    // Se estava em fade-out mas o buffer acabou, termina o áudio
    // e marca o ficheiro para fecho.
    if (emFadeOut) {
      AudioDACAtivo = false;
      emFadeOut = false;
      contadorFadeOut = 0;
      FechoFicheiroPendente = true;
    }
  }
  // Limpa o flag de interrupção do TC5.
  TC5->COUNT16.INTFLAG.bit.MC0 = 1;
}

// Configura o Timer TC5 para gerar interrupções na frequência de amostragem requerida.
// Liga o relógio ao TC5, faz reset, escolhe modo 16-bit, frequência, prescaler, etc.
void configurarTimerHardware() {
  // Ativa o clock para o TC5.
  PM->APBCMASK.reg |= PM_APBCMASK_TC5;

  // Liga o TC5 ao gerador de clock GCLK0.
  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(TC5_GCLK_ID) |
                      GCLK_CLKCTRL_CLKEN |
                      GCLK_CLKCTRL_GEN_GCLK0;
  while (GCLK->STATUS.bit.SYNCBUSY);

  // Faz reset ao TC5.
  TC5->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
  while (TC5->COUNT16.STATUS.bit.SYNCBUSY);
  while (TC5->COUNT16.CTRLA.bit.SWRST);

  // Configura TC5 em modo 16 bits, frequência (MFRQ) e prescaler 1.
  TC5->COUNT16.CTRLA.reg =
    TC_CTRLA_MODE_COUNT16 |
    TC_CTRLA_WAVEGEN_MFRQ |
    TC_CTRLA_PRESCALER_DIV1;
  while (TC5->COUNT16.STATUS.bit.SYNCBUSY);

  // Calcula o número de "ticks" para a frequência de amostragem desejada
  // assumindo clock principal de 48 MHz.
  uint32_t ticks = (48000000 / FREQUENCIA_AMOSTRAGEM) - 1;
  TC5->COUNT16.CC[0].reg = ticks;
  while (TC5->COUNT16.STATUS.bit.SYNCBUSY);

  // Configura a interrupção NVIC para o TC5.
  NVIC_DisableIRQ(TC5_IRQn);
  NVIC_ClearPendingIRQ(TC5_IRQn);
  NVIC_SetPriority(TC5_IRQn, 0);
  NVIC_EnableIRQ(TC5_IRQn);

  // Ativa a interrupção por comparação (MC0) e o próprio contador.
  TC5->COUNT16.INTENSET.bit.MC0 = 1;
  TC5->COUNT16.CTRLA.bit.ENABLE = 1;
  while (TC5->COUNT16.STATUS.bit.SYNCBUSY);
}

// Devolve a posição onde terminam os dados de áudio de um WAV.
// Se o ficheiro não estiver aberto, devolve o tamanho do cabeçalho.
uint32_t ObterFimDadosWav() {
  if (!FicheiroAudioDAC) return WAV_HEADER_SIZE;
  return FicheiroAudioDAC.size();
}

// Lê a próxima amostra do ficheiro WAV e devolve-a escalada para 10 bits (0–1023),
// independentemente de o WAV ser de 8 ou 16 bits.
uint16_t LerAmostraAtualWav() {
  if (BITS_POR_AMOSTRA == 16) {
    // Se faltarem menos de 2 bytes, devolve valor médio (silêncio).
    if (FicheiroAudioDAC.available() < 2) return 512;

    // Lê dois bytes (little-endian: primeiro low, depois high).
    byte lowByte  = FicheiroAudioDAC.read();
    byte highByte = FicheiroAudioDAC.read();
    int16_t amostra16 = (highByte << 8) | lowByte;

    // Converte de intervalo [-32768..32767] para [0..65535] e depois
    // ajusta para 10 bits (>> 6).
    return ((int32_t)amostra16 + 32768) >> 6;

  } else {
    // WAV de 8 bits.
    if (FicheiroAudioDAC.available() < 1) return 512;

    uint8_t amostra8 = FicheiroAudioDAC.read();
    // Escala de [0..255] para [0..1023].
    return (uint16_t)((amostra8 * 1023) / 255);
  }
}

// Abre um ficheiro WAV no SD, prepara o buffer e ativa o DAC.
void IniciarFicheiroDAC(const String& nomeFicheiro) {
  // Se já há um ficheiro aberto, fecha-o primeiro.
  if (FicheiroAudioDAC) {
    FicheiroAudioDAC.close();
  }

  // Tenta abrir o ficheiro solicitado.
  FicheiroAudioDAC = SD.open(nomeFicheiro.c_str());
  if (!FicheiroAudioDAC) {
    // Se falhar, limpa o estado de reprodução e mostra erro em debug.
    FicheiroEmReproducao = "";
    AudioDACAtivo = false;

    if (DEBUG_AUDIO) {
      Serial.print("Erro a abrir ficheiro WAV: ");
      Serial.println(nomeFicheiro);
    }
    return;
  }

  // Salta o cabeçalho WAV para começar diretamente na zona dos dados.
  FicheiroAudioDAC.seek(WAV_HEADER_SIZE);

  // Secção crítica: reset ao buffer circular e flags de áudio.
  noInterrupts();
  head = 0;
  tail = 0;
  emFadeOut = false;
  contadorFadeOut = 0;
  FechoFicheiroPendente = false;
  AudioDACAtivo = true;
  interrupts();

  // Atualiza o nome do ficheiro atualmente em reprodução.
  FicheiroEmReproducao = nomeFicheiro;

  if (DEBUG_AUDIO) {
    Serial.print("A tocar: ");
    Serial.println(nomeFicheiro);
  }

  // Enche o buffer com as primeiras amostras.
  EncherBufferAudio();
}

// Inicia o processo de fade-out, se o áudio estiver ativo.
void IniciarFadeOutAudio() {
  if (!AudioDACAtivo) return;

  noInterrupts();
  if (!emFadeOut) {
    emFadeOut = true;
    contadorFadeOut = 0;
  }
  interrupts();
}

// Interface simples para parar o áudio (sempre via fade-out).
void PararAudioDAC() {
  IniciarFadeOutAudio();
}

// Fecha o ficheiro de áudio se a flag FechoFicheiroPendente estiver ativa.
// Esta função é chamada no loop principal (não na interrupção).
void FecharAudioSePendente() {
  if (!FechoFicheiroPendente) return;

  // Secção crítica: limpa flag e índices do buffer.
  noInterrupts();
  FechoFicheiroPendente = false;
  head = 0;
  tail = 0;
  interrupts();

  // Fecha o ficheiro se ainda estiver aberto.
  if (FicheiroAudioDAC) {
    FicheiroAudioDAC.close();
  }

  // Limpa o nome do ficheiro em reprodução.
  FicheiroEmReproducao = "";

  // Repõe o DAC no valor médio.
  DAC->DATA.reg = 512;
  while (DAC->STATUS.bit.SYNCBUSY);

  if (DEBUG_AUDIO) {
    Serial.println("Audio fechado");
  }
}

// Enche o buffer circular de áudio com amostras do ficheiro WAV.
// Pára de encher se:
//  - o áudio deixar de estar ativo,
//  - estiver em fade-out,
//  - o ficheiro não estiver válido,
//  - o buffer estiver cheio.
void EncherBufferAudio() {
  if (!AudioDACAtivo) return;
  if (emFadeOut) return;
  if (!FicheiroAudioDAC) return;

  const uint16_t bytesPorAmostra = (BITS_POR_AMOSTRA == 16) ? 2 : 1;
  const uint32_t fimDados = ObterFimDadosWav();

  while (true) {
    if (!AudioDACAtivo) return;
    if (emFadeOut) return;
    if (!FicheiroAudioDAC) return;

    // Calcula próxima posição para escrita no buffer circular.
    uint16_t proximoHead = (head + 1) % BUFFER_SIZE;
    if (proximoHead == tail) return; // Buffer cheio.

    // Se chegaste ao fim dos dados ou não há bytes suficientes, faz loop
    // para o início dos dados de áudio (comportamento “loop” do sample).
    if (FicheiroAudioDAC.position() >= fimDados ||
        FicheiroAudioDAC.available() < bytesPorAmostra) {
      FicheiroAudioDAC.seek(WAV_HEADER_SIZE);
    }

    // Lê uma amostra já escalada para o DAC e escreve-a no buffer.
    uint16_t valorDAC = LerAmostraAtualWav();
    bufferAudio[head] = valorDAC;
    head = proximoHead;
  }
}

// Função de alto nível chamada em cada loop().
// Gere o estado do áudio, incluindo:
//  - fechar ficheiro após fade-out,
//  - decidir se deve tocar, parar ou mudar de ficheiro,
//  - manter o buffer abastecido.
void GerirAudioDAC() {
  // Primeiro trata de fechar ficheiros pendentes (pós fade-out).
  FecharAudioSePendente();

  // Se ainda está em fade-out, não faz mais nada aqui.
  if (emFadeOut) {
    return;
  }

  // Se não há sopro válido ou não há nota mapeada, para o áudio se estiver a tocar.
  if (!SoproValido || NotaFicheiroAtual == "") {
    if (AudioDACAtivo) {
      PararAudioDAC();
    }
    return;
  }

  // Se há sopro e temos um ficheiro de nota, mas o DAC não está ativo,
  // inicia a reprodução da nota atual.
  if (!AudioDACAtivo) {
    IniciarFicheiroDAC(NotaFicheiroAtual);
    return;
  }

  // Se o ficheiro atual em reprodução não corresponde à nota atual,
  // inicia um fade-out para trocar de nota.
  if (NotaFicheiroAtual != FicheiroEmReproducao) {
    PararAudioDAC();
    return;
  }

  // Se tudo está alinhado (mesma nota e áudio ativo), continua a encher o buffer.
  EncherBufferAudio();
}