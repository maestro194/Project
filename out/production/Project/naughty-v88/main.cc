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
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "config.h"
#include "nono.h"
#include "naughty.h"

void usage(const char *program_name)
{
    printf("Usage: %s [-v#] [-u] [-l file] [-C#] < non_format_puzzle\n",
	    program_name);
    printf("    -u        Check uniqueness.\n");
    printf("    -l file   Specify log filename, default no log.\n");
    printf("    -v#       Verbose level.\n");
    printf("    -C#       Hint max cache size, MB.\n");
}

void parse_flags(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-u") == 0) {
	    flag_single_solution = false;
	} else if (strncmp(argv[i], "-v", 2) == 0) {
	    int arg = atoi(argv[i]+2);
	    flag_verbose = arg;
	} else if (strcmp(argv[i], "-l") == 0 &&
	       	argv[i+1] && argv[i+1][0] != '-') {
	    flag_log_filename = strdup(argv[i+1]);
	} else if (strncmp(argv[i], "-C", 2) == 0) {
	    int arg = atoi(argv[i]+2);
	    flag_max_cache_size = arg;
	} else if (strcmp(argv[i], "-h") == 0) {
	    usage(argv[0]);
	    exit(0);
	} else {
	    printf("Unknown argument: %s\n", argv[i]);
	    usage(argv[0]);
	    exit(1);
	}
    }
}

int main(int argc, char *argv[])
{
    Solving ctx;
    init_naughty();

    flag_verbose = 2;
    flag_single_solution = true;
    parse_flags(argc, argv);
    while (GetDataNon(&ctx)) {
	clock_t st,et;
	st = clock();
	solve(&ctx);
	et = clock();
	printf("Processing Time: %f sec\n",
		(double)(et-st)/CLOCKS_PER_SEC);
	for (int i = 0; i < (int)ctx.solution.size(); i++) {
	    memcpy(ctx.board, ctx.solution[i].board, sizeof(ctx.board));
	    check_goal(&ctx);
	}
	stat_show(&ctx);
    }

    return 0;
}

