# Revisão das camadas do Corne — candidato v03

Data: 24/10/2026

Base de comparação: candidato v02 compilado no commit `622cdf0fc1bcc47c66a6d6c1ccd1a015e3bfcb61`.

## Decisão para o polegar esquerdo

- Tecla de camada: toque envia **Espaço**; segurar ativa **NAV**.
- Tecla grande adjacente, antes Espaço: passa a ser **Esc dedicado**.
- `Tab/FN` permanece como está. Usar “segurar Tab = Esc” atrasaria o Esc e eliminaria o acesso direto à camada `NUM_FN`.
- `quick-tap-ms = 175` foi adicionado ao Espaço/NAV para permitir dois espaços rápidos e repetição de Espaço, igual à lógica já usada no Backspace/SYM.

Essa é a única mudança funcional da v03. O restante foi preservado para que o teste físico isole claramente o resultado.

## Como ler o mapa

- `tecla/função`: toque envia a tecla; segurar ativa a função.
- `transparente`: usa a tecla da camada inferior, normalmente a Base. Não é uma tecla perdida.
- `sem ação`: bloqueada intencionalmente. Só existe em posições de segurança da camada SYSTEM.

## L0 — BASE

| Fileira | Mão esquerda | Mão direita |
|---|---|---|
| Superior | Tab/FN · Q · W · E · R · T | Y · U · I · O · P · Backspace |
| Central | Ctrl · A/Shift · S/Ctrl · D/Alt · F/GUI · G | H · J/GUI · K/Alt · L/Ctrl · ;/Shift · ' |
| Inferior | Shift · Z · X · C · V · B | N · M · , · . · / · Shift |
| Polegares | GUI · **Espaço/NAV** · **Esc** | Enter · Backspace/SYM · Alt direito |

Avaliação: completa e coerente. Backspace aparece também no canto superior direito, mas a duplicação é útil durante a adaptação. Os modificadores da fileira central continuam dependendo de teste de digitação rápida.

Combos da Base:

- Shift esquerdo + Shift direito → Caps Lock, janela de 50 ms.
- C + vírgula → `Alt+C` para cedilha no ambiente macOS configurado. No Linux, o resultado depende do layout do sistema.

## L1 — SYM

| Fileira | Mão esquerda | Mão direita |
|---|---|---|
| Superior | Esc · ! · @ · # · $ · % | ^ · & · * · barra invertida · ~ · transparente |
| Central | transparente · { · } · ( · ) · barra vertical | _ · + · : · aspas duplas · apóstrofo · crase |
| Inferior | transparente · [ · ] · < · > · = | ? · / · transparente · transparente · - · transparente |
| Polegares | todos transparentes | todos transparentes |

Avaliação: os pares de símbolos estão agrupados e todos os símbolos essenciais aparecem. As posições transparentes mantêm modificadores, vírgula, ponto, Backspace e polegares da Base; não são lacunas.

## L2 — NAV

| Grupo | Funções |
|---|---|
| Mouse esquerdo | ESDF move; W/R rolam verticalmente; A volta; G clique esquerdo; V clique central; B clique direito |
| Navegação direita | IJKL são setas; U/O rolam horizontalmente; Y/Home e P/End; H/Page Up; apóstrofo/Page Down; ponto e vírgula/Delete |
| Inferior esquerda | Z/X = Ctrl+←/→; C = Insert |
| Inferior direita | N/M = Alt+←/→; vírgula/ponto = GUI+↑/↓; barra = GUI+Z |
| Acesso | T entra em NUMPAD; Esc também existe no canto superior esquerdo |

Avaliação: o bloco ESDF/IJKL está ergonomicamente espelhado e completo. Os comandos com `GUI` e `Alt` são específicos do fluxo macOS; no Linux podem ter outro efeito. Foram preservados para não quebrar o uso no Mac.

## L3 — NUMPAD

| Grupo | Funções |
|---|---|
| Direita | `/ 7 8 9 *` · `- 4 5 6 +` · `0 1 2 3 .` |
| Esquerda superior/central | Tab · PgUp/PgDn · Home/End · setas · Delete · Shift+Tab |
| Esquerda inferior | GUI+Z/X/C/V e `=` |
| Polegares | polegar esquerdo de camada volta à Base; Enter é herdado; Backspace dedicado à direita |

Avaliação: bloco numérico completo, com vírgula e ponto decimal disponíveis. A saída para a Base e o Backspace estão explícitos. GUI+Z/X/C/V é macOS; no Linux, Ctrl seria o modificador usual.

## L4 — SYSTEM

| Mão esquerda | Mão direita |
|---|---|
| Reset 2 s · Bootloader 2 s · USB · Bluetooth · Caps | Reset 2 s · volume −/mute/+ · Bootloader 2 s · voltar à Base |
| Perfis Bluetooth 1–5 | faixa anterior · play/pause · próxima |
| Desbloquear Studio · NUMPAD · capturas macOS · LIVE | brilho −/+ · bloquear macOS |

Avaliação: ações perigosas exigem a camada dupla e reset/bootloader exigem 2 segundos. `BT_CLR` não está exposto, evitando apagar pareamentos. As posições `sem ação` são deliberadas. Captura e bloqueio são atalhos de macOS.

## L5 — LIVE

Camada dedicada ao Ableton Live no macOS: edição, desfazer/refazer, quantizar, duplicar, agrupar, navegação IJKL, Delete, continuar reprodução e comandos de clip. O polegar esquerdo de camada volta à Base; Backspace permanece dedicado. A macro Renomear envia GUI+R e volta à Base para digitar o nome.

Avaliação: internamente consistente, mas precisa ser testada dentro da versão instalada do Ableton. No Linux, os atalhos GUI não são equivalentes aos atalhos macOS.

## L6 — NUM_FN

| Grupo | Funções |
|---|---|
| Superior | números 1–0 |
| Central | Ctrl e F1–F11 |
| Inferior | atalhos macOS/Ableton · F9 · GUI+Shift+F9 · entrada em NUMPAD · GUI+4/5 · Num Lock · F12 |

Avaliação: números e F1–F12 estão completos. O nome `NUM_FN` continua adequado, embora a fileira inferior também carregue atalhos de aplicativo. O acesso por segurar Tab foi preservado.

## Pontos que exigem teste físico

1. Toque rápido no Espaço/NAV deve gerar somente Espaço; segurar e usar IJKL deve entrar em NAV sem inserir Espaço.
2. Dois toques rápidos e toque+segurar no Espaço devem repetir Espaço de modo natural.
3. Esc dedicado deve fechar menus somente quando a tecla grande for pressionada.
4. Backspace/SYM, Enter e Tab/FN devem permanecer iguais à v02.
5. NAV+SYM deve abrir SYSTEM e sair ao soltar uma delas.
6. Entrada e saída de NUMPAD e LIVE não podem deixar a camada presa.
7. Home-row mods, combo de Caps Lock, combo de cedilha e mouse precisam de teste real.
8. Reset e bootloader não devem ser testados durante a digitação comum; a revisão estática apenas confirma a proteção de 2 segundos.

## Resultado da revisão estática

- 7 camadas.
- 42 posições em cada camada; 294/294 bindings presentes.
- Nenhuma alteração fora do comportamento Espaço/NAV e da tecla Esc adjacente.
- Compilação e gravação devem ser registradas separadamente; a revisão estática não substitui o teste no teclado.
