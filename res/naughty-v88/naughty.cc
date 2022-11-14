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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <time.h>

#include <vector>
#include <algorithm>
#include <queue>
#include <stack>

#include "config.h"
#include "nono.h"
#include "MurmurHash3.h"
#include "naughty.h"

using namespace std;

#define STAT

bool flag_single_solution;
int64_t flag_max_cache_size = 1000; // MB
unsigned int cache_entries;
unsigned int cache_entries_mask;
LineCacheEntry *cache;

Line mask[N];

inline int node_index(int r, int c, bool v) {
    assert(1 <= r && r <= n);
    assert(1 <= c && c <= n);
    int idx = (r * (n+1) + c) << 1 | v;
    return idx;
}
inline void node_decode(int idx, int *r, int *c, Value *v) {
    *r = idx/2/(n+1);
    *c = idx/2%(n+1);
    *v = (Value)(idx%2);
}

int myrand()
{
    static int seed = 0;
    // rand formula from FreeBSD rand.c
    seed = seed * 1103515245 + 12345;
    return seed % ((unsigned long)RAND_MAX + 1);
}

int C(int n, int r)
{
    int c = 1;
    for (int i = 0; i < r; i++)
        c = c * (n - i) / (i+1);
    return c;
}

int H(int n, int r)
{
    return C(n+r-1, r);
}

int calculate_num_inter(Puzzle *puz, Dir d, int i)
{
    int r = 0;
    for (int z = 1; z <= puz->clue[d][i][0]; z++)
	r += puz->clue[d][i][z] + 1;
    r -= 1;
    return H(puz->clue[d][i][0]+1, n-r);
}

void cache_init()
{
    assert(IS_2xxN(CACHE_SLOT));
    cache_entries = 1<<15; // 530kb
    //cache_entries = 1<<19; // 8.5MB
    //cache_entries = 1<<23; // 136MB
    cache_entries_mask = cache_entries - 1;
    int cache_size = (cache_entries + CACHE_SLOT) * sizeof(LineCacheEntry);
    free(cache);
    cache = (LineCacheEntry*)calloc(cache_size, 1);
    logging(1, "cache_size = %.2f MB", cache_size / 1048576.0);
}

int cache_key(const LineCacheKey *key)
{
    int hash;
    assert(sizeof(int)==4);
    MurmurHash3_x86_32(key, sizeof(*key), 0, &hash);
    return hash & cache_entries_mask;
}

int count_cache_full;
void cache_enlarge()
{
    if (cache_entries * sizeof(LineCacheEntry) > (1llu<<20)*flag_max_cache_size) {
	count_cache_full++;
	// erase 25%
	for (int i = count_cache_full; i < cache_entries + CACHE_SLOT; i+=4) {
	    LineCacheEntry &e = cache[i];
	    e.key.dir_idx = 0;
	}
	return;
    }

    LineCacheEntry *cache_old = cache;
    unsigned cache_entries_old = cache_entries;
    if (cache_entries * sizeof(LineCacheEntry) < 100*(1<<20))
	cache_entries *= 4;
    else
	cache_entries *= 2;
    cache_entries_mask = cache_entries - 1;
    int cache_size = (cache_entries + CACHE_SLOT) * sizeof(LineCacheEntry);
    cache = (LineCacheEntry*)calloc(cache_size, 1);
    logging(1, "cache_size = %.2f MB", cache_size / 1048576.0);

    for (unsigned int i = 0; i < cache_entries_old; i++) {
	if (cache_old[i].key.dir_idx == 0)
	    continue;
	int keyhash = cache_key(&cache_old[i].key);
	
	for (int j = 0; j < CACHE_SLOT; j++) {
	    LineCacheEntry &e = cache[keyhash + j];
	    if (e.key.dir_idx == 0) {
		e = cache_old[i];
		break;
	    }
	}
    }
    free(cache_old);
}

LineCacheEntry* cache_get(Solving *ctx, Dir d, int idx, const Line known[2])
{
    if (idx == 0)
	return NULL;
    ctx->stat_counter[STAT_cache_get]++;
    LineCacheKey key = { d|idx<<1, { known[0], known[1] } };
    int keyhash = cache_key(&key);
    for (int i = 0; i < CACHE_SLOT; i++) {
	LineCacheEntry &e = cache[keyhash+i];
	if (e.key.dir_idx == 0)
	    return NULL;
	if (e.key == key) {
	    ctx->stat_counter[STAT_cache_hit]++;
	    return &e;
	}
    }
    return NULL;
}

void cache_set(Solving *ctx, Dir d, int idx, const Line known[2], const Line new_known[2], int count)
{
    int i;
    LineCacheKey key = { d|idx<<1, { known[0], known[1] } };
    int keyhash = cache_key(&key);
    LineCacheEntry *which = &cache[keyhash + (ctx->stat_counter[STAT_cache_get] & (CACHE_SLOT-1))];
    for (i = 0; i < CACHE_SLOT; i++) {
	LineCacheEntry &e = cache[keyhash + i];
	if (e.key.dir_idx == 0 || e.key == key) {
#if 0
	    static int max = 0;
	    if (i > max) {
		max = i;
		printf("max = %d\n", max);
	    }
#endif
	    which = &e;
	    break;
	}

//	if (which->count > e.count)
//	    which = &e;
    }

    which->key = key;
    which->new_known[0] = new_known[0];
    which->new_known[1] = new_known[1];
    //which->count = count;

    if (i == CACHE_SLOT)
	cache_enlarge();
}

