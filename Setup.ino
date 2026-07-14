#pragma once  // Evita múltiplas inclusões deste cabeçalho no mesmo projeto (em builds maiores).

// As linhas de include foram exportadas sem o nome das bibliotecas.
// Aqui estariam, tipicamente, as bibliotecas: Wire, SPI, SD, etc., necessárias ao projeto.
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

// =====================================================
// DEBUG
// =====================================================
// Flags de debug para ativar/desativar mensagens na Serial
// em diferentes partes do sistema.
const bool DEBUG_SOPRO  = false; // Debug relacionado com o sensor de sopro.
const bool DEBUG_I2C    = false; // Debug da leitura via I2C (MCP23017).
const bool DEBUG_NOME   = false; // Debug do mapeamento de notas/nomes de ficheiros.
const bool DEBUG_AUDIO  = false; // Debug do subsistema de áudio/DAC.

// =====================================================
// PINOS
// =====================================================
// Pino analógico onde o sensor de sopro está ligado.
const int LeituraSopro = A1;
// Pino de chip select do cartão SD (vem da board/definição do core).
const int chipSelect   = SDCARD_SS_PIN;

// =====================================================
// SOPRO
// =====================================================
// Tensão máxima correspondente ao valor máximo do ADC (3.3 V).
const float V_MaxADC = 3.3;
// Resolução do ADC (10 bits → valores [0..1023]).
const int ResolADC   = 1023;

// Tensão mínima do sensor a partir da qual consideras que há sopro útil.
const float V_MinSopro    = 1.8;
// Tensão à qual consideras 100 % de sopro (escala superior).
const float V_MaxSopro    = 3.30;
// Percentagem mínima de sopro para ser considerado “válido” (nota pode tocar).
const float ThresholdSopro = 20.0;
// Margem mínima para histerese ou ajustes (não é usada no excerto de código visível).
const float MargemMinSopro = 5.0;

// =====================================================
// AUDIO
// =====================================================
// Frequência de amostragem de reprodução do áudio (Hz).
const unsigned long FREQUENCIA_AMOSTRAGEM = 44100;
// Profundidade de bits das amostras WAV (8 ou 16 bits, aqui configurado para 8).
const int BITS_POR_AMOSTRA = 8;
// Tamanho fixo do cabeçalho WAV padrão (44 bytes).
const uint32_t WAV_HEADER_SIZE = 44;

// Tamanho do buffer circular para as amostras enviadas ao DAC.
#define BUFFER_SIZE 1024

// Número de amostras usado para realizar o fade-out quando paras o áudio.
const uint16_t AMOSTRAS_FADE_OUT = 512;

// Buffer circular de áudio que o ISR do Timer/DAC vai consumir.
volatile uint16_t bufferAudio[BUFFER_SIZE];
// Índice de escrita no buffer.
volatile uint16_t head = 0;
// Índice de leitura no buffer.
volatile uint16_t tail = 0;
// Indica se o DAC está ativamente a tocar áudio.
volatile bool AudioDACAtivo = false;

// Indica se está em curso um fade-out.
volatile bool emFadeOut = false;
// Contador de quantas amostras já foram processadas em fade-out.
volatile uint16_t contadorFadeOut = 0;
// Flag para indicar que, após terminar o fade-out, é preciso fechar o ficheiro.
volatile bool FechoFicheiroPendente = false;

// =====================================================
// ESTADO GLOBAL
// =====================================================
// Percentagem de sopro atual (0–100 %, calculada a partir da tensão).
float IntensidadeSopro = 0.0;
// Eventual parâmetro de dinâmica/volume (não usado no excerto em baixo).
float DinSom           = 0.0;
// Indica se o sopro atual é considerado válido (acima de ThresholdSopro).
bool SoproValido       = false;

// Endereços I2C dos dois MCP23017 usados para ler o teclado.
uint8_t LeituraMCP1 = 0x20;
uint8_t LeituraMCP2 = 0x27;

// Código decimal que representa o estado atual das chaves (32 bits).
unsigned long NotaDecAtual = 0;
// Nome do ficheiro WAV correspondente à nota atual (ex: "Do4.wav").
String NotaFicheiroAtual   = "";
// Nome do ficheiro WAV que está efetivamente em reprodução neste momento.
String FicheiroEmReproducao = "";

// Handle para o ficheiro de áudio atualmente aberto no cartão SD.
File FicheiroAudioDAC;

// =====================================================
// REGISTOS MCP23017
// =====================================================
// Endereços dos registos internos do MCP23017 (configuração e GPIO).
#define MCP23017_IODIRA 0x00  // Direção dos pinos do banco A (entrada/saída).
#define MCP23017_IODIRB 0x01  // Direção dos pinos do banco B.
#define MCP23017_GPIOA  0x12  // Leitura/escrita dos pinos do banco A.
#define MCP23017_GPIOB  0x13  // Leitura/escrita dos pinos do banco B.
#define MCP23017_GPPUA  0x0C  // Pull-ups do banco A.
#define MCP23017_GPPUB  0x0D  // Pull-ups do banco B.

