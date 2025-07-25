#include <algorithm>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

using namespace std;

struct Item {
    int chip;
    int generator;
};

struct State {
    int elevator;
    vector<string> material;
    vector<pair<int, int>> items;  // (chip_floor, gen_floor)
    int steps;

    bool operator==(const State &other) const {
        return elevator == other.elevator && items == other.items;
    }

    bool operator<(const State &other) const {
        return elevator < other.elevator || items < other.items;  // Vergleichen Sie die relevanten Mitglieder
    }

    bool is_goal(int top_floor) const {
        for (const auto &[chip, gen] : items)
            if (chip != top_floor || gen != top_floor) return false;
        return true;
    }

    bool is_valid() const {
        for (int f = 0; f < 4; ++f) {
            bool has_generator = false;
            for (const auto &[chip, gen] : items)
                if (gen == f) has_generator = true;

            for (const auto &[chip, gen] : items) {
                if (chip == f && chip != gen && has_generator)
                    return false;  // chip is fried
            }
        }
        return true;
    }

    // Canonical form for hashing: sort relative positions
    void normalize() {
        for (auto &[chip, gen] : items) {
            if (chip > gen) swap(chip, gen);  // optional: canonical order
        }
        sort(items.begin(), items.end());
    }
};

ostream &operator<<(ostream &os, const State &s) {
    vector<map<string, pair<bool, bool>>> floors(4);
    int j = 0;
    for (const auto &[chip, gen] : s.items) {
        floors[chip][s.material[j]].first = true;
        floors[gen][s.material[j++]].second = true;
    }
    for (int i = 3; i >= 0; --i) {
        os << (i == s.elevator ? "[E] " : "[ ] ") << i << ": ";
        for (auto it = floors[i].begin(); it != floors[i].end(); ++it) {
            os << it->first << " [" << (it->second.first ? "C" : "-") << "|" << (it->second.second ? "G" : "-") << "]";
            if (next(it) != floors[i].end()) {
                os << ", ";
            }
        }
        os << endl;
    }
    return os;
}

struct StateHasher {
    size_t operator()(const State &s) const {
        size_t h = s.elevator;
        for (const auto &[chip, gen] : s.items) {
            h ^= chip * 31 + gen * 131;
        }
        return h;
    }
};

// Heuristic: weighted sum of distances from top floor
int heuristic(const State &s, int top_floor) {
    int h = 0;
    for (const auto &[chip, gen] : s.items) {
        h += (top_floor - chip);
        h += (top_floor - gen);
    }
    return h;
}

int solve_astar(State start, int top_floor) {
    using QueueItem = tuple<int, int, State>;  // (estimated cost, steps, state)

    unordered_set<State, StateHasher> seen;
    priority_queue<QueueItem, vector<QueueItem>, greater<>> pq;

    start.normalize();
    pq.emplace(heuristic(start, top_floor), 0, start);
    cout << endl
         << start;
    while (!pq.empty()) {
        auto [est, steps, state] = pq.top();
        pq.pop();

        if (state.is_goal(top_floor)) return steps;

        if (!seen.insert(state).second) continue;

        // Try combinations of 1 or 2 items
        vector<int> indices;
        for (int i = 0; i < state.items.size(); ++i) {
            if (state.items[i].first == state.elevator) indices.push_back(i);
            if (state.items[i].second == state.elevator) indices.push_back(~i);  // ~i: encode gen
        }

        vector<vector<int>> moves;
        for (int i = 0; i < indices.size(); ++i) {
            moves.push_back({indices[i]});
            for (int j = i + 1; j < indices.size(); ++j) {
                moves.push_back({indices[i], indices[j]});
            }
        }

        for (int dir : {-1, 1}) {
            int new_floor = state.elevator + dir;
            if (new_floor < 0 || new_floor > top_floor) continue;

            for (const auto &mv : moves) {
                State next = state;
                next.elevator = new_floor;
                for (int id : mv) {
                    int idx = abs(id);
                    if (id >= 0)
                        next.items[idx].first = new_floor;
                    else
                        next.items[idx].second = new_floor;
                }
                cout << state << endl;
                cout << next << endl;
                if (!next.is_valid()) continue;

                next.steps = steps + 1;
                cout << next << endl;
                next.normalize();
                if (!seen.count(next)) {
                    pq.emplace(next.steps + heuristic(next, top_floor), next.steps, next);
                }
            }
        }
    }

    return -1;  // no solution
}

int main() {
    // Sample input: 2 elements
    State start;
    start.elevator = 0;
    start.steps = 0;
    start.items = {
        {0, 0},
        {2, 1},
        {2, 1},
        {2, 1},
        {2, 1}};
    start.material = {"promethium", "cobalt", "curium", "ruthenium", "plutonium"};  // H: chip, L: generator
    int top_floor = 3;
    int result = solve_astar(start, top_floor);
    cout << "Minimum steps: " << result << endl;

    return 0;
}
