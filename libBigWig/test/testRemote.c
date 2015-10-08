#include "bigWig.h"
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <assert.h>

//Print overly verbose header information
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

//This is an example call back function
CURLcode callBack(CURL *curl) {
    CURLcode rv;

    rv = curl_easy_setopt(curl, CURLOPT_USERNAME, "anonymous");
    if(rv != CURLE_OK) return rv;

    rv = curl_easy_setopt(curl, CURLOPT_PASSWORD, "libBigWig@github.com");
    return rv;
}

int main(int argc, char *argv[]) {
    bigWigFile_t *fp = NULL;
    bwOverlappingIntervals_t *intervals = NULL;
    double *stats = NULL;
    char chrom[] = "chr1";
    if(argc != 2) {
        fprintf(stderr, "Usage: %s {file.bw|URL://path/file.bw}\n", argv[0]);
        return 1;
    }

    if(bwInit(1<<17) != 0) {
        fprintf(stderr, "Received an error in bwInit\n");
        return 1;
    }

    fp = bwOpen(argv[1], callBack);
    if(!fp) {
        fprintf(stderr, "An error occured while opening %s\n", argv[1]);
        return 1;
    }

    bwPrintHdr(fp);
    bwPrintIndexTree(fp);

    //Try to get some blocks
    printf("%s:10000000-10000100 Intervals\n", chrom);
    intervals = bwGetOverlappingIntervals(fp, chrom, 10000000, 10000100);
    printIntervals(intervals,0);
    bwDestroyOverlappingIntervals(intervals);

    printf("%s:10000000-10000100 Values\n", chrom);
    intervals = bwGetValues(fp, chrom, 10000000, 10000100, 0);
    printIntervals(intervals,0);
    bwDestroyOverlappingIntervals(intervals);

    stats = bwStats(fp, chrom, 10000000, 10000100, 1, mean);
    if(stats) {
        printf("%s:10000000-10000100 mean: %f\n", chrom, stats[0]);
        free(stats);
    }

    stats = bwStats(fp, chrom, 10000000, 10000100, 2, mean);
    if(stats) {
        printf("%s:10000000-10000100 mean: %f %f\n", chrom, stats[0], stats[1]);
        free(stats);
    }

    stats = bwStats(fp, chrom, 10000000, 10000100, 1, dev);
    if(stats) {
        printf("%s:10000000-10000100 std. dev.: %f\n", chrom, stats[0]);
        free(stats);
    }

    stats = bwStats(fp, chrom, 10000000, 10000100, 2, dev);
    if(stats) {
        printf("%s:10000000-10000100 std. dev.: %f %f\n", chrom, stats[0], stats[1]);
        free(stats);
    }

    stats = bwStats(fp, chrom, 10000000, 10000100, 1, min);
    if(stats) {
        printf("%s:10000000-10000100 min: %f\n", chrom, stats[0]);
        free(stats);
    }

    stats = bwStats(fp, chrom, 10000000, 10000100, 2, min);
    if(stats) {
        printf("%s:10000000-10000100 min: %f %f\n", chrom, stats[0], stats[1]);
        free(stats);
    }

    stats = bwStats(fp, chrom, 10000000, 10000100, 1, max);
    if(stats) {
        printf("%s:10000000-10000100 max: %f\n", chrom, stats[0]);
        free(stats);
    }

    stats = bwStats(fp, chrom, 10000000, 10000100, 2, max);
    if(stats) {
        printf("%s:10000000-10000100 max: %f %f\n", chrom, stats[0], stats[1]);
        free(stats);
    }

    stats = bwStats(fp, chrom, 10000000, 10000100, 1, cov);
    if(stats) {
        printf("%s:10000000-10000100 coverage: %f\n", chrom, stats[0]);
        free(stats);
    }

    stats = bwStats(fp, chrom, 10000000, 10000100, 2, cov);
    if(stats) {
        printf("%s:10000000-10000100 coverage: %f %f\n", chrom, stats[0], stats[1]);
        free(stats);
    }

    bwClose(fp);
    bwCleanup();
    return 0;
}