// =====================================================
// FUNÇÕES GERAIS
// =====================================================
// Prototipo da função que inicializa todo o projeto (chamada em setup()).
void SetupProjeto();

// =====================================================
// FUNÇÕES DO SOPRO
// =====================================================
// Calcula a média de várias leituras de ADC do sensor de sopro.
int   LerMediaSopro(int amostras);
// Converte uma leitura ADC de sopro em tensão (V).
float LerTensaoSopro();
// Converte a tensão de sopro em percentagem 0–100 %.
float LerPercentagemSopro();
// Atualiza as variáveis globais relacionadas com o sopro.
void  AtualizarEstadoSopro();

// =====================================================
// FUNÇÕES I2C / MCP23017
// =====================================================
// Escreve num registo do MCP23017 via I2C.
void     EscreverMCP(uint8_t endereco, uint8_t registo, uint8_t valor);
// Configura o MCP23017 (direção dos pinos e pull-ups).
void     InicializarMCP(uint8_t endereco);
// Faz a configuração inicial dos MCPs e da velocidade I2C.
void     InicializarLeituraI2C();
// Lê 16 bits de um MCP (bancos A e B).
uint16_t Ler16BitsMCP(uint8_t endereco);
// Lê o “comboio” de 32 bits (MCP1 + MCP2).
uint32_t LerComboioBruto();
// Inverte os bits de um inteiro de 32 bits (versão antiga com 23 bits não é usada aqui).
uint32_t InverterBits23(uint32_t valor);
// Atualiza o valor global NotaDecAtual com base na leitura dos MCPs.
void     AtualizarNotaDecAtual();

// =====================================================
// FUNÇÕES DAS NOTAS
// =====================================================
// Devolve o nome do ficheiro de nota correspondente ao código de chaves.
String obterNomeNota(unsigned long codigoChaves);
// Atualiza a string NotaFicheiroAtual com base em NotaDecAtual.
void   AtualizarNotaFicheiroAtual();

// =====================================================
// FUNÇÕES DO ÁUDIO DAC
// =====================================================
// Configura o timer hardware (TC5) para gerar a taxa de amostragem do DAC.
void configurarTimerHardware();
// Abre um ficheiro WAV e prepara o buffer/DAC para o reproduzir.
void IniciarFicheiroDAC(const String& nomeFicheiro);
// Inicia o processo de fade-out do áudio atualmente a tocar.
void IniciarFadeOutAudio();
// Pede a paragem do áudio (via fade-out).
void PararAudioDAC();
// Fecha o ficheiro WAV, quando a flag FechoFicheiroPendente está ativa.
void FecharAudioSePendente();
// Enche o buffer circular de áudio com mais amostras do ficheiro.
void EncherBufferAudio();
// Função de alto nível que coordena o estado do áudio em cada loop().
void GerirAudioDAC();

// Calcula o fim dos dados de áudio (posição final) num ficheiro WAV.
uint32_t ObterFimDadosWav();
// Lê a próxima amostra do ficheiro WAV e devolve-a escalada para o DAC.
uint16_t LerAmostraAtualWav();

// Função principal de setup do projeto, chamada pelo Arduino em setup().
void SetupProjeto() {
  // Se qualquer flag de debug estiver ativa, inicializa a porta série
  // para poderes ver mensagens de diagnóstico.
  if (DEBUG_SOPRO || DEBUG_I2C || DEBUG_NOME || DEBUG_AUDIO) {
    Serial.begin(115200);
    delay(500);
  }

  // Configura o pino do sensor de sopro como entrada analógica.
  pinMode(LeituraSopro, INPUT);

  // Inicia o bus I2C e configura os MCP23017 para leitura do teclado.
  Wire.begin();
  InicializarLeituraI2C();

  // Tenta inicializar o cartão SD.
  if (!SD.begin(chipSelect)) {
    // Se falhar e o debug de áudio estiver ativo, mostra mensagem de erro.
    if (DEBUG_AUDIO) {
      Serial.println("Falha ao inicializar SD");
    }
  } else {
    // Se o SD for inicializado com sucesso e o debug áudio estiver ativo,
    // imprime mensagem de sucesso.
    if (DEBUG_AUDIO) {
      Serial.println("SD inicializado");
    }

    // Define a resolução do PWM/DAC (10 bits) para trabalhar em 0–1023.
    analogWriteResolution(10);

    // As instruções seguintes garantem que o DAC está pronto e sincronizado
    // antes de ser usado.
    while (DAC->STATUS.bit.SYNCBUSY);
    DAC->CTRLA.bit.ENABLE = 1;
    while (DAC->STATUS.bit.SYNCBUSY);

    // Coloca o DAC a meio da escala (512 em 10 bits) para evitar offset DC.
    DAC->DATA.reg = 512;
    while (DAC->STATUS.bit.SYNCBUSY);

    // Configura o timer hardware que vai disparar a interrupção de saída de áudio.
    configurarTimerHardware();
  }

  // (O ficheiro original pode ter mais código de setup após esta linha.)
}