#include <iostream>
#include <fstream>
#include <sstream>
#include <array>
#include <vector>
#include <cstdlib>
#include <ctime>
#define DEBUG 1
#define INF 99999999
#define DEPTH 2
#define MAXLEVEL 1
#define MINLEVEL 0
using namespace std;

struct Point {
    int x, y;
	Point() : Point(0, 0) {}    // default: Point(0, 0)
	Point(float x, float y) : x(x), y(y) {}
	bool operator==(const Point& rhs) const { return x == rhs.x && y == rhs.y; }
	bool operator!=(const Point& rhs) const { return !operator==(rhs); }
	Point operator+(const Point& rhs) const { return Point(x + rhs.x, y + rhs.y); }
	Point operator-(const Point& rhs) const { return Point(x - rhs.x, y - rhs.y); }
    Point operator=(const Point &rhs) const {
        Point ret;
        ret.x = rhs.x;
        ret.y = rhs.y;
        return ret;
    }
};

int player;
const int SIZE = 8;
// array<array<int, SIZE>, SIZE> board;
// vector<Point> next_valid_spots;

class OthelloBoard {
    public:
        // constants
        enum SPOT_STATE { EMPTY = 0, BLACK = 1, WHITE = 2 };
        static const int SIZE = 8;
        const array<Point, 8> directions{{
            Point(-1, -1), Point(-1, 0), Point(-1, 1),
            Point(0, -1), /*{0, 0}, */Point(0, 1),
            Point(1, -1), Point(1, 0), Point(1, 1)
        }};
        // attributes
        array<array<int, SIZE>, SIZE> board;
        vector<Point> next_valid_spots;
        array<int, 3> disc_count;
        int heuristic_value;
        int cur_player;
        bool done;   // game ended or not
        int winner;
        Point next_disc;

    private:
        int get_next_player(int player) const { return 3 - player; }    // convert 1 and 2
        bool is_spot_on_board(Point p) const { return 0 <= p.x && p.x < SIZE && 0 <= p.y && p.y < SIZE; }
        int get_disc(Point p) const { return board[p.x][p.y]; }
        void set_disc(Point p, int disc) { board[p.x][p.y] = disc; }
        bool is_disc_at(Point p, int disc) const {    // is disc at p
            if (!is_spot_on_board(p))
                return false;
            if (get_disc(p) != disc)
                return false;
            return true;
        }
        // can filp opponent's disc or not
        bool is_spot_valid(Point center) const {
            if (get_disc(center) != EMPTY)
                return false;
            for (Point dir: directions) {
                // Move along the direction while testing.
                Point p = center + dir;
                if (!is_disc_at(p, get_next_player(cur_player)))
                    continue;    // 以 center 延伸的方向不是敵人的棋子
                
                // 繼續延伸，若遇到自己表示可以將該方向的直線圍起來
                // 否則延伸到超出範圍會遇到 EMPTY 則 invalid
                p = p + dir;
                while (is_spot_on_board(p) && get_disc(p) != EMPTY) {
                    if (is_disc_at(p, cur_player))
                        return true;
                    p = p + dir;
                }
            }
            return false;
        }
        void flip_discs(Point center) {
            for (Point dir: directions) {
                // Move along the direction while testing.
                Point p = center + dir;
                if (!is_disc_at(p, get_next_player(cur_player)))
                    continue;
                vector<Point> discs({p});
                p = p + dir;
                while (is_spot_on_board(p) && get_disc(p) != EMPTY) {
                    if (is_disc_at(p, cur_player)) {
                        for (Point s: discs) {
                            set_disc(s, cur_player);
                        }
                        disc_count[cur_player] += discs.size();
                        disc_count[get_next_player(cur_player)] -= discs.size();
                        break;
                    }
                    discs.push_back(p);
                    p = p + dir;
                }
            }
        }
    public:
        OthelloBoard() { reset(); }
        OthelloBoard operator=(OthelloBoard& othello) {
            OthelloBoard ret;
            for (int i = 0; i < SIZE; i++)
                for (int j = 0; j < SIZE; j++)
                    this->board[i][j] = othello.board[i][j];
            ret.next_valid_spots = othello.next_valid_spots;
            ret.disc_count = othello.disc_count;
            ret.heuristic_value = othello.heuristic_value;
            ret.cur_player = othello.cur_player;
            ret.done = othello.done;
            ret.winner = othello.winner;
            ret.next_disc = othello.next_disc;
            return ret;
        }
        void reset() {
            for (int i = 0; i < SIZE; i++) {
                for (int j = 0; j < SIZE; j++) {
                    board[i][j] = EMPTY;
                }
            }
            board[3][4] = board[4][3] = BLACK;
            board[3][3] = board[4][4] = WHITE;
            cur_player = BLACK;
            disc_count[EMPTY] = 60;
            disc_count[BLACK] = 2;
            disc_count[WHITE] = 2;
            next_valid_spots = get_valid_spots();
            done = false;
            winner = -1;
            next_disc = Point(-1, -1);
            calculate_heuristic();
        }
        vector<Point> get_valid_spots() const {
            vector<Point> valid_spots;
            for (int i = 0; i < SIZE; i++) {
                for (int j = 0; j < SIZE; j++) {
                    Point p = Point(i, j);
                    if (board[i][j] != EMPTY)
                        continue;
                    if (is_spot_valid(p))
                        valid_spots.push_back(p);
                }
            }
            return valid_spots;
        }
        void calculate_heuristic() {
            // !!!
            if (!done)
                heuristic_value = disc_count[cur_player] - disc_count[get_next_player(cur_player)];
            else if (winner == player)
                heuristic_value = disc_count[cur_player] - disc_count[get_next_player(cur_player)] + INF;
            else
                heuristic_value = disc_count[cur_player] - disc_count[get_next_player(cur_player)] - INF;
        }
        bool put_disc(Point p) {
            if(!is_spot_valid(p)) {
                winner = get_next_player(cur_player);
                done = true;
                return false;
            }
            set_disc(p, cur_player);
            disc_count[cur_player]++;
            disc_count[EMPTY]--;
            flip_discs(p);
            // Give control to the other player.
            cur_player = get_next_player(cur_player);
            next_valid_spots = get_valid_spots();
            // Check Win
            if (next_valid_spots.size() == 0) {
                cur_player = get_next_player(cur_player);
                next_valid_spots = get_valid_spots();
                if (next_valid_spots.size() == 0) {
                    // Game ends
                    done = true;
                    int white_discs = disc_count[WHITE];
                    int black_discs = disc_count[BLACK];
                    if (white_discs == black_discs) winner = EMPTY;
                    else if (black_discs > white_discs) winner = BLACK;
                    else winner = WHITE;
                }
            }
            calculate_heuristic();
            return true;
        }
};

