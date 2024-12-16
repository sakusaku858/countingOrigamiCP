/*
45°格子パターン展開図の数え上げをZDDを用いて行う。
36個の局所平坦条件を満たす内部頂点の組み合わせによって探索する。
したがって、通常のZDDと異なり、各ノードは最大36個の子を持つ。
*/

#include <stdlib.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

using namespace std;
using ULL = unsigned long long;

enum Direction {
    UP,
    UPPER_RIGHT,
    RIGHT,
    LOWER_RIGHT,
    DOWN,
    LOWER_LEFT,
    LEFT,
    UPPER_LEFT
};

Direction DIRECTIONS[8] = {UP,   UPPER_RIGHT, RIGHT, LOWER_RIGHT,
                           DOWN, LOWER_LEFT,  LEFT,  UPPER_LEFT};

#define UNDEFINED -1
#define USE 1
#define NOT_USE 0

// 局所平坦条件を満たす頂点
vector<vector<int>> TILE;

int M, N;  // 格子の縦横のサイズ
int E, V;  // 辺の本数, 内部頂点の数

// 高速化のため、一度計算したら以後変化しない変数をグローバル変数とする
vector<vector<int>> VERTEX_LIST;  // 頂点iに接続する辺のリスト
vector<set<int>> FRONTIERS;  // 頂点iを置いたときのフロンティア頂点

// 接続する辺を返す
int calc_connected_edge_index(int x, int y, Direction dir) {
    switch (dir) {
        case UP:
            return M * N * 2 + (M - 1) * N + x + y * (N - 1);
        case UPPER_RIGHT:
            return M * N + x + y * N + 1;
        case RIGHT:
            return M * N * 2 + x + 1 + y * N;
        case LOWER_RIGHT:
            return (x + 1) + (y + 1) * N;
        case DOWN:
            return M * N * 2 + (M - 1) * N + x + (y + 1) * (N - 1);
        case LOWER_LEFT:
            return M * N + x + (y + 1) * N;
        case LEFT:
            return M * N * 2 + x + y * N;
        case UPPER_LEFT:
            return x + y * N;
    }
    cout << "in function get_conected_edge, dir must be positive number."
         << endl;
    exit(0);
}

void initialize(int m, int n) {
    M = m;
    N = n;
    E = 4 * M * N - M - N;
    V = (M - 1) * (N - 1);

    // TILEの読み込み
    ifstream file("tiles.csv");
    string line;
    while (getline(file, line)) {
        vector<int> row;
        stringstream ss(line);
        string cell;
        while (getline(ss, cell, ',')) {
            row.push_back(stoi(cell));
        }
        TILE.push_back(row);
    }

    // VERTEX_LISTの設定
    VERTEX_LIST = vector<vector<int>>(V, vector<int>(8));
    for (int i = 0; i < V; i++) {
        int x = i % (N - 1);
        int y = i / (N - 1);
        for (Direction dir : DIRECTIONS) {
            VERTEX_LIST[i][dir] = calc_connected_edge_index(x, y, dir);
        }
    }

    // FRONTIERSの設定
    // i番目の頂点を置いたとき
    // i番目以前の頂点はすでにフロンティアでないとする
    set<int> frontier_set;
    set<int> visited;
    for (int i = 0; i < V; i++) {
        visited.insert(i);
        int x = i % (N - 1);
        int y = i / (N - 1);
        if (x != N - 2) frontier_set.insert(i + 1);
        if (y != M - 2) frontier_set.insert(i + N - 1);
        set<int> difference;
        set_difference(frontier_set.begin(), frontier_set.end(),
                       visited.begin(), visited.end(),
                       inserter(difference, difference.begin()));
        FRONTIERS.push_back(difference);
    }
}

class CP {
    vector<int> edges;  // edges[i] : 辺iが折り筋として使われるか

   public:
    CP() { edges = vector<int>(E, UNDEFINED); }

    void set_edge(int e, int v) { edges[e] = v; }

    vector<int> get_connected_edge_values(int vertex) {
        vector<int> edge_values;
        for (Direction dir : DIRECTIONS) {
            int edge_index = VERTEX_LIST[vertex][dir];
            edge_values.push_back(edges[edge_index]);
        }
        return edge_values;
    }

    bool can_put(int vertex, int tile) {
        vector<int> connected_edge_values = get_connected_edge_values(vertex);
        for (Direction dir : DIRECTIONS) {
            if (connected_edge_values[dir] == UNDEFINED)
                continue;
            else if (connected_edge_values[dir] != TILE[tile][dir])
                return false;
        }
        return true;
    }

