#include <iostream>
#include <fstream>
#include <algorithm>
#include <array>
#include <vector>
#include <map>
#include <cstdlib>
#include <ctime>

const int SIZE = 8;
const int ALL = 64;
const int VALUE_MAX = 20000;
const int VALUE_MIN = -20000;

struct Point {
    int x, y;
    Point() : Point(0, 0) {}
	Point(float x, float y) : x(x), y(y) {}
	bool operator==(const Point& rhs) const {
		return x == rhs.x && y == rhs.y;
	}
	bool operator!=(const Point& rhs) const {
		return !operator==(rhs);
	}
	Point operator+(const Point& rhs) const {
		return Point(x + rhs.x, y + rhs.y);
	}
	Point operator-(const Point& rhs) const {
		return Point(x - rhs.x, y - rhs.y);
	}
};

int player;
std::array<std::array<int, SIZE>, SIZE> board;
std::vector<Point> next_valid_spots;
const std::array<Point, 8> dir{{Point(-1, -1), Point(-1, 0), Point(-1, 1), Point(0, -1),
                                Point(0, 1), Point(1, -1), Point(1, 0), Point(1, 1)}};

class State {
public:
    std::array<std::array<int, SIZE>, SIZE> othello;
    std::vector<Point> valid_spots;
    int cur_player;
    int level;
    int value;
    bool game_end;

    State(std::array<std::array<int, SIZE>, SIZE> o): cur_player(player), level(0), value(0), game_end(false) {
        for(int i = 0; i < SIZE; i++)
            for(int j = 0; j < SIZE; j++)
                othello[i][j] = o[i][j];
    }
    State(const State& s) {
        for(int i = 0; i < SIZE; i++)
            for(int j = 0; j < SIZE; j++)
                othello[i][j] = s.othello[i][j];
        valid_spots = s.valid_spots;
        cur_player = s.cur_player;
        level = s.level;
        value = s.value;
        game_end = s.game_end;
    }
    int stability() {
        const int edge = 200;
        int val = 0;
        if(othello[0][0]) {
            int mul = (othello[0][0] == player)? 1 : -1;
            for(int i = 1; i < 7; i++) {
                if(othello[0][0] == othello[i][0]) val += mul * edge;
                else break;
            }
            for(int j = 1; j < 7; j++) {
                if(othello[0][0] == othello[0][j]) val += mul * edge;
                else break;
            }
        }
        if(othello[0][7]) {
            int mul = (othello[0][7] == player)? 1 : -1;
            for(int i = 1; i < 7; i++) {
                if(othello[0][7] == othello[i][7]) val += mul * edge;
                else break;
            }
            for(int j = 6; j > 0; j--) {
                if(othello[0][7] == othello[0][j]) val += mul * edge;
                else break;
            }
        }
        if(othello[7][0]) {
            int mul = (othello[7][0] == player)? 1 : -1;
            for(int i = 6; i > 0; i--) {
                if(othello[7][0] == othello[i][0]) val += mul * edge;
                else break;
            }
            for(int j = 1; j < 7; j++) {
                if(othello[7][0] == othello[7][j]) val += mul * edge;
                else break;
            }
        }
        if(othello[7][7]) {
            int mul = (othello[7][7] == player)? 1 : -1;
            for(int i = 6; i > 0; i--) {
                if(othello[7][7] == othello[i][7]) val += mul * edge;
                else break;
            }
            for(int j = 6; j > 0; j--) {
                if(othello[7][7] == othello[7][j]) val += mul * edge;
                else break;
            }
        }
        if((othello[0][1] == othello[0][2] == othello[0][3] ==
            othello[0][4] == othello[0][5] == othello[0][6] == player)
           && (othello[0][0] && othello[0][7]))
           value += edge * 6;
        if((othello[0][1] == othello[0][2] == othello[0][3] ==
            othello[0][4] == othello[0][5] == othello[0][6] == (3 - player))
           && (othello[0][0] && othello[0][7]))
           value -= edge * 6;
        if((othello[1][0] == othello[2][0] == othello[3][0] ==
            othello[4][0] == othello[5][0] == othello[6][0] == player)
           && (othello[0][0] && othello[7][0]))
           value += edge * 6;
        if((othello[1][0] == othello[2][0] == othello[3][0] ==
            othello[4][0] == othello[5][0] == othello[6][0] == (3 - player))
           && (othello[0][0] && othello[7][0]))
           value -= edge * 6;
        if((othello[7][1] == othello[7][2] == othello[7][3] ==
            othello[7][4] == othello[7][5] == othello[7][6] == player)
           && (othello[7][0] && othello[7][7]))
           value += edge * 6;
        if((othello[7][1] == othello[7][2] == othello[7][3] ==
            othello[7][4] == othello[7][5] == othello[7][6] == (3 - player))
           && (othello[7][0] && othello[7][7]))
           value -= edge * 6;
        if((othello[1][7] == othello[2][7] == othello[3][7] ==
            othello[4][7] == othello[5][7] == othello[6][7] == player)
           && (othello[0][7] && othello[7][7]))
           value += edge * 6;
        if((othello[1][7] == othello[2][7] == othello[3][7] ==
            othello[4][7] == othello[5][7] == othello[6][7] == (3 - player))
           && (othello[0][7] && othello[7][7]))
           value -= edge * 6;
        return val;
    }
    int weight() {
        const int weight_table[SIZE][SIZE] =
        {
            { 500, -100, 20, 15, 15, 20, -100,  500},
            {-100, -200, 10, 3,  3,  10, -200, -100},
            { 20,   10,  15, 10, 10, 15,  10,   20},
            { 15,   3,   10, 3,  3,  10,  3,    15},
            { 15,   3,   10, 3,  3,  10,  3,    15},
            { 20,   10,  15, 10, 10, 15,  10,   20},
            {-100, -200, 10, 3,  3,  10, -200, -100},
            { 500, -100, 20, 15, 15, 20, -100,  500},
        };
        int val = 0;
        for(int i = 0; i < SIZE; i++)
            for(int j = 0; j < SIZE; j++) {
                if(othello[i][j] == player)
                    val += weight_table[i][j];
                else if(othello[i][j] == (3 - player))
                    val -= weight_table[i][j];
            }
        return val;
    }
    void setValue() {
        value = 0;
        int me = 0, op = 0;
        for(int i = 0; i < SIZE; i++)
            for(int j = 0; j < SIZE; j++) {
                if(othello[i][j] == player) me++;
                else if(othello[i][j] == (3 - player)) op++;
            }
        /* stability */
        value += stability();
        /* weight */
        value += weight();
        /* mobility (high to low) */
        int mul = (cur_player == player)? 1 : -1;
        value += mul * valid_spots.size() * 2 * (1 - (me + op) / ALL);
        /* difference (end) */
        if(game_end)
            value = me - op;
    }
    void update(Point center) {
        flip(center);

        cur_player = 3 - cur_player;
        change_valid_spots();

        if(!valid_spots.size()) {
            std::vector<Point> tmp_valid_spots = valid_spots;
            cur_player = 3 - cur_player;
            change_valid_spots();
            if(!valid_spots.size())
                game_end = true;
            else {
                cur_player = 3 - cur_player;
                valid_spots = tmp_valid_spots;
            }
        }
        level++;
    }

private:
    bool is_in_board(Point p) {
        return p.x >= 0 && p.x < SIZE && p.y >= 0 && p.y < SIZE;
    }
    void flip(Point center) {
        for (Point d: dir) {
            Point p = center + d;
            if(is_in_board(p) && (othello[p.x][p.y] == (3 - cur_player))) {
                do {
                    if(othello[p.x][p.y] == cur_player) {
                        while(p != center) {
                            p = p - d;
                            othello[p.x][p.y] = cur_player;
                        }
                        break;
                    }
                    p = p + d;
                } while(is_in_board(p) && othello[p.x][p.y]);
            }
        }
    }
    bool is_valid_spot(Point center) {
        for (Point d: dir) {
            Point p = center + d;
            if(is_in_board(p) && (othello[p.x][p.y] == (3 - cur_player))) {
                do {
                    if(othello[p.x][p.y] == cur_player)
                        return true;
                    p = p + d;
                } while(is_in_board(p) && othello[p.x][p.y]);
            }
        }
        return false;
    }
    void change_valid_spots() {
        valid_spots.clear();
        for(int i = 0; i < SIZE; i++)
            for(int j = 0; j < SIZE; j++) {
                Point p(i, j);
                if(!othello[i][j] && is_valid_spot(p))
                    valid_spots.push_back(p);
            }
    }
};

