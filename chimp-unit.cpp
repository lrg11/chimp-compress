#include "chimp.h"
#include <cassert>
#include <limits.h>
#include <cmath>
#include <cstring>
#include <vector>

const int ENCODING_UNALIGNED_BUFFER = -2;
const int ENCODING_UNSUPPORT_TYPE_WIDTH = -1;
const int ENCODING_BUFFER_TOO_SMALL = -3;
const int ENCODING_BUFFER_OVERFLOW = -4;

struct ChimpN
{
    const uint64_t NAN_LONG = 0x7ff8000000000000L;
    int storedLeadingZeros = INT_MAX;
    uint64_t *storedValues;
    bool first = true;
    int size;
    int previousValuesLog2;
    int threshold;

    short leadingRepresentation[64] = {0, 0, 0, 0, 0, 0, 0, 0,
                                       1, 1, 1, 1, 2, 2, 2, 2,
                                       3, 3, 4, 4, 5, 5, 6, 6,
                                       7, 7, 7, 7, 7, 7, 7, 7,
                                       7, 7, 7, 7, 7, 7, 7, 7,
                                       7, 7, 7, 7, 7, 7, 7, 7,
                                       7, 7, 7, 7, 7, 7, 7, 7,
                                       7, 7, 7, 7, 7, 7, 7, 7};

    short leadingRound[64] = {0, 0, 0, 0, 0, 0, 0, 0,
                              8, 8, 8, 8, 12, 12, 12, 12,
                              16, 16, 18, 18, 20, 20, 22, 22,
                              24, 24, 24, 24, 24, 24, 24, 24,
                              24, 24, 24, 24, 24, 24, 24, 24,
                              24, 24, 24, 24, 24, 24, 24, 24,
                              24, 24, 24, 24, 24, 24, 24, 24,
                              24, 24, 24, 24, 24, 24, 24, 24};
    //      final static short FIRST_DELTA_BITS = 27;

    //      BitOutput obs;

    // OutputBitStream obs;
    // std::vector<uint8_t>obstr(8000, 0);

    OutputBitStream obs;

    int previousValues;

    int setLsb;

    int *indices;

    int index = 0;

    int current = 0;

    int flagOneSize;

    int flagZeroSize;

    // We should have access to the series?
    ChimpN(int preValues, uint32_t NITEMS)
    {
        uint8_t *obstr = new uint8_t[8 * NITEMS];
        obs = OutputBitStream(obstr);
        size = 0;
        this->previousValues = preValues;
        this->previousValuesLog2 = (int)(log(previousValues) / log(2));
        this->threshold = 6 + previousValuesLog2;
        this->setLsb = (int)pow(2, threshold + 1) - 1;
        this->indices = new int[(int)pow(2, threshold + 1)];
        this->storedValues = new uint64_t[previousValues];
        this->flagZeroSize = previousValuesLog2 + 2;
        this->flagOneSize = previousValuesLog2 + 11;
    }

    uint8_t *getOut()
    {
        return obs.buffer;
    }

    /**
     * Adds a new uint64_t value to the series. Note, values must be inserted in order.
     *
     * @param value next floating point value in the series
     */

    void addValue(uint64_t value)
    {
        if (first)
        {
            writeFirst(value);
        }
        else
        {
            compressValue(value);
        }
    }

    /**
     * Adds a new double value to the series. Note, values must be inserted in order.
     *
     * @param value next floating point value in the series
     */

    void addValue(double value)
    {
        if (first)
        {
            writeFirst(*((uint64_t *)&value));
        }
        else
        {
            compressValue(*((uint64_t *)&value));
        }
    }

    void writeFirst(uint64_t value)
    {
        first = false;
        storedValues[current] = value;
        obs.writeLong(storedValues[current], 64);
        indices[(int)value & setLsb] = index;
        size += 64;
    }

    /**
     * Closes the block and writes the remaining stuff to the BitOutput.
     */

    void close()
    {
        // C++ the unlike float8 value
        addValue(NAN_LONG);
        obs.writeBit(false);
        obs.flush();
    }

