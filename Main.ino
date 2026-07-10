void setup() {
  SetupProjeto();
}

void loop() {
  AtualizarEstadoSopro();
  AtualizarNotaDecAtual();
  AtualizarNotaFicheiroAtual();
  GerirAudioDAC();
}