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
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>

#include "nono.h"

int GetDataNon(Puzzle *puz)
{
    int i,k;
    char line[N*N+1024];
    char keyword[1024];
    bool ok = false;

    init_puzzle(puz);
    while (fgets(line, sizeof(line), stdin)) {
	if (sscanf(line, "width %d", &width)== 1) {
	    ok = true;
	    continue;
	}
	if (sscanf(line, "height %d", &height)== 1)
	    continue;

	if (sscanf(line, "%s", keyword) == 1 && 
		(strcmp(keyword, "rows")==0 || strcmp(keyword, "columns")==0)) {
	    if (width != height) {
		non_square = true;
		n = width > height ? width : height;
	    } else {
		non_square = false;
		n = width;
	    }
	    if (n + 1 > N) {
		printf("! unsupported size (%d, %d)\n", width, height);
		return false;
	    }

	    int nline;
	    if (strcmp(keyword, "rows")==0) {
		k = LR;
		nline = height;
	    } else {
		k = UD;
		nline = width;
	    }

	    for(i=1;i<=nline;i++) {
		char *p;
		int j = 1;
		fgets(line, sizeof(line), stdin);
		for (p = strtok(line, ", \t\n"); p; p = strtok(NULL, ", \t\n")) {
		    puz->clue[k][i][j++] = atoi(p);
		    //printf("%d ", atoi(p));
		}
		if (j == 2 && puz->clue[k][i][1] == 0)
		    j--;
		//printf("\n");
		puz->clue[k][i][0] = j - 1;
	    }

        for (int i=1;i<=n;i++)
            for (int j=1;j<=n;j++) {
                puz->goal[i][j] = Unknown;
	    }
	}
	char goalline[N*N+1024];
	if (sscanf(line, "goal %s", goalline) == 1) {
	    char *p = goalline;
	    for (int i=1;i<=height;i++) {
		for (int j=1;j<=width;j++) {
		    int c;
		    do {
			c = *p++;
			if (c == '\0')
			    return 0;
		    } while(!isdigit(c));
		    if (c == '0')
			puz->board[i][j] = WHITE;
		    else
			puz->board[i][j] = BLACK;
		}
	    }
	    memcpy(puz->goal, puz->board, sizeof(puz->board));
	} else if (sscanf(line, "%s", keyword) == 1 && strcmp(keyword, "goal") == 0) {
	    for (int i=1;i<=height;i++) {
		for (int j=1;j<=width;j++) {
		    int c;
		    do {
			c = fgetc(stdin);
			if (c == EOF)
			    return 0;
		    } while(!isdigit(c));
		    if (c == '0')
			puz->board[i][j] = WHITE;
		    else
			puz->board[i][j] = BLACK;
		}
	    }
	    memcpy(puz->goal, puz->board, sizeof(puz->board));
	}
    }

    return ok;
}

int GetDataTCGA(Puzzle *puz)
{
    int i,k;
    char line[1024];

    init_puzzle(puz);
    if (!fgets(line, sizeof(line), stdin))
        return 0;
    if (line[0] == '$') {
        n = 25;
        for(k=1;k>=0;k--)
            for(i=1;i<=n;i++) {
                char *p;
                int j = 1;
                fgets(line, sizeof(line), stdin);
                for (p = strtok(line, " \t\n"); p; p = strtok(NULL, " \t\n")) {
                    puz->clue[k][i][j++] = atoi(p);
                    //printf("%d ", atoi(p));
                }
                //printf("\n");
                puz->clue[k][i][0] = j - 1;
            }
        return 1;
    } else {
        return 1;
    }
    return 0;
}

void OutputNon(const Puzzle *puz)
{
    int i,j;
    int clue[N];

    printf("width %d\n", width);
    printf("height %d\n", height);

    printf("\n");
    printf("rows\n");
    for(i=1;i<=n;i++) {
	boardline_to_clue(&puz->board[i][0], 1, clue);
	if (clue[0] == 0)
	    printf("0\n");
	for(j=1;j<=clue[0];j++)
	    printf("%d%c",clue[j], j == clue[0]?'\n':',');
    }

    printf("\n");
    printf("columns\n");
    for(i=1;i<=n;i++) {
	boardline_to_clue(&puz->board[0][i], N, clue);
	if (clue[0] == 0)
	    printf("0\n");
	for(j=1;j<=clue[0];j++)
	    printf("%d%c",clue[j], j==clue[0]?'\n':',');
    }

    printf("\n");
    printf("goal\n");
    for (int i=1; i <= n; i++) {
	for (int j=1; j <= n; j++)
	    printf("%d", puz->board[i][j]);
	printf("\n");
    }
}

void OutputTCGA(const Puzzle *puz, int idx)
{
    int i,j;
    int clue[N];

    printf("$%d\n", idx);

    for(i=1;i<=n;i++) {
	boardline_to_clue(&puz->board[0][i], N, clue);
	if (clue[0] == 0)
	    printf("0\n");
	for(j=1;j<=clue[0];j++)
	    printf("%d%c",clue[j], j==clue[0]?'\n':'\t');
    }
    for(i=1;i<=n;i++) {
	boardline_to_clue(&puz->board[i][0], 1, clue);
	if (clue[0] == 0)
	    printf("0\n");
	for(j=1;j<=clue[0];j++)
	    printf("%d%c",clue[j], j == clue[0]?'\n':'\t');
    }
}

