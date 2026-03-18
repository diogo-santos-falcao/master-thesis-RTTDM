#include "TD3Parser.h"
#include <iomanip>
#include <iostream>

// --- CONSTRUTOR ---
TD3Parser::TD3Parser(Instance& inst, std::string basePath) 
    : instance_(inst), basePath_(basePath) {}


// --- PONTO DE ENTRADA ---
void TD3Parser::parse(std::string trkFile, std::string mainSchFile) {
    std::cout << ">>> STARTING PARSER TD3 <<<" << std::endl;
    parseTopology(trkFile);
    parseSchedule(mainSchFile);
    std::cout << ">>> FINISHING CONCLUÍDO <<<" << std::endl;
}

// ---------------------------------------------------------
// 1. TOPOLOGIA (.trk)
// ---------------------------------------------------------
void TD3Parser::parseTopology(std::string filename) {
    std::string fullPath = basePath_ + filename;
    std::ifstream file(fullPath);
    
    if (!file.is_open()) {
        std::cerr << "[ERRO] Falha ao abrir TRK: " << fullPath << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '[') continue;

        // Formato TD3: Type, X, Y, ...
        std::vector<std::string> parts = split(line, ',');
        if (parts.size() < 3) continue;

        int type = std::stoi(parts[0]);
        std::string x = parts[1];
        std::string y = parts[2];
        
        // ID único baseado na posição (ex: "res_10_20")
        std::string resID = "res_" + x + "_" + y;

        // Criar o Recurso
        // Nota: O construtor de Resource pode pedir ID. Ajusta se necessário.
        Resource res = Resource(resID, "PT10S", false); // Exemplo com release_time=10 e following_allowed=false

        // Se for Estação (Type 4), guardar nome para o mapa
        if (type == 4 && parts.size() > 4) {
            std::string stationName = parts[4];
            stationToResourceId_[stationName] = resID;
        }

        // ADICIONAR À LISTA (std::list usa push_back)
        instance_.resource.push_back(res);
    }
    std::cout << "[TRK] Topologia carregada: " << instance_.resource.size() << " recursos." << std::endl;
}




