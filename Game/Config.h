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

    // ������ ���������� ��������� ������������
    void reload()
    {
        std::ifstream fin(project_path + "settings.json");
        fin >> config;
        fin.close();
    }

    // �������� ��������� ������ ����������� ��� �������. ����: config("WindowSize", "Width")
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
        "Width": 0,  // ������ ����
        "Hight": 0   // ������ ����
    },
    "Bot": {
        "IsWhiteBot": false,  // ����������� �� ����� ����� ����� 
        "IsBlackBot": true,   // ����������� �� ������ ����� �����
        "WhiteBotLevel": 0,   // ������� ��������� ���� ��� ����� 
        "BlackBotLevel": 5,   // ������� ��������� ���� ��� ������ 
        "BotScoringType": "NumberAndPotential",  // ��� ������ �����: "NumberAndPotential" ��� ������
        "BotDelayMS": 0,      // �������� ���� ���� � ������������� 
        "NoRandom": false,    // ���������� ����������� � ������ ����� ���� 
        "Optimization": "O1"  // ������� ����������� ���������: "O0" (��� �����������), "O1" 
    },
    "Game": {
        "MaxNumTurns": 120  // ������������ ���������� ����� � ���� (��� �������������� ����������� ������)
    }
}

*/