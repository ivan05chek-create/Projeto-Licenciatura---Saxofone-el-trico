int LerMediaSopro(int amostras) {
  int soma = 0;

  for (int i = 0; i < amostras; i++) {
    soma += analogRead(LeituraSopro);
  }

  return soma / amostras;
}

float LerTensaoSopro() {
  int leitura = analogRead(LeituraSopro);
  return (leitura * V_MaxADC) / ResolADC;
}

float LerPercentagemSopro() {
  float tensao = LerTensaoSopro();

  if (tensao <= V_MinSopro) return 0.0;
  if (tensao >= V_MaxSopro) return 100.0;

  return ((tensao - V_MinSopro) / (V_MaxSopro - V_MinSopro)) * 100.0;
}

void AtualizarEstadoSopro() {
  IntensidadeSopro = LerPercentagemSopro();

  if (IntensidadeSopro >= ThresholdSopro) {
    SoproValido = true;
  } else {
    SoproValido = false;
  }

  if (DEBUG_SOPRO) {
    Serial.print("Sopro=");
    Serial.print(IntensidadeSopro, 2);
    Serial.print(" | Valido=");
    Serial.println(SoproValido ? "SIM" : "NAO");
  }
}