#include "io.h"
#include "bwValues.h"
#include <inttypes.h>

/*! \mainpage libBigWig
 *
 * \section Introduction
 *
 * libBigWig is a C library for parsing local/remote bigWig files. This is similar to Kent's library from UCSC, except 
 *  * The license is much more liberal
 *  * This code doesn't call `exit()` on error, thereby killing the calling application.
 *
 * External files are accessed using [curl](http://curl.haxx.se/).
 *
 * Please submit issues and pull requests [here](https://github.com/dpryan79/libBigWig).
 *
 * \section Compilation
 *
 * Assuming you already have the curl libraries installed (not just the curl binary!):
 *
 *     make install prefix=/some/path
 *
 * \section Examples
 * 
 * Please see [README.md](README.md) and the files under `test/` for examples.
 */
 

/*! \file bigWig.h
 *
 * These are the functions and structured that should be used by external users. While I don't particularly recommend dealing with some of the structures (e.g., a bigWigHdr_t), they're described here in case you need them.
 *
 * BTW, this library doesn't switch endianness as appropriate, since I kind of assume that there's only one type produced these days.
 */


/*!
 * The magic number of a bigWig file.
 */
#define BIGWIG_MAGIC 0x888FFC26
/*!
 * The magic number of a "cirTree" block in a file.
 */
#define CIRTREE_MAGIC 0x78ca8c91
/*!
 * The magic number of an index block in a file.
 */
#define IDX_MAGIC 0x2468ace0

/*!
 * An enum that dictates the type of statistic to fetch for a given interval
 */
enum bwStatsType {
    doesNotExist = -1,
    mean = 0,
    average = 0,
    std = 1,
    stdev = 1,
    dev = 1,
    max = 2,
    min = 3,
    cov = 4,
    coverage = 4,
};

//Should hide this from end users
/*!
 * @brief BigWig files have multiple "zoom" levels, each of which has its own header. This hold those headers
 *
 * N.B., there's 4 bytes of padding in the on disk representation of level and dataOffset.
 */
typedef struct {
    uint32_t *level; /**<The zoom level, which is an integer starting with 0.*/
    //There's 4 bytes of padding between these
    uint64_t *dataOffset; /**<The offset to the on-disk start of the data. This isn't used currently.*/
    uint64_t *indexOffset; /**<The offset to the on-disk start of the index. This *is* used.*/
    bwRTree_t **idx; /**<Index for each zoom level. Represented as a tree*/
} bwZoomHdr_t;

/*!
 * @brief The header section of a bigWig file.
 *
 * Some of the values aren't currently used for anything. Others may optionally not exist.
 */
typedef struct {
    uint16_t version; /**<The version information of the file.*/
    uint16_t nLevels; /**<The number of "zoom" levels.*/
    uint64_t ctOffset; /**<The offset to the on-disk chromosome tree list.*/
    uint64_t dataOffset; /**<The on-disk offset to the first block of data.*/
    uint64_t indexOffset; /**<The on-disk offset to the data index.*/
    uint64_t sqlOffset; /**<The on-disk offset to an SQL string. This is unused.*/
    uint64_t summaryOffset; /**<If there's a summary, this is the offset to it on the disk.*/
    uint32_t bufSize; /**<The compression buffer size (if the data is compressed).*/
    uint64_t extensionOffset; /**<Unused*/
    bwZoomHdr_t *zoomHdrs; /**<Pointers to the header for each zoom level.*/
    //total Summary
    uint64_t nBasesCovered; /**<The total bases covered in the file.*/
    double minVal; /**<The minimum value in the file.*/
    double maxVal; /**<The maximum value in the file.*/
    double sumData; /**<The sum of all values in the file.*/
    double sumSquared; /**<The sum of the squared values in the file.*/
} bigWigHdr_t;

//Should probably replace this with a hash
/*!
 * @brief Holds the chromosomes and their lengths
 */
typedef struct {
    int64_t nKeys; /**<The number of chromosomes */
    char **chrom; /**<A list of null terminated chromosomes */
    uint32_t *len; /**<The lengths of each chromosome */
} chromList_t;