#define HMASK(i) (~mask[i]<<1)

// check pos[idx~] obey known
bool line_prefix_check(const int clue[N], const Line known[2], int pos[N], int idx, Line line)
{
    Line prefix_mask = HMASK(pos[idx]-1);
    if (known[0] & prefix_mask & line)
	return false;
    if (known[1] & prefix_mask & ~line)
	return false;
    if (idx == 1 && (~prefix_mask & known[1]))
	return false;
    return true;
}

bool line_inc(Solving *ctx, const int clue[N], const Line known[2], int pos[N], int idx, const int leftmost[N], Line &line)
{
#ifdef STAT
    ctx->stat_counter[STAT_line_inc]++;
#endif
    assert(pos[clue[0]+1] == n+2); // sentinel
    if (idx == clue[0] + 1)
	return false;

    do {
	while (pos[idx] + 1 + clue[idx] >= pos[idx+1]) {
	    if (idx == clue[0])
		return false;
	    idx++;
	}

	line ^= (Line)1 << pos[idx] | (Line)1 << (clue[idx] + pos[idx]);
	pos[idx]++;
	if (!line_prefix_check(clue, known, pos, idx, line))
	    continue;

	// TODO cache partial-leftmost
	for (int i = 1; i < idx; i++) {
	    line ^= mask[clue[i]] << pos[i];
	    pos[i] = leftmost[i];
	    line ^= mask[clue[i]] << pos[i];
	}

	do {
	    idx--;
	    if (idx <= 0)
		return true;
	} while (line_prefix_check(clue, known, pos, idx, line));
    } while (1);

    return true;
}

bool get_leftmost(Solving *ctx, const int clue[N], const Line known[2], Line &line, int pos[N])
{
    int p = 1;

    // leftmost without known
    line = 0;
    for (int i=1; i<= clue[0]; i++) {
	pos[i] = p;
	line |= mask[clue[i]] << p;
	p += 1 + clue[i];
    }
    pos[clue[0]+1] = n+2; // sentinel
#if 0
    printf("------\n");
    output_clue(clue);
    output_line(line);
    output_known(known);
#endif

    // fix for known
    int idx = clue[0];
    do {
	while (!line_prefix_check(clue, known, pos, idx, line)) {
	    if (!line_inc(ctx, clue, known, pos, idx, pos, line))
		return false;
	}
	idx--;
    } while (idx > 0);

    return true;
}

void line_reverse(Line line, Line &result)
{
    result = 0;
    for (int i = 1; i <= n; i++) {
	if (line & ((Line)1<<i))
	    result |= (Line)1<<(n-i+1);
    }
}

bool get_rightmost(Solving *ctx, const int clue[N], const Line known[2], Line &line, int pos[N])
{
    int clue2[N];
    Line known2[2];

    clue2[0] = clue[0];
    for (int i = 1; i <= clue[0]; i++)
	clue2[i] = clue[clue[0]-i+1];

    line_reverse(known[0], known2[0]);
    line_reverse(known[1], known2[1]);

    Line line2;
    int pos2[N];
    bool ret = get_leftmost(ctx, clue2, known2, line2, pos2);

    line_reverse(line2, line);
    for (int i = 1; i <= clue[0]; i++)
	pos[i] = n-pos2[clue[0]-i+1]+1-clue[i]+1;
    return ret;
}

