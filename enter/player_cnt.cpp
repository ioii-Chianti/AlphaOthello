#include <iostream>
#include <fstream>
#include <algorithm>
#include <array>
#include <vector>
#include <utility>
#include <cstdlib>
#include <ctime>
#define INF 99999
#define MAXMODE 1
#define MINIMODE 2
using namespace std;

struct Point {
    int x, y;
	Point() : Point(0, 0) {}
	Point(float x, float y) : x(x), y(y) {}
	bool operator==(const Point& rhs) const { return x == rhs.x && y == rhs.y; }
	bool operator!=(const Point& rhs) const { return !operator==(rhs); }
	Point operator+(const Point& rhs) const { return Point(x + rhs.x, y + rhs.y); }
	Point operator-(const Point& rhs) const { return Point(x - rhs.x, y - rhs.y); }
};

int Player;   // this AI (Fixed)
const int SIZE = 8;
const int weight_matrix[SIZE][SIZE] = {
    { 100, -10,  11,   6,   6,  11, -10, 100},
    { -10, -20,   1,   2,   2,   1, -20, -10},
    {  10,   1,   5,   4,   4,   5,   1,  10},
    {   6,   2,   4,   2,   2,   4,   2,   6},
    {   6,   2,   4,   2,   2,   4,   2,   6},
    {  10,   1,   5,   4,   4,   5,   1,  10},
    { -10, -20,   1,   2,   2,   1, -20, -10},
    { 100, -10,  11,   6,   6,  11, -10, 100}
};

class OthelloState {
    public:
        /* const */
        enum SPOT_STATE { EMPTY = 0, BLACK = 1, WHITE = 2 };
        const array<Point, 8> directions{{
            Point(-1, -1), Point(-1, 0), Point(-1, 1),
            Point(0, -1), /*{0, 0}, */Point(0, 1),
            Point(1, -1), Point(1, 0), Point(1, 1)
        }};

        /* attributes */
        array<array<int, SIZE>, SIZE> board;    // 版面
        vector<Point> next_valid_spots;    // 對此版面和玩家來說的可走
        int cur_player;   // the player in this state (Moved)
        int round;
        bool done;        // game ends or not
        int heuristic;

        /* methods */
        // 從 read 讀入的 state
        OthelloState(array<array<int, SIZE>, SIZE> arr) {
            for (int i = 0; i < SIZE; i++)
                for (int j = 0; j < SIZE; j++)
                    board[i][j] = arr[i][j];
            cur_player = Player;
            round = 0;
            done = false;
            heuristic = 0;
        }
        // copy constructor
        OthelloState(const OthelloState& rhs) {
            for (int i = 0; i < SIZE; i++)
                for (int j = 0; j < SIZE; j++)
                    board[i][j] = rhs.board[i][j];
            cur_player = rhs.cur_player;
            round = rhs.round;
            done = rhs.done;
            heuristic = rhs.heuristic;
            next_valid_spots = rhs.next_valid_spots;
        }
        // 找對當前玩家的可放位置
        vector<Point> get_valid_spots() const {
            vector<Point> valid_spots;
            for (int i = 0; i < SIZE; i++) {
                for (int j = 0; j < SIZE; j++) {
                    if (board[i][j] != EMPTY)
                        continue;
                    Point p = Point(i, j);
                    if (is_spot_valid(p))
                        valid_spots.push_back(p);
                }
            }
            return valid_spots;
        }

