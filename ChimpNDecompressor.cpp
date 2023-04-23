#include <limits.h>
#include <cinttypes>
#include "InputBitStream.cpp"
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

    //  parameter name must be diff with member data, 
    // otherwise using this->namexxx = namexxx
    ChimpNDecompressor(uint8_t *bs, int preValues)
    {
        in = InputBitStream(bs);
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

        while (ct < NITEMS && !endOfStream)
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