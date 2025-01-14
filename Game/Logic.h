#pragma once
#include <random>
#include <vector>

#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

const int INF = 1e9;

class Logic
{
public:
	Logic(Board* board, Config* config) : board(board), config(config)
	{
		rand_eng = std::default_random_engine(
			!((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0);
		scoring_mode = (*config)("Bot", "BotScoringType");
		optimization = (*config)("Bot", "Optimization");
	}



private:
	// применяет ход turn к доске mtx и возвращает новое состояние доски
	vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const
	{
		if (turn.xb != -1)
			mtx[turn.xb][turn.yb] = 0;
		if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7))
			mtx[turn.x][turn.y] += 2;
		mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y];
		mtx[turn.x][turn.y] = 0;
		return mtx;
	}
	
	// вычисляет оценку состояния доски mtx для игрока с цветом first_bot_color
	double calc_score(const vector<vector<POS_T>>& mtx, const bool first_bot_color) const
	{
		// color - who is max player
		double w = 0, wq = 0, b = 0, bq = 0;
		for (POS_T i = 0; i < 8; ++i)
		{
			for (POS_T j = 0; j < 8; ++j)
			{
				w += (mtx[i][j] == 1);
				wq += (mtx[i][j] == 3);
				b += (mtx[i][j] == 2);
				bq += (mtx[i][j] == 4);
				if (scoring_mode == "NumberAndPotential")
				{
					w += 0.05 * (mtx[i][j] == 1) * (7 - i);
					b += 0.05 * (mtx[i][j] == 2) * (i);
				}
			}
		}
		if (!first_bot_color)
		{
			swap(b, w);
			swap(bq, wq);
		}
		if (w + wq == 0)
			return INF;
		if (b + bq == 0)
			return 0;
		int q_coef = 4;
		if (scoring_mode == "NumberAndPotential")
		{
			q_coef = 5;
		}
		return (b + bq * q_coef) / (w + wq * q_coef);
	}

public:
	// находит все возможные ходы для игрока с цветом color на текущей доске.
	void find_turns(const bool color)
	{
		find_turns(color, board->get_board());
	}

	// находит все возможные ходы для шашки, находящейся на клетке (x, y)
	void find_turns(const POS_T x, const POS_T y)
	{
		find_turns(x, y, board->get_board());
	}

	vector<move_pos> find_best_turns(const bool color)
	{
		// очищаем предыдущие состояния и ходы
		next_best_state.clear();
		next_move.clear();

		// запускаем поиск лучших ходов с начального состояния
		find_first_best_turn(board->get_board(), color, -1, -1, 0);

		// построение результата на основе сохраненных состояний и ходов
		int cur_state = 0;
		vector<move_pos> res;
		do
		{
			res.push_back(next_move[cur_state]);
			cur_state = next_best_state[cur_state];
		} while (cur_state != -1 && next_move[cur_state].x != -1);
		return res;
	}

