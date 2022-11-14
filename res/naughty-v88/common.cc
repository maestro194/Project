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
#include <stdarg.h>
#include <assert.h>
#ifdef WIN32
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif
#include "config.h"
#include "nono.h"
#include "naughty.h"

int n;
bool non_square;
int width, height;
int flag_verbose;
const char *flag_log_filename = NULL; //"nonogram.log";
const char *log_tag = "-";


void boardline_to_clue(const Value *boardline, int stride, int clue[N])
{
    int begin=-1;

    clue[0] = 0;
    for(int j=1;j<=n;j++) {
	if(boardline[j*stride] && (j==1 || boardline[j*stride]!=boardline[(j-1)*stride])) {
	    begin=j;
	} else if(boardline[j*stride]!=boardline[(j-1)*stride]) {
	    clue[++clue[0]]=j-begin;
	}
    }
    if (boardline[n*stride])
	clue[++clue[0]]=n+1-begin;
}

void output_line(Line line)
{
    for (int i=1; i <= n; i++)
	putchar((line & ((Line)1<<i)) ? 'X':'.');
    putchar('\n');
}

void output_clue(const int clue[N])
{
    printf("clue: (%d)", clue[0]);
    for (int i=1;i<=clue[0];i++)
	printf(" %d", clue[i]);
    putchar('\n');
}

void output_known(const Line known[2])
{
    for (int i=1; i <= n; i++) {
	assert(!(BGET(known[0],i) && BGET(known[1],i)));
	if (BGET(known[0],i))
	    putchar('-');
	else if (BGET(known[1],i))
	    putchar('+');
	else
	    putchar('.');
	fflush(stdout);
    }
    putchar('\n');
}


void Show(Puzzle *puz)
{
  int i,j;
  char line[1024];
  int pos = 0;

  pos += sprintf(line+pos, "   ");
  for (i=1; i<= width;i++)
      pos += sprintf(line+pos, "%d", (i)%10);
  logging(1, line);

  for(i=1;i<=height;i++) {
      pos = 0;
      pos += sprintf(line, "%2d ", i);
      for(j=1;j<=width;j++) {
          if (puz->board[i][j] == Unknown)
              pos += sprintf(line+pos, "\033[1;30;43m?\033[0m");
          else if (puz->goal[i][j] != Unknown && puz->goal[i][j] != puz->board[i][j])
              pos += sprintf(line+pos,"\033[1;30;41m%c\033[0m", "?.X"[puz->board[i][j]+1]);
	  else
              pos += sprintf(line+pos,"%c", "?.X"[puz->board[i][j]+1]);
      }
      logging(1, line);
  }
  logging(1, "");
}

void InitBoard(Puzzle *puz)
{
    int i, j;
    assert(n);
    if (!non_square)
	width = height = n;

    for(i=1;i<=n;i++)
        for(j=1;j<=n;j++) {
            puz->board[i][j] = Unknown;
	}
}

void check_goal(Puzzle *puz)
{
    for (int i = 1; i <= height ; i++) {
	for (int j = 1; j <= width; j++)
	    if (puz->board[i][j] != Unknown && puz->goal[i][j] != Unknown && puz->board[i][j] != puz->goal[i][j]) {
		printf("! mismatch\n");
		Show(puz);
		return;
	    }
    }
    for (int i = 1; i <= height; i++) {
	for (int j = 1; j <= width; j++)
	    if (puz->board[i][j] == Unknown) {
		printf("! not solved yet\n");
		Show(puz);
		return;
	    }
    }
    printf("! match\n");
    Show(puz);
}

void init_puzzle(Puzzle *puz)
{
    memset(puz, 0, sizeof(*puz));
    for (int i = 1; i < N; i++)
	for (int j = 1; j < N; j++)
	    puz->goal[i][j] = Unknown;
}

double now(void)
{
#ifndef WIN32
    struct timeval tnow;
    double tval;
    gettimeofday(&tnow,0);
    tval=tnow.tv_sec+tnow.tv_usec/1e6;
#else
    struct timeb tnow;
    double tval;
    ftime(&tnow);
    tval=tnow.time+tnow.millitm/1e3;
#endif
    return tval;
}

void logit(const char *format,...)
{
    char fmt[1024];
    FILE *fp;
    va_list ap;

    if (!flag_log_filename)
	return;
    fp=fopen(flag_log_filename,"a");
    sprintf(fmt,"%f %s %s\n",now(), log_tag, format);
    va_start(ap,format);
    vfprintf(fp,fmt,ap);
    va_end(ap);
    fclose(fp);
}

void logging(int v,const char *format,...)
{
    char buf[1024];
    int len;

    va_list ap;

    if(flag_verbose>=v) {
	va_start(ap,format);
	vprintf(format,ap);
	va_end(ap);
	printf("\n");
	fflush(stdout);
    }

    len=sprintf(buf,"# %d ",v);
    va_start(ap,format);
    vsprintf(buf+len,format,ap);
    logit("%s",buf);
    va_end(ap);
}

