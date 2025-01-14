#pragma once
#include <iostream>
#include <fstream>
#include <vector>

#include "../Models/Move.h"
#include "../Models/Project_path.h"

#ifdef __APPLE__
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#else
#include <SDL.h>
#include <SDL_image.h>
#endif

using namespace std;

// класс, представляющий игровую доску и отвечающий за её отрисовку и управление
class Board
{
public:
    Board() = default;
    Board(const unsigned int W, const unsigned int H) : W(W), H(H)
    {
    }

    // инициализация и отрисовка начальной доски
    int start_draw()
    {
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0) // инициализация SDL
        {
            print_exception("SDL_Init can't init SDL2 lib");
            return 1;
        }
        if (W == 0 || H == 0) // если размеры окна не заданы, используем размеры экрана
        {
            SDL_DisplayMode dm;
            if (SDL_GetDesktopDisplayMode(0, &dm)) // получаем размеры экрана
            {
                print_exception("SDL_GetDesktopDisplayMode can't get desctop display mode");
                return 1;
            }
            W = min(dm.w, dm.h); // задаём размеры окна
            W -= W / 15;
            H = W;
        }
        win = SDL_CreateWindow("Checkers", 0, H / 30, W, H, SDL_WINDOW_RESIZABLE); // создание окна
        if (win == nullptr)
        {
            print_exception("SDL_CreateWindow can't create window");
            return 1;
        }
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC); // создание рендерера
        if (ren == nullptr)
        {
            print_exception("SDL_CreateRenderer can't create renderer");
            return 1;
        }
        // загрузка текстур для доски, шашек и кнопок
        board = IMG_LoadTexture(ren, board_path.c_str());
        w_piece = IMG_LoadTexture(ren, piece_white_path.c_str());
        b_piece = IMG_LoadTexture(ren, piece_black_path.c_str());
        w_queen = IMG_LoadTexture(ren, queen_white_path.c_str());
        b_queen = IMG_LoadTexture(ren, queen_black_path.c_str());
        back = IMG_LoadTexture(ren, back_path.c_str());
        replay = IMG_LoadTexture(ren, replay_path.c_str());
        if (!board || !w_piece || !b_piece || !w_queen || !b_queen || !back || !replay)
        {
            print_exception("IMG_LoadTexture can't load main textures from " + textures_path);
            return 1;
        }
        SDL_GetRendererOutputSize(ren, &W, &H); // получаем размеры рендерера
        make_start_mtx(); // создаём начальную расстановку шашек
        rerender(); // отрисовываем доску
        return 0;
    }

    // перерисовка доски (сброс состояния)
    void redraw()
    {
        game_results = -1; // сбрасываем результат игры
        history_mtx.clear(); // очищаем историю ходов
        history_beat_series.clear(); // очищаем историю серий взятий
        make_start_mtx(); // создаём начальную расстановку
        clear_active(); // сбрасываем активную клетку
        clear_highlight(); // сбрасываем подсветку
    }

    // перемещение шашки с учётом взятия
    void move_piece(move_pos turn, const int beat_series = 0)
    {
        if (turn.xb != -1) // если есть побитая шашка
        {
            mtx[turn.xb][turn.yb] = 0; // убираем её с доски
        }
        move_piece(turn.x, turn.y, turn.x2, turn.y2, beat_series); // перемещаем шашку
    }

    // перемещение шашки на новую позицию
    void move_piece(const POS_T i, const POS_T j, const POS_T i2, const POS_T j2, const int beat_series = 0)
    {
        if (mtx[i2][j2]) // проверка, что конечная клетка пуста
        {
            throw runtime_error("final position is not empty, can't move");
        }
        if (!mtx[i][j]) // проверка, что начальная клетка не пуста
        {
            throw runtime_error("begin position is empty, can't move");
        }
        if ((mtx[i][j] == 1 && i2 == 0) || (mtx[i][j] == 2 && i2 == 7)) // превращение в дамку
            mtx[i][j] += 2;
        mtx[i2][j2] = mtx[i][j]; // перемещаем шашку
        drop_piece(i, j); // убираем шашку с начальной позиции
        add_history(beat_series); // добавляем ход в историю
    }

    // убрать шашку с доски
    void drop_piece(const POS_T i, const POS_T j)
    {
        mtx[i][j] = 0; // очищаем клетку
        rerender(); // перерисовываем доску
    }

    // превращение шашки в дамку
    void turn_into_queen(const POS_T i, const POS_T j)
    {
        if (mtx[i][j] == 0 || mtx[i][j] > 2) // проверка, что шашка может стать дамкой
        {
            throw runtime_error("can't turn into queen in this position");
        }
        mtx[i][j] += 2; // превращаем шашку в дамку
        rerender(); // перерисовываем доску
    }

    // получить текущее состояние доски
    vector<vector<POS_T>> get_board() const
    {
        return mtx;
    }

    // подсветка клеток
    void highlight_cells(vector<pair<POS_T, POS_T>> cells)
    {
        for (auto pos : cells)
        {
            POS_T x = pos.first, y = pos.second;
            is_highlighted_[x][y] = 1; // подсвечиваем клетку
        }
        rerender(); // перерисовываем доску
    }

    // сброс подсветки
    void clear_highlight()
    {
        for (POS_T i = 0; i < 8; ++i)
        {
            is_highlighted_[i].assign(8, 0); // сбрасываем подсветку всех клеток
        }
        rerender(); // перерисовываем доску
    }

    // установка активной клетки
    void set_active(const POS_T x, const POS_T y)
    {
        active_x = x;
        active_y = y;
        rerender(); // перерисовываем доску
    }

    // сброс активной клетки
    void clear_active()
    {
        active_x = -1;
        active_y = -1;
        rerender(); // перерисовываем доску
    }

    // проверка, подсвечена ли клетка
    bool is_highlighted(const POS_T x, const POS_T y)
    {
        return is_highlighted_[x][y];
    }

    // откат хода
    void rollback()
    {
        auto beat_series = max(1, *(history_beat_series.rbegin())); // определяем серию взятий
        while (beat_series-- && history_mtx.size() > 1) // откатываем ходы
        {
            history_mtx.pop_back();
            history_beat_series.pop_back();
        }
        mtx = *(history_mtx.rbegin()); // восстанавливаем состояние доски
        clear_highlight(); // сбрасываем подсветку
        clear_active(); // сбрасываем активную клетку
    }

    // отображение результата игры
    void show_final(const int res)
    {
        game_results = res; // устанавливаем результат игры
        rerender(); // перерисовываем доску
    }

    // сброс размера окна
    void reset_window_size()
    {
        SDL_GetRendererOutputSize(ren, &W, &H); // получаем новые размеры окна
        rerender(); // перерисовываем доску
    }

    // завершение работы с SDL
    void quit()
    {
        SDL_DestroyTexture(board); // освобождаем текстуры
        SDL_DestroyTexture(w_piece);
        SDL_DestroyTexture(b_piece);
        SDL_DestroyTexture(w_queen);
        SDL_DestroyTexture(b_queen);
        SDL_DestroyTexture(back);
        SDL_DestroyTexture(replay);
        SDL_DestroyRenderer(ren); // освобождаем рендерер
        SDL_DestroyWindow(win); // закрываем окно
        SDL_Quit(); // завершаем работу SDL
    }

    ~Board()
    {
        if (win)
            quit(); // завершаем работу, если окно было создано
    }