bool line_solve_dp_bit(Solving *ctx, Dir dir, int idx, const int clue[N], const Line known[2], Line new_known[2], int *count)
{
#ifdef LARGE_BOARD
    Line flag_l_1[N];
    memset(flag_l_1, 0, sizeof(Line)*(n+1));
    Line flag_l_24[N];
    memset(flag_l_24, 0, sizeof(Line)*(n+1));
#else
    // 2.6% faster for n=25
    Line flag_l_1[N]={0};
    Line flag_l_24[N]={0};
#endif

    if (clue[0] == 0) {
	new_known[0] = ~0;
	new_known[1] = 0;
	*count = 1;
	return true;
    }

#ifdef USE_CACHE
    LineCacheEntry *entry = cache_get(ctx, dir, idx, known);
    if (entry) {
	new_known[0] = entry->new_known[0];
	new_known[1] = entry->new_known[1];
	//*count = entry->count;
	return true;
    }
#endif
    ctx->stat_counter[STAT_line_dp]++;

    for (int i = 0; i <= n; i++) {
	if (BGET(known[1], i))
	    break;
	BSET(flag_l_1[i], 0);
    }

    int left[N];
    left[0] = 0;
    for (int i = 1; i <= clue[0]; i++)
	left[i] = left[i-1] + clue[i] + 1;

    for (int k = 1; k <= clue[0]; k++) {
	for (int i = left[k]-1; i <= n; i++) {
	    if (BGET(flag_l_1[i-1],k) && (known[1]&((Line)1<<i)) == 0) {
		BSET(flag_l_1[i], k);
	    }
	    if ((mask[clue[k]] << (i-clue[k]+1) & known[0])) {
		continue;
	    }
	    if (k == 1) {
		if (BGET(flag_l_1[i-clue[k]], k-1)) {
		    BSET(flag_l_1[i], k);
		    BSET(flag_l_24[i], k);
		}
	    } else {
		if (clue[k] < i && 
			BGET(flag_l_1[i-clue[k]-1], k-1) &&
			(known[1]&((Line)1<<(i-clue[k]))) == 0) {
		    BSET(flag_l_1[i], k);
		    BSET(flag_l_24[i], k);
		}
	    }
	}
    }

    if (!BGET(flag_l_1[n], clue[0]))
	return false;

    Line flag_r_1[N]={0};
    Line flag_r_24[N]={0};
    for (int i = n+1; i >= 1; i--) {
	if (BGET(known[1], i))
	    break;
	BSET(flag_r_1[i], clue[0]+1);
    }
    int right[N];
    right[clue[0]+1] = 0;
    for (int i = clue[0]; i >= 1; i--)
	right[i] = right[i+1] + clue[i] + 1;
    for (int k = clue[0]; k >= 1; k--) {
	for (int i = n-right[k]+1+1; i >= 1; i--) {
	    if (BGET(flag_r_1[i+1],k) && (known[1]&((Line)1<<i)) == 0) {
		BSET(flag_r_1[i], k);
	    }
	    if ((mask[clue[k]] << i & known[0])) {
		continue;
	    }
	    if (k == clue[0]) {
		if (BGET(flag_r_1[i+clue[k]],k+1)) {
		    BSET(flag_r_1[i], k);
		    BSET(flag_r_24[i], k);
		}
	    } else {
		if (i < n-clue[k]+1 && 
			BGET(flag_r_1[i+clue[k]+1], k+1) &&
			(known[1]&((Line)1<<(i+clue[k]))) == 0) {
		    BSET(flag_r_1[i], k);
		    BSET(flag_r_24[i], k);
		}
	    }
	}
    }
    assert(!!BGET(flag_l_1[n],clue[0]) == !!BGET(flag_r_1[1],1));

    Line candi[2];
    candi[0] = ~0;
    candi[1] = ~0;

    // for all possible position of black
    for (int i = 1; i <= n; i++) {
#if 0 // slower?
	if (BGET(known[0], i)) {
	    continue;
	}
#endif
	for (int k = 1; k <= clue[0]; k++) {
	    if (BGET(flag_l_24[i+clue[k]-1], k) &&
		    BGET(flag_r_24[i], k)) {
		candi[0] &= ~(mask[clue[k]] << i);
	    }
	}
    }

    // for all possible position of white
    for (int i = 1; i <= n; i++) {
	if (BGET(known[1], i)) {
	    continue;
	}
	if (BGET(known[0], i)) {
	    BCLR(candi[1], i);
	    continue;
	}

	if ((flag_l_1[i-1]<<1) & flag_r_1[i+1])
	    BCLR(candi[1], i);
    }

    new_known[0] = known[0] | candi[0];
    new_known[1] = known[1] | candi[1];

#ifdef USE_CACHE
    cache_set(ctx, dir, idx, known, new_known, *count);
#endif
    return true;
}
//#define DEBUG_FAST
bool line_solve_fast(Solving *ctx, Dir dir, int idx_, const int clue[N], const Line known[2], Line new_known[2], int *count)
{
    if (clue[0] == 0) {
	new_known[0] = ~0;
	new_known[1] = 0;
	return true;
    }

#ifdef USE_CACHE
    LineCacheEntry *entry = cache_get(ctx, dir, idx_, known);
    if (entry) {
	new_known[0] = entry->new_known[0];
	new_known[1] = entry->new_known[1];
	return true;
    }
#endif

#ifdef STAT
    ctx->stat_counter[STAT_line_fast]++;
#endif
    Line line_l, line_r;
    int pos_l[N], pos_r[N];
    if (!get_leftmost(ctx, clue, known, line_l, pos_l))
	return false;
    get_rightmost(ctx, clue, known, line_r, pos_r);
    
#ifdef DEBUG_FAST
    printf("=====================\n");
    output_clue(clue);
    output_known(known);
    printf("left, right:\n");
    output_line(line_l);
    output_line(line_r);
    for (int i = 1; i <= clue[0]; i++)
	printf("%d%c", pos_l[i], i==clue[0]?'\n':' ');
    for (int i = 1; i <= clue[0]; i++)
	printf("%d%c", pos_r[i], i==clue[0]?'\n':' ');
#endif

    int idx_l[N], idx_r[N];
    memset(idx_l, 0, sizeof(idx_l));
    memset(idx_r, 0, sizeof(idx_r));
    for (int i = 1; i <= clue[0]; i++)
	for (int j=0; j<clue[i]; j++) {
	    idx_l[pos_l[i]+j] = i;
	    idx_r[pos_r[i]+j] = i;
	}

#ifdef DEBUG_FAST
    for (int i = 1; i <= n; i++)
	printf("%d%c", idx_l[i], i==n?'\n':' ');
    for (int i = 1; i <= n; i++)
	printf("%d%c", idx_r[i], i==n?'\n':' ');
#endif

    new_known[0] = known[0];
    new_known[1] = known[1];

    // whites before first black
    for (int i = 1; i < pos_l[1]; i++)
	new_known[0] |= (Line)1 << i;
    // whites between black
    for (int i = 1; i < clue[0]; i++)
	for (int j = pos_r[i]+clue[i]; j < pos_l[i+1]; j++)
	    new_known[0] |= (Line)1 << j;
    // whites after last black
    for (int i = pos_r[clue[0]] + clue[clue[0]]; i <= n; i++)
	new_known[0] |= (Line)1 << i;

    // black overlaped
    for (int i = 1; i <= n; i++) {
	if (idx_l[i] == idx_r[i]) {
	    if (idx_l[i] != 0) 
		new_known[1] |= (Line)1 << i;
	}
    }

    // ------------------------------------
    // Inference Rules
    // ------------------------------------
    // TODO match clue with possible segment, inference with uncertain segments
    int maxclue = 0;
    for (int i = 1; i <= clue[0]; i++)
	if (maxclue < clue[i])
	    maxclue = clue[i];
    int minclue = N;
    for (int i = 1; i <= clue[0]; i++)
	if (minclue > clue[i])
	    minclue = clue[i];
#if 1
    // IR1 if maxclue = 3
    // known ..+++..
    // imply .-+++-.
    for (int i = 1; i <= clue[0]; i++)
	if (maxclue == clue[i]) {
	    // max clue reveals
	    if ((~new_known[1] & (mask[clue[i]]<<pos_l[i])) == 0) {
		if (pos_l[i] > 1)
		    new_known[0] |= (Line)1 << (pos_l[i] - 1);
		if (pos_l[i] + clue[i] <= n)
		    new_known[0] |= (Line)1 << (pos_l[i] + clue[i]);
	    }
	}
#else
    // tiny effect & slower
    // IR1' maxclue in sub-region
    // clue 2 2 3 2 2
    // known ....++....-+++-...++.....
    // imply ...-++-...-+++-..-++-....
    for (int i = 1; i <= clue[0]; i++) {
	int p = pos_l[i];
	assert(idx_l[p] == i);
	if (idx_r[p] == 0)
	    continue;
	if ((~(new_known[1]) & (mask[clue[i]]<<p)) == 0) {
	    bool ismax = true;
	    assert(idx_l[p] >= idx_r[p]);
	    for (int j = idx_r[p]; j < i; j++)
		if (clue[i] < clue[j]) {
		    ismax = false;
		    break;
		}
	    if (ismax) {
		if (p > 1) {
		    new_known[0] |= (Line)1 << (p - 1);
		}
		if (p + clue[i] <= n)
		    new_known[0] |= (Line)1 << (p + clue[i]);
	    }
	}
    }
#endif

#if 1
    // IR4 if maxclue = 2
    // known ..+.+..
    // imply ..+-+..
    for (int i = 1; i + maxclue  - 1 < n; i++)
	if (BGET(new_known[1], i) && BGET(new_known[1], i + maxclue)) {
	    Line tmp = (new_known[1]&(mask[maxclue+1] << i)) ^ (mask[maxclue+1]<<i);
	    if (IS_2xxN(tmp))
		new_known[0] |= tmp;
	}
#endif

#if 1
    // IR2 if minclue = 5
    // known ..-+......
    // imply ..-+++++..
    if (minclue > 1) {
	for (int i = 1; i <= n; i++)
	    if (BGET(new_known[1], i)) {
		if (i == 1 || BGET(new_known[0], i-1)) 
		    new_known[1] |= mask[minclue] << i;
		if (i == n || BGET(new_known[0], i+1))
		    new_known[1] |= mask[minclue] << (i - minclue + 1);
	    }
    }
#else
    // IR2'
    for (int i = 1; i <= clue[0]; i++) {
	int p = pos_l[i];
	if (BGET(new_known[1], p)) {
	    if (p == 1 || BGET(new_known[0], p-1)) {
		int min = clue[i];
		for (int j = idx_r[p]; j < i; j++)
		    if (min > clue[j])
			min = clue[j];
		new_known[1] |= mask[min] << p;
	    }
	}
	p = pos_l[i] + clue[i] - 1;
	if (BGET(new_known[1], p)) {
	    if (p == n || BGET(new_known[0], p+1)) {
		int min = clue[i];
		for (int j = idx_r[p]; j < i; j++)
		    if (min > clue[j])
			min = clue[j];
		new_known[1] |= mask[min] << (p-min+1);
	    }
	}
    }
#endif

#if 1
    // IR3 if minclue = 3
    // known ...-..-...
    // imply ...----...
    if (minclue > 1) {
	for (int i = 1; i <= n; i++) {
	    if ((i == 1 || BGET(new_known[0], i-1)) &&
		    !BGET(new_known[0]|new_known[1], i) &&
		    (i + minclue - 1 > n || (new_known[0] & mask[minclue] << i) )) {
		for (int j = i; j <= n && !BGET(new_known[0], j); j++)
		    new_known[0] |= (Line)1 << j;
	    }
	}
    }
#endif

    // IR5L Rule 1.3 ? i'm not sure
    // TODO it could be generalized to more than 1
#if 1
    for (int i = 2; i <= clue[0]; i++) {
	if (!BGET(new_known[1], pos_l[i]))
	    continue;
	bool all_one = true;
#if 0
	for (int j = 1; j < i; j++)
	    if (pos_r[j] + clue[j] - 1 >= pos_l[i])
#else
	for (int j = idx_r[pos_l[i]]; j < i; j++)
#endif
		if (clue[j] != 1) {
		    all_one = false;
		    break;
		}
	if (all_one)
	    new_known[0] |= (Line)1 << (pos_l[i]-1);
    }
#endif
#if 1
    // IR5R
    for (int i = 1; i < clue[0]; i++) {
	if (!BGET(new_known[1], pos_r[i] + clue[i] - 1))
	    continue;
	bool all_one = true;
#if 0
	for (int j = i + 1; j <= clue[0]; j++)
	    if (pos_l[j] <= pos_r[i] + clue[i] - 1)
#else
	for (int j = idx_l[pos_r[i]+clue[i]-1]; j > i; j--)
#endif
		if (clue[j] != 1) {
		    all_one = false;
		    break;
		}
	if (all_one)
	    new_known[0] |= (Line)1 << (pos_r[i]+clue[i]);
    }
#endif


#ifdef DEBUG_FAST
    printf("new known:\n");
    output_known(new_known);
#endif

    if (new_known[0] & new_known[1])
	return false;

    if (0) {
	Line new_known2[2];
	int count2 = 0;
       	line_solve_dp_bit(ctx, dir, idx_, clue, known, new_known2, &count2);
	if (KNOW_MORE(new_known[0], new_known2[0]) ||
		KNOW_MORE(new_known[1], new_known2[1])) {
	    printf("!!!!!!!!!!!!!!\n");
	    printf("new known2:\n");
	    output_known(new_known2);
	}
    }

    return true;
}

