#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <queue>
#include <climits>
#include <algorithm>

using namespace std;

enum Estrategia {
    FIRST_FIT = 1,
    BEST_FIT  = 2,
    WORST_FIT = 3
};

enum AlgoritmoSubstituicao {
    FIFO = 1,
    LRU = 2,
    OTIMO = 3
};

struct Processo {
    string nome;
    int tamanho;
};

struct Segmento {
    int inicio;
    int tamanho;
    bool livre;
    bool fragmentacao_interna;
    bool coalescido;
    string processo;
};

namespace {
    vector<Processo> g_processos_carregados;
    vector<Segmento> g_memoria;
    int  g_tamanho_memoria_carregado = 0;
    bool g_simulacao_contigua_realizada = false;
    int g_ultima_simulacao = 0; 
    int g_page_faults = 0;      
    double g_fault_rate = 0.0;  
    double g_hit_rate = 0.0;   
}

vector<Processo> ler_processos() {
    int n;
    while (true) {
        cout << "Quantos processos deseja inserir? ";
        cin >> n;
        if (cin.fail() || n < 0) {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "Valor invalido!\n";
            continue;
        }
        break;
    }

    vector<Processo> lista;
    for (int i = 0; i < n; i++) {
        Processo p;
        cout << "Nome do processo " << (i + 1) << ": ";
        cin >> p.nome;
        while (true) {
            cout << "Tamanho (KB): ";
            cin >> p.tamanho;
            if (cin.fail() || p.tamanho <= 0) {
                cin.clear();
                cin.ignore(1000, '\n');
                cout << "Valor invalido!\n";
                continue;
            }
            break;
        }
        lista.push_back(p);
    }
    return lista;
}

Estrategia escolher_estrategia() {
    int op;
    while (true) {
        cout << "\nEscolha a estrategia de alocacao:\n";
        cout << "1 - First Fit\n";
        cout << "2 - Best Fit\n";
        cout << "3 - Worst Fit\n";
        cout << "Opcao: ";
        cin >> op;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "Opcao invalida!\n";
            continue;
        }
        if (op >= 1 && op <= 3) return (Estrategia)op;
        cout << "Opcao invalida!\n";
    }
}

int escolher_segmento(const vector<Segmento>& memoria, int tamanho, Estrategia estrategia) {
    int indice_escolhido = -1;
    int melhor_sobra = 0;

    for (size_t i = 0; i < memoria.size(); i++) {
        const auto& bloco = memoria[i];
        if (!bloco.livre || bloco.tamanho < tamanho) continue;

        int sobra = bloco.tamanho - tamanho;

        if (estrategia == FIRST_FIT) return (int)i;

        if (indice_escolhido == -1) {
            indice_escolhido = (int)i;
            melhor_sobra = sobra;
            continue;
        }

        if (estrategia == BEST_FIT) {
            if (sobra < melhor_sobra || (sobra == melhor_sobra && bloco.inicio < memoria[indice_escolhido].inicio)) {
                indice_escolhido = (int)i;
                melhor_sobra = sobra;
            }
        } else if (estrategia == WORST_FIT) {
            if (sobra > melhor_sobra || (sobra == melhor_sobra && bloco.inicio < memoria[indice_escolhido].inicio)) {
                indice_escolhido = (int)i;
                melhor_sobra = sobra;
            }
        }
    }
    return indice_escolhido;
}

bool existe_buracos_adjacentes(const vector<Segmento>& memoria) {
    for (size_t i = 0; i + 1 < memoria.size(); i++) {
        if (memoria[i].livre && memoria[i+1].livre) return true;
    }
    return false;
}

bool coalescer_buracos(vector<Segmento>& memoria) {
    vector<Segmento> nova;
    nova.reserve(memoria.size());
    bool houve_coalescencia_global = false;

    for (size_t i = 0; i < memoria.size(); ) {
        if (!memoria[i].livre) {
            nova.push_back(memoria[i]);
            i++;
            continue;
        }
        int inicio = memoria[i].inicio;
        int soma = memoria[i].tamanho;
        bool coalesceu = false;
        size_t j = i + 1;
        while (j < memoria.size() && memoria[j].livre) {
            soma += memoria[j].tamanho;
            coalesceu = true;
            j++;
        }
        Segmento merged;
        merged.inicio = inicio;
        merged.tamanho = soma;
        merged.livre = true;
        merged.fragmentacao_interna = false;
        merged.coalescido = coalesceu;
        merged.processo = "";
        nova.push_back(merged);
        if (coalesceu) houve_coalescencia_global = true;
        i = j;
    }
    memoria.swap(nova);
    return houve_coalescencia_global;
}