    void compressValue(uint64_t value)
    {
        int key = (int)value & setLsb;
        uint64_t xorvalue;
        int previousIndex;
        int trailingZeros = 0;
        int currIndex = indices[key];
        if ((index - currIndex) < previousValues)
        {
            uint64_t tempXor = value ^ storedValues[currIndex % previousValues];
            trailingZeros = __builtin_ctzll(tempXor);
            if (trailingZeros > threshold)
            {
                previousIndex = currIndex % previousValues;
                xorvalue = tempXor;
            }
            else
            {
                previousIndex = index % previousValues;
                xorvalue = storedValues[previousIndex] ^ value;
            }
        }
        else
        {
            previousIndex = index % previousValues;
            xorvalue = storedValues[previousIndex] ^ value;
        }

        if (xorvalue == 0)
        {
            obs.writeInt(previousIndex, this->flagZeroSize);
            size += this->flagZeroSize;
            storedLeadingZeros = 65;
        }
        else
        {
            int leadingZeros = leadingRound[__builtin_clzll(xorvalue)];

            if (trailingZeros > threshold)
            {
                int significantBits = 64 - leadingZeros - trailingZeros;
                obs.writeInt(512 * (previousValues + previousIndex) + 64 * leadingRepresentation[leadingZeros] + significantBits, this->flagOneSize);
                obs.writeLong(xorvalue >> trailingZeros, significantBits); // Store the meaningful bits of XOR
                size += significantBits + this->flagOneSize;
                storedLeadingZeros = 65;
            }
            else if (leadingZeros == storedLeadingZeros)
            {
                obs.writeInt(2, 2);
                int significantBits = 64 - leadingZeros;
                obs.writeLong(xorvalue, significantBits);
                size += 2 + significantBits;
            }
            else
            {
                storedLeadingZeros = leadingZeros;
                int significantBits = 64 - leadingZeros;
                obs.writeInt(24 + leadingRepresentation[leadingZeros], 5);
                obs.writeLong(xorvalue, significantBits);
                size += 5 + significantBits;
            }
        }
        current = (current + 1) % previousValues;
        storedValues[current] = value;
        index++;
        indices[key] = index;
    }

    int getSize()
    {
        return size;
    }
};

/**
 * Decompresses a compressed stream created by the Compressor. Returns pairs of timestamp and floating point value.
 *
 */
struct ChimpNDecompressor
{
    int storedLeadingZeros = INT_MAX;
    int storedTrailingZeros = 0;
    uint64_t storedVal = 0;
    uint64_t *storedValues;
    int current = 0;
    bool first = true;
    bool endOfStream = false;

    InputBitStream in;
    int previousValues;
    int previousValuesLog2;
    int initialFill;

    short leadingRepresentation[8] = {0, 8, 12, 16, 18, 20, 22, 24};

    const uint64_t NAN_LONG = 0x7ff8000000000000L;
    std::vector<double> list;

    uint32_t numItems;

    //  parameter name must be diff with member data,
    // otherwise using this->namexxx = namexxx
    ChimpNDecompressor(uint8_t *bs, int preValues, uint32_t NITEMS)
    {
        in = InputBitStream(bs, NITEMS);
        this->numItems = NITEMS;
        previousValues = preValues;
        previousValuesLog2 = (int)(log(previousValues) / log(2));
        initialFill = previousValuesLog2 + 9;
        storedValues = new uint64_t[previousValues];
    }

    /**
     * Returns the next pair in the time series, if available.
     *
     * @return Pair if there's next value, null if series is done.
     */
    double readValue()
    {
        next();

        if (endOfStream)
        {
            return -1.0;
        }
        return *((double *)&storedVal);
    }

    std::vector<double> getValues()
    {
        list.clear();
        double value = readValue();
        int ct = 0;

        while (ct < numItems && !endOfStream)
        {
            list.push_back(value);
            value = readValue();
            ct++;
        }
        return list;
    }

    void next()
    {
        if (first)
        {
            first = false;
            storedVal = in.readLong(64);
            storedValues[current] = storedVal;
            if (storedValues[current] == NAN_LONG)
            {
                endOfStream = true;
                return;
            }
        }
        else
        {
            nextValue();
        }
    }

    void nextValue()
    {
        // Read value
        int flag = in.readInt(2);
        uint64_t value;
        switch (flag)
        {
        case 3:
            storedLeadingZeros = leadingRepresentation[in.readInt(3)];
            value = in.readLong(64 - storedLeadingZeros);
            value = storedVal ^ value;

            if (value == NAN_LONG)
            {
                endOfStream = true;
                return;
            }
            else
            {
                storedVal = value;
                current = (current + 1) % previousValues;
                storedValues[current] = storedVal;
            }
            break;
        case 2:
            value = in.readLong(64 - storedLeadingZeros);
            value = storedVal ^ value;
            if (value == NAN_LONG)
            {
                endOfStream = true;
                return;
            }
            else
            {
                storedVal = value;
                current = (current + 1) % previousValues;
                storedValues[current] = storedVal;
            }
            break;
        case 1:
        {
            int fill = initialFill;
            int temp = in.readInt(fill);
            int index = temp >> (fill -= previousValuesLog2) & (1 << previousValuesLog2) - 1;
            storedLeadingZeros = leadingRepresentation[temp >> (fill -= 3) & (1 << 3) - 1];
            int significantBits = temp >> (fill -= 6) & (1 << 6) - 1;
            storedVal = storedValues[index];
            if (significantBits == 0)
            {
                significantBits = 64;
            }
            storedTrailingZeros = 64 - significantBits - storedLeadingZeros;
            value = in.readLong(64 - storedLeadingZeros - storedTrailingZeros);
            value <<= storedTrailingZeros;
            value = storedVal ^ value;
            if (value == NAN_LONG)
            {
                endOfStream = true;
                return;
            }
            else
            {
                storedVal = value;
                current = (current + 1) % previousValues;
                storedValues[current] = storedVal;
            }
            break;
        }
        default:
            // else -> same value as before
            storedVal = storedValues[(int)in.readLong(previousValuesLog2)];
            current = (current + 1) % previousValues;
            storedValues[current] = storedVal;
            break;
        }
    }
};

