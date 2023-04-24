#pragma once
#include <cinttypes>

struct OutputBitStream
{
    /** The number of bits written to this bit stream. */
    uint64_t writtenBits;
    /** Current bit buffer. */
    int current;
    /** The stream buffer. */
    uint8_t *buffer;
    /** Current number of free bits in the bit buffer (the bits in the buffer are stored high). */
    int free;
    /** Current position in the uint8_t buffer. */
    int pos;
    /** Current number of bytes available in the uint8_t buffer. */
    int avail;
    /** True if we are wrapping an array. */
    bool wrapping;

    OutputBitStream()
    {
        wrapping = false;
    }

    OutputBitStream(uint8_t *a)
    {
        // os = NULL;
        free = 8;
        buffer = a;
        pos = 0;
        avail = 8000;
        current = 0;
        wrapping = true;
    }

    void flush()
    {
        align();
    }

    /* * Closes the bit stream. All resources associated with the stream are released.
     */

    void close()
    {
        flush();
        // if (os != NULL && os != System.out && os != System.err)
        // 	os.close();
        buffer = nullptr;
    }

    void write(int b)
    {
        --avail;
        buffer[pos++] = (uint8_t)b;
    }

    /** Writes bits in the bit buffer, possibly flushing it.
     *
     * You cannot write more than {@link #free} bits with this method. However,
     * after having written {@link #free} bits the bit buffer will be empty. In
     * particular, there should never be 0 free bits in the buffer.
     *
     * @param b the bits to write in the <strong>lower</strong> positions; the remaining positions must be zero.
     * @param len the number of bits to write (0 is safe and causes no action).
     * @return the number of bits written.
     * @throws IllegalArgumentException if one tries to write more bits than available in the buffer and debug is enabled.
     */

    int writeInCurrent(int b, int len)
    {
        current |= (b & ((1 << len) - 1)) << (free -= len);
        if (free == 0)
        {
            write(current);
            free = 8;
            current = 0;
        }

        writtenBits += len;
        return len;
    }

    /** Aligns the stream.
     *
     * After a call to this method, the stream is uint8_t aligned. Zeroes
     * are used to pad it if necessary.
     *
     * @return the number of padding bits.
     */

    int align()
    {
        if (free != 8)
            return writeInCurrent(0, free);
        else
            return 0;
    }

    uint64_t write(uint8_t *bits, uint64_t len)
    {
        return writeByteOffset(bits, 0, len);
    }

    /** Writes a sequence of bits, starting from a given offset.
     *
     * Bits will be written in the natural way: the first bit is bit 7 of the
     * first uint8_t, the eightth bit is bit 0 of the first uint8_t, the ninth bit is
     * bit 7 of the second uint8_t and so on.
     *
     * @param bits a vector containing the bits to be written.
     * @param offset a bit offset from which to start to write.
     * @param len a bit length.
     * @return the number of bits written (<code>len</code>).
     */

    uint64_t write(uint8_t *bits, uint64_t offset, uint64_t len)
    {
        int initial = (int)(8 - (offset & 0x7));
        if (initial == 8)
            return writeByteOffset(bits, (int)offset >> 3, len);
        if (len <= initial)
            return writeInt((0xFF & bits[(int)(offset >> 3)]) >> (initial - len), (int)len);
        return writeInt(bits[(int)(offset >> 3)], initial) + writeByteOffset(bits, (int)((offset >> 3) + 1), len - initial);
    }