void compactar_memoria() {
    if (g_memoria.empty()) {
        cout << "Memoria ainda nao configurada.\n";
        return;
    }
    vector<Segmento> nova;
    nova.reserve(g_memoria.size());
    int endereco = 0;
    for (const auto& b : g_memoria) {
        if (!b.livre) {
            Segmento x;
            x.inicio = endereco;
            x.tamanho = b.tamanho;
            x.livre = false;
            x.fragmentacao_interna = false;
            x.coalescido = false;
            x.processo = b.processo;
            nova.push_back(x);
            endereco += b.tamanho;
        }
    }
    int livre_total = 0;
    for (const auto& b : g_memoria) if (b.livre) livre_total += b.tamanho;
    if (livre_total > 0) {
        Segmento livre;
        livre.inicio = endereco;
        livre.tamanho = livre_total;
        livre.livre = true;
        livre.fragmentacao_interna = false;
        livre.coalescido = true;
        livre.processo = "";
        nova.push_back(livre);
    }
    g_memoria.swap(nova);
    cout << "[OK] Compactacao executada.\n";
}

void imprimir_mapa_e_estatisticas(const vector<Segmento>& memoria, bool mostrar_estatisticas) {
    if (g_ultima_simulacao == 1) { 
        cout << "\n===== Mapa da Memoria =====\n";
        for (const auto& bloco : memoria) {
            if (bloco.livre) {
                cout << "Endereco " << setw(5) << bloco.inicio << " | "
                     << setw(6) << bloco.tamanho << " KB | LIVRE";
                if (bloco.fragmentacao_interna) cout << " (FRAGMENTACAO INTERNA)";
                if (bloco.coalescido)          cout << " (COALESCIDO)";
                cout << "\n";
            } else {
                cout << "Endereco " << setw(5) << bloco.inicio << " | "
                     << setw(6) << bloco.tamanho << " KB | Processo: " << bloco.processo << "\n";
            }
        }
        cout << "===========================\n";

        if (mostrar_estatisticas) {
            int buracos = 0, soma_buracos = 0, frag_interna = 0;
            for (const auto& b : memoria) {
                if (b.livre) {
                    buracos++;
                    soma_buracos += b.tamanho;
                    if (b.fragmentacao_interna) frag_interna += b.tamanho;
                }
            }
            cout << "Estatisticas:\n";
            cout << " - Buracos livres: " << buracos << " | Total livre: " << soma_buracos << " KB\n";
            cout << " - Fragmentacao interna (somas marcadas): " << frag_interna << " KB\n\n";
        }
    } else if (g_ultima_simulacao == 2) {
        cout << "\n===== Metricas da Ultima Simulacao de Paginacao =====\n";
        cout << "Total de page faults: " << g_page_faults << "\n";
        cout << "Taxa de page faults: " << fixed << setprecision(2) << g_fault_rate << "%\n";
        cout << "Taxa de acertos: " << fixed << setprecision(2) << g_hit_rate << "%\n";
    } else {
        cout << "Nenhuma simulação realizada ainda.\n";
    }
}

bool liberar_processo_por_nome(const string& nome) {
    bool achou = false;
    for (auto& b : g_memoria) {
        if (!b.livre && b.processo == nome) {
            b.livre = true;
            b.processo = "";
            b.fragmentacao_interna = false;
            b.coalescido = false;
            achou = true;
        }
    }
    if (!achou) return false;
    coalescer_buracos(g_memoria);
    cout << "[OK] Processo \"" << nome << "\" liberado e buracos coalescidos (se possivel).\n";
    return true;
}

auto esperar_passo = []() {
    while (true) {
        cout << "Digite 1 para proximo passo ou 0 para interromper: ";
        cout.flush();
        string entrada;
        cin >> entrada;
        cout << "\x1b[1A" << "\r" << "\x1b[2K";
        cout.flush();
        if (entrada == "0") return 0;
        if (entrada == "1") return 1;
        cout << "Entrada invalida! Digite 1 ou 0.\n";
    }
};

