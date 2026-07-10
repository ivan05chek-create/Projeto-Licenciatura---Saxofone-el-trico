void EscreverMCP(uint8_t endereco, uint8_t registo, uint8_t valor) {
  Wire.beginTransmission(endereco);
  Wire.write(registo);
  Wire.write(valor);
  Wire.endTransmission();
}

void InicializarMCP(uint8_t endereco) {
  EscreverMCP(endereco, MCP23017_IODIRA, 0xFF);
  EscreverMCP(endereco, MCP23017_IODIRB, 0xFF);
  EscreverMCP(endereco, MCP23017_GPPUA, 0xFF);
  EscreverMCP(endereco, MCP23017_GPPUB, 0xFF);
}

void InicializarLeituraI2C() {
  Wire.setClock(400000);
  InicializarMCP(LeituraMCP1);
  InicializarMCP(LeituraMCP2);
  if (DEBUG_I2C) Serial.println(F("Leitura I2C inicializada"));
}

uint16_t Ler16BitsMCP(uint8_t endereco) {
  Wire.beginTransmission(endereco);
  Wire.write(MCP23017_GPIOA);
  if (Wire.endTransmission(false) != 0) return 0xFFFF;
  uint8_t recebidos = Wire.requestFrom((int)endereco, 2);
  if (recebidos != 2) return 0xFFFF;
  uint8_t portaA = Wire.read();
  uint8_t portaB = Wire.read();
  return ((uint16_t)portaB << 8) | portaA;
}

uint32_t LerComboioBruto() {
  uint16_t valorMCP1 = Ler16BitsMCP(LeituraMCP1);
  uint16_t valorMCP2 = Ler16BitsMCP(LeituraMCP2);
  return ((uint32_t)valorMCP2 << 16) | valorMCP1;
}

// Corrigido para manter a máscara de 32 bits completa
uint32_t InverterBits32(uint32_t valor) {
  return (~valor) & 0xFFFFFFFF;
}

// Função auxiliar para imprimir 16 bits em binário com zeros à esquerda
void Imprimir16BitsBinario(uint16_t valor) {
  for (int i = 15; i >= 0; i--) {
    byte bit = (valor >> i) & 1;
    Serial.print(bit);
  }
}

void AtualizarNotaDecAtual() {
  // CORREÇÃO: Agora também lê o teclado se o DEBUG_NOME estiver ativo!
  if (!SoproValido && !DEBUG_I2C && !DEBUG_NOME) {
    NotaDecAtual = 0;
    return;
  }

  uint16_t valorMCP1 = Ler16BitsMCP(LeituraMCP1);
  uint16_t valorMCP2 = Ler16BitsMCP(LeituraMCP2);
  
  uint32_t comboioBruto = ((uint32_t)valorMCP2 << 16) | valorMCP1;
  uint32_t comboioInvertido = InverterBits32(comboioBruto);
  NotaDecAtual = comboioInvertido;

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