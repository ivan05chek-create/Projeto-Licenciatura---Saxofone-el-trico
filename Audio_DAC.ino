void TC5_Handler() {
  if (!AudioDACAtivo) { //
    DAC->DATA.reg = 512; //
    while (DAC->STATUS.bit.SYNCBUSY); //
    TC5->COUNT16.INTFLAG.bit.MC0 = 1; //
    return; //
  }

  if (head != tail) {
    uint16_t amostra = bufferAudio[tail];

    if (emFadeOut) {
      if (contadorFadeOut < AMOSTRAS_FADE_OUT) {
        
        // =======================================================
        // NOVO CÓDIGO OPTIMIZADO (MATEMÁTICA DE INTEIROS):
        // =======================================================
        int32_t fatorInt = AMOSTRAS_FADE_OUT - contadorFadeOut; 
        int32_t amostraCentro = (int32_t)amostra - 512;        
        amostra = (int16_t)((amostraCentro * fatorInt) >> 9) + 512; 
        contadorFadeOut++;
        // =======================================================

      } else {
        AudioDACAtivo = false;
        emFadeOut = false;
        contadorFadeOut = 0;
        FechoFicheiroPendente = true;
        amostra = 512;
      }
    }

    DAC->DATA.reg = amostra;
    while (DAC->STATUS.bit.SYNCBUSY);
    tail = (tail + 1) % BUFFER_SIZE;
  } else {
    DAC->DATA.reg = 512;
    while (DAC->STATUS.bit.SYNCBUSY);
    if (emFadeOut) {
      AudioDACAtivo = false;
      emFadeOut = false;
      contadorFadeOut = 0;
      FechoFicheiroPendente = true;
    }
  }

  TC5->COUNT16.INTFLAG.bit.MC0 = 1;
}

void configurarTimerHardware() {
  PM->APBCMASK.reg |= PM_APBCMASK_TC5;

  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(TC5_GCLK_ID) |
                      GCLK_CLKCTRL_CLKEN |
                      GCLK_CLKCTRL_GEN_GCLK0;
  while (GCLK->STATUS.bit.SYNCBUSY);

  TC5->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
  while (TC5->COUNT16.STATUS.bit.SYNCBUSY);
  while (TC5->COUNT16.CTRLA.bit.SWRST);

  TC5->COUNT16.CTRLA.reg =
      TC_CTRLA_MODE_COUNT16 |
      TC_CTRLA_WAVEGEN_MFRQ |
      TC_CTRLA_PRESCALER_DIV1;
  while (TC5->COUNT16.STATUS.bit.SYNCBUSY);

  uint32_t ticks = (48000000 / FREQUENCIA_AMOSTRAGEM) - 1;
  TC5->COUNT16.CC[0].reg = ticks;
  while (TC5->COUNT16.STATUS.bit.SYNCBUSY);

  NVIC_DisableIRQ(TC5_IRQn);
  NVIC_ClearPendingIRQ(TC5_IRQn);
  NVIC_SetPriority(TC5_IRQn, 0);
  NVIC_EnableIRQ(TC5_IRQn);

  TC5->COUNT16.INTENSET.bit.MC0 = 1;
  TC5->COUNT16.CTRLA.bit.ENABLE = 1;
  while (TC5->COUNT16.STATUS.bit.SYNCBUSY);
}

uint32_t ObterFimDadosWav() {
  if (!FicheiroAudioDAC) return WAV_HEADER_SIZE;
  return FicheiroAudioDAC.size();
}

uint16_t LerAmostraAtualWav() {
  if (BITS_POR_AMOSTRA == 16) {
    if (FicheiroAudioDAC.available() < 2) return 512;

    byte lowByte = FicheiroAudioDAC.read();
    byte highByte = FicheiroAudioDAC.read();
    int16_t amostra16 = (highByte << 8) | lowByte;
    return ((int32_t)amostra16 + 32768) >> 6;
  } else {
    if (FicheiroAudioDAC.available() < 1) return 512;

    uint8_t amostra8 = FicheiroAudioDAC.read();
    return (uint16_t)((amostra8 * 1023) / 255);
  }
}

void IniciarFicheiroDAC(const String& nomeFicheiro) {
  if (FicheiroAudioDAC) {
    FicheiroAudioDAC.close();
  }

  FicheiroAudioDAC = SD.open(nomeFicheiro.c_str());
  if (!FicheiroAudioDAC) {
    FicheiroEmReproducao = "";
    AudioDACAtivo = false;

    if (DEBUG_AUDIO) {
      Serial.print("Erro a abrir ficheiro WAV: ");
      Serial.println(nomeFicheiro);
    }
    return;
  }

  FicheiroAudioDAC.seek(WAV_HEADER_SIZE);

  noInterrupts();
  head = 0;
  tail = 0;
  emFadeOut = false;
  contadorFadeOut = 0;
  FechoFicheiroPendente = false;
  AudioDACAtivo = true;
  interrupts();

  FicheiroEmReproducao = nomeFicheiro;

  if (DEBUG_AUDIO) {
    Serial.print("A tocar: ");
    Serial.println(nomeFicheiro);
  }

  EncherBufferAudio();
}

void IniciarFadeOutAudio() {
  if (!AudioDACAtivo) return;

  noInterrupts();
  if (!emFadeOut) {
    emFadeOut = true;
    contadorFadeOut = 0;
  }
  interrupts();
}

void PararAudioDAC() {
  IniciarFadeOutAudio();
}

void FecharAudioSePendente() {
  if (!FechoFicheiroPendente) return;

  noInterrupts();
  FechoFicheiroPendente = false;
  head = 0;
  tail = 0;
  interrupts();

  if (FicheiroAudioDAC) {
    FicheiroAudioDAC.close();
  }

  FicheiroEmReproducao = "";

  DAC->DATA.reg = 512;
  while (DAC->STATUS.bit.SYNCBUSY);

  if (DEBUG_AUDIO) {
    Serial.println("Audio fechado");
  }
}

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

    uint16_t proximoHead = (head + 1) % BUFFER_SIZE;
    if (proximoHead == tail) return;

    if (FicheiroAudioDAC.position() >= fimDados || FicheiroAudioDAC.available() < bytesPorAmostra) {
      FicheiroAudioDAC.seek(WAV_HEADER_SIZE);
    }

    uint16_t valorDAC = LerAmostraAtualWav();
    bufferAudio[head] = valorDAC;
    head = proximoHead;
  }
}

void GerirAudioDAC() {
  FecharAudioSePendente();

  if (emFadeOut) {
    return;
  }

  if (!SoproValido || NotaFicheiroAtual == "") {
    if (AudioDACAtivo) {
      PararAudioDAC();
    }
    return;
  }

  if (!AudioDACAtivo) {
    IniciarFicheiroDAC(NotaFicheiroAtual);
    return;
  }

  if (NotaFicheiroAtual != FicheiroEmReproducao) {
    PararAudioDAC();
    return;
  }

  EncherBufferAudio();
}