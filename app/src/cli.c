#include "editor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Utility: Read entire file into a memory buffer
static unsigned char *slurp(const char *fn, size_t *len) {
    FILE *f = fopen(fn, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open file: %s\n", fn);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    size_t sz = ftell(f);
    rewind(f);
    unsigned char *buf = malloc(sz ? sz : 1);
    if (!buf) {
        fprintf(stderr, "Out of memory\n");
        fclose(f);
        return NULL;
    }
    if (fread(buf, 1, sz, f) != sz) {
        fprintf(stderr, "Failed to read file: %s\n", fn);
        fclose(f);
        free(buf);
        return NULL;
    }
    fclose(f);
    if (len) *len = sz;
    return buf;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s input.png instructions.yaml > output.jpg 2> log.txt\n", argv[0]);
        return 1;
    }
    size_t imglen = 0, yamllen = 0;
    unsigned char *imgbuf = slurp(argv[1], &imglen);
    unsigned char *yamlbuf = slurp(argv[2], &yamllen);
    if (!imgbuf || !yamlbuf) {
        free(imgbuf); free(yamlbuf);
        return 2;
    }

    int rc = editor_process(
        imgbuf, imglen,
        (char*)yamlbuf, yamllen,
        stdout
    );

    free(imgbuf);
    free(yamlbuf);
    return rc;
}
