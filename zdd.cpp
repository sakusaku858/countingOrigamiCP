/*
45°格子パターン展開図の数え上げをZDDを用いて行う。
36個の局所平坦条件を満たす内部頂点の組み合わせによって探索する。
したがって、通常のZDDと異なり、各ノードは最大36個の子を持つ。
*/

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <stdlib.h>
#include <string>
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

int M, N; // 格子の縦横のサイズ
int E, V; // 辺の本数, 内部頂点の数

// 高速化のため、一度計算したら以後変化しない変数をグローバル変数とする
vector<vector<int>> VERTEX_LIST; // 頂点iに接続する辺のリスト
vector<set<int>> FRONTIERS; // 頂点iを置いたときのフロンティア頂点

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
        if (x != N - 2)
            frontier_set.insert(i + 1);
        if (y != M - 2)
            frontier_set.insert(i + N - 1);
        set<int> difference;
        set_difference(frontier_set.begin(), frontier_set.end(),
                       visited.begin(), visited.end(),
                       inserter(difference, difference.begin()));
        FRONTIERS.push_back(difference);
    }
}

class CP {
    vector<int> edges; // edges[i] : 辺iが折り筋として使われるか

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
            if (edges[VERTEX_LIST[v][d]] == UNDEFINED)
                count_undefined++;
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
        for (auto e : edges)
            cout << str[e] << " ";
        cout << endl;
    }
};

class Node {
    CP cp;
    int label; // 置くかどうかを選ぶ頂点

  public:
    ULL count_route = 1;
    vector<Node *> children;

    Node() { label = 0; }

    void create_children() {
        for (int i = 0; i < 36; i++) {
            Node *child = copy();
            child->label = this->label + 1;
            child->count_route = this->count_route;
            // 置けなかったらnode0に接続したほうが良い
            if (child->cp.can_put(label, i)) {
                child->cp.put(label, i);
                children.push_back(child);
            } else {
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
            for (auto child : children)
                child->print();
    }

    bool is_finished() { return this->label >= V; }

    void set_count_route(int count) { this->count_route = count; }

    bool has_same_frontier(Node *n) {
        return this->cp.is_same_frontier(&n->cp);
    }
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
            if (n->has_same_frontier(node))
                return node;
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

        MyQueue que;
        que.push(head);

        while (!que.empty()) {
            Node *n = que.pop();

            if (n->is_finished()) {
                count += n->count_route;
                continue;
            }

            n->create_children();
            for (int i = 0; i < n->children.size(); i++) {
                Node *child = n->children[i];
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
};

int main(void) {
    initialize(4, 4);

    ZDD *zdd = new ZDD();
    zdd->createZDD();
    cout << "No Considering Symmetry : " << zdd->count * 16 << endl;

    // ZDDをもとに平坦に折れる展開図をすべて生成し
    // 回転して同一になる展開図を除去する
    // すべての展開図に対し、大小関係を定義し
    // 回転して同一になる展開図の中で最大または最小のもののみをカウントする
    return 0;
}