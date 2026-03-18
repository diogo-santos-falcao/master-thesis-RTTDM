#ifndef TD3PARSER_H
#define TD3PARSER_H

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

#include "problem/Instance.h"
#include "problem/Train.h"
#include "problem/Resource.h"
#include "problem/Route.h"
#include "problem/route_path.h"
#include "problem/route_section.h"

class TD3Parser {
private:
    /// --- LIGAÇÃO À MEMÓRIA DO SOLVER ---
    Instance& instance_;

    // Caminho base para encontrar os ficheiros (ex: "data/LierHerentals/")
    std::string basePath_;

    // Mapa auxiliar (TD3 -> Solver)
    std::map<std::string, std::string> stationToResourceId_;

public:
    TD3Parser(Instance& instance, const std::string basePath);

    void parse(std::string trkFile, std::string mainSchFile);
    void buildAuxiliaryStructures();
    void debugDump();

private:
    void parseTopology(std::string filename);
    void parseSchedule(std::string filename);
   
    std::string cleanTime(std::string rawTime);

    std::vector<std::string> split(const std::string& str, char delimiter);
};

#endif // TD3PARSER_H