private:
    // добавление хода в историю
    void add_history(const int beat_series = 0)
    {
        history_mtx.push_back(mtx); // сохраняем текущее состояние доски
        history_beat_series.push_back(beat_series); // сохраняем серию взятий
    }

    // создание начальной расстановки шашек
    void make_start_mtx()
    {
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                mtx[i][j] = 0; // очищаем клетку
                if (i < 3 && (i + j) % 2 == 1) // расставляем чёрные шашки
                    mtx[i][j] = 2;
                if (i > 4 && (i + j) % 2 == 1) // расставляем белые шашки
                    mtx[i][j] = 1;
            }
        }
        add_history(); // добавляем начальное состояние в историю
    }

    // перерисовка всех элементов доски
    void rerender()
    {
        SDL_RenderClear(ren); // очищаем рендерер
        SDL_RenderCopy(ren, board, NULL, NULL); // отрисовываем доску

        // отрисовываем шашки
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (!mtx[i][j])
                    continue;
                int wpos = W * (j + 1) / 10 + W / 120;
                int hpos = H * (i + 1) / 10 + H / 120;
                SDL_Rect rect{ wpos, hpos, W / 12, H / 12 };

                SDL_Texture* piece_texture;
                if (mtx[i][j] == 1)
                    piece_texture = w_piece;
                else if (mtx[i][j] == 2)
                    piece_texture = b_piece;
                else if (mtx[i][j] == 3)
                    piece_texture = w_queen;
                else
                    piece_texture = b_queen;

                SDL_RenderCopy(ren, piece_texture, NULL, &rect);
            }
        }

        // отрисовываем подсветку
        SDL_SetRenderDrawColor(ren, 0, 255, 0, 0);
        const double scale = 2.5;
        SDL_RenderSetScale(ren, scale, scale);
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (!is_highlighted_[i][j])
                    continue;
                SDL_Rect cell{ int(W * (j + 1) / 10 / scale), int(H * (i + 1) / 10 / scale), int(W / 10 / scale),
                              int(H / 10 / scale) };
                SDL_RenderDrawRect(ren, &cell);
            }
        }

        // отрисовываем активную клетку
        if (active_x != -1)
        {
            SDL_SetRenderDrawColor(ren, 255, 0, 0, 0);
            SDL_Rect active_cell{ int(W * (active_y + 1) / 10 / scale), int(H * (active_x + 1) / 10 / scale),
                                 int(W / 10 / scale), int(H / 10 / scale) };
            SDL_RenderDrawRect(ren, &active_cell);
        }
        SDL_RenderSetScale(ren, 1, 1);

        // отрисовываем кнопки "назад" и "реплей"
        SDL_Rect rect_left{ W / 40, H / 40, W / 15, H / 15 };
        SDL_RenderCopy(ren, back, NULL, &rect_left);
        SDL_Rect replay_rect{ W * 109 / 120, H / 40, W / 15, H / 15 };
        SDL_RenderCopy(ren, replay, NULL, &replay_rect);

        // отрисовываем результат игры
        if (game_results != -1)
        {
            string result_path = draw_path;
            if (game_results == 1)
                result_path = white_path;
            else if (game_results == 2)
                result_path = black_path;
            SDL_Texture* result_texture = IMG_LoadTexture(ren, result_path.c_str());
            if (result_texture == nullptr)
            {
                print_exception("IMG_LoadTexture can't load game result picture from " + result_path);
                return;
            }
            SDL_Rect res_rect{ W / 5, H * 3 / 10, W * 3 / 5, H * 2 / 5 };
            SDL_RenderCopy(ren, result_texture, NULL, &res_rect);
            SDL_DestroyTexture(result_texture);
        }

        SDL_RenderPresent(ren); // обновляем рендерер
        SDL_Delay(10); // небольшая задержка для стабильности
        SDL_Event windowEvent;
        SDL_PollEvent(&windowEvent); // обрабатываем события
    }

    // запись ошибки в лог
    void print_exception(const string& text) {
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Error: " << text << ". " << SDL_GetError() << endl;
        fout.close();
    }

