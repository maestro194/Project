/*
   Copyright 2011 Kuang-che Wu

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#ifndef __NAUGHTY_H__
#define __NAUGHTY_H__

#include <stdint.h>
#include <vector>

#ifdef LARGE_BOARD
#define BGET(x, idx) ((x) & (1llu<< (idx)))
#define BSET(x, idx) ((x) |= 1llu << (idx))
#define BCLR(x, idx) ((x) &= ~(1llu << (idx)))
#else
#define BGET(x, idx) ((x) & (1<< (idx)))
#define BSET(x, idx) ((x) |= 1 << (idx))
#define BCLR(x, idx) ((x) &= ~(1 << (idx)))
#endif

// A has more bits than B
#define KNOW_MORE(A, B) ((A)&~(B))
#define IS_2xxN(a)      ((a) && ((a)&((a)-1))==0)

typedef uint8_t idx_t;

struct LineCacheKey {
    char dir_idx;
    Line known[2];
    bool operator==(const LineCacheKey &a) const {
	return dir_idx==a.dir_idx && known[0]==a.known[0] && known[1]==a.known[1];
    }
} __attribute__ ((packed));

struct LineCacheEntry {
    // input
    LineCacheKey key;
    // output
    Line new_known[2];
    //int count;
} __attribute__ ((packed));

enum DirtyLevel {
    DIRTY_NONE,
    DIRTY_LINE_FAST,
    DIRTY_LINE_ALL,
    DIRTY_MAX_LEVEL,
};

enum {
    STAT_uncover_by_fast,
    STAT_line_fast,
    STAT_line_inc,
    STAT_line_dp,
    STAT_cache_get,
    STAT_cache_hit,
    STAT_by_contradiction,
    STAT_search,
    STAT_MAX,
};

struct LineWeight {
    int dir;
    int idx;
    int weight;

    LineWeight(int dir_, int idx_, int weight_) {
	dir = dir_;
	idx = idx_;
	weight = weight_;
    }
    bool operator<(const LineWeight &rhs) const {
	return weight < rhs.weight;
    }
};


struct Cell {
    idx_t r, c;
    int8_t v;
    Cell() :r(0),c(0),v(Unknown) {}
    Cell(int r_, int c_, Value v_) :r(r_),c(c_),v(v_) {}
    bool operator<(const Cell &rhs) const {
	if (r != rhs.r) return r < rhs.r;
	if (c != rhs.c) return c < rhs.c;
	return v < rhs.v;
    }
} __attribute__ ((packed));

struct CellWeight {
    int i, j;
    int weight;
    CellWeight(int i_, int j_, int weight_) {
	i = i_;
	j = j_;
	weight = weight_;
    }
    bool operator<(const CellWeight &rhs) const {
	return weight < rhs.weight;
    }
};

struct SearchSuggest {
    int i, j;
    int remain[2];
    int remain_max;
#if 0
    vector<CellWeight> cand;
#endif
};

struct BacktrackState {
    int nchange0;
    int i, j;
    int v;
    int next_color;
    std::vector<Cell> imply_list[N*N*2]; 
    std::vector<Cell> scc_root;
    SearchSuggest suggest;
};

struct Solution {
    Value board[N][N];  // -1 unknown, 0 ., 1 *
};

struct Solving : public Puzzle {
    Line known[NDIR][N][2];

    // dirty
    // After solve_all_line(), it will become non-dirty, which means dirty[*]=0
    // and dirtyweight is empty.
    Line dirty[DIRTY_MAX_LEVEL][2];
    std::vector<LineWeight> dirtyweight[DIRTY_MAX_LEVEL];

    // for ordering
    int line_filled[NDIR][N];
    int complexity[NDIR][N];
    std::vector<CellWeight> cell_order;

    // for backtracking
    int changes[N*N][3];
    int nchange;

    std::vector<BacktrackState> stack;

    long long stat_counter[STAT_MAX];
    std::vector<Solution> solution;
};
extern LineCacheEntry *cache;
extern Line mask[N];

extern void solve(Solving *ctx);
extern void init_naughty();
extern void init_solving(Solving *ctx);

extern void stat_show(Solving *ctx);
extern bool line_solve_fast(Solving *ctx, Dir dir, int idx_, const int clue[N], const Line known[2], Line new_known[2], int *count);
extern bool line_solve_dp_bit(Solving *ctx, Dir dir, int idx, const int clue[N], const Line known[2], Line new_known[2], int *count);

#endif
