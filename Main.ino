void setup() {
  SetupProjeto();
}

// Loop principal do microcontrolador.
// Em cada iteração:
// 1) Atualiza o estado do sopro (sensor de pressão);
// 2) Lê o teclado (MCPs) e calcula o código decimal da nota;
// 3) Atualiza o nome do ficheiro WAV correspondente à nota atual;
// 4) Gere o motor de áudio/DAC (início/paragem/fade-out e enchimento de buffer).
void loop() {
  AtualizarEstadoSopro();
  AtualizarNotaDecAtual();
  AtualizarNotaFicheiroAtual();
  GerirAudioDAC();
}