void simular_alocacao_contigua() {
    g_memoria.clear();
    int q;
    while (true) {
        cout << "Quantos espacos livres (particoes) a memoria tem inicialmente? ";
        cin >> q;
        if (cin.fail() || q < 0) {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "Valor invalido!\n";
            continue;
        }
        break;
    }
    int endereco_atual = 0;
    for (int i = 0; i < q; i++) {
        int tam;
        while (true) {
            cout << "Tamanho do espaco livre " << (i + 1) << " (KB): ";
            cin >> tam;
            if (cin.fail() || tam <= 0) {
                cin.clear();
                cin.ignore(1000, '\n');
                cout << "Valor invalido!\n";
                continue;
            }
            break;
        }
        g_memoria.push_back({endereco_atual, tam, true, false, false, ""});
        endereco_atual += tam;
    }

    vector<Processo> processos = ler_processos();
    Estrategia estrategia = escolher_estrategia();

    g_processos_carregados = processos;
    g_tamanho_memoria_carregado = endereco_atual;
    g_simulacao_contigua_realizada = true;
    g_ultima_simulacao = 1; 

    cout << "\n===== Iniciando simulacao de alocacao contigua =====\n";

    for (const auto& processo : processos) {
        int passo = esperar_passo();
        if (passo == 0) {
            cout << "\nSimulacao interrompida pelo usuario.\n";
            imprimir_mapa_e_estatisticas(g_memoria, true);
            return;
        }

        int indice = escolher_segmento(g_memoria, processo.tamanho, estrategia);

        if (indice == -1) {
            if (existe_buracos_adjacentes(g_memoria)) {
                cout << "[INFO] Coalescendo buracos livres adjacentes para tentar alocar "
                     << processo.nome << " (" << processo.tamanho << " KB)...\n";
                coalescer_buracos(g_memoria);
                indice = escolher_segmento(g_memoria, processo.tamanho, estrategia);
                if (indice == -1) {
                    cout << "[FALHA] Ainda nao foi possivel alocar " << processo.nome
                         << ". Considere: compactar memoria, liberar processos, mudar estrategia, reordenar carga, paginacao/swapping, aumentar memoria.\n";
                    continue;
                }
            } else {
                cout << "[FALHA] Processo " << processo.nome << " (" << processo.tamanho
                     << " KB) nao cabe em nenhum espaco livre.\n";
                cout << "Sugestoes: compactar; coalescer; liberar processos; mudar estrategia; reordenar; paginacao/swapping; aumentar memoria.\n";
                continue;
            }
        }

        auto& bloco = g_memoria[indice];
        int sobra = bloco.tamanho - processo.tamanho;

        cout << "[OK] Alocando processo " << processo.nome << " (" << processo.tamanho
             << " KB) no endereco " << bloco.inicio << " usando "
             << (estrategia == FIRST_FIT ? "First Fit" : (estrategia == BEST_FIT ? "Best Fit" : "Worst Fit"))
             << ".\n";

        bloco.livre = false;
        bloco.processo = processo.nome;
        bloco.tamanho = processo.tamanho;

        if (sobra > 0) {
            Segmento novo_bloco{ bloco.inicio + processo.tamanho, sobra, true, true, false, "" };
            g_memoria.insert(g_memoria.begin() + (indice + 1), novo_bloco);
        }
    }

    imprimir_mapa_e_estatisticas(g_memoria, true);
}

vector<int> ler_requisicoes() {
    int n;
    while (true) {
        cout << "Quantas requisicoes de paginas deseja inserir? ";
        cin >> n;
        if (cin.fail() || n < 0) {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "Valor invalido!\n";
            continue;
        }
        break;
    }

    vector<int> lista(n);
    for (int i = 0; i < n; i++) {
        while (true) {
            cout << "Requisicao de pagina " << (i + 1) << ": ";
            cin >> lista[i];
            if (cin.fail() || lista[i] < 0) {
                cin.clear();
                cin.ignore(1000, '\n');
                cout << "Valor invalido!\n";
                continue;
            }
            break;
        }
    }
    return lista;
}

AlgoritmoSubstituicao escolher_algoritmo_substituicao() {
    int op;
    while (true) {
        cout << "\nEscolha o algoritmo de substituicao:\n";
        cout << "1 - FIFO\n";
        cout << "2 - LRU\n";
        cout << "3 - Otimo\n";
        cout << "Opcao: ";
        cin >> op;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "Opcao invalida!\n";
            continue;
        }
        if (op >= 1 && op <= 3) return (AlgoritmoSubstituicao)op;
        cout << "Opcao invalida!\n";
    }
}