// ---------------------------------------------------------
// 2. HORÁRIOS (.sch) -> Preenche instance_.train (VECTOR)
// ---------------------------------------------------------
void TD3Parser::parseSchedule(std::string filename) {
    std::string fullPath = basePath_ + filename;
    std::ifstream file(fullPath);
    
    if (!file.is_open()) {
        file.open(fullPath);
        if(!file.is_open()) {
             std::cerr << "[AVISO] Não consegui abrir: [" << fullPath << "]" << std::endl;
             return;
        }
    }
    std::cout << "[SCH] A processar: " << filename << std::endl;

    std::string line;
    Train currentTrain;
    Route currentRoute;
    bool parsingTrain = false;
    int sequenceCounter = 1;
    int previous_exit_sec = -1;  // <-- ADICIONA ISTO (Guarda a hora de saída da paragem anterior)

    // Lista temporária para a rota física (caminho)
    std::list<route_section*> routeSectionsList; 

    while (std::getline(file, line)) {
        size_t first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos) continue; 
        std::string cleanLine = line.substr(first);

        // --- INCLUDE ---
        if (cleanLine.find("Include:") == 0) {
            std::string subFile = cleanLine.substr(8);
            
            // 1. TRIM ESQUERDA (Remove o espaço depois do ':')
            size_t start = subFile.find_first_not_of(" \t\r");
            if (start != std::string::npos) {
                subFile = subFile.substr(start);
            }

            // 2. TRIM DIREITA (Remove o '\r' do Windows se existir)
            size_t end = subFile.find_last_not_of(" \t\r");
            if (end != std::string::npos) {
                subFile = subFile.substr(0, end + 1);
            }

            std::cout << "   -> A entrar no ficheiro: [" << subFile << "]" << std::endl;
            parseSchedule(subFile); 
        }
        
        // --- NOVO COMBOIO ---
        else if (cleanLine.find("Train:") == 0) {
            if (parsingTrain) {
                // Fechar o comboio anterior
                route_path rp;
                rp.route_sections = routeSectionsList;
                currentRoute.route_paths.push_front(rp);
                instance_.route[currentRoute.id] = currentRoute;
                instance_.train.push_back(currentTrain);
            }

            parsingTrain = true;
            sequenceCounter = 1;
            previous_exit_sec = -1;
            routeSectionsList.clear();

            currentTrain = Train();
            currentTrain.id = cleanLine.substr(6); 
            // Limpar espaços no ID
            currentTrain.id.erase(0, currentTrain.id.find_first_not_of(" \t"));
            currentTrain.id.erase(currentTrain.id.find_last_not_of(" \t\r") + 1);

            currentRoute = Route();
            currentRoute.id = "route_" + currentTrain.id;
            currentTrain.route = currentRoute.id;
        }
        // --- PARAGENS / ENTRADAS ---
        else if (parsingTrain && (cleanLine.find("Enter:") == 0 || isdigit(cleanLine[0]))) {
            std::string content = cleanLine;
            bool isFirstStop = false;

            if (content.find("Enter:") == 0) {
                content = content.substr(6);
                isFirstStop = true;
            }

            std::vector<std::string> parts = split(content, ',');
            std::string entryTimeStr, exitTimeStr, stationName;

            if (isFirstStop && parts.size() >= 2) {
                entryTimeStr = cleanTime(parts[0]);
                exitTimeStr = entryTimeStr; 
                stationName = parts.back();
            }
            else if (parts.size() >= 3) {
                entryTimeStr = cleanTime(parts[0]);      
                std::string rawExit = cleanTime(parts[1]); 
                stationName = parts[2];

                if (rawExit.find("-") != std::string::npos) {
                    exitTimeStr = entryTimeStr; 
                } else {
                    exitTimeStr = rawExit;
                }
            } else {
                continue; 
            }

            // LIMPEZA FINAL DO NOME DA ESTAÇÃO
            stationName.erase(0, stationName.find_first_not_of(" "));
            stationName.erase(stationName.find_last_not_of(" \r\t") + 1);

            std::string fmtEntry = entryTimeStr + ":00";
            std::string fmtExit  = exitTimeStr  + ":00";

            // =========================================================
            // 1. FABRICAR OS LIMITES DE TEMPO ("LATEST") PARA O SOLVER
            // =========================================================
            int h1 = 0, m1 = 0, h2 = 0, m2 = 0;
            // Extrair as horas e minutos das strings lidas
            sscanf(entryTimeStr.c_str(), "%d:%d", &h1, &m1);
            sscanf(exitTimeStr.c_str(), "%d:%d", &h2, &m2);

            // Converter para segundos e somar 1800 segundos (30 minutos de tolerância)
            int entry_latest_sec = (h1 * 3600) + (m1 * 60) + 1800; 
            int exit_latest_sec  = (h2 * 3600) + (m2 * 60) + 1800;

            // Converter novamente para o formato HH:MM:SS (usando % 24 para evitar passar das 24h)
            char buf1[16], buf2[16];
            sprintf(buf1, "%02d:%02d:%02d", (entry_latest_sec / 3600) % 24, (entry_latest_sec / 60) % 60, entry_latest_sec % 60);
            sprintf(buf2, "%02d:%02d:%02d", (exit_latest_sec / 3600) % 24,  (exit_latest_sec / 60) % 60,  exit_latest_sec % 60);

            std::string fmtEntryLatest = buf1;
            std::string fmtExitLatest  = buf2;
            // =========================================================

            route_section* section = new route_section();
            section->sequence_number = sequenceCounter; 
            section->section_marke.push_back(stationName); 
            section->penalty = 0;
            
            // =========================================================
            // CALCULAR TEMPO DE VIAGEM REAL (A "Speed" convertida)
            // =========================================================
            int current_entry_sec = (h1 * 3600) + (m1 * 60); // Hora a que chega
            int current_exit_sec  = (h2 * 3600) + (m2 * 60); // Hora a que sai
            
            int travel_time_sec = 60; // Default (caso seja a primeira estação)
            
            if (previous_exit_sec != -1) {
                // Viagem = Hora que chega aqui - Hora que saiu da estação anterior
                travel_time_sec = current_entry_sec - previous_exit_sec;
                
                // Prevenção de bugs (ex: comboio passa a meia-noite)
                if (travel_time_sec <= 0) {
                    travel_time_sec = 60; 
                }
            }
            
            // Atualizar a memória para a próxima paragem
            previous_exit_sec = current_exit_sec;

            // Injetar o tempo calculado no Solver! (Ex: PT180S)
            section->minimum_running_time = "PT" + std::to_string(travel_time_sec) + "S"; 
            // =========================================================
            
            routeSectionsList.push_back(section);

            std::string reqID = "req_" + currentTrain.id + "_" + std::to_string(sequenceCounter);

            // =========================================================
            // 2. CRIAR O REQUIREMENT COM OS LIMITES E PENALIZAÇÃO (Peso "1")
            // =========================================================
            Requirement* req = new Requirement(
                reqID, stationName, "stop", "PT0S", 
                fmtEntry, "1", fmtExit, fmtEntryLatest, fmtExitLatest
            );
            // Nota: Passámos "1" em vez de "0" no parâmetro do entry_delay_weight!

            currentTrain.t.push_back(req);
            sequenceCounter++;
        }
    }
    // Salvar o último
    if (parsingTrain) {
        route_path rp;
        rp.route_sections = routeSectionsList;
        currentRoute.route_paths.push_front(rp);
        instance_.route[currentRoute.id] = currentRoute;
        instance_.train.push_back(currentTrain);
    }
}

