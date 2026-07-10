String ObterNomeNota(unsigned long codigoChaves) {
  switch (codigoChaves) {
    case 1UL:         return "SiB3.wav";
    case 2UL:         return "Si3.wav";
    case 3UL:         return "Do4.wav";
    case 4UL:         return "DoS4.wav";
            case 1835568UL:        return "Re4.wav";
    case 5UL:        return "ReS4.wav";
            case 1573424UL:        return "Mi4.wav";
            case 1049136UL:       return "Fa4.wav";
            case 524848UL:       return "FaS4.wav";
            case 560UL:       return "Sol4.wav";
    case 6UL:      return "SolS4.wav";
            case 544UL:      return "La4.wav";
    case 7UL:      return "SiB4.wav";
            case 512UL:      return "Si4.wav";
            case 32UL:     return "Do5.wav";
            case 0UL:     return "DoS5.wav";
            case 1835696UL:     return "Re5.wav";
    case 88UL: return "ReS5.wav";
            case 1573552UL:    return "Mi5.wav";
            case 1049264UL:    return "Fa5.wav";
            case 524976UL:   return "FaS5.wav";
          case 688UL:   return "Sol5.wav";
    case 9UL:   return "SolS5.wav";
            case 672UL:   return "La5.wav";
    case 10UL:  return "SiB5.wav";
            case 640UL:  return "Si5.wav";
            case 160UL:  return "Do6.wav";
            case 128UL: return "DoS6.wav";
    case 11UL: return "Re6.wav";
    case 12UL: return "ReS6.wav";
    case 13UL: return "Mi6.wav";
    case 14UL: return "Fa6.wav";
    case 15UL: return "FaS6.wav"; 
    
    default: return "";
  }
}

void AtualizarNotaFicheiroAtual() {
  NotaFicheiroAtual = ObterNomeNota(NotaDecAtual); // [cite: 66]
  if (DEBUG_NOME) {                               // [cite: 66]
    Serial.print(F("NotaDecAtual="));             // [cite: 67]
    Serial.print(NotaDecAtual);                   // [cite: 67]
    Serial.print(F(" | Ficheiro="));
    Serial.println(NotaFicheiroAtual);
    Serial.println();
    Serial.println();
    delay(2000);
  }
}