void do_change(Solving *ctx, int i, int j, int v)
{
    ctx->line_filled[LR][i]++;
    ctx->line_filled[UD][j]++;
    ctx->changes[ctx->nchange][0] = i;
    ctx->changes[ctx->nchange][1] = j;
    ctx->nchange++;
    assert(ctx->nchange <= n*n);
    ctx->known[LR][i][v] |= (Line)1 << j;
    ctx->known[UD][j][v] |= (Line)1 << i;
    assert(ctx->board[i][j] == Unknown);
    ctx->board[i][j] = (Value)v;
}

void undo_change(Solving *ctx)
{
    ctx->nchange--;
    assert(0 <= ctx->nchange);
    int i = ctx->changes[ctx->nchange][0];
    int j = ctx->changes[ctx->nchange][1];
    Value v = ctx->board[i][j];
    ctx->line_filled[LR][i]--;
    ctx->line_filled[UD][j]--;

    ctx->known[LR][i][v] ^= (Line)1 << j;
    ctx->known[UD][j][v] ^= (Line)1 << i;
    ctx->board[i][j] = Unknown;
}

void init_naughty()
{
    for (int i = 1; i < N; i++)
	mask[i] = ((Line)1 << i) - 1;
}

void init_solving(Solving *ctx)
{
    InitBoard(ctx);
    memset(ctx->known, 0, sizeof(ctx->known));
    ctx->nchange = 0;
    memset(ctx->stat_counter, 0, sizeof(ctx->stat_counter));
    memset(ctx->dirty, 0, sizeof(ctx->dirty));
    memset(ctx->line_filled, 0, sizeof(ctx->line_filled));
    ctx->stack.clear();
    ctx->solution.clear();

    for(int i=1;i<=n;i++)
        for(int j=1;j<=n;j++) {
	    if (i > height || j > width) {
		do_change(ctx, i, j, WHITE);
	    }
	}

#ifdef USE_CACHE
    cache_init();
#endif
}