/*!
 * @brief A structure that holds everything needed to access a bigWig file.
 */
typedef struct {
    URL_t *URL; /**<A pointer that can handle both local and remote files (including a buffer if needed).*/
    bigWigHdr_t *hdr; /**<The file header.*/
    chromList_t *cl; /**<A list of chromosome names (the order is the ID).*/
    bwRTree_t *idx; /**<The index for the full dataset.*/
} bigWigFile_t;

//This should be hidden from end users
/*!
 * @brief Holds interval:value associations
 */
typedef struct {
    uint32_t l; /**<Number of intervals held*/
    uint32_t m; /**<Maximum number of values/intervals the struct can hold*/
    uint32_t *start; /**<The start positions (o-based half open)*/
    uint32_t *end; /**<The end positions (0-based half open)*/
    float *value; /**<The value associated with each position*/
} bwOverlappingIntervals_t;

/*!
 * @brief Initializes curl and global variables. This *MUST* be called before other functions (at least if you want to connect to remote files).
 * For remote file, curl must be initialized and regions of a file read into an internal buffer. If the buffer is too small then an excessive number of connections will be made. If the buffer is too large than more data than required is fetched. 128KiB is likely sufficient for most needs.
 * @param bufSize The internal buffer size used for remote connection.
 * @see bwCleanup
 * @return 0 on success and 1 on error.
 */
int bwInit(size_t bufSize);

/*!
 * @brief The counterpart to bwInit, this cleans up curl.
 * @see bwInit
 */
void bwCleanup(void);

/*!
 * @brief Opens a local or remote bigWig file.
 * This will open a local or remote bigWig file.
 * @param fname The file name or URL (http, https, and ftp are supported)
 * @param callBack An optional user-supplied function. This is applied to remote connections so users can specify things like proxy and password information. See `test/testRemote` for an example.
 * @return A bigWigFile_t * on success and NULL on error.
 */
bigWigFile_t *bwOpen(char *fname, CURLcode (*callBack)(CURL*));

/*!
 * @brief Closes a bigWigFile_t and frees up allocated memory
 * @param fp The file pointer.
 */
void bwClose(bigWigFile_t *fp);

/*!
 * @brief Like fsetpos, but for local or remote bigWig files.
 * This will set the file position indicator to the specified point. For local files this literally is `fsetpos`, while for remote files it fills a memory buffer with data starting at the desired position.
 * @param fp A valid opened bigWigFile_t.
 * @param pos The position within the file to seek to.
 * @return 0 on success and -1 on error.
 */
int bwSetPos(bigWigFile_t *fp, size_t pos);

/*!
 * @brief A local/remote version of `fread`.
 * Reads data from either local or remote bigWig files.
 * @param data An allocated memory block big enough to hold the data.
 * @param sz The size of each member that should be copied.
 * @param nmemb The number of members to copy.
 * @param fp The bigWigFile_t * from which to copy the data.
 * @see bwSetPos
 * @return For nmemb==1, the size of the copied data. For nmemb>1, the number of members fully copied (this is equivalent to `fread`).
 */
size_t bwRead(void *data, size_t sz, size_t nmemb, bigWigFile_t *fp);

/*!
 * @brief Determine what the file position indicator say.
 * This is equivalent to `ftell` for local or remote files.
 * @param fp The file.
 * @return The position in the file.
 */
long bwTell(bigWigFile_t *fp);

/*******************************************************************************
*
* The following are in bwStats.c
*
*******************************************************************************/

/*!
 * @brief Converts between chromosome name and ID
 *
 * @param fp A valid bigWigFile_t pointer
 * @param chrom A chromosome name
 * @return An ID, -1 will be returned on error (note that this is an unsigned value, so that's ~4 billion. BigWig files can't store that many chromosomes anyway.
 */
uint32_t bwGetTid(bigWigFile_t *fp, char *chrom);

