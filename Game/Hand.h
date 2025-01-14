#pragma once
#include <tuple>

#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// класс для обработки взаимодействия пользователя с игрой (мышь, окно и т.д.)
class Hand
{
public:
    // конструктор, принимающий указатель на доску
    Hand(Board* board) : board(board)
    {
    }

    // метод для получения выбранной клетки или действия от пользователя
    tuple<Response, POS_T, POS_T> get_cell() const
    {
        SDL_Event windowEvent; // событие SDL (например, клик мыши или закрытие окна)
        Response resp = Response::OK; // ответ по умолчанию
        int x = -1, y = -1; // координаты мыши
        int xc = -1, yc = -1; // координаты клетки на доске

        // бесконечный цикл для обработки событий
        while (true)
        {
            if (SDL_PollEvent(&windowEvent)) // проверяем, есть ли новое событие
            {
                switch (windowEvent.type) // обрабатываем тип события
                {
                case SDL_QUIT: // если пользователь закрыл окно
                    resp = Response::QUIT; // устанавливаем ответ "выход"
                    break;

                case SDL_MOUSEBUTTONDOWN: // если пользователь нажал кнопку мыши
                    x = windowEvent.motion.x; // получаем координаты мыши по X
                    y = windowEvent.motion.y; // получаем координаты мыши по Y
                    xc = int(y / (board->H / 10) - 1); // преобразуем координаты мыши в координаты клетки по X
                    yc = int(x / (board->W / 10) - 1); // преобразуем координаты мыши в координаты клетки по Y

                    // если пользователь нажал на кнопку "назад" (левый верхний угол)
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1)
                    {
                        resp = Response::BACK; // устанавливаем ответ "назад"
                    }
                    // если пользователь нажал на кнопку "реплей" (правый верхний угол)
                    else if (xc == -1 && yc == 8)
                    {
                        resp = Response::REPLAY; // устанавливаем ответ "реплей"
                    }
                    // если пользователь нажал на клетку в пределах доски
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8)
                    {
                        resp = Response::CELL; // устанавливаем ответ "выбор клетки"
                    }
                    else // если нажатие было вне доски или кнопок
                    {
                        xc = -1; // сбрасываем координаты клетки
                        yc = -1;
                    }
                    break;

                case SDL_WINDOWEVENT: // если произошло событие окна
                    if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) // если изменился размер окна
                    {
                        board->reset_window_size(); // сбрасываем размер окна доски
                        break;
                    }
                }

                // если получен ответ, отличный от "OK", выходим из цикла
                if (resp != Response::OK)
                    break;
            }
        }

        // возвращаем ответ и координаты клетки
        return { resp, xc, yc };
    }

    // метод для ожидания действия от пользователя (например, реплей или выход)
    Response wait() const
    {
        SDL_Event windowEvent; // событие SDL
        Response resp = Response::OK; // ответ по умолчанию

        // бесконечный цикл для обработки событий
        while (true)
        {
            if (SDL_PollEvent(&windowEvent)) // проверяем, есть ли новое событие
            {
                switch (windowEvent.type) // обрабатываем тип события
                {
                case SDL_QUIT: // если пользователь закрыл окно
                    resp = Response::QUIT; // устанавливаем ответ "выход"
                    break;

                case SDL_WINDOWEVENT_SIZE_CHANGED: // если изменился размер окна
                    board->reset_window_size(); // сбрасываем размер окна доски
                    break;

                case SDL_MOUSEBUTTONDOWN: // если пользователь нажал кнопку мыши
                {
                    int x = windowEvent.motion.x; // получаем координаты мыши по X
                    int y = windowEvent.motion.y; // получаем координаты мыши по Y
                    int xc = int(y / (board->H / 10) - 1); // преобразуем координаты мыши в координаты клетки по X
                    int yc = int(x / (board->W / 10) - 1); // преобразуем координаты мыши в координаты клетки по Y

                    // если пользователь нажал на кнопку "реплей" (правый верхний угол)
                    if (xc == -1 && yc == 8)
                        resp = Response::REPLAY; // устанавливаем ответ "реплей"
                }
                break;
                }

                // если получен ответ, отличный от "OK", выходим из цикла
                if (resp != Response::OK)
                    break;
            }
        }

        // возвращаем ответ
        return resp;
    }

private:
    Board* board; // указатель на доску для взаимодействия с её методами
};