void make_dirty(Solving *ctx, int i, int j)
{
    for (int dl = 0; dl < DIRTY_MAX_LEVEL; dl++) {
	if (!BGET(ctx->dirty[dl][LR], i)) {
	    ctx->dirtyweight[dl].push_back(LineWeight(LR, i, -ctx->complexity[LR][i]));
	    //push_heap(ctx->dirtyweight[dl].begin(), ctx->dirtyweight[dl].end());
	    BSET(ctx->dirty[dl][LR], i);
	}
	if (!BGET(ctx->dirty[dl][UD], j)) {
	    ctx->dirtyweight[dl].push_back(LineWeight(UD, j, -ctx->complexity[UD][j]));
	    //push_heap(ctx->dirtyweight[dl].begin(), ctx->dirtyweight[dl].end());
	    BSET(ctx->dirty[dl][UD], j);
	}
    }
}

void do_imply(Solving *ctx, int i, int j)
{
    const Value &v = ctx->board[i][j];
    int idx = node_index(i, j, v);
    std::vector<Cell> *imply_list = ctx->stack.back().imply_list;
    for (vector<Cell>::const_iterator it = imply_list[idx].begin();
	    it != imply_list[idx].end(); ++it) {
	const Cell &c = *it;
	if (ctx->board[c.r][c.c] == Unknown) {
	    do_change(ctx, c.r, c.c, c.v);
	    make_dirty(ctx, c.r, c.c);
	}
    }
}

void known_cross(Solving *ctx, Dir d, int i, Line new_known[2], int dirtylevel)
{
    for (int v = 0; v < 2; v++) {
	Line knowmore = KNOW_MORE(new_known[v], ctx->known[d][i][v]) & (mask[n]<<1);
	for (int j = 1; knowmore; j++) {
	    if (BGET(knowmore, j)) {
		BCLR(knowmore, j);
		if (d == LR)
		    do_change(ctx, i, j, v);
		else
		    do_change(ctx, j, i, v);
		for (int dl=dirtylevel; dl < DIRTY_MAX_LEVEL; dl++) {
		    if ((ctx->dirty[dl][!d] & ((Line)1 << j)) == 0) {
			ctx->dirtyweight[dl].push_back(LineWeight(!d, j, -ctx->complexity[!d][j]));
			//push_heap(ctx->dirtyweight[dl].begin(), ctx->dirtyweight[dl].end());
			ctx->dirty[dl][!d] |= (Line)1 << j;
		    }
		}
	    }
	}
    }
}