/*!
 * @brief Reads a data index (either full data or a zoom level) from a bigWig file.
 * There is little reason for end users to use this function. This must be freed with `bwDestroyIndex`
 * @param fp A valid bigWigFile_t pointer
 * @param offset The file offset where the index begins
 * @return A bwRTree_t pointer or NULL on error.
 */
bwRTree_t *bwReadIndex(bigWigFile_t *fp, uint64_t offset);

/*!
 * @brief Frees space allocated by `bwReadIndex`
 * There is generally little reason to use this, since end users should typically not need to run `bwReadIndex` themselves.
 * @param idx A bwRTree_t pointer allocated by `bwReadIndex`.
 */
void bwDestroyIndex(bwRTree_t *idx);

/*!
 * @brief Frees space allocated by `bwGetOverlappingIntervals`
 * @param o A valid `bwOverlappingIntervals_t` pointer.
 * @see bwGetOverlappingIntervals
 */
void bwDestroyOverlappingIntervals(bwOverlappingIntervals_t *o);

/*!
 * @brief Return entries overlapping an interval.
 * Find all entries overlapping a range and returns them, including their associated values.
 * @param fp A valid bigWigFile_t pointer.
 * @param chrom A valid chromosome name.
 * @param start The start position of the interval. This is 0-based half open, so 0 is the first base.
 * @param end The end position of the interval. Again, this is 0-based half open, so 100 will include the 100th base...which is at position 99.
 * @return NULL on error or no overlapping values, otherwise a `bwOverlappingIntervals_t *` holding the values and intervals.
 * @see bwOverlappingIntervals_t
 * @see bwDestroyOverlappingIntervals
 * @see bwGetValues
 */
bwOverlappingIntervals_t *bwGetOverlappingIntervals(bigWigFile_t *fp, char *chrom, uint32_t start, uint32_t end);

/*!
 * @brief Return all per-base values in a given interval.
 * Given an interval (e.g., chr1:0-100), return the value at each position. Positions without associated values are suppressed by default, but may be returned if `includeNA` is not 0.
 * @param fp A valid bigWigFile_t pointer.
 * @param chrom A valid chromosome name.
 * @param start The start position of the interval. This is 0-based half open, so 0 is the first base.
 * @param end The end position of the interval. Again, this is 0-based half open, so 100 will include the 100th base...which is at position 99.
 * @param includeNA If not 0, report NA values as well (as NA).
 * @return NULL on error or no overlapping values, otherwise a `bwOverlappingIntervals_t *` holding the values and positions.
 * @see bwOverlappingIntervals_t
 * @see bwDestroyOverlappingIntervals
 * @see bwGetOverlappingIntervals
 */
bwOverlappingIntervals_t *bwGetValues(bigWigFile_t *fp, char *chrom, uint32_t start, uint32_t end, int includeNA);

/// @cond SKIP
//These should be hidden from users and moved elsewhere.
bwOverlapBlock_t *walkRTreeNodes(bigWigFile_t *bw, bwRTreeNode_t *root, uint32_t tid, uint32_t start, uint32_t end);
void destroyBWOverlapBlock(bwOverlapBlock_t *b);
/// @endcond

/*!
 * @brief Determines per-interval statistics
 * Can determine mean/min/max/coverage/standard deviation of values in one or more intervals. You can optionally give it an interval and ask for values from X number of sub-intervals.
 * @param fp The file from which to extract statistics.
 * @param chrom A valid chromosome name.
 * @param start The start position of the interval. This is 0-based half open, so 0 is the first base.
 * @param end The end position of the interval. Again, this is 0-based half open, so 100 will include the 100th base...which is at position 99.
 * @param nBins The number of bins within the interval to calculate statistics for.
 * @param type The type of statistic.
 * @see bwStatsType
 * @return A pointer to an array of double precission floating point values. Note that bigWig files only hold 32-bit values, so this is done to help prevent overflows.
 */
double *bwStats(bigWigFile_t *fp, char *chrom, uint32_t start, uint32_t end, uint32_t nBins, enum bwStatsType type);
