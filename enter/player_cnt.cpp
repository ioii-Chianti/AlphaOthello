#include <algorithm>
#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <map>
#include <utility>
#include <cstdlib>
#include <ctime>
#define INF 99999
#define MAXMODE 1
#define MINIMODE 2
#define PRECISION 6
using namespace std;

struct Point {
    int x, y;
	Point() : Point(0, 0) {}
	Point(float x, float y) : x(x), y(y) {}
	bool operator==(const Point &rhs) const { return x == rhs.x && y == rhs.y; }
	bool operator!=(const Point &rhs) const { return !operator==(rhs); }
	Point operator+(const Point &rhs) const { return Point(x + rhs.x, y + rhs.y); }
	Point operator-(const Point &rhs) const { return Point(x - rhs.x, y - rhs.y); }
};

int Player;
const int SIZE = 8;
array<array<int, SIZE>, SIZE> Board;   // read_board: read board from file 'state'
vector<Point> Next_valid_spots;    // read_valid_spots: read valid spots from file 'state'

class OthelloState {
    public:
        /* const */
        enum SPOT_STATE { EMPTY = 0, BLACK = 1, WHITE = 2 };
        const array<Point, 8> directions{{
            Point(-1, -1), Point(-1, 0), Point(-1, 1),
            Point(0, -1), /*{0, 0}, */ Point(0, 1),
            Point(1, -1), Point(1, 0), Point(1, 1)
        }};

        /* attributes */
        array<array<int, SIZE>, SIZE> board;    // board setting
        vector<Point> next_valid_spots;    // possible moves based on cur_player and board
        int cur_player;   // the Player in this state
        int round;        // level of tree
        bool done;        // game ends or not
        int heuristic;
        int disc_counts[3];

        /* methods */
        // init by Board from read_board and Next_valid_spots from read_valid_spots
        OthelloState(array<array<int, SIZE>, SIZE> arr) {
            for (int i = 0; i < SIZE; i++)
                for (int j = 0; j < SIZE; j++)
                    board[i][j] = arr[i][j];
            cur_player = Player;
            round = 0;
            done = false;
            heuristic = 0;
            next_valid_spots = Next_valid_spots;
            disc_counts[EMPTY] = disc_counts[BLACK] = disc_counts[WHITE] = 0;
        }
        // copy constructor
        OthelloState(const OthelloState &rhs) {
            for (int i = 0; i < SIZE; i++)
                for (int j = 0; j < SIZE; j++)
                    board[i][j] = rhs.board[i][j];
            cur_player = rhs.cur_player;
            round = rhs.round;
            done = rhs.done;
            heuristic = rhs.heuristic;
            next_valid_spots = rhs.next_valid_spots;
            for (int i = 0; i < 3; i++)
                disc_counts[i] = rhs.disc_counts[i];
        }
        vector<Point> get_valid_spots() {
            next_valid_spots.clear();
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

        // for updating next_state
        void put_disc(Point point) {
            flip_discs(point);    // flip flanked points
            // Give control to the other Player, update state's attributes
            cur_player = get_next_player(cur_player);
            next_valid_spots = get_valid_spots();
            // Check win
            if (next_valid_spots.size() == 0) {
                vector<Point> copy = next_valid_spots;
                cur_player = get_next_player(cur_player);
                next_valid_spots = get_valid_spots();
                // game ends := both can't find valid next spots
                if (next_valid_spots.size() == 0)
                    done = true;
                else {
                    cur_player = get_next_player(cur_player);
                    next_valid_spots = copy;
                }
            }
            round++;   // put a disc and go down next round
        }
    // num of possible next moves
    int mobility() { return (cur_player == Player ? 1 : -1) * next_valid_spots.size(); }
    // num of spots along the sides of board from each corner
    int stability() {
        // left-top -> right-top -> left-buttom -> right-buttom
        const Point col_corner[4] = {
            Point(0, 0), Point(0, 7),
            Point(7, 0), Point(7, 7)};
        const Point col_vector[4] = {
            Point(0, 1), Point(0, -1)};
        // left-top -> left-buttom -> right-top -> right-buttom
        const Point row_corner[4] = {
            Point(0, 0), Point(7, 0),
            Point(0, 7), Point(7, 7)};
        const Point row_vector[4] = {
            Point(0, 1), Point(-1, 0)};

        // no need to check stability
        if (disc_counts[EMPTY] >= 40)
            return 0;

        int move = 0, stable_discs = 0;
            // check player: moving col by col from each corner
            for (int cn = 0; cn < 4; cn++) {
                Point pos = col_corner[cn];
                for (move = 1; move <= 6; move++) {
                    if (get_disc(pos) != Player)
                        break;
                    stable_discs++;
                    pos = pos + col_vector[cn % 2];
                }
            }
            // check player: moving row by row from each corner
            for (int cn = 0; cn < 4; cn++) {
                Point pos = row_corner[cn];
                for (move = 1; move <= 6; move++) {
                    if (get_disc(pos) != Player)
                        break;
                    stable_discs++;
                    pos = pos + row_vector[cn % 2];
                }
            }
            // check opponent: moving col by col from each corner
            for (int cn = 0; cn < 4; cn++) {
                Point pos = col_corner[cn];
                for (move = 1; move <= 6; move++) {
                    if (get_disc(pos) != get_next_player(Player))
                        break;
                    stable_discs--;
                    pos = pos + col_vector[cn % 2];
                }
            }
            // check opponent: moving row by row from each corner
            for (int cn = 0; cn < 4; cn++) {
                Point pos = row_corner[cn];
                for (move = 1; move <= 6; move++) {
                    if (get_disc(pos) != get_next_player(Player))
                        break;
                    stable_discs--;
                    pos = pos + row_vector[cn % 2];
                }
            }
        return stable_discs;
    }

    int weight() {
        // plus := player, minus := opponent
        const int weight_matrix[SIZE][SIZE] = {
            {4, -3, 2, 2, 2, 2, -3, 4},
            {-3, -4, -1, -1, -1, -1, -4, -3},
            {2, -1, 1, 0, 0, 1, -1, 2},
            {2, -1, 0, 1, 1, 0, -1, 2},
            {2, -1, 0, 1, 1, 0, -1, 2},
            {2, -1, 1, 0, 0, 1, -1, 2},
            {-3, -4, -1, -1, -1, -1, -4, -3},
            {4, -3, 2, 2, 2, 2, -3, 4}};

        int ret = 0, opponent = get_next_player(Player);
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                if (board[i][j] == Player)
                    ret += weight_matrix[i][j];
                else if (board[i][j] == opponent)
                    ret -= weight_matrix[i][j];
            }
        }
        return ret;
    }
    
    void set_heuristic() {
        // count the number of discs
        int opponent = get_next_player(Player);
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++){
                if (board[i][j] == Player)
                    disc_counts[Player]++;
                else if (board[i][j] == opponent)
                    disc_counts[opponent]++;
                else
                    disc_counts[EMPTY]++;
            }
        }
        int bias = disc_counts[Player] - disc_counts[opponent];
        if (done) {
            heuristic = bias;
            return;
        }
        heuristic = weight() + 2 * mobility() + 2 * stability();
    }

    private: // methods from main.cpp
        int get_next_player(int Player) const { return 3 - Player; }
        int get_disc(Point p) const { return board[p.x][p.y]; }
        void set_disc(Point p, int disc) { board[p.x][p.y] = disc; }
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

