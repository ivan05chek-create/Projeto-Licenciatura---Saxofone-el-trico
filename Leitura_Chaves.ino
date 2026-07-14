// Escreve um valor num registo específico do MCP23017 via I2C.
// - endereco: endereço I2C do chip (LeituraMCP1 ou LeituraMCP2);
// - registo: registo interno do MCP (IODIRA, GPPUA, etc.);
// - valor: valor a ser escrito nesse registo.
void EscreverMCP(uint8_t endereco, uint8_t registo, uint8_t valor) {
  Wire.beginTransmission(endereco);
  Wire.write(registo);
  Wire.write(valor);
  Wire.endTransmission();
}

// Inicializa um MCP23017:
// - Configura todos os pinos dos bancos A e B como entradas (IODIRx = 0xFF);
// - Ativa os pull-ups internos em todos os pinos (GPPUx = 0xFF).
void InicializarMCP(uint8_t endereco) {
  EscreverMCP(endereco, MCP23017_IODIRA, 0xFF);
  EscreverMCP(endereco, MCP23017_IODIRB, 0xFF);
  EscreverMCP(endereco, MCP23017_GPPUA, 0xFF);
  EscreverMCP(endereco, MCP23017_GPPUB, 0xFF);
}

// Configura a velocidade do bus I2C e inicializa os dois MCPs usados
// para ler o teclado do sax eletrónico.
void InicializarLeituraI2C() {
  Wire.setClock(400000);          // Define I2C rápido (400 kHz) para leituras mais rápidas.
  InicializarMCP(LeituraMCP1);    // Inicializa primeiro MCP (metade do teclado).
  InicializarMCP(LeituraMCP2);    // Inicializa segundo MCP (restante teclado).
  if (DEBUG_I2C) Serial.println(F("Leitura I2C inicializada"));
}

// Lê 16 bits de um MCP23017 (bancos A + B).
// Devolve 0xFFFF em caso de erro de comunicação ou número de bytes inesperado.
uint16_t Ler16BitsMCP(uint8_t endereco) {
  Wire.beginTransmission(endereco);
  Wire.write(MCP23017_GPIOA);                 // Começa a leitura a partir do registo GPIOA.
  if (Wire.endTransmission(false) != 0)       // 'false' mantém o bus ativo (repeated start).
    return 0xFFFF;                            // Se houver erro, devolve código de erro.

  uint8_t recebidos = Wire.requestFrom((int)endereco, 2); // Pede 2 bytes (A e B).
  if (recebidos != 2) return 0xFFFF;          // Se não vierem 2 bytes, devolve erro.

  uint8_t portaA = Wire.read();               // Leitura dos 8 bits do banco A.
  uint8_t portaB = Wire.read();               // Leitura dos 8 bits do banco B.

  // Junta banco B (bits mais significativos) e banco A (menos significativos)
  // num único inteiro de 16 bits: [portaB | portaA].
  return ((uint16_t)portaB << 8) | portaA;
}

// Lê os dois MCPs e junta tudo num valor de 32 bits.
// O MCP1 ocupa os 16 bits menos significativos, MCP2 os 16 mais significativos.
uint32_t LerComboioBruto() {
  uint16_t valorMCP1 = Ler16BitsMCP(LeituraMCP1);
  uint16_t valorMCP2 = Ler16BitsMCP(LeituraMCP2);
  return ((uint32_t)valorMCP2 << 16) | valorMCP1;
}

// Corrigido para manter a máscara de 32 bits completa.
// Inverte todos os bits de um valor de 32 bits, garantindo que só os 32
// menos significativos são usados.
uint32_t InverterBits32(uint32_t valor) {
  return (~valor) & 0xFFFFFFFF;
}

// Função auxiliar para imprimir 16 bits em binário com zeros à esquerda.
// Útil para debug visual do estado das entradas dos MCPs.
void Imprimir16BitsBinario(uint16_t valor) {
  for (int i = 15; i >= 0; i--) {
    byte bit = (valor >> i) & 1;
    Serial.print(bit);
  }
}

// Atualiza a variável global NotaDecAtual com base na leitura atual do teclado.
// Só lê o teclado se houver sopro válido ou se algum modo de debug relacionado
// estiver ativo.
void AtualizarNotaDecAtual() {
  // CORREÇÃO: Agora também lê o teclado se o DEBUG_NOME estiver ativo!
  // Se não houver sopro válido e nenhum debug de I2C ou nome ativo,
  // a nota é considerada 0 (sem nota) e retorna-se de imediato.
  if (!SoproValido && !DEBUG_I2C && !DEBUG_NOME) {
    NotaDecAtual = 0;
    return;
  }

  // Lê os 16 bits de cada MCP (dois blocos do teclado).
  uint16_t valorMCP1 = Ler16BitsMCP(LeituraMCP1);
  uint16_t valorMCP2 = Ler16BitsMCP(LeituraMCP2);

  // Junta os dois valores num “comboio” de 32 bits.
  uint32_t comboioBruto = ((uint32_t)valorMCP2 << 16) | valorMCP1;
  // Inverte a lógica (teclas com pull-up: 1 = solto, 0 = pressionado).
  // Inverter converte para uma representação mais conveniente.
  uint32_t comboioInvertido = InverterBits32(comboioBruto);
  // Guarda resultado globalmente para ser usado no mapeamento de notas.
  NotaDecAtual = comboioInvertido;

  // Se o debug I2C estiver ativo, imprime estado detalhado do teclado.
  if (DEBUG_I2C) {
    Serial.println(F("--- ESTADO DO TECLADO ---"));

    Serial.print(F("MCP 1: "));
    Imprimir16BitsBinario(valorMCP1);
    Serial.println();

    Serial.print(F("MCP 2: "));
    Imprimir16BitsBinario(valorMCP2);
    Serial.println();

    Serial.print(F("Valor decimal: "));
    Serial.println(NotaDecAtual);
    Serial.println();
  }
}