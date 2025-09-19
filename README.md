# Simulador_Memoria📌 Simulador de Sistemas Operacionais II (ASCII)

Este projeto implementa um simulador didático de alocação de memória e substituição de páginas, com interface em terminal (ASCII/cores ANSI).
Funcionalidades principais:

Alocação Contígua de Memória (First Fit, Best Fit, Worst Fit), com suporte a coalescência e compactação.

Paginação com substituição de páginas usando FIFO, LRU e Ótimo.

Exibição de tabelas coloridas no terminal (PowerShell/VSCode).

Métricas de page faults e hit rate ao final das simulações.

⚙️ Requisitos

Linguagem: C++17 ou superior

Compilador: g++ (MinGW recomendado no Windows, ou GCC/Clang no Linux/macOS)

Terminal: compatível com códigos ANSI (PowerShell, Git Bash, Linux terminal, VSCode integrado).

🚀 Como Compilar e Executar
Windows (PowerShell ou VSCode Terminal)
g++ main.cpp -o main
./main

Linux/macOS (bash/zsh)
g++ -std=c++17 -O2 main.cpp -o main
./main

🛠️ Decisões Arquiteturais

Único arquivo fonte (main.cpp): facilita compilação e entrega acadêmica.

Estado global controlado: variáveis globais para armazenar a simulação corrente.

Estruturas de dados:

vector<Segmento> para gerenciar memória contígua.

queue<int> para FIFO.

Vetores de timestamps para LRU.

Busca futura na sequência para o Ótimo.

Interface ASCII com cores ANSI: verde para acertos, vermelho para substituições, tornando a execução mais visual.

📖 Exemplos de Uso
1) Alocação Contígua

Entrada:

1   # Seleciona Alocação Contígua
2   # Quantos espaços livres
100
200
2   # Quantos processos
P1
50
P2
120
1   # Estratégia: First Fit


Saída:

[OK] Alocando processo P1 (50 KB) no endereço 0 usando First Fit.
[OK] Alocando processo P2 (120 KB) no endereço 50 usando First Fit.

===== Mapa da Memoria =====
Endereco     0 |     50 KB | Processo: P1
Endereco    50 |    120 KB | Processo: P2
Endereco   170 |    130 KB | LIVRE
===========================

2) Paginação (FIFO)

Entrada:

2   # Seleciona Paginação
3   # Número de frames
9   # Número de requisições
1 2 3 4 1 2 5 1 2
1   # Algoritmo FIFO


Saída:

Req   F0        F1        F2        Evento                Page Faults
----------------------------------------------------------------------
1     [1]       [ ]       [ ]       Carrega 1             1
2     [1]       [2]       [ ]       Carrega 2             2
3     [1]       [2]       [3]       Carrega 3             3
4     [4]       [2]       [3]       Substitui 1->4        4
...

===== Metricas Finais =====
Total de page faults: 7
Taxa de page faults: 77.78%
Taxa de acertos: 22.22%

👨‍💻 Autores

Lucian Fernando Bellini – Matrícula 192558

Frederico Linck Poerschke – Matrícula 189234