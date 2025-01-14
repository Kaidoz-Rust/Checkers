#pragma once
#include <chrono>
#include <thread>

#include "../Models/Project_path.h"
#include "Board.h"
#include "Config.h"
#include "Hand.h"
#include "Logic.h"

class Game
{
  public:
    Game() : board(config("WindowSize", "Width"), config("WindowSize", "Hight")), hand(&board), logic(&board, &config)
    {
        ofstream fout(project_path + "log.txt", ios_base::trunc);
        fout.close();
    }

    // to start checkers
    int play()
    {
        // засекаем время начала игры
        auto start = chrono::steady_clock::now();

        // если это повтор игры (переигровка), перезагружаем логику, конфигурацию и перерисовываем доску
        if (is_replay)
        {
            logic = Logic(&board, &config);
            config.reload();
            board.redraw();
        }
        else
        {
            // иначе начинаем новую игру с начальной отрисовкой доски
            board.start_draw();
        }
        is_replay = false; // сбрасываем флаг переигровки

        int turn_num = -1; // номер хода, начинаем с -1, так как в цикле сразу увеличиваем
        bool is_quit = false; // флаг для выхода из игры
        const int Max_turns = config("Game", "MaxNumTurns"); // максимальное количество ходов из конфигурации

        // основной цикл игры, продолжаем, пока не достигнем максимального числа ходов
        while (++turn_num < Max_turns)
        {
            beat_series = 0; // сбрасываем счётчик серии взятий. например: при последовательных взятиях шашек

            // находим возможные ходы для текущего игрока: 0 - белые, 1 - черные
            logic.find_turns(turn_num % 2);

            // если ходов нет, завершаем игру
            if (logic.turns.empty())
                break;

            // устанавливаем уровень сложности бота для текущего игрока
            logic.Max_depth = config("Bot", string((turn_num % 2) ? "Black" : "White") + string("BotLevel"));

            // если текущий игрок - человек, обрабатываем его ход
            if (!config("Bot", string("Is") + string((turn_num % 2) ? "Black" : "White") + string("Bot")))
            {
                auto resp = player_turn(turn_num % 2); // получаем ответ от игрока

                // обрабатываем возможные ответы от игрока
                if (resp == Response::QUIT)
                {
                    is_quit = true; // игрок решил выйти из игры
                    break;
                }
                else if (resp == Response::REPLAY)
                {
                    is_replay = true; // игрок решил переиграть
                    break;
                }
                else if (resp == Response::BACK)
                {
                    // если игрок решил отменить ход и предыдущий ход был сделан ботом, откатываем ход
                    if (config("Bot", string("Is") + string((1 - turn_num % 2) ? "Black" : "White") + string("Bot")) &&
                        !beat_series && board.history_mtx.size() > 2)
                    {
                        board.rollback();
                        --turn_num;
                    }
                    if (!beat_series)
                        --turn_num;

                    // откатываем ход и уменьшаем счётчик ходов
                    board.rollback();
                    --turn_num;
                    beat_series = 0; // сбрасываем счётчик серии взятий
                }
            }
            else
            {
                // если текущий игрок - бот, выполняем его ход
                bot_turn(turn_num % 2);
            }
        }

        // засекаем время окончания игры
        auto end = chrono::steady_clock::now();

        // записываем время игры в лог-файл
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Game time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();

        // если был запрошен переигровка, запускаем игру заново
        if (is_replay)
            return play();

        // исли игрок решил выйти, возвращаем 0
        if (is_quit)
            return 0;

        // определяем результат игры
        int res = 2; // По умолчанию результат - ничья
        if (turn_num == Max_turns)
        {
            res = 0; // ничья, если достигнуто максимальное количество ходов
        }
        else if (turn_num % 2)
        {
            res = 1; // победа черных, если последний ход был сделан черными
        }

        // показываем финальный экран с результатом игры
        board.show_final(res);

        // ожидаем ответа от игрока. например: переигровка или выход
        auto resp = hand.wait();
        if (resp == Response::REPLAY)
        {
            is_replay = true; // игрок решил переиграть
            return play(); // запускаем игру заново
        }

        // возвращаем результат игры
        return res;
    }

  private:
    void bot_turn(const bool color)
    {
        auto start = chrono::steady_clock::now();

        auto delay_ms = config("Bot", "BotDelayMS");
        // new thread for equal delay for each turn
        thread th(SDL_Delay, delay_ms);
        auto turns = logic.find_best_turns(color);
        th.join();
        bool is_first = true;
        // making moves
        for (auto turn : turns)
        {
            if (!is_first)
            {
                SDL_Delay(delay_ms);
            }
            is_first = false;
            beat_series += (turn.xb != -1);
            board.move_piece(turn, beat_series);
        }

        auto end = chrono::steady_clock::now();
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Bot turn time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();
    }