public:
    int W = 0; // ширина окна
    int H = 0; // высота окна
    vector<vector<vector<POS_T>>> history_mtx; // история состояний доски

private:
    SDL_Window* win = nullptr; // окно SDL
    SDL_Renderer* ren = nullptr; // рендерер SDL
    SDL_Texture* board = nullptr; 
    SDL_Texture* w_piece = nullptr; 
    SDL_Texture* b_piece = nullptr;
    SDL_Texture* w_queen = nullptr;
    SDL_Texture* b_queen = nullptr; 
    SDL_Texture* back = nullptr; 
    SDL_Texture* replay = nullptr;
    const string textures_path = project_path + "Textures/"; 
    const string board_path = textures_path + "board.png"; 
    const string piece_white_path = textures_path + "piece_white.png";
    const string piece_black_path = textures_path + "piece_black.png"; 
    const string queen_white_path = textures_path + "queen_white.png"; 
    const string queen_black_path = textures_path + "queen_black.png"; 
    const string white_path = textures_path + "white_wins.png"; 
    const string black_path = textures_path + "black_wins.png"; 
    const string draw_path = textures_path + "draw.png"; 
    const string back_path = textures_path + "back.png"; 
    const string replay_path = textures_path + "replay.png"; 
    int active_x = -1, active_y = -1; 
    int game_results = -1; // результат игры
    vector<vector<bool>> is_highlighted_ = vector<vector<bool>>(8, vector<bool>(8, 0)); 
    vector<vector<POS_T>> mtx = vector<vector<POS_T>>(8, vector<POS_T>(8, 0)); 
    vector<int> history_beat_series; // история серий взятий
};