// current state
OthelloBoard cur_othello;

void read_board(ifstream& fin) {
    fin >> player;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            fin >> cur_othello.board[i][j];
        }
    }
}

void read_valid_spots(ifstream& fin) {
    int n_valid_spots;
    fin >> n_valid_spots;
    int x, y;
    for (int i = 0; i < n_valid_spots; i++) {
        fin >> x >> y;
        cur_othello.next_valid_spots.push_back({x, y});
    }
}

OthelloBoard Minimax(OthelloBoard cur_state, int depth, int mode, int alpha, int beta) {
    if (cur_state.done || !depth)
        return cur_state;

    OthelloBoard pivot;
    if (mode == MAXLEVEL) {
        pivot.heuristic_value = -INF;
        for (Point spot : cur_state.next_valid_spots) {
            OthelloBoard next_state = cur_state;
            next_state.put_disc(spot);
            OthelloBoard next_heuristic = Minimax(next_state, depth - 1, MINLEVEL, alpha, beta);
            if (pivot.heuristic_value < next_heuristic.heuristic_value) {
                pivot = next_heuristic;
                cur_state.next_disc = spot;
            }
            if (alpha < pivot.heuristic_value)
                alpha = pivot.heuristic_value;
            if (beta <= alpha)
                break;
        }
    } else if (mode == MINLEVEL) {
        pivot.heuristic_value = INF;
        for (Point spot : cur_state.next_valid_spots) {
            OthelloBoard next_state = cur_state;
            next_state.put_disc(spot);
            OthelloBoard next_heuristic = Minimax(next_state, depth - 1, MAXLEVEL, alpha, beta);
            if (pivot.heuristic_value < next_heuristic.heuristic_value) {
                pivot = next_heuristic;
                cur_state.next_disc = spot;
            }
            if (beta > pivot.heuristic_value)
                beta = pivot.heuristic_value;
            if (beta <= alpha)
                break;
        }
    }
}

void write_valid_spot(ofstream& fout) {
    // int n_valid_spots = next_valid_spots.size();
    srand(time(NULL));
    // Choose random spot. (Not random uniform here)
    // int index = (rand() % n_valid_spots);
    Point p = cur_othello.next_disc;

    // Remember to flush the output to ensure the last action is written to file.
    fout << p.x << " " << p.y << endl;
    fout.flush();

    int alpha = -INF, beta = INF;
    OthelloBoard next_othello = Minimax(cur_othello, DEPTH, MAXLEVEL, alpha, beta);
    p = cur_othello.next_disc;
    fout << p.x << " " << p.y << endl;
    fout.flush();
}

int main(int, char** argv) {
    ifstream fin(argv[1]);
    ofstream fout(argv[2]);
    read_board(fin);
    read_valid_spots(fin);
    cur_othello.cur_player = player;
    if (DEBUG) { cout << "-- player " << player << '\n'; }
    cur_othello.calculate_heuristic();
    write_valid_spot(fout);
    fin.close();
    fout.close();
    return 0;
}