// ---------------------------------------------------------
// 3. A PONTE: MAPEAR DADOS PARA O SOLVER (VERSÃO CORRIGIDA)
// ---------------------------------------------------------
void TD3Parser::buildAuxiliaryStructures() {
    std::cout << ">>> A CONSTRUIR MAPAS DO SOLVER (LINKING)..." << std::endl;

    // Limpar mapas para garantir estado limpo
    instance_.sectionMap.clear();
    instance_.markerMap.clear();
    instance_.end.clear();
    instance_.entryMap.clear();
    instance_.exitMap.clear();

    // Iteramos sobre todos os comboios
    for (auto& train : instance_.train) {
        std::string t_id = train.id;
        std::string route_id = train.route;

        // Tentar encontrar a rota associada a este comboio
        if (instance_.route.find(route_id) == instance_.route.end()) {
            continue; // Se não encontrar a rota, salta
        }
        
        Route& r = instance_.route[route_id];

        // Percorrer os caminhos desta rota
        for (auto& rp : r.route_paths) {
            if (rp.route_sections.empty()) continue;

            // entryMap: Primeira secção do percurso (usa o Train ID)
            instance_.entryMap[t_id].push_back(rp.route_sections.front());

            // exitMap: Última secção do percurso (usa o Train ID)
            instance_.exitMap[t_id].push_back(rp.route_sections.back());

            // Iterar sobre as secções para preencher o resto
            auto it = rp.route_sections.begin();
            while (it != rp.route_sections.end()) {
                route_section* current_sec = *it;
                int seq_num = current_sec->sequence_number;
                
                // 1. sectionMap: "train id -> section id -> section"
                instance_.sectionMap[t_id][seq_num] = current_sec;

                // 2. end map: "train -> end nodes -> sections"
                // Liga a secção atual à PRÓXIMA secção usando ponteiros!
                auto next_it = std::next(it);
                if (next_it != rp.route_sections.end()) {
                    route_section* next_sec = *next_it;
                    // Agora sim, o push_back funciona perfeitamente!
                    instance_.end[t_id][seq_num].push_back(next_sec);
                }

                // 3. markerMap: Associar as paragens (Requirements) do comboio a esta secção
                for (Requirement* req : train.t) {
                    std::string marker = req->section_marker;
                    // Chave padrão: "ComboioID^NomeEstacao"
                    std::string key = t_id + "^" + marker; 

                    // Verifica se o marker (nome da paragem) está na lista de nomes desta secção
                    if (std::find(current_sec->section_marke.begin(), current_sec->section_marke.end(), marker) != current_sec->section_marke.end()) {
                        instance_.markerMap[key].push_back(current_sec);
                    }
                }
                
                it++;
            }
        }
    }

    std::cout << ">>> LINKING CONCLUIDO <<<" << std::endl;
}

// --- HELPERS ---

// Mantém apenas este helper, os outros de tempo já não são precisos 
// porque o Requirement faz o parse sozinho.
std::string TD3Parser::cleanTime(std::string rawTime) {
    // Remove espaços iniciais
    rawTime.erase(0, rawTime.find_first_not_of(" "));
    // Remove o !900/1
    size_t pos = rawTime.find('!');
    if (pos != std::string::npos) {
        return rawTime.substr(0, pos);
    }
    return rawTime;
}

std::vector<std::string> TD3Parser::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        token.erase(0, token.find_first_not_of(" "));
        tokens.push_back(token);
    }
    return tokens;
}

void TD3Parser::debugDump() {
    std::cout << "\n=== RELATÓRIO DO PARSER ===" << std::endl;
    std::cout << "Recursos (Nós): " << instance_.resource.size() << std::endl;
    std::cout << "Comboios: " << instance_.train.size() << std::endl;
    std::cout << "Rotas: " << instance_.route.size() << std::endl;
    if (!instance_.train.empty()) {
        std::cout << "Exemplo: Comboio [" << instance_.train[0].id 
                  << "] usa rota [" << instance_.train[0].route << "]" << std::endl;
    }
    std::cout << "===========================\n" << std::endl;
}