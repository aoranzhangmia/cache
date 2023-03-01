#include "cachelab.h"
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int h;
int v;
int s = -1;
int E;
int b = -1;
int S;
unsigned long B;
char *t = NULL;

int main(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt) {
        case 'h':
            printf(
                "Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n"
                "-h: Optional help flag that prints usage info\n"
                "-v: Optional verbose flag that displays trace info\n"
                "-s <s>: Number of set index bits (S = 2^s is the number of "
                "sets)\n"
                "-E <E>: Associativity (number of lines per set)\n"
                "-b <b>: Number of block bits (B = 2^b is the block size)\n"
                "-t <tracefile>: Name of the memory trace to replay");
            return 1;
        case 'v':
            v = 1;
            break;
        case 's':
            s = (int)atol(optarg);
            break;
        case 'E':
            E = (int)atol(optarg);
            break;
        case 'b':
            b = (int)atol(optarg);
            break;
        case 't':
            t = optarg;
            break;
        default:
            exit(EXIT_FAILURE);
            // not recognized option
        }
    }
    if (s == -1 || b == -1 || E == 0 || t == NULL) {
        printf("Mandatory arguments missing or zero.");
        exit(-1);
    } // s, b, E, t value not supplied/missing
    if (s < 0 || b < 0 || E < 0 || s + b >= 64) {
        printf("Invalid Arguments.");
        exit(-1);
    } // s, b, E value not positive or is too large
    B = (unsigned long)(1 << b);
    return 0;
}