    uint64_t writeByteOffset(uint8_t *bits, int offset, uint64_t len)
    {
        if (len == 0)
            return 0;
        if (len <= free)
            return writeInCurrent(bits[offset] >> 8 - len, (int)len);
        else
        {
            int shift = free;
            int i, j;

            writeInCurrent(bits[offset] >> 8 - shift, shift);

            len -= shift;

            j = offset;
            i = (int)(len >> 3);
            while (i-- != 0)
            {
                write(bits[j] << shift | (bits[j + 1] & 0xFF) >> 8 - shift);
                writtenBits += 8;
                j++;
            }

            int queue = (int)(len & 7);
            if (queue != 0)
                if (queue <= 8 - shift)
                    writeInCurrent(bits[j] >> 8 - shift - queue, queue);
                else
                {
                    writeInCurrent(bits[j], 8 - shift);
                    writeInCurrent(bits[j + 1] >> 16 - queue - shift, queue + shift - 8);
                }

            return len + shift;
        }
    }

    /** Writes a bit.
     *
     * @param bit a bit.
     * @return the number of bits written.
     */

    int writeBit(bool bit)
    {
        return writeInCurrent(bit ? 1 : 0, 1);
    }

    /** Writes a bit.
     *
     * @param bit a bit.
     * @return the number of bits written.
     */

    int writeBit(int bit)
    {
        // if (bit < 0 || bit > 1)
        // 	throw new IllegalArgumentException("The argument " + bit + " is not a bit.");
        return writeInCurrent(bit, 1);
    }

    int writeInt(int x, int len)
    {
        if (len <= free)
            return writeInCurrent(x, len);

        int i = len - free;
        int queue = i & 7;

        if (free != 0)
            writeInCurrent(x >> i, free);

        // Dirty trick: since queue < 8, we pre-write the last bits in the bit buffer.
        if (queue != 0)
        {
            i -= queue;
            writeInCurrent(x, queue);
            x >>= queue;
        }

        if (i == 32)
            write(x >> 24);
        if (i > 23)
            write(x >> 16);
        if (i > 15)
            write(x >> 8);
        if (i > 7)
            write(x);

        writtenBits += i;

        return len;
    }

    int writeLong(uint64_t x, int len)
    {
        if (len <= free)
            return writeInCurrent((int)x, len);

        int i = len - free;
        int queue = i & 7;

        if (free != 0)
            writeInCurrent((int)(x >> i), free);

        // Dirty trick: since queue < 8, we pre-write the last bits in the bit buffer.
        if (queue != 0)
        {
            i -= queue;
            writeInCurrent((int)x, queue);
            x >>= queue;
        }

        if (i == 64)
            write((int)(x >> 56));
        if (i > 55)
            write((int)(x >> 48));
        if (i > 47)
            write((int)(x >> 40));
        if (i > 39)
            write((int)(x >> 32));
        if (i > 31)
            write((int)x >> 24);
        if (i > 23)
            write((int)x >> 16);
        if (i > 15)
            write((int)x >> 8);
        if (i > 7)
            write((int)x);

        writtenBits += i;

        return len;
    }
};

struct InputBitStream
{
    /** True if we are wrapping an array. */
    bool wrapping;
    /** The number of bits actually read from this bit stream. */
    uint64_t readBits;
    /** Current bit buffer: the lowest {@link #fill} bits represent the current content (the remaining bits are undefined). */
    int current;
    /** The stream buffer. */
    uint8_t *buffer;
    /** Current number of bits in the bit buffer (stored low). */
    int fill;
    /** Current position in the byte buffer. */
    int pos;
    /** Current number of bytes available in the byte buffer. */
    int avail;

    /** This (non- ) constructor exists just to provide fake initialisation for classes such as {@link DebugInputBitStream}.
     */
    InputBitStream()
    {
        wrapping = false;
    }

    /** Creates a new input bit stream wrapping a given byte array.
     *
     * @param a the byte array to wrap.
     */
    InputBitStream(uint8_t *a, int NITEMS)
    {
        buffer = a;
        avail = 8 * NITEMS;
        wrapping = true;
        pos = 0;
        fill = 0;
        current = 0;
        readBits = 0;
    }

