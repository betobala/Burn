# BURN — manual

**betobullet** · 10 HP · gerador de função duplo com VCAs

O Burn tem dois geradores de função (attack/decay, ASR e LFO até a faixa de áudio), cada um com o
seu VCA, saída de fim de ciclo e uma saída de soma. Ele recria o comportamento publicado do
Cosmotronic Delta-V, que é um módulo analógico.

---

## 1. Controles (por canal)

| Controle | Faixa | O que faz |
|---|---|---|
| **ATTACK** | 0,3 ms – ~40 s | Tempo de subida. A dica de mouse mostra o tempo real. |
| **DECAY** | 0,3 ms – ~40 s | Tempo de descida. |
| **SHAPE** | exp · lin · log | Curva dos segmentos. Exponencial começa devagar no ataque e cai rápido no decay; logarítmica faz o contrário. |
| **LFO** | one shot / LFO | No modo LFO, o envelope se repete sozinho e chega a cerca de 1,6 kHz. |
| **LEVEL** | −100 % … +100 % | Atenuverter da saída (±10 V). |
| **LINK** | off / on | Com LINK ligado, o TRIG 1 dispara também o canal 2. |

## 2. Jacks

| Jack | O que faz |
|---|---|
| **TRIG** | Qualquer subida de sinal dispara um envelope attack/decay. |
| **SLEW** | Um gate vira um envelope **ASR**: sobe com o attack, sustenta enquanto o gate fica alto e desce com o decay. Funciona também como slew limiter de qualquer CV. |
| **VCA 1 / VCA 2** | Entrada do VCA. Sem cabo, recebe **5 V**: a saída é o próprio envelope. Com áudio, sai o áudio com o envelope aplicado. Com CV, controla a amplitude do envelope, útil para acentos. |
| **ATK / DEC** | CV dos tempos: +1 V divide o tempo por 2. Com CV negativo, os envelopes podem durar muitos minutos. |
| **OUT 1 / OUT 2** | Saída de cada canal (envelope × VCA × level). |
| **EOC 1 / EOC 2** | Trigger de 0,5 ms no fim de cada ciclo. |
| **SUM** | OUT 1 + OUT 2. |

## 3. Comportamento

- **Retrigger**: durante o **attack**, novos triggers são **ignorados**. Durante o **decay**, um
  trigger reinicia a subida a partir do nível atual. Isso permite divisões de clock e ritmos.
- **LFO**: ao fim do decay o canal se redispara sozinho, e o EOC marca cada ciclo.

## 4. Dicas

- **Bumbo:** TRIG com clock, attack mínimo, decay médio, SHAPE exp e um VCO senoidal em VCA 1.
- **Divisão de clock:** um attack longo ignora os triggers que caem dentro dele.
- **LFO com reset:** modo LFO e um trigger durante o decay ressincroniza.
- **Envelope de filtro com acentos:** uma sequência de CV em VCA.

## 5. Menu de contexto

- **Theme**: Black ou White.
