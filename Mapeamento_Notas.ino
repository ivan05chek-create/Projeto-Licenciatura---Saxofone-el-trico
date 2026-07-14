// Dado o código de chaves (NotaDecAtual) devolve o nome do ficheiro WAV
// correspondente à nota. Cada case associa uma combinação de chaves a um
// ficheiro específico (por ex. "Do4.wav").
// Os valores são os códigos decimais que resultam da combinação dos bits
// das chaves lidos pelos MCP23017 após a inversão lógica.
String ObterNomeNota(unsigned long codigoChaves) {
  switch (codigoChaves) {
    
    // --- 3ª Oitava ---
    case 1901105UL:   return "SiB3.wav";
    case 1901106UL:   return "Si3.wav";

    // --- 4ª Oitava ---
    case 1901104UL:   return "Do4.wav";
    case 69009968UL:  return "DoS4.wav";
    case 1835568UL:   return "Re4.wav";
    case 1966640UL:   return "ReS4.wav";
    case 1573424UL:   return "Mi4.wav";
    case 1049136UL:   return "Fa4.wav";
    case 524848UL:    return "FaS4.wav";
    case 560UL:       return "Sol4.wav";
    case 568UL:       return "SolS4.wav";
    case 544UL:       return "La4.wav";
    case 16777760UL:  return "SiB4.wav";
    case 512UL:       return "Si4.wav";

    // --- 5ª Oitava ---
    case 32UL:        return "Do5.wav";
    case 0UL:         return "DoS5.wav";
    case 1835696UL:   return "Re5.wav";
    case 1966768UL:   return "ReS5.wav";
    case 1573552UL:   return "Mi5.wav";
    case 1049264UL:   return "Fa5.wav";
    case 524976UL:    return "FaS5.wav";
    case 688UL:       return "Sol5.wav";
    case 696UL:       return "SolS5.wav";
    case 672UL:       return "La5.wav";
    case 16777888UL:  return "SiB5.wav";
    case 640UL:       return "Si5.wav";

    // --- 6ª Oitava ---
    case 160UL:       return "Do6.wav";
    case 128UL:       return "DoS6.wav";
    case 1152UL:      return "Re6.wav";
    case 3200UL:      return "ReS6.wav";
    case 33557632UL:  return "Mi6.wav";
    case 33557888UL:  return "Fa6.wav";
    case 33566080UL:  return "FaS6.wav";

    default:
      // Se não houver mapeamento para o código atual,
      // devolve string vazia (sem nota associada).
      return "";
  }
}

// Atualiza NotaFicheiroAtual a partir de NotaDecAtual.
// Esta função é chamada no loop principal, depois de AtualizarNotaDecAtual(),
// para obter o nome do ficheiro WAV que deve ser reproduzido.
void AtualizarNotaFicheiroAtual() {
  // Converte o código de chaves em nome de ficheiro WAV.
  NotaFicheiroAtual = ObterNomeNota(NotaDecAtual);

  // Se o modo de debug de nome estiver ativo, mostra na Serial
  // o código decimal da nota e o ficheiro correspondente.
  if (DEBUG_NOME) {
    Serial.print(F("NotaDecAtual="));
    Serial.print(NotaDecAtual);
    Serial.print(F(" | Ficheiro="));
    Serial.println(NotaFicheiroAtual);
    Serial.println();
  }
}