    /** Flushes the bit stream. All state information associated with the stream is reset. This
     * includes bytes prefetched from the stream, bits in the bit buffer and unget'd bits.
     *
     * <P>This method is provided so that users of this class can easily wrap repositionable
     * streams (for instance, file-based streams, which can be repositioned using
     * the underlying {@link java.nio.channels.FileChannel}). It is guaranteed that after calling
     * this method the underlying stream can be repositioned, and that the next read
     * will draw data from the stream.
     */

    void flush()
    {
        if (!wrapping)
        {
            avail = 0;
            pos = 0;
        }
        fill = 0;
    }

    /** Closes the bit stream. All resources associated with the stream are released.
     */

    void close()
    {
        // if (is != NULL && is != System.in)
        //     is.close();
        buffer = nullptr;
    }

    /** Reads the next byte from the stream.
     *
     * <P>This method takes care of managing the buffering logic
     * transparently.
     *
     * <P>However, this method does <em>not</em> update {@link #readBits}.
     * The caller should increment {@link #readBits} by 8 at each call, unless
     * the bit are used to load {@link #current}.
     */

    int read()
    {
        avail--;
        return buffer[pos++] & 0xFF;
    }

    /** Feeds 16 more bits into {@link #current}, assuming that {@link #fill} is less than 16.
     *
     * <p>This method will never throw an {@link EOFException}&mdash;simply, it will refill less than 16 bits.
     *
     * @return {@link #fill}.
     */

    int refill()
    {
        if (avail > 1)
        {
            // If there is a byte in the buffer, we use it directly.
            avail -= 2;
            current = current << 16 | (buffer[pos++] & 0xFF) << 8 | buffer[pos++] & 0xFF;
            return fill += 16;
        }
        else
        {
        }

        current = (current << 8) | read();
        fill += 8;
        current = (current << 8) | read();
        fill += 8;

        return fill;
    }

    /** Reads bits from the bit buffer, possibly refilling it.
     *
     * <P>This method is the basic mean for extracting bits from the underlying stream.
     *
     * <P>You cannot read more than {@link #fill} bits with this method (unless {@link #fill} is 0,
     * and <code>len</code> is nonzero, in which case the buffer will be refilled for you with 8 bits), and if you
     * read exactly {@link #fill} bits the buffer will be empty afterwards. In particular,
     * there will never be 8 bits in the buffer.
     *
     * <P>The bit buffer stores its content in the lower {@link #fill} bits. The content
     * of the remaining bits is undefined.
     *
     * <P>This method updates {@link #readBits}.
     *
     * @param len the number of bits to read.
     * @return the bits read (in the <strong>lower</strong> positions).
     * @throws AssertionError if one tries to read more bits than available in the buffer and assertions are enabled.
     */

    int readFromCurrent(int len)
    {
        if (fill == 0)
        {
            current = read();
            fill = 8;
        }

        return current >> (fill -= len) & (1 << len) - 1;
    }

    /** Aligns the stream.
     *
     * After a call to this function, the stream is byte aligned. Bits that have been
     * read to align are discarded.
     */

    void align()
    {
        if ((fill & 7) == 0)
            return;
        readBits += fill & 7;
        fill &= ~7;
    }

    /** Reads a sequence of bits.
     *
     * Bits will be read in the natural way: the first bit is bit 7 of the
     * first byte, the eightth bit is bit 0 of the first byte, the ninth bit is
     * bit 7 of the second byte and so on.
     *
     * @param bits an array of bytes to store the result.
     * @param len the number of bits to read.
     */