std::map<int, Point> val_move;
int minimax(State s, int depth, int alpha, int beta) {
    if(depth == 0 || s.game_end) {
        s.setValue();
        return s.value;
    }
    else if(s.cur_player == player) {
        int val = VALUE_MIN;
        int n = s.valid_spots.size();
        for(int i = 0; i < n; i++) {
            State next = s;
            next.update(s.valid_spots[i]);
            int m = minimax(next, depth - 1, alpha, beta);
            val = std::max(val, m);
            alpha = std::max(alpha, val);
            if(s.level == 0)
                val_move[m] = next_valid_spots[i];
            if(beta <= alpha)
                break;
        }
        return val;
    }
    else if(s.cur_player == (3 - player)) {
        int val = VALUE_MAX;
        int n = s.valid_spots.size();
        for(int i = 0; i < n; i++) {
            State next = s;
            next.update(s.valid_spots[i]);
            int m = minimax(next, depth - 1, alpha, beta);
            val = std::min(val, m);
            beta = std::min(beta, val);
            if(s.level == 0)
                val_move[m] = next_valid_spots[i];
            if(beta <= alpha)
                break;
        }
        return val;
    }
}

void read_board(std::ifstream& fin) {
    fin >> player;
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            fin >> board[i][j];
}

void read_valid_spots(std::ifstream& fin) {
    int n_valid_spots;
    fin >> n_valid_spots;
    int x, y;
    for (int i = 0; i < n_valid_spots; i++) {
        fin >> x >> y;
        next_valid_spots.push_back({x, y});
    }
}

void write_valid_spot(std::ofstream& fout) {
    State s(board);
    s.valid_spots = next_valid_spots;

    int val = minimax(s, 1, VALUE_MIN, VALUE_MAX);
    Point best = val_move[val];
    /* fout */
    fout << best.x << " " << best.y << std::endl;
    fout.flush();

    val = minimax(s, 6, VALUE_MIN, VALUE_MAX);
    best = val_move[val];
    /* fout */
    fout << best.x << " " << best.y << std::endl;
    fout.flush();
}

int main(int, char** argv) {
    std::ifstream fin(argv[1]);
    std::ofstream fout(argv[2]);
    read_board(fin);
    read_valid_spots(fin);
    write_valid_spot(fout);
    fin.close();
    fout.close();
    return 0;
}