void assert_line_stable(Solving *ctx)
{
#ifndef NDEBUG
    for (int d = 0; d < NDIR ; d++) {
	for (int i = 1; i <= n; i++) {
	    int count = 0;
	    Line new_known[2];
	    bool ret;

	    ret = line_solve_fast(ctx, (Dir)d, i, ctx->clue[d][i],
		    ctx->known[d][i], new_known, &count);
	    assert(ret);
	    assert(ctx->known[d][i][0] == (new_known[0] & (mask[n]<<1)));
	    assert(ctx->known[d][i][1] == (new_known[1] & (mask[n]<<1)));

	    ret = line_solve_dp_bit(ctx, Dir(d), i, ctx->clue[d][i],
		    ctx->known[d][i], new_known, &count);
	    assert(ret);
	    assert(ctx->known[d][i][0] == (new_known[0] & (mask[n]<<1)));
	    assert(ctx->known[d][i][1] == (new_known[1] & (mask[n]<<1)));
	}
    }
#endif
}

// solve_all_line(2) == FullSettle()
bool solve_all_line(Solving *ctx, int strength)
{
    // fast
    DirtyLevel dl = DIRTY_LINE_FAST;
#ifdef STAT
    int nchange0 = ctx->nchange;
#endif
    while (!ctx->dirtyweight[dl].empty()) {
        LineWeight dw = ctx->dirtyweight[dl].front();
        int d = dw.dir;
        int i = dw.idx;
        pop_heap(ctx->dirtyweight[dl].begin(), ctx->dirtyweight[dl].end());
        ctx->dirtyweight[dl].pop_back();

        BCLR(ctx->dirty[dl][d], i);

	int count = 0;
        Line new_known[2];
        if (!line_solve_fast(ctx, (Dir)d, i, ctx->clue[d][i], ctx->known[d][i], new_known, &count))
	    return false;
        known_cross(ctx, Dir(d), i, new_known, dl);
    }
#ifdef STAT
    ctx->stat_counter[STAT_uncover_by_fast] += ctx->nchange - nchange0;
#endif
    assert(ctx->dirtyweight[dl].size() == 0);
    assert(!ctx->dirty[dl][LR] && !ctx->dirty[dl][UD]);

#if 1
    if (strength == 1)
	return true;
#endif

    // all
    dl = DIRTY_LINE_ALL;
    while (!ctx->dirtyweight[dl].empty()) {
	LineWeight dw = ctx->dirtyweight[dl].front();
	int d = dw.dir;
	int i = dw.idx;
	pop_heap(ctx->dirtyweight[dl].begin(), ctx->dirtyweight[dl].end());
	ctx->dirtyweight[dl].pop_back();

	BCLR(ctx->dirty[dl][d], i);

	int count = 0;
	Line new_known[2];
	bool ret = line_solve_dp_bit(ctx, Dir(d), i, ctx->clue[d][i], ctx->known[d][i], new_known, &count);
	if (!ret)
	    return false;
	//printf("(%d,%d) %d -> %d\n", d, i, ctx->complexity[d][i], count);
	//ctx->complexity[d][i] = count;
	known_cross(ctx, Dir(d), i, new_known, dl);
    }

    assert(!ctx->dirty[dl][LR] && !ctx->dirty[dl][UD]);

    return true;
}

void make_cell_order(Solving *ctx)
{
    ctx->cell_order.clear();
    for (int i = 1; i <= n; i++)
	for (int j = 1; j <= n; j++) {
	    if (ctx->board[i][j] != Unknown)
		continue;
	    int w = -ctx->line_filled[LR][i] + -ctx->line_filled[UD][j];
	    ctx->cell_order.push_back(CellWeight(i, j, -w));
	}
    // Use stable_sort() instead of sort() to keep consistency behavior across platform
    stable_sort(ctx->cell_order.begin(), ctx->cell_order.end());
}

void query_dirty_mask(Solving *ctx, int b, int e, Line mask[2])
{
    for (int k = b; k < e; k++) {
	int ti = ctx->changes[k][0];
	int tj = ctx->changes[k][1];
	BSET(mask[0], ti);
	BSET(mask[1], tj);
    }
}