void read_board(ifstream &fin) {
    // read Player no.
    fin >> Player;
    // read the board (0, 1, 2)
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            fin >> Board[i][j];
}

void read_valid_spots(ifstream &fin) {
    // read spots that can filp opponent's disc
    int num_valid_spots;
    fin >> num_valid_spots;
    int x, y;
    for (int i = 0; i < num_valid_spots; i++) {
        fin >> x >> y;
        Next_valid_spots.push_back(Point(x, y));
    }
}

map<int, Point> decisions;
int Minimax(OthelloState cur_state, int depth, int alpha, int beta, int mode) {
    if (depth == 0 || cur_state.done) {
        cur_state.set_heuristic();
        return cur_state.heuristic;
    } 
    if (mode == MAXMODE) {
        int value = -INF;
        for (Point spot : cur_state.next_valid_spots) {
            // 1. set up next state_state
            OthelloState next_state = cur_state;
            next_state.put_disc(spot);
            // 2. the heuristic value chosen by next_state
            int rec = Minimax(next_state, depth - 1, alpha, beta, MINIMODE);
            value = max(value, rec);
            alpha = max(alpha, value);
            // 3. if this state is the outer state which needs to choose next move,
            //    then rec will be one of the suitable decision
            //    so put the heuristic value and corresponding spot into map.
            if (cur_state.round == 0)
                decisions[rec] = spot;
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
                decisions[rec] = spot;
            if (beta <= alpha)
                break;
        }
        return value;
    }
    return 0;
}

// output back to action
void write_valid_spot(ofstream &fout) {
    OthelloState cur_othello(Board);
    // find the heuristic value we should choose this state
    int value = Minimax(cur_othello, 1, -INF, INF, MAXMODE);
    Point next_disc = decisions[value];
    fout << next_disc.x << " " << next_disc.y << endl;
    fout.flush();
}

int main(int, char **argv) {
    ifstream fin(argv[1]);   // state
    ofstream fout(argv[2]);  // action
    read_board(fin);
    read_valid_spots(fin);
    write_valid_spot(fout);
    fin.close();
    fout.close();
    return 0;
}