        // update
        void put_disc(Point point) {
            flip_discs(point);    // flip flanked points
            // Give control to the other player, update state's attributes
            cur_player = get_next_player(cur_player);
            next_valid_spots = get_valid_spots();
            // Check win
            if (next_valid_spots.size() == 0) {
                vector<Point> copy = next_valid_spots;
                cur_player = get_next_player(cur_player);
                next_valid_spots = get_valid_spots();
                if (next_valid_spots.size() == 0)
                    done = true;
                else {
                    cur_player = get_next_player(cur_player);
                    next_valid_spots = copy;
                }
            }
            round++;
        }
    int weight() {
        int ret = 0, next_player = get_next_player(Player);
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                if (board[i][j] == Player)
                    ret += weight_matrix[i][j];
                else if (board[i][j] == next_player)
                    ret -= weight_matrix[i][j];
            }
        }
        return ret;
    }
    int stability() {
        const int edge = 200;
        int ret = 0;

        if (board[0][0]) {
            int sign = board[0][0] == Player ? 1 : -1;
            for (int row = 1; row < 7; row++)
                if (board[0][0] == board[row][0])
                    ret += sign * edge;
                else break;
            for (int col = 1; col < 7; col++)
                if (board[0][0] == board[0][col])
                    ret += sign * edge;
                else break;
        }
        if (board[0][7]) {
            int sign = board[0][7] == Player ? 1 : -1;
            for (int row = 1; row < 7; row++)
                if (board[0][7] == board[row][7])
                    ret += sign * edge;
                else break;
            for (int col = 6; col > 0; col--)
                if (board[0][7] == board[0][col])
                    ret += sign * edge;
                else break;
        }
        if (board[7][0]) {
            int sign = board[7][0] == Player ? 1 : -1;
            for (int row = 6; row > 0; row--)
                if (board[7][0] == board[row][0])
                    ret += sign * edge;
                else break;
            for (int col = 1; col < 7; col++)
                if (board[7][0] == board[7][col])
                    ret += sign * edge;
                else break;
        }
        if (board[7][7]) {
            int sign = board[7][7] == Player ? 1 : -1;
            for (int row = 6; row > 0; row--)
                if (board[7][7] == board[row][7])
                    ret += sign * edge;
                else break;
            for (int col = 6; col > 0; col--)
                if (board[7][7] == board[7][col])
                    ret += sign * edge;
                else break;
        }

        if (board[0][0] && board[0][7] && (((((board[0][1] == board[0][2]) == board[0][3]) == board[0][4]) == board[0][5]) == board[0][6])) {
            if (board[0][1] == Player)
                ret += edge * 6;
            else if (board[0][1] == get_next_player(Player))
                ret -= edge * 6;
        }

        if (board[0][0] && board[7][0] && (((((board[1][0] == board[2][0]) == board[3][0]) == board[4][0]) == board[5][0]) == board[6][0])) {
            if (board[1][0] == Player)
                ret += edge * 6;
            else if (board[1][0] == get_next_player(Player))
                ret -= edge * 6;
        }

        if (board[0][7] && board[7][7] && (((((board[1][7] == board[2][7]) == board[3][7]) == board[4][7]) == board[5][7]) == board[6][7])) {
            if (board[1][7] == Player)
                ret += edge * 6;
            else if (board[1][7] == get_next_player(Player))
                ret -= edge * 6;
        }

        if (board[7][0] && board[7][7] && (((((board[7][1] == board[7][2]) == board[7][3]) == board[7][4]) == board[7][5]) == board[7][6])) {
            if (board[7][1] == Player)
                ret += edge * 6;
            else if (board[7][1] == get_next_player(Player))
                ret -= edge * 6;
        }
        return ret;
    }
    int mobility() {
        int ret = next_valid_spots.size();
        if (cur_player != Player)
            ret *= -1;
        return ret;
    }
    
    void set_heuristic() {
        heuristic = 0;
        // number of discs
        int num_player_discs, num_oppenent_discs;
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++){
                if (board[i][j] == Player)
                    num_player_discs++;
                else if (board[i][j] == get_next_player(Player))
                    num_oppenent_discs++;
            }
        }
        if (done) {
            heuristic = num_player_discs - num_oppenent_discs;
            return;
        }
        // add more components
        heuristic += weight();
        heuristic += stability();
        heuristic += mobility() * 2 * (1 - (num_player_discs + num_oppenent_discs) / 64);
    }

    private: // methods
        int get_next_player(int player) const { return 3 - player; }
        int get_disc(Point p) const { return board[p.x][p.y]; }   // __getitem__
        void set_disc(Point p, int disc) { board[p.x][p.y] = disc; }   // ___setitem__
        bool is_spot_on_board(Point p) const { return 0 <= p.x && p.x < SIZE && 0 <= p.y && p.y < SIZE; }
        bool is_disc_at(Point p, int disc) const {
            if (!is_spot_on_board(p))  // whether p is on the board
                return false;
            if (get_disc(p) != disc)  // whether <int disc> is at <Point p>
                return false;
            return true;
        }
        bool is_spot_valid(Point center) const {
            if (get_disc(center) != EMPTY)
                return false;
            for (Point dir : directions) {
                // Move along the direction while testing.
                Point p = center + dir;
                // at least one opponent's disc in this direction
                if (!is_disc_at(p, get_next_player(cur_player)))
                    continue;
                // extend until exceed the boundary or meet EMPTY, i.e. can't be flanked
                p = p + dir;
                while (is_spot_on_board(p) && get_disc(p) != EMPTY) {
                    // meet cur_player's disc, i.e. can be flanked
                    if (is_disc_at(p, cur_player))
                        return true;
                    p = p + dir;
                }
            }
            return false;
        }
        // flip all opponent's discs which are flanked
        void flip_discs(Point center) {
            for (Point dir: directions) {
                // Move along the direction while testing.
                Point p = center + dir;
                if (!is_disc_at(p, get_next_player(cur_player)))
                    continue;
                vector<Point> discs({p});   // record the points that should be fliped
                p = p + dir;
                while (is_spot_on_board(p) && get_disc(p) != EMPTY) {
                    // meet cur_player's disc, filp all points in record
                    if (is_disc_at(p, cur_player)) {
                        for (Point s: discs)
                            set_disc(s, cur_player);
                        break;
                    }
                    // record all points that should be fliped
                    discs.push_back(p);
                    p = p + dir;
                }
            }
        }
};