    void put(int vertex, int tile) {
        for (Direction dir : DIRECTIONS) {
            int e = VERTEX_LIST[vertex][dir];
            edges[e] = TILE[tile][dir];
        }
    }

    bool is_frontier(int v) {
        int count_undefined = 0;
        for (Direction d : DIRECTIONS) {
            if (edges[VERTEX_LIST[v][d]] == UNDEFINED) count_undefined++;
        }
        return count_undefined > 0 && count_undefined < 8;
    }

    vector<int> get_frontier() {
        vector<int> frontier;
        for (int i = 0; i < V; i++) {
            if (is_frontier(i)) {
                for (auto dir : DIRECTIONS) {
                    frontier.push_back(edges[VERTEX_LIST[i][dir]]);
                }
            }
        }
        return frontier;
    }

    bool is_same_frontier(CP *cp) {
        vector<int> f1 = this->get_frontier();
        vector<int> f2 = cp->get_frontier();
        return f1.size() == f2.size() &&
               equal(f1.begin(), f1.end(), f2.begin());
    }

    void print() {
        map<int, string> str = {{UNDEFINED, "-"}, {USE, "1"}, {NOT_USE, "0"}};
        for (auto e : edges) cout << str[e] << " ";
        cout << endl;
    }

    // 同一方向のエッジを90°回転する
    vector<int> rotate_edge_block(vector<int> block, int width) {
        int height = block.size() / width;
        vector<int> ret(block.size());
        int x1, y1, x2, y2, i1, i2;
        for (int i1 = 0; i1 < block.size(); i1++) {
            auto [x1, y1] = toCoordinate(i1, width);
            auto [x2, y2] = make_tuple(y1, width - x1 - 1);
            i2 = toIndex(x2, y2, height);
            ret[i2] = block[i1];
        }
        return ret;
    }

    // 大なりイコール
    bool geq(CP *cp) {
        for (int i = 0; i < E; i++) {
            if (this->edges[i] > cp->edges[i]) return true;
            if (this->edges[i] < cp->edges[i]) return false;
        }
        return true;
    }

    // 1方向の辺を抜き出したブロックに対して
    // 左右反転させたブロックを返す
    vector<int> flipBlock(vector<int> block, int width) {
        vector<int> ret(block.size());
        int i1, i2, x1, x2, y1, y2;
        for (i1 = 0; i1 < block.size(); i1++) {
            auto [x1, y1] = toCoordinate(i1, width);
            auto [x2, y2] = make_tuple(width - x1 - 1, y1);
            i2 = toIndex(x2, y2, width);
            ret[i2] = block[i1];
        }
        return ret;
    }

    CP *filpCP() {
        vector<int> diag1, diag2, hol, ver, diag12, diag22, hol2, ver2;
        diag1 = get_diagonal_edges1();
        diag2 = get_diagonal_edges2();
        hol = get_Horizontal_edges();
        ver = get_vertial_edges();
        diag12 = flipBlock(diag2, N);
        diag22 = flipBlock(diag1, N);
        hol2 = flipBlock(hol, N);
        ver2 = flipBlock(ver, N - 1);
        diag12.insert(diag12.end(), diag22.begin(), diag22.end());
        diag12.insert(diag12.end(), hol2.begin(), hol2.end());
        diag12.insert(diag12.end(), ver2.begin(), ver2.end());
        CP *f = new CP();
        f->edges = diag12;
        return f;
    }

    CP *rotateCP(int angle) {
        CP *r = new CP();
        int v = countVerticalEdge();
        int h = countHorizontalEdge();
        int d = countdiagonalEdge();
        vector<int> diag1 = get_diagonal_edges1();
        vector<int> diag2 = get_diagonal_edges2();
        vector<int> hol = get_Horizontal_edges();
        vector<int> ver = get_vertial_edges();
        for (int i = 0; i < angle / 90; i++) {
            vector<int> diag12, diag22, hol2, ver2;
            diag12 = rotate_edge_block(diag2, N);
            diag22 = rotate_edge_block(diag1, N);
            hol2 = rotate_edge_block(ver, N);
            ver2 = rotate_edge_block(hol, N);
            diag1 = diag12;
            diag2 = diag22;
            hol = hol2;
            ver = ver2;
        }
        diag1.insert(diag1.end(), diag2.begin(), diag2.end());
        diag1.insert(diag1.end(), hol.begin(), hol.end());
        diag1.insert(diag1.end(), ver.begin(), ver.end());
        r->edges = diag1;
        return r;
    }