void simular_paginacao() {
    int num_frames;
    while (true) {
        cout << "Numero de frames na memoria: ";
        cin >> num_frames;
        if (cin.fail() || num_frames <= 0) {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "Valor invalido!\n";
            continue;
        }
        break;
    }

    vector<int> requisicoes = ler_requisicoes();
    AlgoritmoSubstituicao algoritmo = escolher_algoritmo_substituicao();

    cout << "\n===== Iniciando simulacao de paginacao =====\n";

    string seq_str;
    for (size_t i = 0; i < requisicoes.size(); ++i) {
        seq_str += to_string(requisicoes[i]);
        if (i < requisicoes.size() - 1) seq_str += ", ";
    }

    cout << "Numero de frames: " << num_frames << endl;
    cout << "Sequencia de requisicoes: " << seq_str << endl;
    cout << "Algoritmo: " << (algoritmo == FIFO ? "FIFO" : (algoritmo == LRU ? "LRU" : "Otimo")) << endl;


    cout << left << setw(6) << "Req";
    for (int i = 0; i < num_frames; ++i) {
        cout << setw(10) << ("F" + to_string(i));
    }
    cout << setw(22) << "Evento" << "Page Faults" << endl;
    cout << string(6 + num_frames * 10 + 22 + 11, '-') << endl;

    const string RED = "\033[31m";
    const string GREEN = "\033[32m";
    const string RESET = "\033[0m";
    const int INF = 999999;

    auto format_frame = [](int val, bool is_victim) -> string {
        if (is_victim) {
            return "[" + to_string(val) + "](X)";
        } else if (val == -1) {
            return "[ ]";
        } else {
            return "[" + to_string(val) + "]";
        }
    };

    vector<int> frames(num_frames, -1);
    vector<int> use_time(num_frames, 0);
    queue<int> fifo_queue;
    int tempo = 0;
    int page_faults = 0;

    for (size_t pos = 0; pos < requisicoes.size(); ++pos) {
        int req = requisicoes[pos];

        int passo = esperar_passo();
        if (passo == 0) {
            cout << "\nSimulacao interrompida pelo usuario.\n";
            break;
        }

        ++tempo;
        bool fault = true;
        int hit_idx = -1;
        int empty_idx = -1;

        for (int i = 0; i < num_frames; ++i) {
            if (frames[i] == req) {
                fault = false;
                hit_idx = i;
                break;
            }
            if (frames[i] == -1 && empty_idx == -1) empty_idx = i;
        }

        if (!fault) {
            if (algoritmo == LRU) use_time[hit_idx] = tempo;
            string event = "Hit " + to_string(req);
            cout << left << setw(6) << req;
            for (int i = 0; i < num_frames; ++i) {
                string content = format_frame(frames[i], false);
                string color = (i == hit_idx ? GREEN : "");
                if (!color.empty()) cout << color;
                cout << left << setw(10) << content;
                if (!color.empty()) cout << RESET;
            }
            cout << setw(22) << event << page_faults << endl;
            continue;
        }


        int victim_idx = empty_idx;
        bool is_replace = (empty_idx == -1);

        if (is_replace) {
            if (algoritmo == FIFO) {
                victim_idx = fifo_queue.front();
            } else if (algoritmo == LRU) {
                int min_t = INT_MAX;
                for (int i = 0; i < num_frames; ++i) {
                    if (use_time[i] < min_t) {
                        min_t = use_time[i];
                        victim_idx = i;
                    }
                }
            } else if (algoritmo == OTIMO) {
                int max_dist = -1;
                for (int i = 0; i < num_frames; ++i) {
                    int dist = INF;
                    for (size_t j = pos + 1; j < requisicoes.size(); ++j) {
                        if (requisicoes[j] == frames[i]) {
                            dist = (int)(j - pos);
                            break;
                        }
                    }
                    if (dist > max_dist) {
                        max_dist = dist;
                        victim_idx = i;
                    }
                }
            }
        }

        if (is_replace) {
            string event1 = "Substitui " + to_string(frames[victim_idx]) + "->" + to_string(req);
            cout << left << setw(6) << req;
            for (int i = 0; i < num_frames; ++i) {
                string content = format_frame(frames[i], i == victim_idx);
                string color = (i == victim_idx ? RED : "");
                if (!color.empty()) cout << color;
                cout << left << setw(10) << content;
                if (!color.empty()) cout << RESET;
            }
            cout << setw(22) << event1 << page_faults << endl;

            if (algoritmo == FIFO) fifo_queue.pop();
        }


        frames[victim_idx] = req;
        if (algoritmo == FIFO) fifo_queue.push(victim_idx);
        if (algoritmo == LRU) use_time[victim_idx] = tempo;

        ++page_faults;

 
        string event2 = is_replace ? "Apos troca" : "Carrega " + to_string(req);
        cout << left << setw(6) << req;
        for (int i = 0; i < num_frames; ++i) {
            string content = format_frame(frames[i], false);
            string color = (i == victim_idx ? GREEN : "");
            if (!color.empty()) cout << color;
            cout << left << setw(10) << content;
            if (!color.empty()) cout << RESET;
        }
        cout << setw(22) << event2 << page_faults << endl;
    }

    cout << "\nRepresentacao ASCII (frames) :\n";
    for (int i = 0; i < num_frames; ++i) {
        cout << "[F" << i << "] ";
    }
    cout << endl;
    for (int i = 0; i < num_frames; ++i) {
        cout << (frames[i] == -1 ? "[ ]" : "[" + to_string(frames[i]) + "]") << " ";
    }
    cout << endl;

    if (!requisicoes.empty()) {
        double fault_rate = static_cast<double>(page_faults) / requisicoes.size() * 100.0;
        double hit_rate = 100.0 - fault_rate;
        cout << "\n===== Metricas Finais =====\n";
        cout << "Total de page faults: " << page_faults << "\n";
        cout << "Taxa de page faults: " << fixed << setprecision(2) << fault_rate << "%\n";
        cout << "Taxa de acertos: " << fixed << setprecision(2) << hit_rate << "%\n";

  
        g_page_faults = page_faults;
        g_fault_rate = fault_rate;
        g_hit_rate = hit_rate;
        g_ultima_simulacao = 2;
    }
}