private:

	// находит все возможные ходы для игрока с цветом color на переданной доске mtx
	void find_turns(const bool color, const vector<vector<POS_T>>& mtx)
	{
		vector<move_pos> res_turns;
		bool have_beats_before = false;
		for (POS_T i = 0; i < 8; ++i)
		{
			for (POS_T j = 0; j < 8; ++j)
			{
				if (mtx[i][j] && mtx[i][j] % 2 != color)
				{
					find_turns(i, j, mtx);
					if (have_beats && !have_beats_before)
					{
						have_beats_before = true;
						res_turns.clear();
					}
					if ((have_beats_before && have_beats) || !have_beats_before)
					{
						res_turns.insert(res_turns.end(), turns.begin(), turns.end());
					}
				}
			}
		}
		turns = res_turns;
		shuffle(turns.begin(), turns.end(), rand_eng);
		have_beats = have_beats_before;
	}
	
	// находит все возможные ходы для шашки на клетке (x, y) на переданной доске mtx
	void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>>& mtx)
	{
		turns.clear();
		have_beats = false;
		POS_T type = mtx[x][y];
		// check beats
		switch (type)
		{
		case 1:
		case 2:
			// check pieces
			for (POS_T i = x - 2; i <= x + 2; i += 4)
			{
				for (POS_T j = y - 2; j <= y + 2; j += 4)
				{
					if (i < 0 || i > 7 || j < 0 || j > 7)
						continue;
					POS_T xb = (x + i) / 2, yb = (y + j) / 2;
					if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2)
						continue;
					turns.emplace_back(x, y, i, j, xb, yb);
				}
			}
			break;
		default:
			// check queens
			for (POS_T i = -1; i <= 1; i += 2)
			{
				for (POS_T j = -1; j <= 1; j += 2)
				{
					POS_T xb = -1, yb = -1;
					for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
					{
						if (mtx[i2][j2])
						{
							if (mtx[i2][j2] % 2 == type % 2 || (mtx[i2][j2] % 2 != type % 2 && xb != -1))
							{
								break;
							}
							xb = i2;
							yb = j2;
						}
						if (xb != -1 && xb != i2)
						{
							turns.emplace_back(x, y, i2, j2, xb, yb);
						}
					}
				}
			}
			break;
		}
		// check other turns
		if (!turns.empty())
		{
			have_beats = true;
			return;
		}
		switch (type)
		{
		case 1:
		case 2:
			// check pieces
		{
			POS_T i = ((type % 2) ? x - 1 : x + 1);
			for (POS_T j = y - 1; j <= y + 1; j += 2)
			{
				if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j])
					continue;
				turns.emplace_back(x, y, i, j);
			}
			break;
		}
		default:
			// check queens
			for (POS_T i = -1; i <= 1; i += 2)
			{
				for (POS_T j = -1; j <= 1; j += 2)
				{
					for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
					{
						if (mtx[i2][j2])
							break;
						turns.emplace_back(x, y, i2, j2);
					}
				}
			}
			break;
		}
	}
	
	// рекурсивно ищет лучший ход для игрока с цветом color на доске mtx
	double find_first_best_turn(std::vector<std::vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y, size_t state,
		double alpha = -1) {
		// инициализируем состояние для текущего уровня.
		next_best_state.push_back(-1); // значение -1 указывает на отсутствие следующего состояния.
		next_move.emplace_back(-1, -1, -1, -1); // заполняем пустым ходом.

		double best_score = -1; // изначально лучшее значение минимально возможное.

		// если состояние не начальное, находим возможные ходы для текущей позиции.
		if (state != 0)
			find_turns(x, y, mtx);

		// сохраняем текущие ходы и флаг наличия ударов.
		auto turns_now = turns;       // список доступных ходов.
		bool have_beats_now = have_beats; // флаг, указывающий, есть ли ходы с побитием.

		// если нет ударов и состояние не начальное, переходим к следующему игроку.
		if (!have_beats_now && state != 0) {
			return find_best_turns_rec(mtx, 1 - color, 0, alpha);
		}

		// перебираем все доступные ходы.
		for (auto turn : turns_now) {
			size_t next_state = next_move.size(); // индекс для следующего состояния.
			double score; // оценка текущего хода.

			if (have_beats_now) {
				// если есть удары, продолжаем искать лучшие ходы для текущего игрока.
				score = find_first_best_turn(make_turn(mtx, turn), color, turn.x2, turn.y2, next_state, best_score);
			}
			else {
				// если ударов нет, переходим к следующему игроку.
				score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, 0, best_score);
			}

			// сравниваем текущую оценку с наилучшей найденной.
			if (score > best_score) {
				best_score = score; // Обновляем лучший результат.
				next_best_state[state] = (have_beats_now ? int(next_state) : -1); // сохраняем индекс следующего состояния.
				next_move[state] = turn; // сохраняем лучший ход.
			}
		}

		return best_score; // возвращаем лучшую оценку.
	}
	
	// рекурсивно ищет лучший ход с использованием алгоритма минимакс и альфа-бета отсечения.
	double find_best_turns_rec(std::vector<std::vector<POS_T>> mtx, const bool color, const size_t depth, double alpha = -1,
		double beta = INF + 1, const POS_T x = -1, const POS_T y = -1) {
		// если достигнута максимальная глубина рекурсии, возвращаем оценку текущего состояния доски
		if (depth == Max_depth) {
			return calc_score(mtx, (depth % 2 == color));
		}

		// если переданы координаты фигуры (x, y), ищем ходы для нее
		if (x != -1) {
			find_turns(x, y, mtx);
		}
		// иначе ищем все доступные ходы для текущего игрока
		else {
			find_turns(color, mtx);
		}

		// сохраняем доступные ходы и наличие обязательных ударов
		auto turns_now = turns;
		bool have_beats_now = have_beats;

		// если ударов нет и текущая фигура задана, передаем ход сопернику
		if (!have_beats_now && x != -1) {
			return find_best_turns_rec(mtx, 1 - color, depth + 1, alpha, beta);
		}

		// если доступных ходов нет, возвращаем оценку: победа или поражение в зависимости от игрока
		if (turns.empty()) {
			return (depth % 2 ? 0 : INF);
		}

		// инициализируем минимальное и максимальное значения для текущей оценки
		double min_score = INF + 1;
		double max_score = -1;

		// перебираем все доступные ходы
		for (auto turn : turns_now) {
			double score = 0.0;

			// если ходов с обязательным побитием нет и конкретная фигура не задана
			if (!have_beats_now && x == -1) {
				// Рекурсивно оцениваем состояние доски для соперника
				score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, depth + 1, alpha, beta);
			}
			// иначе продолжаем искать ходы для текущей фигуры
			else {
				score = find_best_turns_rec(make_turn(mtx, turn), color, depth, alpha, beta, turn.x2, turn.y2);
			}

			// если текущая глубина соответствует максимизирующему игроку
			if (depth % 2) {
				// Обновляем максимум и альфа-значение
				max_score = std::max(max_score, score);
				alpha = std::max(alpha, max_score);
			}
			// если текущая глубина соответствует минимизирующему игроку
			else {
				// Обновляем минимум и бета-значение
				min_score = std::min(min_score, score);
				beta = std::min(beta, min_score);
			}

			// прерываем перебор ходов, если найдено альфа-бета отсечение
			if (optimization != "O0" && alpha >= beta) {
				return (depth % 2 ? max_score + 1 : min_score - 1);
			}
		}

		// возвращаем максимальную или минимальную оценку в зависимости от игрока
		return (depth % 2 ? max_score : min_score);
	}

public:
	vector<move_pos> turns; // список возможных ходов для текущего состояния
	bool have_beats; // флаг, указывающий, есть ли ходы с побитием шашек
	int Max_depth; // максимальная глубина рекурсии для поиска лучшего хода

private:
	default_random_engine rand_eng; // генератор случайных чисел для перемешивания ходов
	string scoring_mode; // режим оценки состояния доски (например, "NumberAndPotential")
	string optimization; // уровень оптимизации (например, "O0" — без оптимизации)
	vector<move_pos> next_move; // список лучших ходов для текущего состояния
	vector<int> next_best_state; // список индексов следующих состояний для лучших ходов
	Board* board; // указатель на объект доски
	Config* config; // указатель на объект конфигурации
};
