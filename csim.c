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


/** Process a memory -access trace file.
 *
 * @param trace Name of the trace file to process.
 * @return 0 if successful , 1 if there were errors.
 */
int process_trace_file(const char *trace) {
    FILE *tfp = fopen(trace, "rt");
    if (!tfp) {
        fprintf(stderr, "Error opening '%s ': %s\n", trace, strerror(errno));
        return 1;
    }
    S = 1 << s;
    /*Initialize Cache */
    cache = (cache_t *)malloc(sizeof(cache_t));
    {
        if (cache == NULL) {
            return 1;
        }
        cache->s = s;
        cache->b = b;
        cache->E = E;
        cache->sets = (set_t *)malloc(sizeof(set_t) * (unsigned long)S);
        if (cache->sets == NULL) {
            free(cache);
            return 1;
        }
        for (int i = 0; i < S; i++) {
            cache->sets[i].lines =
                (line_t *)malloc(sizeof(line_t) * (unsigned long)E);
            if (cache->sets[i].lines == NULL) {
                free(cache->sets);
                free(cache);
                return 1;
            }
            for (int j = 0; j < E; j++) {
                cache->sets[i].lines[j].valid = 0;
                cache->sets[i].lines[j].dirty_bit = 0;
                cache->sets[i].lines[j].tag = 0;
                cache->sets[i].lines[j].time_stamp = 0;
            }
        }
    }

    int LINELEN = 40;
    char linebuf[LINELEN]; // How big should LINELEN be?
    int parse_error = 0;
    while (fgets(linebuf, LINELEN, tfp)) {
        // Parse the line of text in linebuf.
        // What do you do if the line is incorrect ?
        // What do you do if the line is longer than
        // LINELEN -1 chars?
        char *op = strtok(linebuf, " ");
        char *char_addr = strtok(NULL, ",");
        unsigned long addr = strtoul(char_addr, NULL, 16);
        char *char_size = strtok(NULL, ",");
        unsigned long size = strtoul(char_size, NULL, 10);
        if (strlen(linebuf) > (unsigned long)(LINELEN - 1))
            parse_error = 1;
        if (op == NULL)
            parse_error = 1;
        if (strcmp(op, "S") != 0 && strcmp(op, "L") != 0)
            parse_error = 1;
        if (char_addr == NULL || addr < 0)
            parse_error = 1;
        if (size < 0 || (size != 1 && size != 2 && size != 4 && size != 8))
            parse_error = 1;
        unsigned long index = (addr >> b) & ((unsigned long)S - 1);
        unsigned long tag = addr >> (s + b);
        if (v) {
            printf("%s %lx,%d\n", op, addr, (int)size);
        }
        sim(op, tag, index);
    }
    fclose(tfp);
    return parse_error;
}


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
