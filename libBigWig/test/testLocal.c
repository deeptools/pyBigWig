#include "bigWig.h"
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <assert.h>

void bwPrintHdr(bigWigFile_t *bw) {
    uint64_t i;
    printf("Version:    %"PRIu16"\n", bw->hdr->version);
    printf("Levels:     %"PRIu16"\n", bw->hdr->nLevels);
    printf("ctOffset:   0x%"PRIx64"\n", bw->hdr->ctOffset);
    printf("dataOffset: 0x%"PRIx64"\n", bw->hdr->dataOffset);
    printf("indexOffset:        0x%"PRIx64"\n", bw->hdr->indexOffset);
    printf("sqlOffset:  0x%"PRIx64"\n", bw->hdr->sqlOffset);
    printf("summaryOffset:      0x%"PRIx64"\n", bw->hdr->summaryOffset);
    printf("bufSize:    %"PRIu32"\n", bw->hdr->bufSize);
    printf("extensionOffset:    0x%"PRIx64"\n", bw->hdr->extensionOffset);

    if(bw->hdr->nLevels) {
        printf("	i	level	data	index\n");
    }
    for(i=0; i<bw->hdr->nLevels; i++) {
        printf("\t%"PRIu64"\t%"PRIu32"\t%"PRIx64"\t%"PRIx64"\n", i, bw->hdr->zoomHdrs->level[i], bw->hdr->zoomHdrs->dataOffset[i], bw->hdr->zoomHdrs->indexOffset[i]);
    }

    printf("nBasesCovered:      %"PRIu64"\n", bw->hdr->nBasesCovered);
    printf("minVal:     %f\n", bw->hdr->minVal);
    printf("maxVal:     %f\n", bw->hdr->maxVal);
    printf("sumData:    %f\n", bw->hdr->sumData);
    printf("sumSquared: %f\n", bw->hdr->sumSquared);

    //Chromosome idx/name/length
    if(bw->cl) {
        printf("Chromosome List\n");
        printf("  idx\tChrom\tLength (bases)\n");
        for(i=0; i<bw->cl->nKeys; i++) {
            printf("  %"PRIu64"\t%s\t%"PRIu32"\n", i, bw->cl->chrom[i], bw->cl->len[i]);
        }
    }
}

void bwPrintIndexNode(bwRTreeNode_t *node, int level) {
    uint16_t i;
    if(!node) return;
    for(i=0; i<node->nChildren; i++) {
        if(node->isLeaf) {
            printf("  %i\t%"PRIu32"\t%"PRIu32"\t%"PRIu32"\t%"PRIu32"\t0x%"PRIx64"\t%"PRIu64"\n", level,\
                node->chrIdxStart[i], \
                node->baseStart[i], \
                node->chrIdxEnd[i], \
                node->baseEnd[i], \
                node->dataOffset[i], \
                node->x.size[i]);
        } else {
            printf("  %i\t%"PRIu32"\t%"PRIu32"\t%"PRIu32"\t%"PRIu32"\t0x%"PRIx64"\tNA\n", level,\
                node->chrIdxStart[i], \
                node->baseStart[i], \
                node->chrIdxEnd[i], \
                node->baseEnd[i], \
                node->dataOffset[i]);
            bwPrintIndexNode(node->x.child[i], level+1);
        }
    }
}

void bwPrintIndexTree(bigWigFile_t *fp) {
    printf("\nIndex tree:\n");
    printf("nItems:\t%"PRIu64"\n", fp->idx->nItems);
    printf("chrIdxStart:\t%"PRIu32"\n", fp->idx->chrIdxStart);
    printf("baseStart:\t%"PRIu32"\n", fp->idx->baseStart);
    printf("chrIdxEnd:\t%"PRIu32"\n", fp->idx->chrIdxEnd);
    printf("baseEnd:\t%"PRIu32"\n", fp->idx->baseEnd);
    printf("idxSize:\t%"PRIu64"\n", fp->idx->idxSize);
    printf("  level\tchrIdxStart\tbaseStart\tchrIdxEnd\tbaseEnd\tchild\tsize\n");
    bwPrintIndexNode(fp->idx->root, 0);
}

