#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "../Models/Project_path.h"

class Config
{
  public:
    Config()
    {
        reload();
    }

    // чтение актуальной локальной конфигруации
    void reload()
    {
        std::ifstream fin(project_path + "settings.json");
        fin >> config;
        fin.close();
    }

    // оператор позвол€ет классу действовать как функци€. прим: config("WindowSize", "Width")
    auto operator()(const string &setting_dir, const string &setting_name) const
    {
        return config[setting_dir][setting_name];
    }

  private:
    json config;
};


/*

{
    "WindowSize": {
        "Width": 0,  // ширина окна
        "Hight": 0   // высота окна
    },
    "Bot": {
        "IsWhiteBot": false,  // управл€ютс€ ли белые шашки ботом 
        "IsBlackBot": true,   // управл€ютс€ ли чЄрные шашки ботом
        "WhiteBotLevel": 0,   // уровень сложности бота дл€ белых 
        "BlackBotLevel": 5,   // уровень сложности бота дл€ чЄрных 
        "BotScoringType": "NumberAndPotential",  // “ип оценки ходов: "NumberAndPotential" или другой
        "BotDelayMS": 0,      // задержка хода бота в миллисекундах 
        "NoRandom": false,    // отключение случайности в выборе ходов бота 
        "Optimization": "O1"  // уровень оптимизации алгоритма: "O0" (без оптимизации), "O1" 
    },
    "Game": {
        "MaxNumTurns": 120  // максимальное количество ходов в игре (дл€ предотвращени€ бесконечных партий)
    }
}

*/