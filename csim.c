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
unsigned long hit = 0;             /* number of hits */
unsigned long miss = 0;            /* number of misses */
unsigned long eviction = 0;        /* number of evictions */
unsigned long dirty_bytes = 0;     /* number of dirty bytes in cache at end */
unsigned long dirty_evictions = 0; /* number of dirty bytes evicted */

/**
 * @brief Line structure of a set
 */
typedef struct {
    int valid;     /* valid bit set 1 if line has data loaded */
    int dirty_bit; /* dirty bit set 1 if payload has been modified, but has not
                      written back to memory */
    unsigned long tag;        /* tag of line */
    unsigned long time_stamp; /* counter to achieve LRU counter */
} line_t;

/**
 * @brief Set structure of a cache
 */
typedef struct {
    line_t *lines; /* pointer to lines of a set */
} set_t;

/**
 * @brief Cache structure
 */
typedef struct {
    int s;       /* Number of set index bits */
    int E;       /* Associativity (number of lines per set) */
    int b;       /* Number of block bits */
    set_t *sets; /* pointer to sets of a cache */
} cache_t;

cache_t *cache;

void sim(char *op, unsigned long tag, unsigned long index) {
    set_t *set = &(cache->sets[index]);
    for (int i = 0; i < E; i++) {
        if (set->lines[i].valid == 1) {
            set->lines[i].time_stamp++;
        }
    }
    // hit case
    int hit_place = -1;
    for (int i = 0; i < E; i++) {
        line_t *line = &(set->lines[i]);
        if (line->valid == 1 && line->tag == tag) {
            hit++;
            line->time_stamp = 0;
            hit_place = i;
            break;
        }
    }
    if (hit_place >= 0) {
        for (int j = hit_place; j < E; j++) {
            set->lines[j].time_stamp++;
        }
        if (strcmp(op, "S") == 0 && set->lines[hit_place].dirty_bit == 0) {
            set->lines[hit_place].dirty_bit = 1;
            dirty_bytes += B;
        }
        if (v) {
            printf("hit\n");
        }
        return;
    }
    // miss case
    // not valid case
    miss++;
    int has_unused = -1;
    for (int i = 0; i < E; i++) {
        line_t *line = &(set->lines[i]);
        if (line->valid == 0) {
            line->valid = 1;
            line->tag = tag;
            line->time_stamp = 0;
            has_unused = i;
            break;
        }
    }
    if (has_unused >= 0) {
        for (int j = has_unused; j < E; j++) {
            set->lines[j].time_stamp++;
        }
        if (strcmp(op, "S") == 0) {
            set->lines[has_unused].dirty_bit = 1;
            dirty_bytes += B;
        }
        if (v) {
            printf("miss\n");
        }
        return;
    }
    // eviction case
    if (v) {
        printf("miss ");
    }
    eviction++;
    int evind = 0;
    unsigned long max = set->lines[0].time_stamp;
    for (int i = 0; i < E; i++) {
        line_t *line = &(set->lines[i]);
        if (line->time_stamp > max) {
            evind = i;
            max = line->time_stamp;
        }
    }
    set->lines[evind].time_stamp = 0;
    set->lines[evind].tag = tag;
    for (int j = 0; j < evind; j++) {
        set->lines[j].time_stamp++;
    }
    for (int j = evind + 1; j < E; j++) {
        set->lines[j].time_stamp++;
    }
    if (set->lines[evind].dirty_bit == 1) {
        dirty_evictions += B;
        if (strcmp(op, "L") == 0) {
            dirty_bytes -= B;
            set->lines[evind].dirty_bit = 0;
        }
    } else {
        if (strcmp(op, "S") == 0) {
            set->lines[evind].dirty_bit = 1;
            dirty_bytes += B;
        }
    }
    if (v) {
        printf("eviction\n");
    }
}

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
    int parse = process_trace_file(t);
    csim_stats_t stats;
    stats.hits = hit;
    stats.misses = miss;
    stats.evictions = eviction;
    stats.dirty_bytes = dirty_bytes;
    stats.dirty_evictions = dirty_evictions;
    printSummary(&stats);

    return 0;
}