    vector<int> get_diagonal_edges1() {
        return vector<int>(edges.begin(),
                           edges.begin() + countdiagonalEdge() / 2);
    }

    vector<int> get_diagonal_edges2() {
        return vector<int>(edges.begin() + countdiagonalEdge() / 2,
                           edges.begin() + countdiagonalEdge());
    }

    vector<int> get_Horizontal_edges() {
        return vector<int>(
            edges.begin() + countdiagonalEdge(),
            edges.begin() + countdiagonalEdge() + countHorizontalEdge());
    }

    vector<int> get_vertial_edges() {
        return vector<int>(
            edges.begin() + countdiagonalEdge() + countHorizontalEdge(),
            edges.end());
    }

    int countVerticalEdge() { return (N - 1) * M; }
    int countHorizontalEdge() { return N * (M - 1); }
    int countdiagonalEdge() { return 2 * M * N; }

    tuple<int, int> toCoordinate(int index, int width) {
        return make_tuple(index % width, index / width);
    }

    int toIndex(int x, int y, int width) { return x + y * width; }
};

class Node {
    CP cp;
    int label;  // 置くかどうかを選ぶ頂点

   public:
    ULL count_route = 1;
    vector<Node *> children;

    Node() { label = 0; }

    void create_children(Node *node0) {
        for (int i = 0; i < 36; i++) {
            Node *child = copy();
            child->label = this->label + 1;
            child->count_route = this->count_route;
            if (child->cp.can_put(label, i)) {
                child->cp.put(label, i);
                children.push_back(child);
            } else {
                children.push_back(node0);
                node0->count_route += this->count_route;
                delete (child);
            }
        }
    }

    Node *copy() {
        Node *c = new Node();
        c->cp = this->cp;
        c->label = this->label;
        return c;
    }

    void set_edge(int e, int state) { cp.set_edge(e, state); }

    void print(bool draw_children = false) {
        cout << "label : " << label << endl;
        cout << "route : " << count_route << endl;
        cp.print();
        cout << "children : " << children.size() << endl << endl;
        if (draw_children)
            for (auto child : children) child->print();
    }

    bool is_finished() { return this->label >= V; }

    void set_count_route(int count) { this->count_route = count; }

    bool has_same_frontier(Node *n) {
        return this->cp.is_same_frontier(&n->cp);
    }

    CP *get_cp() { return &cp; }
};

class MyQueue {
    list<Node *> lst;

   public:
    void push(Node *n) { lst.push_back(n); }

    Node *pop() {
        Node *front = lst.front();
        lst.pop_front();
        return front;
    }

    Node *get_same_frontier_node(Node *n) {
        for (auto node : lst) {
            if (n->has_same_frontier(node)) return node;
        }
        return NULL;
    }

    bool empty() { return lst.empty(); }
};

class ZDD {
   public:
    Node *node0;
    Node *node1;
    Node *head;
    ULL count = 0;

    int push_count = 0;

    void createZDD() {
        head = new Node();
        node0 = new Node();
        node0->count_route = 0;
        node1 = new Node();
        node1->count_route = 0;

        MyQueue que;
        que.push(head);

        while (!que.empty()) {
            Node *n = que.pop();

            if (n->is_finished()) {
                count += n->count_route;
                continue;
            }

            n->create_children(node0);
            for (int i = 0; i < n->children.size(); i++) {
                Node *child = n->children[i];
                if (child == node0) continue;
                Node *same = que.get_same_frontier_node(child);
                if (same != NULL) {
                    marge(n, i, same);
                    continue;
                }
                que.push(child);
            }
        }
    }

    void marge(Node *parent, int child_index, Node *same_node) {
        Node *child = parent->children[child_index];
        same_node->count_route += child->count_route;
        parent->children[child_index] = same_node;
        delete (child);
    }

    list<CP *> get_flat_CPs() {}
};

int main(void) {
    initialize(3, 3);

    ZDD *zdd = new ZDD();
    zdd->createZDD();
    cout << "No Considering Symmetry : " << zdd->count * 16 << endl;

    // ZDDをもとに平坦に折れる展開図をすべて生成し
    // 回転して同一になる展開図を除去する
    // すべての展開図に対し、大小関係を定義し
    // 回転して同一になる展開図の中で最大または最小のもののみをカウントする
    return 0;
}