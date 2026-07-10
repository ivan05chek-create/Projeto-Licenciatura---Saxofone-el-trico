#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <math.h>

// =====================================================
// DEBUG
// =====================================================
const bool DEBUG_SOPRO = false;
const bool DEBUG_I2C   = false;
const bool DEBUG_NOME  = false;
const bool DEBUG_AUDIO = false;

// =====================================================
// PINOS
// =====================================================
const int LeituraSopro = A1;
const int chipSelect   = SDCARD_SS_PIN;

// =====================================================
// SOPRO
// =====================================================
const float V_MaxADC = 3.3;
const int ResolADC   = 1023;

const float V_MinSopro     = 1.8;
const float V_MaxSopro     = 3.30;
const float ThresholdSopro = 20.0;
const float MargemMinSopro = 5.0;

// =====================================================
// AUDIO
// =====================================================
const unsigned long FREQUENCIA_AMOSTRAGEM = 44100;
const int BITS_POR_AMOSTRA = 8;
const uint32_t WAV_HEADER_SIZE = 44;

#define BUFFER_SIZE 1024

const uint16_t AMOSTRAS_FADE_OUT = 512;

volatile uint16_t bufferAudio[BUFFER_SIZE];
volatile uint16_t head = 0;
volatile uint16_t tail = 0;
volatile bool AudioDACAtivo = false;

volatile bool emFadeOut = false;
volatile uint16_t contadorFadeOut = 0;
volatile bool FechoFicheiroPendente = false;

// =====================================================
// ESTADO GLOBAL
// =====================================================
float IntensidadeSopro = 0.0;
float DinSom = 0.0;
bool SoproValido = false;

uint8_t LeituraMCP1 = 0x20;
uint8_t LeituraMCP2 = 0x27;

unsigned long NotaDecAtual = 0;
String NotaFicheiroAtual = "";
String FicheiroEmReproducao = "";

File FicheiroAudioDAC;

// =====================================================
// REGISTOS MCP23017
// =====================================================
#define MCP23017_IODIRA 0x00
#define MCP23017_IODIRB 0x01
#define MCP23017_GPIOA  0x12
#define MCP23017_GPIOB  0x13
#define MCP23017_GPPUA  0x0C
#define MCP23017_GPPUB  0x0D

// =====================================================
// FUNÇÕES GERAIS
// =====================================================
void SetupProjeto();

// =====================================================
// FUNÇÕES DO SOPRO
// =====================================================
int LerMediaSopro(int amostras);
float LerTensaoSopro();
float LerPercentagemSopro();
void AtualizarEstadoSopro();

// =====================================================
// FUNÇÕES I2C / MCP23017
// =====================================================
void EscreverMCP(uint8_t endereco, uint8_t registo, uint8_t valor);
void InicializarMCP(uint8_t endereco);
void InicializarLeituraI2C();
uint16_t Ler16BitsMCP(uint8_t endereco);
uint32_t LerComboioBruto();
uint32_t InverterBits23(uint32_t valor);
void AtualizarNotaDecAtual();

// =====================================================
// FUNÇÕES DAS NOTAS
// =====================================================
String obterNomeNota(unsigned long codigoChaves);
void AtualizarNotaFicheiroAtual();

// =====================================================
// FUNÇÕES DO ÁUDIO DAC
// =====================================================
void configurarTimerHardware();
void IniciarFicheiroDAC(const String& nomeFicheiro);
void IniciarFadeOutAudio();
void PararAudioDAC();
void FecharAudioSePendente();
void EncherBufferAudio();
void GerirAudioDAC();

uint32_t ObterFimDadosWav();
uint16_t LerAmostraAtualWav();

void SetupProjeto() {
  if (DEBUG_SOPRO || DEBUG_I2C || DEBUG_NOME || DEBUG_AUDIO) {
    Serial.begin(115200);
    delay(500);
  }

  pinMode(LeituraSopro, INPUT);

  Wire.begin();
  InicializarLeituraI2C();

  if (!SD.begin(chipSelect)) {
    if (DEBUG_AUDIO) {
      Serial.println("Falha ao inicializar SD");
    }
  } else {
    if (DEBUG_AUDIO) {
      Serial.println("SD inicializado");
    }
  }

  analogWriteResolution(10);

  while (DAC->STATUS.bit.SYNCBUSY);
  DAC->CTRLA.bit.ENABLE = 1;
  while (DAC->STATUS.bit.SYNCBUSY);

  DAC->DATA.reg = 512;
  while (DAC->STATUS.bit.SYNCBUSY);

  configurarTimerHardware();
}