void printIntervals(bwOverlappingIntervals_t *ints, uint32_t start) {
    uint32_t i;
    if(!ints) return;
    for(i=0; i<ints->l; i++) {
        if(ints->start && ints->end) {
            printf("Interval %"PRIu32"\t%"PRIu32"-%"PRIu32": %f\n",i, ints->start[i], ints->end[i], ints->value[i]);
        } else if(ints->start) {
            printf("Interval %"PRIu32"\t%"PRIu32"-%"PRIu32": %f\n",i, ints->start[i], ints->start[i]+1, ints->value[i]);
        } else {
            printf("Interval %"PRIu32"\t%"PRIu32"-%"PRIu32": %f\n",i, start+i, start+i+1, ints->value[i]);
        }
    }
}

int main(int argc, char *argv[]) {
    bigWigFile_t *fp = NULL;
    bwOverlappingIntervals_t *intervals = NULL;
    double *stats = NULL;
    if(argc != 2) {
        fprintf(stderr, "Usage: %s {file.bw|URL://path/file.bw}\n", argv[0]);
        return 1;
    }

    if(bwInit(1<<17) != 0) {
        fprintf(stderr, "Received an error in bwInit\n");
        return 1;
    }

    fp = bwOpen(argv[1], NULL);
    if(!fp) {
        fprintf(stderr, "An error occured while opening %s\n", argv[1]);
        return 1;
    }

    bwPrintHdr(fp);
    bwPrintIndexTree(fp);

    //Try to get some blocks
    printf("1:0-99\n");
    intervals = bwGetValues(fp, "1", 0, 99, 0);
    printIntervals(intervals,0);
    bwDestroyOverlappingIntervals(intervals);

    printf("1:99-1000\n");
    intervals = bwGetValues(fp, "1", 99, 1000, 0);
    printIntervals(intervals,0);
    bwDestroyOverlappingIntervals(intervals);

    printf("1:99-150\n");
    intervals = bwGetValues(fp, "1", 99, 150, 1);
    printIntervals(intervals,99);
    bwDestroyOverlappingIntervals(intervals);

    printf("1:99-100\n");
    intervals = bwGetValues(fp, "1", 99, 100, 0);
    printIntervals(intervals,0);
    bwDestroyOverlappingIntervals(intervals);

    printf("1:151-1000\n");
    intervals = bwGetValues(fp, "1", 151, 1000, 0);
    printIntervals(intervals,0);
    bwDestroyOverlappingIntervals(intervals);

    printf("chr1:0-100\n");
    intervals = bwGetValues(fp, "chr1", 0, 100, 0);
    printIntervals(intervals,0);
    bwDestroyOverlappingIntervals(intervals);

    stats = bwStats(fp, "1", 0, 200, 1, mean);
    assert(stats);
    printf("1:0-1000 mean: %f\n", *stats);
    free(stats);

    stats = bwStats(fp, "1", 0, 200, 2, mean);
    assert(stats);
    printf("1:0-1000 mean: %f %f\n", stats[0], stats[1]);
    free(stats);

    stats = bwStats(fp, "1", 0, 200, 1, dev);
    assert(stats);
    printf("1:0-1000 std. dev.: %f\n", *stats);
    free(stats);

    stats = bwStats(fp, "1", 0, 200, 2, dev);
    assert(stats);
    printf("1:0-1000 std. dev.: %f %f\n", stats[0], stats[1]);
    free(stats);

    stats = bwStats(fp, "1", 0, 200, 1, min);
    assert(stats);
    printf("1:0-1000 min: %f\n", *stats);
    free(stats);

    stats = bwStats(fp, "1", 0, 200, 2, min);
    assert(stats);
    printf("1:0-1000 min: %f %f\n", stats[0], stats[1]);
    free(stats);

    stats = bwStats(fp, "1", 0, 200, 1, max);
    assert(stats);
    printf("1:0-1000 max: %f\n", *stats);
    free(stats);

    stats = bwStats(fp, "1", 0, 200, 2, max);
    assert(stats);
    printf("1:0-1000 max: %f %f\n", stats[0], stats[1]);
    free(stats);

    stats = bwStats(fp, "1", 0, 200, 1, cov);
    assert(stats);
    printf("1:0-1000 coverage: %f\n", *stats);
    free(stats);

    stats = bwStats(fp, "1", 0, 200, 2, cov);
    assert(stats);
    printf("1:0-1000 coverage: %f %f\n", stats[0], stats[1]);
    free(stats);

    bwClose(fp);
    bwCleanup();
    return 0;
}