int num_valid_spots;
array<array<int, SIZE>, SIZE> Board;   // read_board 從 state 讀入的版面
vector<Point> Next_valid_spots;    // read_valid_spots 從 state 讀入的最下面的可放列表

void read_board(ifstream& fin) {
    // read player no.
    fin >> Player;
    // read the board (0, 1, 2)
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            fin >> Board[i][j];
        }
    }
}

void read_valid_spots(ifstream& fin) {
    // spots that can filp opponent's disc
    fin >> num_valid_spots;
    int x, y;
    for (int i = 0; i < num_valid_spots; i++) {
        fin >> x >> y;
        Next_valid_spots.push_back(Point(x, y));
    }
}

vector<pair<int, Point>> decisions;
int Minimax(OthelloState cur_state, int depth, int alpha, int beta, int mode) {
    if (depth == 0 || cur_state.done) {
        cur_state.set_heuristic();
        return cur_state.heuristic;
    }
    if (mode == MAXMODE) {
        int value = -INF;
        for (Point spot : cur_state.next_valid_spots) {
            OthelloState next_state = cur_state;
            next_state.put_disc(spot);
            int rec = Minimax(next_state, depth - 1, alpha, beta, MINIMODE);
            value = max(value, rec);
            alpha = max(alpha, value);
            if (cur_state.round == 0)
                decisions.push_back(pair<int, Point>(value, spot));
            if (alpha >= beta)
                break;
        }
        return value;
    } else if (mode == MINIMODE) {
        int value = INF;
        for (Point spot : cur_state.next_valid_spots) {
            OthelloState next_state = cur_state;
            next_state.put_disc(spot);
            int rec = Minimax(next_state, depth - 1, alpha, beta, MAXMODE);
            value = min(value, rec);
            beta = min(beta, value);
            if (cur_state.round == 0)
                decisions.push_back(pair<int, Point>(value, spot));
            if (beta <= alpha)
                break;
        }
        return value;
    }
    return -1;
}

// 隨機從可用清單選一個，放回 action
void write_valid_spot(ofstream& fout) {
    OthelloState cur_othello(Board);
    cur_othello.next_valid_spots = Next_valid_spots;

    int value = Minimax(cur_othello, 1, -INF, INF, (cur_othello.cur_player == Player ? MAXMODE : MINIMODE));
    Point next_disc;
    for (auto it : decisions)
        if (it.first == value) {
            next_disc = it.second;
            break;
        }
    fout << next_disc.x << " " << next_disc.y << endl;
    fout.flush();

    value = Minimax(cur_othello, 5, -INF, INF, (cur_othello.cur_player == Player ? MAXMODE : MINIMODE));
    for (auto it : decisions)
        if (it.first == value) {
            next_disc = it.second;
            break;
        }
    fout << next_disc.x << " " << next_disc.y << endl;
    fout.flush();
}

int main(int, char** argv) {
    // argv[1] fin = state, argv[2] fout = actions
    ifstream fin(argv[1]);
    ofstream fout(argv[2]);
    // read from init state
    read_board(fin);
    read_valid_spots(fin);
    write_valid_spot(fout);
    fin.close();
    fout.close();
    return 0;
}
