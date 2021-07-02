# Alpha Othello Report

### State Value Function

#### 1. Mobility
- Def. 可選擇的合法下一步的數量
- Returns:
    - 我方：可走數量 *1
    - 敵方：可走數量 *(-1)

<details>
<summary>code</summary>

```cpp=
// num of possible next moves
int mobility() { return (cur_player == Player ? 1 : -1) * next_valid_spots.size(); }
```
</details>

---

#### 2. Stability
- Def. 從角落開始沿著邊界延伸的連續的棋子，無法被圍起來
- 空的位置小於 40 才可以計算
- 從四個角落當起點，並分成 col 前進和 row 前進
    - 計算我方的棋子 col by col
    - 計算我方的棋子 row by row
    - 計算敵方的棋子 col by col
    - 計算敵方的棋子 row by row
- col 和 row 各對照一組 Point 型態的向量輔助前進
- 延伸直到斷掉，斷掉後則無法保證穩定度，之後有可能會被圍起來
- Returns: 我方穩定棋子總和 - 敵方穩定棋子總和

![](https://i.imgur.com/OuWCFqZ.jpg)


<details>
<summary>code</summary>

```cpp=
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

    int stable_discs = 0;
    for (int cn = 0; cn < 4; cn++) {
        // check player: moving col by col from each corner
        Point pos = col_corner[cn];
        for (int move = 1; move <= 6; move++) {
            if (get_disc(pos) != Player)
                break;
            stable_discs++;
            pos = pos + col_vector[cn % 2];
        }
        pos = row_corner[cn];
        for (int move = 1; move <= 6; move++) {
            if (get_disc(pos) != Player)
                break;
            stable_discs++;
            pos = pos + row_vector[cn % 2];
        }
        // check opponent: moving col by col from each corner
        pos = col_corner[cn];
        for (int move = 1; move <= 6; move++) {
            if (get_disc(pos) != get_next_player(Player))
                break;
            stable_discs--;
            pos = pos + col_vector[cn % 2];
        }
        pos = row_corner[cn];
        for (int move = 1; move <= 6; move++) {
            if (get_disc(pos) != get_next_player(Player))
                break;
            stable_discs--;
            pos = pos + row_vector[cn % 2];
        }
    }
    return stable_discs;
}
```

</details>

---

#### Weight
- Def. 根據圍棋規則設定位置權重
- 若位置上是我方則 + value；敵方則 - value
- Returns: 整個版面計算出來的 value

<details>
<summary>code</summary>

```cpp=
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
```
</details>

---

- 調配比例之後加總起來變成 Heuxxx value

<details>
<summary>Set Heuristic Value</summary>

```cpp=
void set_heuristic() {
    heuristic = 0;
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
```
</details>

---


### Alpha-beta Pruning

- Base case: 達到指定深度 or 遇到滿版遊戲結束 (leaf)，則回傳這個 state 的 heuxxx value
- Recursive Case:
    1. 複製 cur_state 並依照所有可能下下一步棋，變成 next_state、轉換 player、找出 next_valid_spots
    2. MAXMODE 是我方選擇，故挑選較大的 value；MINIMODE 是敵方選擇，故挑選較小的 value
    3. 選完 value 之後利用 alpha beta 計算出邊界，當 alpha >= beta 時可以省略後面的其他的 next_valid_spots
- 達到 base case 會回傳該 state 的 heuxxx value；base case 往上的所有上層則回傳選擇過的 value
- !!! 當 cur_state.round == 0 的時候，表示已經計算出所有 child state 的 value，已經可以從裡面挑選要下哪一顆棋 (最外層回傳的 value)；所以先把這一層的這些 value 和對應的 spot 存在全域變數 decisions 裡面
- caller 拿到最外層的 value 後即可找到 spot

![](https://i.imgur.com/QApdCWe.jpg)


<details>
<summary>callee</summary>

```cpp=
vector<pair<int, Point>> decisions;
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
                decisions.push_back(pair<int, Point>(rec, spot));
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
                decisions.push_back(pair<int, Point>(rec, spot));
            if (beta <= alpha)
                break;
        }
        return value;
    }
    return 0;
}
```
</details>

<details>
<summary>caller</summary>

```cpp=
void write_valid_spot(ofstream &fout) {
    OthelloState cur_othello(Board);
    // find the heuristic value we should choose this state
    int value = Minimax(cur_othello, PRECISION, -INF, INF, MAXMODE);
    Point next_disc;
    for (pair<int, Point> it : decisions) {
        if (it.first == value)
            next_disc = it.second;
    }
    fout << next_disc.x << " " << next_disc.y << endl;
    fout.flush();
}
```

</details>