bool solve_by_contradiction(Solving *ctx, int level, int depth, int start_idx0, int lastnchange, SearchSuggest *suggest)
{
    bool stable;
    int strength = 1;
    int padage = 1;
    int pad[N][N][2] = {{{0}}};
    assert(depth > 0);
    Line cell_dirty_mask[N][N][2][2]; // [row][col][val][dir]
    int last_check[N][N][2][3]; // [row][col][val][strength]
    memset(cell_dirty_mask, 0, sizeof(cell_dirty_mask));
    memset(last_check, 0, sizeof(last_check));

    short remain[N][N][2]; // [row][col][val]

    if (level == 1) {
	for (int i=1; i<=n; i++)
	    for (int j=1; j<=n; j++)
		for (int v=0; v<=1; v++) {
		    last_check[i][j][v][1] = lastnchange;
		    last_check[i][j][v][2] = lastnchange;
		}
    }

    if (level == 0 && ctx->stack.size() == 1) {
	logging(1, "contradiction %d:", depth);
    }

    ctx->stat_counter[STAT_by_contradiction]++;

    if (suggest) {
	suggest->remain_max = n*n+1000;
	suggest->i = 10000;
	suggest->j = 10000;
	for (int i=1;i<=n;i++)
	    for(int j=1; j<=n; j++)
		for (int v=0; v<=1; v++)
		    remain[i][j][v]=n*n+1000;
    }

    int start_idx = start_idx0;
    do {
	int it_idx = 0;
	stable = true;
	for (vector<CellWeight>::iterator it = ctx->cell_order.begin();
		it != ctx->cell_order.end(); ++it) {
	    it_idx++;
	    if (it_idx <= start_idx) continue;
	    int i = it->i;
	    int j = it->j;
		if (ctx->board[i][j] != Unknown)
		    continue;

		for (int v = 0; v <= 1; v++) 
		{
		    if (pad[i][j][v] == padage)
			continue;

		    if (last_check[i][j][v][strength] &&
			(cell_dirty_mask[i][j][v][0] != 0) &&
			(cell_dirty_mask[i][j][v][1] != 0)
			    ) {

			Line line_dirty_mask[2]={0,0};
			query_dirty_mask(ctx, last_check[i][j][v][strength],
				ctx->nchange, line_dirty_mask);
#if 1
			if ((cell_dirty_mask[i][j][v][0] & line_dirty_mask[0]) == 0 &&
				(cell_dirty_mask[i][j][v][1] & line_dirty_mask[1]) == 0)
			    continue;
#endif
		    }

		    int nchange0 = ctx->nchange;
		    do_change(ctx, i, j, v);
		    make_dirty(ctx, i, j);
		    do_imply(ctx, i, j);

		    bool ret = true;
		    if (ret)
			ret = solve_all_line(ctx, strength);

		    if (ret) {
			if (flag_single_solution && ctx->nchange == n*n)
				return true;
			if (ctx->nchange < n*n && depth > 1 && strength == 2) {
			    ret = solve_by_contradiction(ctx, level+1, depth-1, it_idx, nchange0, NULL);
			    if (flag_single_solution && ctx->nchange == n*n)
				return true;
			}
		    }

		    if (ret) {
			for (int z = nchange0; z < ctx->nchange; z++) {
			    int i = ctx->changes[z][0];
			    int j = ctx->changes[z][1];
			    int v = ctx->board[i][j];
			    pad[i][j][v] = padage;
			}
		    }

		    if (ret) {
			Line new_mask[2] = {0,0};
			query_dirty_mask(ctx, nchange0, ctx->nchange,
				new_mask);
			cell_dirty_mask[i][j][v][0] = new_mask[0];
			cell_dirty_mask[i][j][v][1] = new_mask[1];
		    }

		    if (ret && level == 0 
			    //&& stable
			    && ctx->nchange-nchange0-1 > 0
			    ) {
			int q = 0;
			int idx = node_index(i, j, v);
			std::vector<Cell> *imply_list = ctx->stack.back().imply_list;
		//	printf("imply_list[%d,%d,%d].len = %d -> %d\n", i,j,v,
		//		imply_list[idx].size(), ctx->nchange-nchange0-1);
			imply_list[idx].resize(ctx->nchange-nchange0-1);
			for (int k = nchange0+1; k < ctx->nchange; k++) {
			    int ti = ctx->changes[k][0];
			    int tj = ctx->changes[k][1];
			    imply_list[idx][q++] = Cell(ti, tj,
				    ctx->board[ti][tj]);
			}
		    }
		    int nchange = ctx->nchange;
		    while (ctx->nchange > nchange0)
			undo_change(ctx);
		    if (!ret) {
			do_change(ctx, i, j, !v);
			make_dirty(ctx, i, j);
			do_imply(ctx, i, j);
			if (!solve_all_line(ctx, 2)) {
			    while (ctx->nchange > nchange0)
				undo_change(ctx);
			    return false;
			}
			if (ctx->nchange == n*n)
			    return true; // no need to check flag_single_solution here

			last_check[i][j][!v][2] = ctx->nchange;

			start_idx = 0;
			stable = false;
			padage++;
			break;
		    } else {
			last_check[i][j][v][strength] = ctx->nchange;
			remain[i][j][v] = n*n-nchange;
		    }
		}

	    }

	//if (level == 0 && strength == 2 && !stable)
	//    for (int i = 0; i < N*N*2; i++)
	//        ctx->imply_list[i].clear();

#if 1
	if (strength == 1 && stable) {
	    padage++;
	    strength = 2;
	    stable = false;
	    start_idx = start_idx0;
	} else
	    strength = 1;
#endif
    } while (!stable);

    if (suggest != NULL) {
	for (int i = 1; i<=n; i++)
	    for (int j=1; j<=n; j++) {
		if (ctx->board[i][j] != Unknown)
		    continue;
		int remain_max = std::max(remain[i][j][0], remain[i][j][1]);

		if (remain_max< suggest->remain_max) {
		    suggest->remain_max = remain_max;
		    suggest->remain[0] = remain[i][j][0];
		    suggest->remain[1] = remain[i][j][1];
		    suggest->i = i;
		    suggest->j = j;
		}
	    }
	assert(suggest->remain_max <= n*n);
    }
    return true;
}

bool solve_logic(Solving *ctx)
{
    bool ret;
    ret = solve_all_line(ctx, 2);
    if (!ret || ctx->nchange == n*n)
	return ret;

    make_cell_order(ctx);

    ret = solve_by_contradiction(ctx, 0, 1, 0, 0, &ctx->stack.back().suggest);
    if (!ret || ctx->nchange == n*n)
	return ret;


    if (
	    ctx->stack.size() <= 10
	   // ctx->nchange < n*n
	    ) {
	ret = solve_by_contradiction(ctx, 0, 2, 0, 0, &ctx->stack.back().suggest);
	if (!ret || ctx->nchange == n*n)
	    return ret;
    }

    return true;
}

