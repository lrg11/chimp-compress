#include <vector>
#include <cinttypes>
#include <iostream>
#include <limits.h>
#include "OutputBitStream.cpp"
/**
 * Implements the Chimp128 time series compression. Value compression
 * is for floating points only.
 *
 * @author Panagiotis Liakos
 */
struct ChimpN
{
    const long NAN_LONG = 0x7ff8000000000000L;
    int storedLeadingZeros = INT_MAX;
    long *storedValues;
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
    ChimpN(int previousValues)
    {
        //        obs = output;
        uint8_t *obstr = new uint8_t[8000];
        obs = OutputBitStream(obstr);
        size = 0;
        this->previousValues = previousValues;
        this->previousValuesLog2 = (int)(log(previousValues) / log(2));
        this->threshold = 6 + previousValuesLog2;
        this->setLsb = (int)pow(2, threshold + 1) - 1;
        this->indices = new int[(int)pow(2, threshold + 1)];
        this->storedValues = new long[previousValues];
        this->flagZeroSize = previousValuesLog2 + 2;
        this->flagOneSize = previousValuesLog2 + 11;
    }

    uint8_t *getOut()
    {
        return obs.buffer;
    }

    /**
     * Adds a new long value to the series. Note, values must be inserted in order.
     *
     * @param value next floating point value in the series
     */

    void addValue(long value)
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
            writeFirst(*((long *)&value));
        }
        else
        {
            compressValue(*((long *)&value));
        }
    }

    void writeFirst(long value)
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

    void compressValue(long value)
    {
        int key = (int)value & setLsb;
        long xorvalue;
        int previousIndex;
        int trailingZeros = 0;
        int currIndex = indices[key];
        if ((index - currIndex) < previousValues)
        {
            long tempXor = value ^ storedValues[currIndex % previousValues];
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