#define WINDOW_SIZE 128

/*
 * ret: -1    buffer overflow, fail to compress.
 */
int32_t
chimp_compress_data(const char *source, uint32_t source_size,
                    char *dest, uint32_t dst_size)
{
    uint32_t nitems;

    nitems = source_size >> 3;

    char *writePos = dest;

    if (sizeof(uint32_t) <= dst_size)
    {
        *((uint32_t *)(writePos)) = nitems;
        writePos += sizeof(uint32_t);
    }
    else
    {
        return ENCODING_BUFFER_TOO_SMALL;
    }

    ChimpN c(WINDOW_SIZE, nitems);

    for (uint32_t i = 0; i < nitems; i++)
    {
        // if (c.)
        //     return ENCODING_BUFFER_TOO_SMALL;
        // else
        c.addValue(*((uint64_t *)(source + i * 8)));
    }
    c.close();
    size_t bytesize = c.getSize() / 8;
    if (bytesize > dst_size - sizeof(uint32_t))
        return ENCODING_BUFFER_TOO_SMALL;
    memcpy(writePos, c.getOut(), bytesize);
    return bytesize + sizeof(uint32_t);
}

int32_t
chimp_decompress_data(const char *source, uint32_t source_size, char *dest, uint32_t dest_size)
{
    uint32_t nitems;

    // Supposed to have enough source buffer
    nitems = *((uint32_t *)(source));
    source += sizeof(uint32_t);

    if (nitems * 8 > dest_size)
        return ENCODING_BUFFER_OVERFLOW;
    uint8_t *src = (uint8_t *)source;
    ChimpNDecompressor dm(src, 128, nitems);
    // /* no enough source data.  TODO: should this be an error? */
    // if (dm.src_overflow)
    //     return -1;

    // uint32_t dmret = dm.decompressvalue(dest, dest_size);

    std::vector<double> uncompressedValues = dm.getValues();

    memcpy(dest, reinterpret_cast<char *>(&uncompressedValues[0]), nitems * 8);
    /* no enough source data.  TODO: should this be an error? */
    // if (dmret == -1)
    //     return -1;

    return nitems * 8;
}

#include <iostream>
#include <fstream>
#include <chrono>
using namespace std::chrono;
using namespace std;


// #define compresstype float
#define compresstype double

const int MAXN = 1200 * 3;

int main(int argc, char *argv[]) {
    int compresswidth = sizeof(compresstype);
    char *src = new char[compresswidth * MAXN];

    char *dst = new char[compresswidth * MAXN];

    if (argc < 2) {
        cout << "lack filename!!!" << endl;
        return -1;
    }
    char *filename = argv[1];
    auto infile = ifstream(filename, std::ios::out | std::ios::binary);

    for (int i = 0; i < MAXN; i++) {
        infile.read((char *) (src + compresswidth * i), compresswidth);
    }

    infile.close();

    char *dst_head = dst;

    char *target = new char[compresswidth * MAXN];
    int compressed_size;

    auto starttime = system_clock::now();

    for (int i = 0; i < 100; i++) {
        compressed_size = chimp_compress_data(src, compresswidth * MAXN,
                                       dst, compresswidth * MAXN);
    }
    duration<double> diff = system_clock::now() - starttime;
    cout << "压缩所耗时间为：" << diff.count() * 1e6 << "us" << endl;
    char *target_head = target;

    chimp_decompress_data(dst_head, compressed_size, target, compresswidth * MAXN);
    // cout << "Decompressed value is below:" << endl;
    int difvalue = 0;
    for (int i = 0; i < MAXN; i++) {
        if (*((compresstype *) (target_head + compresswidth * i)) != *((compresstype *) (src + compresswidth * i))) {
            // cout << "After Compression, exist different values!!!!!" << endl;
            difvalue++;
        }

        cout << *((compresstype *) (target_head + compresswidth * i)) << " ";
    }
    cout << endl;
    cout << "The num of different value is " << difvalue << endl;
    // cout << "correct value is below:" << endl;
    // for (int i = 0; i < MAXN; i++) {
    //     cout << *((compresstype *) (src + compresswidth * i)) << " ";
    // }
    // cout << endl;

    cout << "compressed_size: " << compressed_size << endl;
    cout.precision(6);
    cout << fixed;

    cout << "compressed_rate: " << compressed_size * 1.0 / (compresswidth * MAXN) << endl;

    if (argc >= 3) {
        char *ofile = argv[2];
        auto ofs = ofstream(ofile);
        ofs << "time,value" << endl;
        for (int i = 0; i < MAXN; i++) {
            ofs << i << "," << *((double *) (src + 8 * i)) << endl;
        }
    }
    delete[] src;
    delete[] dst;
    delete[] target;
}