    Response player_turn(const bool color)
    {
        // вектор для хранения координат клеток, которые можно выбрать для хода
        vector<pair<POS_T, POS_T>> cells;

        // заполняем вектор cells координатами всех возможных ходов для текущего игрока
        for (auto turn : logic.turns)
        {
            cells.emplace_back(turn.x, turn.y);
        }

        // подсвечиваем на доске клетки, которые можно выбрать для хода
        board.highlight_cells(cells);

        // структура для хранения информации о выбранном ходе
        move_pos pos = { -1, -1, -1, -1 };

        // переменные для хранения координат выбранной клетки
        POS_T x = -1, y = -1;

        // основной цикл обработки хода игрока
        while (true)
        {
            // получаем ответ от игрока (например, клик по клетке или действие)
            auto resp = hand.get_cell();

            // если ответ не связан с выбором клетки (например, выход или отмена), возвращаем его
            if (get<0>(resp) != Response::CELL)
                return get<0>(resp);

            // получаем координаты выбранной клетки
            pair<POS_T, POS_T> cell{ get<1>(resp), get<2>(resp) };

            // проверяем, является ли выбранная клетка допустимой для хода
            bool is_correct = false;
            for (auto turn : logic.turns)
            {
                // если выбранная клетка совпадает с начальной клеткой хода
                if (turn.x == cell.first && turn.y == cell.second)
                {
                    is_correct = true;
                    break;
                }
                // если выбранная клетка совпадает с конечной клеткой хода
                if (turn == move_pos{ x, y, cell.first, cell.second })
                {
                    pos = turn; // запоминаем выбранный ход
                    break;
                }
            }

            // если ход выбран (pos.x != -1), выходим из цикла
            if (pos.x != -1)
                break;

            // если выбранная клетка недопустима для хода
            if (!is_correct)
            {
                // если ранее уже была выбрана клетка, сбрасываем выделение и подсветку
                if (x != -1)
                {
                    board.clear_active();
                    board.clear_highlight();
                    board.highlight_cells(cells); // подсвечиваем допустимые клетки заново
                }
                x = -1; // сбрасываем координаты выбранной клетки
                y = -1;
                continue; // продолжаем цикл, ожидая корректного выбора
            }

            // если клетка допустима, запоминаем её координаты
            x = cell.first;
            y = cell.second;

            // очищаем предыдущую подсветку и выделяем активную клетку
            board.clear_highlight();
            board.set_active(x, y);

            // вектор для хранения координат клеток, куда можно сделать ход из выбранной клетки
            vector<pair<POS_T, POS_T>> cells2;
            for (auto turn : logic.turns)
            {
                if (turn.x == x && turn.y == y)
                {
                    cells2.emplace_back(turn.x2, turn.y2); // запоминаем конечные клетки хода
                }
            }

            // подсвечиваем клетки, куда можно сделать ход из выбранной клетки
            board.highlight_cells(cells2);
        }

        // очищаем подсветку и выделение после завершения выбора хода
        board.clear_highlight();
        board.clear_active();

        // выполняем выбранный ход на доске
        board.move_piece(pos, pos.xb != -1);

        // если ход не был взятием (pos.xb == -1), возвращаем успешный результат
        if (pos.xb == -1)
            return Response::OK;

        // если ход был взятием, продолжаем серию взятий
        beat_series = 1; // устанавливаем счётчик серии взятий

        // цикл для обработки серии взятий (если игрок может продолжать брать шашки)
        while (true)
        {
            // ищем возможные ходы для продолжения взятий
            logic.find_turns(pos.x2, pos.y2);

            // если больше нет возможных взятий, выходим из цикла
            if (!logic.have_beats)
                break;

            // вектор для хранения координат клеток, куда можно сделать ход для продолжения взятий
            vector<pair<POS_T, POS_T>> cells;
            for (auto turn : logic.turns)
            {
                cells.emplace_back(turn.x2, turn.y2);
            }

            // подсвечиваем клетки, куда можно сделать ход для продолжения взятий
            board.highlight_cells(cells);

            // выделяем активную клетку (текущую позицию шашки)
            board.set_active(pos.x2, pos.y2);

            // цикл для обработки выбора игрока при продолжении взятий
            while (true)
            {
                // получаем ответ от игрока
                auto resp = hand.get_cell();

                // если ответ не связан с выбором клетки, возвращаем его
                if (get<0>(resp) != Response::CELL)
                    return get<0>(resp);

                // получаем координаты выбранной клетки
                pair<POS_T, POS_T> cell{ get<1>(resp), get<2>(resp) };

                // проверяем, является ли выбранная клетка допустимой для продолжения взятий
                bool is_correct = false;
                for (auto turn : logic.turns)
                {
                    if (turn.x2 == cell.first && turn.y2 == cell.second)
                    {
                        is_correct = true;
                        pos = turn; // запоминаем выбранный ход
                        break;
                    }
                }

                // если клетка недопустима, продолжаем ожидание корректного выбора
                if (!is_correct)
                    continue;

                // очищаем подсветку и выделение
                board.clear_highlight();
                board.clear_active();

                // увеличиваем счётчик серии взятий и выполняем ход
                beat_series += 1;
                board.move_piece(pos, beat_series);
                break;
            }
        }

        // возвращаем успешный результат после завершения хода
        return Response::OK;
    }

  private:
    Config config;
    Board board;
    Hand hand;
    Logic logic;
    int beat_series;
    bool is_replay = false;
};
