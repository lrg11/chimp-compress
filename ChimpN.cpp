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
class ChimpN
{

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

    //      BitOutput out;

    // OutputBitStream out;
    std::vector<uint8_t> ob(8000);
    OutputBitStream out(ob);

    int previousValues;

    int setLsb;

    int *indices;

    int index = 0;

    int current = 0;

    int flagOneSize;

    int flagZeroSize;

    // We should have access to the series?
public:
    ChimpN(int previousValues)
    {
        //        out = output;

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
        return out.buffer;
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
            writeFirst(Double.doubleToRawLongBits(value));
        }
        else
        {
            compressValue(Double.doubleToRawLongBits(value));
        }
    }

    void writeFirst(long value)
    {
        first = false;
        storedValues[current] = value;
        out.writeLong(storedValues[current], 64);
        indices[(int)value & setLsb] = index;
        size += 64;
    }

    /**
     * Closes the block and writes the remaining stuff to the BitOutput.
     */

    void close()
    {
        addValue(Double.NaN);
        out.writeBit(false);
        out.flush();
    }

    void compressValue(long value)
    {
        int key = (int)value & setLsb;
        long xor ;
        int previousIndex;
        int trailingZeros = 0;
        int currIndex = indices[key];
        if ((index - currIndex) < previousValues)
        {
            long tempXor = value ^ storedValues[currIndex % previousValues];
            trailingZeros = Long.numberOfTrailingZeros(tempXor);
            if (trailingZeros > threshold)
            {
                previousIndex = currIndex % previousValues;
                xor = tempXor;
            }
            else
            {
                previousIndex = index % previousValues;
                xor = storedValues[previousIndex] ^ value;
            }
        }
        else
        {
            previousIndex = index % previousValues;
            xor = storedValues[previousIndex] ^ value;
        }

        if (xor == 0)
        {
            out.writeInt(previousIndex, this->flagZeroSize);
            size += this->flagZeroSize;
            storedLeadingZeros = 65;
        }
        else
        {
            int leadingZeros = leadingRound[Long.numberOfLeadingZeros(xor)];

            if (trailingZeros > threshold)
            {
                int significantBits = 64 - leadingZeros - trailingZeros;
                out.writeInt(512 * (previousValues + previousIndex) + 64 * leadingRepresentation[leadingZeros] + significantBits, this->flagOneSize);
                out.writeLong(xor >>> trailingZeros, significantBits); // Store the meaningful bits of XOR
                size += significantBits + this->flagOneSize;
                storedLeadingZeros = 65;
            }
            else if (leadingZeros == storedLeadingZeros)
            {
                out.writeInt(2, 2);
                int significantBits = 64 - leadingZeros;
                out.writeLong(xor, significantBits);
                size += 2 + significantBits;
            }
            else
            {
                storedLeadingZeros = leadingZeros;
                int significantBits = 64 - leadingZeros;
                out.writeInt(24 + leadingRepresentation[leadingZeros], 5);
                out.writeLong(xor, significantBits);
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
