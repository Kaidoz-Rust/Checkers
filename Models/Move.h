#pragma once
#include <stdlib.h> // для использования int8_t

// определение типа для координат (8-битное целое число со знаком)
typedef int8_t POS_T;

// структура, представляющая ход в игре
struct move_pos
{
    POS_T x, y;             // координаты начальной клетки (откуда)
    POS_T x2, y2;           // координаты конечной клетки (куда)
    POS_T xb = -1, yb = -1; // координаты побитой шашки (если есть, иначе -1)

    // конструктор для хода без побития шашки
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2) : x(x), y(y), x2(x2), y2(y2)
    {
    }

    // конструктор для хода с побитием шашки
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb)
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    }

    // перегрузка оператора сравнения ==
    bool operator==(const move_pos& other) const
    {
        // два хода равны, если их начальные и конечные координаты совпадают
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
    }

    // перегрузка оператора сравнения !=
    bool operator!=(const move_pos& other) const
    {
        // два хода не равны, если их начальные или конечные координаты различаются
        return !(*this == other);
    }
};