void resetar_alocacao_contigua() {
    g_processos_carregados.clear();
    g_memoria.clear();
    g_tamanho_memoria_carregado = 0;
    g_simulacao_contigua_realizada = false;
    g_ultima_simulacao = 0; 
    g_page_faults = 0;
    g_fault_rate = 0.0;
    g_hit_rate = 0.0;
    cout << "Simulacao resetada com sucesso.\n\n";
}

void apresentacao_menu() {
    cout << "============================================================" << endl;
    cout << "\tSIMULADOR DE SISTEMAS OPERACIONAIS II (ASCII)\n"
            "\tPor: Lucian Fernando Bellini e Frederico Linck Poerschke | Matricula: 192558 / 189234" << endl;
    cout << "============================================================" << endl;

    cout << "\nMenu Principal\n------------------------------" << endl;
    cout << "1 - Alocacao Contigua (simular)\n";
    cout << "2 - Paginacao (Substituicao de Paginas)\n";
    cout << "3 - Resetar Simulacao\n";
    cout << "4 - Liberar processo por nome (com coalescencia)\n";
    cout << "5 - Compactar memoria\n";
    cout << "6 - Mostrar mapa/estatisticas\n";
    cout << "0 - Sair" << endl;
    cout << "------------------------------" << endl;
    cout << "> Escolha uma opcao: ";
}

int main() {
    int opcao;

    while (true) {
        apresentacao_menu();
        cin >> opcao;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "\nOpcao invalida! Digite apenas numeros.\n\n";
            continue;
        }

        switch (opcao) {
            case 0:
                cout << "\nSaindo do simulador...\n";
                return 0;

            case 1:
                cout << "\n[Alocacao Contigua]\n";
                simular_alocacao_contigua();
                break;

            case 2:
                cout << "\n[Paginacao / Substituicao de Paginas]\n";
                simular_paginacao();
                break;

            case 3:
                cout << "\n[Resetar Simulacao]\n";
                resetar_alocacao_contigua();
                break;

            case 4: {
                if (g_memoria.empty()) { cout << "Nenhuma simulacao carregada.\n"; break; }
                string nome;
                cout << "Nome do processo a liberar: ";
                cin >> nome;
                if (!liberar_processo_por_nome(nome)) {
                    cout << "Processo nao encontrado.\n";
                }
                break;
            }

            case 5:
                if (g_memoria.empty()) { cout << "Nenhuma simulacao carregada.\n"; break; }
                compactar_memoria();
                break;

            case 6:
                if (g_memoria.empty() && g_ultima_simulacao == 0) { cout << "Nenhuma simulacao carregada.\n"; break; }
                imprimir_mapa_e_estatisticas(g_memoria, true);
                break;

            default:
                cout << "\nOpcao invalida! Escolha um valor entre 0 e 6.\n\n";
                break;
        }
    }
}