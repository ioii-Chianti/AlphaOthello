#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <cstdlib>
#include <ctime>

struct Point {
    int x, y;
	Point() : Point(0, 0) {}
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
std::array<std::array<int, SIZE>, SIZE> board;   // state 讀入的版面
std::vector<Point> next_valid_spots;    // state 讀入的最下面的可放列表

// 從 state 內讀入
void read_board(std::ifstream& fin) {
    // read player no.
    fin >> player;
    // read the board (0, 1, 2)
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            fin >> board[i][j];
        }
    }
}

void read_valid_spots(std::ifstream& fin) {
    // spots that can filp opponent's disc
    int num_valid_spots;
    fin >> num_valid_spots;
    int x, y;
    for (int i = 0; i < num_valid_spots; i++) {
        fin >> x >> y;
        next_valid_spots.push_back({x, y});
    }
}

// 隨機從可用清單選一個，放回 action
void write_valid_spot(std::ofstream& fout) {
    int num_valid_spots = next_valid_spots.size();
    srand(time(NULL));
    // Choose random spot. (Not random uniform here)
    int index = (rand() % num_valid_spots);
    Point p = next_valid_spots[index];
    // Remember to flush the output to ensure the last action is written to file.
    fout << p.x << " " << p.y << std::endl;
    fout.flush();
}

int main(int, char** argv) {
    // argv[1] fin = state, argv[2] fout = actions
    std::ifstream fin(argv[1]);
    std::ofstream fout(argv[2]);
    // read from init state
    read_board(fin);
    read_valid_spots(fin);
    write_valid_spot(fout);
    fin.close();
    fout.close();
    return 0;
}
