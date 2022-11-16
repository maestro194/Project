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

bool line_solve_all(Dir dir, int idx, const int clue[N], const Line known[2], Line new_known[2], int *count)
{
    if (clue[0] == 0) {
	new_known[0] = ~0;
	new_known[1] = 0;
	*count = 1;
	return true;
    }
    
#ifdef USE_CACHE
    LineCacheEntry *entry = cache_get(dir, idx, known);
    if (entry) {
	new_known[0] = entry->known[0];
	new_known[1] = entry->known[1];
	//*count = entry->count;
	return true;
    }
#endif

    new_known[0] = ~0;
    new_known[1] = ~0;

    Line line;
    int pos[N];
    if (!get_leftmost(clue, known, line, pos))
	return false;

    // loop all
    int c = 0;
    int leftmost[N];
    memcpy(leftmost, pos, sizeof(pos));
    //printf("all:\n");
    do {
	//output_line(line);
	c++;
	new_known[0] &= ~line;
	new_known[1] &= line;
    } while (line_inc(clue, known, pos, 1, leftmost, line));
    *count = c;

#ifdef USE_CACHE
    cache_set(dir, idx, known, new_known, *count);
#endif

    return true;
}

bool line_solve_dp(Dir dir, int idx, const int clue[N], const Line known[2], Line new_known[2], int *count)
{
    int solcount_l[N/2][N]; // [k][i] means, cell 1~i, meet ~k clue.
    int solcount_r[N/2][N]; // [k][i] means, cell i~n, meet k~ clue.
    int flag_l[N/2][N];
    int flag_r[N/2][N];

    if (clue[0] == 0) {
	new_known[0] = ~0;
	new_known[1] = 0;
	*count = 1;
	return true;
    }

#ifdef USE_CACHE
    LineCacheEntry *entry = cache_get(dir, idx, known);
    if (entry) {
	new_known[0] = entry->new_known[0];
	new_known[1] = entry->new_known[1];
	//*count = entry->count;
	return true;
    }
#endif
    stat_line_dp++;

    memset(solcount_l, 0, sizeof(solcount_l));
    memset(flag_l, 0, sizeof(flag_l));

    for (int i = 0; i <= n; i++) {
	if (BGET(known[1], i))
	    break;
	solcount_l[0][i] = 1;
    }

    for (int k = 1; k <= clue[0]; k++) {
	for (int i = clue[k]; i <= n; i++) {
	    if (solcount_l[k][i-1] && (known[1]&(1<<i)) == 0) {
		solcount_l[k][i] += solcount_l[k][i-1];
		flag_l[k][i] |= 1;
	    }
	    if ((mask[clue[k]] << (i-clue[k]+1) & known[0])) {
		continue;
	    }
	    if (k == 1) {
		if (solcount_l[k-1][i-clue[k]]) {
		    solcount_l[k][i] += solcount_l[k-1][i-clue[k]];
		    flag_l[k][i] |= 2;
		}
	    } else {
		if (clue[k] < i && solcount_l[k-1][i-clue[k]-1] && (known[1]&(1<<(i-clue[k]))) == 0) {
		    solcount_l[k][i] += solcount_l[k-1][i-clue[k]-1];
		    flag_l[k][i] |= 4;
		}
	    }
	}
    }

    *count = solcount_l[clue[0]][n];

    if (!solcount_l[clue[0]][n])
	return false;

    memset(solcount_r, 0, sizeof(solcount_r));
    memset(flag_r, 0, sizeof(flag_r));
    for (int i = n+1; i >= 1; i--) {
	if (BGET(known[1], i))
	    break;
	solcount_r[clue[0]+1][i] = 1;
    }
    for (int k = clue[0]; k >= 1; k--) {
	for (int i = n-clue[k]+1; i >= 1; i--) {
	    if (solcount_r[k][i+1] && (known[1]&(1<<i)) == 0) {
		solcount_r[k][i] += solcount_r[k][i+1];
		flag_r[k][i] |= 1;
	    }
	    if ((mask[clue[k]] << i & known[0])) {
		continue;
	    }
	    if (k == clue[0]) {
		if (solcount_r[k+1][i+clue[k]]) {
		    solcount_r[k][i] += solcount_r[k+1][i+clue[k]];
		    flag_r[k][i] |= 2;
		}
	    } else {
		if (i < n-clue[k]+1 && solcount_r[k+1][i+clue[k]+1] && (known[1]&(1<<(i+clue[k]))) == 0) {
		    solcount_r[k][i] += solcount_r[k+1][i+clue[k]+1];
		    flag_r[k][i] |= 4;
		}
	    }
	}
    }
#if 0
    printf("=================================\n");
    output_clue(clue);
    output_known(known);
    printf("solcount_l:\n");
    for (int k = 1; k <= clue[0]; k++) {
	for (int i = 1; i <= n; i++)
	    printf("%d\t", solcount_l[k][i]);
	printf("\n");
    }
    printf("solcount_r:\n");
    for (int k = clue[0]; k >= 1; k--) {
	for (int i = 1; i <= n; i++)
	    printf("%d\t", solcount_r[k][i]);
	printf("\n");
    }
#endif
    assert(solcount_l[clue[0]][n] == solcount_r[1][1]);

    int candi[2];
    candi[0] = ~0;
    candi[1] = ~0;

    // for all possible position of black
    for (int k = 1; k <= clue[0]; k++)
	for (int i = 1; i <= n; i++) {
	    if ((flag_l[k][i+clue[k]-1]&(2|4)) && (flag_r[k][i]&(2|4))) {
		candi[0] &= ~(mask[clue[k]] << i);
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
	for (int k = 0; k <= clue[0]; k++) {
	    // clue[1..k] on left
	    // clue[k+1 .. n] on right

	    if (solcount_l[k][i - 1] && solcount_r[k+1][i+1]) {
		candi[1] &= ~(1 << i);
		break;
	    }
	}
    }

    new_known[0] = known[0] | candi[0];
    new_known[1] = known[1] | candi[1];

#ifdef USE_CACHE
    cache_set(dir, idx, known, new_known, *count);
#endif
    return true;
}