//#define DEBUG_SEARCH
void do_guess(Solving *ctx, int i, int j, int v)
{
    if (ctx->stack.size() == 1)
	logging(1, "search:");
#ifdef DEBUG_SEARCH
    printf("do_guess (%d,%d) %d\n", i, j, v);
    assert_line_stable(ctx);
#endif
    ctx->stack.resize(ctx->stack.size() + 1);

    assert(ctx->stack.size() >= 2);
    BacktrackState &prev = ctx->stack[ctx->stack.size() - 2];
    BacktrackState &state = ctx->stack.back();
    for (int z =0;z<N*N*2;z++)
	state.imply_list[z] = prev.imply_list[z];

    state.i = i;
    state.j = j;
    state.v = v;
    state.nchange0 = ctx->nchange;
    state.next_color = !v;

    do_change(ctx, i, j, v);
    make_dirty(ctx, i, j);
    do_imply(ctx, i, j);
}

void backtrack(Solving *ctx)
{
    //printf("backtrack\n");
    while (ctx->stack.size() > 1) {
	BacktrackState &state = ctx->stack.back();
	while (ctx->nchange > state.nchange0)
	    undo_change(ctx);

	if (state.next_color == Unknown) {
#ifdef DEBUG_SEARCH
	    printf("backtrack up\n");
#endif
	    ctx->stack.resize(ctx->stack.size() - 1);
	} else {
#ifdef DEBUG_SEARCH
	    printf("backtrack (%d,%d) %d\n", state.i, state.j, state.next_color);
#endif
	    assert(ctx->stack.size() >= 2);
	    BacktrackState &prev = ctx->stack[ctx->stack.size() - 2];
	    for (int z =0;z<N*N*2;z++)
		state.imply_list[z] = prev.imply_list[z];
	    do_change(ctx, state.i, state.j, state.next_color);
	    make_dirty(ctx, state.i, state.j);
	    do_imply(ctx, state.i, state.j);

	    state.next_color = Unknown;
	    break;
	}
    }
}

void solve(Solving *ctx)
{
    clock_t t0 = clock();
    init_solving(ctx);

    for (int d = 0; d < NDIR ; d++) {
	for (int i = 1; i <= n; i++) {
	    ctx->complexity[d][i] = calculate_num_inter(ctx, Dir(d), i);
	}
    }

    for (int i=1; i<=n; i++)
	for (int j=1; j<=n; j++)
	    make_dirty(ctx, i, j);

    ctx->stack.push_back(BacktrackState());

    while (1) {
	bool ret = solve_logic(ctx);
	if (ctx->nchange == n*n) {
	    Solution sol;
	    memcpy(sol.board, ctx->board, sizeof(sol.board));
	    ctx->solution.push_back(sol);

	    //printf("solution %d\n", ctx->solution.size());
	    //Show(ctx);
	    if (flag_single_solution || ctx->solution.size() == 2)
		break;
	    ret = false;
	}

	if (ret) {
	    ctx->stat_counter[STAT_search]++;

	    BacktrackState &state = ctx->stack.back();
	    int i = state.suggest.i;
	    int j = state.suggest.j;
	    int v = state.suggest.remain[0] < state.suggest.remain[1] ? 0 : 1;
	    do_guess(ctx, i, j, v);
	} else {
	    if (ctx->stack.size() > 1) {
		backtrack(ctx);
	    }
	    if (ctx->stack.size() == 1) {
		break;
	    }
	}
    }
    clock_t t1 = clock();
    logging(1, "user=%f", (double)(t1-t0)/CLOCKS_PER_SEC);

    if (ctx->solution.size() == 0)
	logging(0, "NO SOLUTION");
    else if (flag_single_solution) {
	logging(0, "STOPPED WITH SOLUTION");
    } else {
	if (ctx->solution.size() == 1)
	    logging(0, "UNIQUE SOLUTION");
	else
	    logging(0, "FOUND MULTIPLE SOLUTIONS");
    }
}

void stat_show(Solving *ctx)
{
#ifdef STAT
	logging(1, "! stat_line_fast = %lld", ctx->stat_counter[STAT_line_fast]);
	logging(1, "! stat_uncover_by_fast = %lld", ctx->stat_counter[STAT_uncover_by_fast]);
	logging(1, "! stat_line_inc = %lld", ctx->stat_counter[STAT_line_inc]);
	logging(1, "! stat_line_dp = %lld", ctx->stat_counter[STAT_line_dp]);
	logging(1, "! stat_by_contradiction = %lld", ctx->stat_counter[STAT_by_contradiction]);
	logging(1, "! stat_search = %lld", ctx->stat_counter[STAT_search]);
#endif
	logging(1, "cache hit %lld/%lld = %f", 
		ctx->stat_counter[STAT_cache_hit],
	       	ctx->stat_counter[STAT_cache_get],
	       	1.0*ctx->stat_counter[STAT_cache_hit]/ctx->stat_counter[STAT_cache_get]);

#ifdef USE_CACHE
    int cache_usage = 0;
    for (unsigned int i = 0; i < cache_entries; i++)
	if (cache[i].key.dir_idx != 0) {
	    cache_usage++;
	}
    logging(1, "cache usage = %f", 1.0 * cache_usage / cache_entries);
#endif
}