    void read(uint8_t *bits, int len)
    {
        // assert(fill < 32 : fill + " >= " + 32);

        if (len <= fill)
        {
            if (len <= 8)
            {
                bits[0] = (uint8_t)(readFromCurrent(len) << 8 - len);
                return;
            }
            else if (len <= 16)
            {
                bits[0] = (uint8_t)(readFromCurrent(8));
                bits[1] = (uint8_t)(readFromCurrent(len - 8) << 16 - len);
                return;
            }
            else if (len <= 24)
            {
                bits[0] = (uint8_t)(readFromCurrent(8));
                bits[1] = (uint8_t)(readFromCurrent(8));
                bits[2] = (uint8_t)(readFromCurrent(len - 16) << 24 - len);
                return;
            }
            else
            {
                bits[0] = (uint8_t)(readFromCurrent(8));
                bits[1] = (uint8_t)(readFromCurrent(8));
                bits[2] = (uint8_t)(readFromCurrent(8));
                bits[3] = (uint8_t)(readFromCurrent(len - 24) << 32 - len);
                return;
            }
        }
        else
        {
            int i, j = 0, b;

            if (fill >= 24)
            {
                bits[j++] = (uint8_t)(readFromCurrent(8));
                bits[j++] = (uint8_t)(readFromCurrent(8));
                bits[j++] = (uint8_t)(readFromCurrent(8));
                len -= 24;
            }
            else if (fill >= 16)
            {
                bits[j++] = (uint8_t)(readFromCurrent(8));
                bits[j++] = (uint8_t)(readFromCurrent(8));
                len -= 16;
            }
            else if (fill >= 8)
            {
                bits[j++] = (uint8_t)(readFromCurrent(8));
                len -= 8;
            }

            int shift = fill;

            if (shift != 0)
            {
                bits[j] = (uint8_t)(readFromCurrent(shift) << 8 - shift);
                len -= shift;
                i = len >> 3;
                while (i-- != 0)
                {
                    b = read();
                    bits[j] |= (b & 0xFF) >> shift;
                    bits[++j] = (uint8_t)(b << 8 - shift);
                }
            }
            else
            {
                i = len >> 3;
                while (i-- != 0)
                    bits[j++] = (uint8_t)read();
            }

            readBits += len & ~7;

            len &= 7;
            if (len != 0)
            {
                if (shift == 0)
                    bits[j] = 0; // We must zero the next uint8_t before OR'ing stuff in
                if (len <= 8 - shift)
                {
                    bits[j] |= (uint8_t)(readFromCurrent(len) << 8 - shift - len);
                }
                else
                {
                    bits[j] |= (uint8_t)(readFromCurrent(8 - shift));
                    bits[j + 1] = (uint8_t)(readFromCurrent(len + shift - 8) << 16 - shift - len);
                }
            }
        }
    }

    /** Reads a bit.
     *
     * @return the next bit from the stream.
     */

    int readBit()
    {
        return readFromCurrent(1);
    }

    /** Reads a fixed number of bits into an integer.
     *
     * @param len a bit length.
     * @return an integer whose lower <code>len</code> bits are taken from the stream; the rest is zeroed.
     * @throws IOException
     */

    int readInt(int len)
    {
        int i, x = 0;

        if (fill < 16)
            refill();
        if (len <= fill)
            return readFromCurrent(len);

        len -= fill;
        x = readFromCurrent(fill);

        i = len >> 3;
        while (i-- != 0)
            x = x << 8 | read();
        readBits += len & ~7;

        len &= 7;

        return (x << len) | readFromCurrent(len);
    }

    /** Reads a fixed number of bits into a uint64_t.
     *
     * @param len a bit length.
     * @return a uint64_t whose lower <code>len</code> bits are taken from the stream; the rest is zeroed.
     */

    uint64_t readLong(int len)
    {
        int i;
        uint64_t x = 0;

        if (fill < 16)
            refill();
        if (len <= fill)
            return readFromCurrent(len);

        len -= fill;
        x = readFromCurrent(fill);

        i = len >> 3;
        while (i-- != 0)
            x = x << 8 | read();

        len &= 7;

        return (x << len) | readFromCurrent(len);
    }

    /** Repositions this bit stream to the position at the time the {@link #mark(int)} method was last called.
     *
     * <P>This method will just {@link #flush() flush the stream} and delegate
     * the reset to the underlying {@link InputStream}.
     */

    void reset()
    {
        flush();
    }
};