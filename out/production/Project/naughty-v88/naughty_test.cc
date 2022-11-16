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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#undef NDEBUG
#include <assert.h>

#include "config.h"
#include "nono.h"
#include "naughty.h"

typedef bool (*line_solver)(Solving *ctx, Dir dir, int idx, const int clue[N], const Line known[2], Line new_known[2], int *count);

void prepare_test(int size, const char *cluestr, const char *knownstr, const char *expectstr,
	int clue[N], Line known[2], Line expect[2])
{
    n = size;

    clue[0] = 0;
    char *cluestr2 = strdup(cluestr);
    for (char *p = strtok(cluestr2, " "); p; p = strtok(NULL, " "))
	clue[++clue[0]] = atoi(p);
    free(cluestr2);

    for (int i = 0; knownstr[i]; i++)
	switch(knownstr[i]) {
	    case '.':
		break;
	    case '-':
		known[0] |= 1 << (i+1);
		break;
	    case '+':
		known[1] |= 1 << (i+1);
		break;
	}
    
    if (expectstr) {
	for (int i = 0; expectstr[i]; i++)
	    switch(expectstr[i]) {
		case '.':
		    break;
		case '-':
		    expect[0] |= 1 << (i+1);
		    break;
		case '+':
		    expect[1] |= 1 << (i+1);
		    break;
	    }
    }
}

void check_line_solver(int size, line_solver solver, const char *cluestr, const char *knownstr, const char *expectstr, int expect_count)
{
    Solving ctx;
    int clue[N];
    Line known[2] = {0,0};
    Line expect[2] = {0,0};
    prepare_test(size, cluestr, knownstr, expectstr,
	    clue, known, expect);

    Line result[2] = {0,0};

    int count = 0;
    bool ret;
    ret = solver(&ctx, LR, 1, clue, known, result, &count);

    if (((result[0]^expect[0]) & (mask[n]<<1)) != 0 ||
	    ((result[1]^expect[1]) & (mask[n]<<1)) != 0 ||
	    !((ret && (count==0 || count==expect_count)) ||
		(!ret && count==0 && count==expect_count))) {
	printf("******************\n");
	printf("known:  ");
	output_known(known);
	printf("clue: %s\n", cluestr);
	printf("expect: (%d) %s\n", expect_count, expectstr);
	printf("result: (%d) ", count);
	output_known(result);
	exit(1);
    }
}

void bench_line_solver(int size, line_solver solver, const char *cluestr, const char *knownstr)
{
    Solving ctx;
    int clue[N];
    Line known[2] = {0,0};
    Line expect[2] = {0,0};
    prepare_test(size, cluestr, knownstr, NULL,
	    clue, known, expect);

    Line result[2] = {0,0};

    int count = 0;
    clock_t st = clock();
    for (int i = 0; i < 10000000; i++)
	solver(&ctx, LR, 1, clue, known, result, &count);
    clock_t et = clock();
    printf("%d clocks\n", (int)(et-st));
}

void test_line_solve_fast()
{
    // IR1'
    if (0) check_line_solver(25, line_solve_fast, "2 2 3 2 2", 
	    "....++.....+++....++.....",
	    "...-++-...-+++-..-++-....", 0);
    // IR2'
    if (0) check_line_solver(25, line_solve_fast, "2 3 4",
	    "..-+..........-+.........",
	    "..-++.........-+++.......", 3);
    // IR5L
    if (1) check_line_solver(20, line_solve_fast, "3 2 1 1", 
	    ".+....-.--.--.-.+--.",
	    "+++-++-.--.--.--+--.", 0);
    // IR5R
    if (1) check_line_solver(20, line_solve_fast, "3 2 1 1", 
	    ".+....-.--.--.--+.-.",
	    "+++-++-.--.--.--+--.", 0);
    line_solver solver = line_solve_dp_bit;
    if (1) check_line_solver(25, solver, "1 4 1 1 1 2 1 2",
	    ".+...+...+...............",
	    "-+-.+++.-+-..............", 112);
    if (1) check_line_solver(25, solver, "1 3 4 1 1",
	    "-.--........+-..-...+-...",
	    "-.--......+++-..-...+-...", 20);
    if (1) check_line_solver(25, solver, "1 3 4 1 1",
	    "-----+----..+..--++++-+-+",
	    "-----+----..+..--++++-+-+", 3);
    if (1) check_line_solver(25, solver, "1 2 2 3 1 1 1",
	    "-+-...-++--.---+++-+-....",
	    "-+-.+.-++------+++-+-....", 6);
    if (1) check_line_solver(25, solver, "1 1 1 1 1 3 2 1",
	    ".....+-+-+-+++---++----+-",
	    "....-+-+-+-+++---++----+-", 3);

}

void tests()
{
    test_line_solve_fast();
#ifdef BENCH
    bench_line_solver(25, line_solve_fast, "2 1 4 1 1 1 1 1 1",
	    ".........................");
    bench_line_solver(25, line_solve_dp_bit, "2 1 4 1 1 1 1 1 1",
	    ".........................");
    bench_line_solver(25, line_solve_dp, "2 1 4 1 1 1 1 1 1",
	    ".........................");
    exit(0);
#endif

}

void stress_test()
{
    Solving ctx;
    printf("stress_test:\n");
    n = 25;
    Value boardline[N] = {WHITE};
    int test_count = 0;
    clock_t st = clock();

    while (1) {
	Line line = 0;
	int clue[N];
	for (int i = 1; i <= n; i++) {
	    int bit = rand() & 1;
	    boardline[i] = bit?BLACK:WHITE;
	    if (bit)
		line |= 1<<i;
	}
	boardline_to_clue(boardline, 1, clue);

	Line known[2] = {0};
	int nknown = rand() % (n + 1);
	int known_idx[N];
	for (int i = 0; i < n; i++)
	    known_idx[i] = i + 1;
	for (int i = 0; i < nknown; i++) {
	    int r = i + rand() % (n - i);
	    int t = known_idx[r];
	    known_idx[r] = known_idx[i];

	    if (boardline[t])
		known[1] |= 1 << t;
	    else
		known[0] |= 1 << t;
	}

	int count;
	Line new_known[2];
	line_solve_fast(&ctx, LR, 0, clue, known, new_known, &count);
	int count2;
	Line new_known2[2];
	line_solve_dp_bit(&ctx, LR, 0, clue, known, new_known2, &count2);

	if (KNOW_MORE(new_known[0], new_known2[0]) ||
		KNOW_MORE(new_known[1], new_known2[1])) {
	    output_line(line);
	    output_clue(clue);
	    output_known(known);
	    output_known(new_known);
	    output_known(new_known2);
	    exit(1);
	}
	if ((mask[n]<<1 & new_known2[1] & ~line) ||
		(mask[n]<<1 & new_known2[0] & line)) {
	    output_line(line);
	    output_clue(clue);
	    output_known(known);
	    output_known(new_known);
	    output_known(new_known2);
	    exit(2);
	}
	test_count++;
	if (test_count % 1000000 == 0) {
	    double t = 1.0 * (clock()-st) / CLOCKS_PER_SEC;
	    printf("tested %d, %.3f/s\n", test_count, test_count/t);
	}
    }
}

int main(void)
{
    init_naughty();
    tests();
    stress_test();
}
