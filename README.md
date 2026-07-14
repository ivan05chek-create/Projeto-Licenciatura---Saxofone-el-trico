# Saxofone Eletrónico — Arduino

Projeto de um **Saxofone Eletrónico** baseado em Arduino (SAMD21), que combina leitura de teclado físico, sensor de sopro e reprodução de amostras WAV em tempo real através do DAC interno do microcontrolador.

---

## Funcionalidades

- **Sensor de sopro** — leitura analógica de pressão com histerese para evitar falsos disparos
- **Teclado físico** — leitura de até 32 chaves via dois expansores I/O MCP23017 em barramento I²C a 400 kHz
- **Mapeamento de notas** — tabela de lookup que converte a combinação de chaves num ficheiro WAV (ex: `Do4.wav`, `Sol5.wav`)
- **Reprodução de áudio** — streaming de ficheiros WAV do cartão SD para o DAC com buffer circular de 1024 amostras, gerido por interrupção do Timer TC5 a 44 100 Hz
- **Fade-out** — transição suave ao largar uma nota ou ao mudar de nota, com 512 amostras de atenuação progressiva
- **Suporte WAV 8-bit e 16-bit** — amostras escaladas automaticamente para os 10 bits do DAC

---

## Hardware necessário

| Componente | Detalhe |
|---|---|
| Microcontrolador | Arduino Zero / MKR ou equivalente SAMD21 |
| Sensor de sopro | Saída analógica ligada ao pino `A1` |
| Expansores I/O | 2× MCP23017 (endereços `0x20` e `0x27`) |
| Cartão SD | Ligado via SPI (`SDCARD_SS_PIN`) |
| Amostras de áudio | Ficheiros `.wav` na raiz do cartão SD |

---

## Estrutura do projeto

```
├── Main.ino              # setup() e loop() — ponto de entrada
├── Setup.ino             # Constantes globais, pinos, variáveis de estado e inicialização
├── Sensor_Sopro.ino      # Leitura e processamento do sensor de sopro
├── Leitura_Chaves.ino    # Comunicação I²C com os MCP23017 e leitura do teclado
├── Mapeamento_Notas.ino  # Tabela de mapeamento código de chaves → ficheiro WAV
└── Audio_DAC.ino         # Motor de áudio: Timer TC5, buffer circular, DAC e fade-out
```

### Fluxo principal (loop)

```
AtualizarEstadoSopro()       → percentagem e validade do sopro
AtualizarNotaDecAtual()      → código de 32 bits das chaves pressionadas
AtualizarNotaFicheiroAtual() → nome do ficheiro WAV correspondente
GerirAudioDAC()              → inicia/para/troca nota no DAC
```

---

## Notas disponíveis

O instrumento cobre **três oitavas**, de **Si♭3** a **Fá#6**:

`SiB3` `Si3` `Do4` `Do#4` `Re4` `Re#4` `Mi4` `Fa4` `Fa#4` `Sol4` `Sol#4` `La4`  
`SiB4` `Si4` `Do5` `Do#5` `Re5` `Re#5` `Mi5` `Fa5` `Fa#5` `Sol5` `Sol#5` `La5`  
`SiB5` `Si5` `Do6` `Do#6` `Re6` `Re#6` `Mi6` `Fa6` `Fa#6`

Os ficheiros WAV devem estar na raiz do cartão SD com o nome exato (ex: `Do4.wav`).

---

## Configuração do áudio

| Parâmetro | Valor |
|---|---|
| Frequência de amostragem | 44 100 Hz |
| Profundidade de bits | 8-bit (configurável para 16-bit) |
| Tamanho do buffer | 1024 amostras |
| Amostras de fade-out | 512 |
| Cabeçalho WAV ignorado | 44 bytes |

---

## Flags de debug

Em `Setup.ino` podem ser ativadas individualmente via `Serial`:

```cpp
const bool DEBUG_SOPRO = false; // Sensor de sopro
const bool DEBUG_I2C   = false; // Leitura do teclado MCP23017
const bool DEBUG_NOME  = false; // Mapeamento de notas
const bool DEBUG_AUDIO = false; // Motor de áudio/DAC
```

---

## Dependências (bibliotecas Arduino)

- `Wire` — comunicação I²C
- `SPI` — barramento SPI
- `SD` — acesso ao cartão SD
- `Arduino.h` — core SAMD21
