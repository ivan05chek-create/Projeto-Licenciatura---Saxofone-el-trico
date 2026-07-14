// Lê 'amostras' vezes o ADC do sensor de sopro e devolve a média inteira.
// Útil para filtrar ruído de leitura.
int LerMediaSopro(int amostras) {
  int soma = 0;

  for (int i = 0; i < amostras; i++) {
    soma += analogRead(LeituraSopro);
  }

  return soma / amostras;
}

// Lê o ADC do sensor de sopro uma vez e converte o valor em tensão (V).
// Usa V_MaxADC e ResolADC definidos em Setup.ino.
float LerTensaoSopro() {
  int leitura = analogRead(LeituraSopro);
  return (leitura * V_MaxADC) / ResolADC;
}

// Converte a tensão lida do sensor em percentagem de sopro (0–100 %).
// Aplica saturação abaixo de V_MinSopro e acima de V_MaxSopro.
float LerPercentagemSopro() {
  float tensao = LerTensaoSopro();

  if (tensao <= V_MinSopro) return 0.0;
  if (tensao >= V_MaxSopro) return 100.0;

  // Escala linear: (tensão - V_Min) / (V_Max - V_Min) → [0..1], depois *100.
  return ((tensao - V_MinSopro) / (V_MaxSopro - V_MinSopro)) * 100.0;
}

// Atualiza as variáveis globais ligadas ao sopro.
// - Calcula IntensidadeSopro como percentagem;
// - Define SoproValido se estiver acima do ThresholdSopro;
// - Opcionalmente escreve na Serial o valor e o estado.
void AtualizarEstadoSopro() {
  // Atualiza a percentagem atual do sopro (0-100%)
  IntensidadeSopro = LerPercentagemSopro();

  // IMPLEMENTAÇÃO DA JANELA DE HISTERESE
  if (!SoproValido) {
    // Se o som estava desligado, só liga se ultrapassar o limiar de disparo (ex: 20%)
    if (IntensidadeSopro >= ThresholdSopro) {
      SoproValido = true;
    }
  } else {
    // Se o som já estava ligado, mantém-se ativo até cair abaixo da margem mínima.
    // Margem de corte = ThresholdSopro - MargemMinSopro (ex: 20% - 5% = 15%)
    if (IntensidadeSopro < (ThresholdSopro - MargemMinSopro)) {
      SoproValido = false; // Corta a nota de forma estável
    }
  }
}


