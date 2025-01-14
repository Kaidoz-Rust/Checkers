#pragma once
#include <tuple>

#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// ����� ��� ��������� �������������� ������������ � ����� (����, ���� � �.�.)
class Hand
{
public:
    // �����������, ����������� ��������� �� �����
    Hand(Board* board) : board(board)
    {
    }

    // ����� ��� ��������� ��������� ������ ��� �������� �� ������������
    tuple<Response, POS_T, POS_T> get_cell() const
    {
        SDL_Event windowEvent; // ������� SDL (��������, ���� ���� ��� �������� ����)
        Response resp = Response::OK; // ����� �� ���������
        int x = -1, y = -1; // ���������� ����
        int xc = -1, yc = -1; // ���������� ������ �� �����

        // ����������� ���� ��� ��������� �������
        while (true)
        {
            if (SDL_PollEvent(&windowEvent)) // ���������, ���� �� ����� �������
            {
                switch (windowEvent.type) // ������������ ��� �������
                {
                case SDL_QUIT: // ���� ������������ ������ ����
                    resp = Response::QUIT; // ������������� ����� "�����"
                    break;

                case SDL_MOUSEBUTTONDOWN: // ���� ������������ ����� ������ ����
                    x = windowEvent.motion.x; // �������� ���������� ���� �� X
                    y = windowEvent.motion.y; // �������� ���������� ���� �� Y
                    xc = int(y / (board->H / 10) - 1); // ����������� ���������� ���� � ���������� ������ �� X
                    yc = int(x / (board->W / 10) - 1); // ����������� ���������� ���� � ���������� ������ �� Y

                    // ���� ������������ ����� �� ������ "�����" (����� ������� ����)
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1)
                    {
                        resp = Response::BACK; // ������������� ����� "�����"
                    }
                    // ���� ������������ ����� �� ������ "������" (������ ������� ����)
                    else if (xc == -1 && yc == 8)
                    {
                        resp = Response::REPLAY; // ������������� ����� "������"
                    }
                    // ���� ������������ ����� �� ������ � �������� �����
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8)
                    {
                        resp = Response::CELL; // ������������� ����� "����� ������"
                    }
                    else // ���� ������� ���� ��� ����� ��� ������
                    {
                        xc = -1; // ���������� ���������� ������
                        yc = -1;
                    }
                    break;

                case SDL_WINDOWEVENT: // ���� ��������� ������� ����
                    if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) // ���� ��������� ������ ����
                    {
                        board->reset_window_size(); // ���������� ������ ���� �����
                        break;
                    }
                }

                // ���� ������� �����, �������� �� "OK", ������� �� �����
                if (resp != Response::OK)
                    break;
            }
        }

        // ���������� ����� � ���������� ������
        return { resp, xc, yc };
    }

    // ����� ��� �������� �������� �� ������������ (��������, ������ ��� �����)
    Response wait() const
    {
        SDL_Event windowEvent; // ������� SDL
        Response resp = Response::OK; // ����� �� ���������

        // ����������� ���� ��� ��������� �������
        while (true)
        {
            if (SDL_PollEvent(&windowEvent)) // ���������, ���� �� ����� �������
            {
                switch (windowEvent.type) // ������������ ��� �������
                {
                case SDL_QUIT: // ���� ������������ ������ ����
                    resp = Response::QUIT; // ������������� ����� "�����"
                    break;

                case SDL_WINDOWEVENT_SIZE_CHANGED: // ���� ��������� ������ ����
                    board->reset_window_size(); // ���������� ������ ���� �����
                    break;

                case SDL_MOUSEBUTTONDOWN: // ���� ������������ ����� ������ ����
                {
                    int x = windowEvent.motion.x; // �������� ���������� ���� �� X
                    int y = windowEvent.motion.y; // �������� ���������� ���� �� Y
                    int xc = int(y / (board->H / 10) - 1); // ����������� ���������� ���� � ���������� ������ �� X
                    int yc = int(x / (board->W / 10) - 1); // ����������� ���������� ���� � ���������� ������ �� Y

                    // ���� ������������ ����� �� ������ "������" (������ ������� ����)
                    if (xc == -1 && yc == 8)
                        resp = Response::REPLAY; // ������������� ����� "������"
                }
                break;
                }

                // ���� ������� �����, �������� �� "OK", ������� �� �����
                if (resp != Response::OK)
                    break;
            }
        }

        // ���������� �����
        return resp;
    }

private:
    Board* board; // ��������� �� ����� ��� �������������� � � ��������
};