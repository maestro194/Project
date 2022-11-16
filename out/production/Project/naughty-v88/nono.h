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
#ifndef __NONO_H_
#define __NONO_H_
#include <stdint.h>
#include "config.h"

#ifdef LARGE_BOARD
#define N (63+1)
//#define N (127+1)
#else
#define N (31+1)
#endif
const int NDIR = 2;

#ifdef LARGE_BOARD
typedef uint64_t Line;
//typedef unsigned __int128 Line;
#else
typedef uint32_t Line;
#endif

enum Dir {
    LR,
    UD,
};

enum Value {
    Unknown = -1,
    WHITE = 0,
    BLACK = 1,
};

extern int n;
extern bool non_square;
extern int width, height;

extern bool flag_single_solution;
extern int flag_verbose;
extern const char *flag_log_filename;
extern int64_t flag_max_cache_size;
extern const char *log_tag;


struct Puzzle {
    // board
    // [0] is not used. start from one
    Value board[N][N];  // -1 unknown, 0 ., 1 *
    Value goal[N][N];  // -1 unknown, 0 ., 1 *

    // condition numbers
    // [dir][index][number]
    // [0] is count.
    int clue[NDIR][N][N];
};


// r[LR][i][j] means, in dir=LR, there are at least r[LR][i][j] cells on right of (i,j)
//extern int r[NDIR][N][N]; 


// common.cc
extern void Show(Puzzle *puz);
extern void init_puzzle(Puzzle *puz);
extern void InitBoard(Puzzle *puz);
extern void check_goal(Puzzle *puz);
extern void boardline_to_clue(const Value *boardline, int stride, int clue[N]);
extern void output_line(Line line);
extern void output_clue(const int clue[N]);
extern void output_known(const Line known[2]);
double now(void);
void logging(int v,const char *format,...);

// read.cc
extern int GetDataNon(Puzzle *puz);
extern int GetDataTCGA(Puzzle *puz);
extern void OutputNon(const Puzzle *puz);
extern void OutputTCGA(const Puzzle *puz, int idx);


#endif
