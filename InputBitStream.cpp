

/** Bit-level input stream.
 *
 * <P>This class wraps any {@link InputStream} so that you can treat it as
 * <em>bit</em> stream.  Constructors and methods closely resemble those of
 * {@link InputStream}. Data can be read from such a stream in several ways:
 * reading a (uint64_t) natural number in fixed-width, unary, &gamma;, shifted &gamma;, &delta;, &zeta; and (skewed)
 * Golomb coding, or reading a number of bits that will be stored in a vector of
 * bytes. There is limited support for {@link #mark(int)}/{@link #reset()}
 * operations.
 *
 * <P>This class can also {@linkplain #InputBitStream(uint8_t *) wrap a byte
 * array}; this is much more lightweight than wrapping a {@link
 * it.unimi.dsi.fastutil.io.FastByteArrayInputStream} wrapping the array. Overflowing the array
 * will cause an {@link java.io.EOFException}.
 *
 * <P>Note that when reading using a vector of bytes bits are read in the
 * stream format (see {@link OutputBitStream}): the first bit is bit 7 of the
 * first byte, the eighth bit is bit 0 of the first byte, the ninth bit is bit
 * 7 of the second byte and so on. When reading natural numbers using some coding,
 * instead, they are stored in the standard way, that is, in the <strong>lower</strong>
 * bits.
 *
 * <P>Additional features:
 *
 * <UL>
 *
 * <LI>This class provides an internal buffer. By setting a buffer of
 * length 0 at creation time, you can actually bypass the buffering system:
 * Note, however, that several classes providing buffering have synchronised
 * methods, so using a wrapper instead of the internal buffer is likely to lead
 * to a performance drop.
 *
 * <LI>To work around the schizophrenic relationship between streams and random
 * access files in {@link java.io}, this class provides a {@link #flush()}
 * method that resets the internal state. At this point, you can safely reposition
 * the underlying stream and read again afterwards. For instance, this is safe
 * and will perform as expected:
 * <PRE>
 * FileInputStream fis = new FileInputStream(...);
 * InputBitStream ibs = new InputBitStream(fis);
 * ... read operations on ibs ...
 * ibs.flush();
 * fis.getChannel().position(...);
 * ... other read operations on ibs ...
 * </PRE>
 *
 * <P>As a commodity, an instance of this class will try to cast the underlying byte
 * stream to a {@link RepositionableStream} and to fetch by reflection the {@link
 * java.nio.channels.FileChannel} underlying the given input stream, in this
 * order.  If either reference can be successfully fetched, you can use
 * directly the {@link #position(uint64_t) position()} method with argument
 * <code>pos</code> with the same semantics of a {@link #flush()}, followed by
 * a call to <code>position(pos / 8)</code> (where the latter method belongs
 * either to the underlying stream or to its underlying file channel), followed
 * by a {@link #skip(uint64_t) skip(pos % 8)}. However, since the reflective checks are quite
 * heavy they can be disabled using a {@linkplain InputBitStream#InputBitStream(InputStream,  bool) suitable constructor}.
 *
 * <li>Finally, this class implements partially the interface of a  bool iterator.
 * More precisely, {@link #nextBoolean()} will return the same bit as {@link #readBit()},
 * and also the same exceptions, whereas <em>{@link #hasNext()} will always return true</em>:
 * you must be prepared to catch a {@link java.lang.RuntimeException} wrapping an {@link IOException}
 * in case the file ends. It
 * is very difficult to implement completely an eager operator using a input-stream
 * based model.
 *
 * </ul>
 *
 * <P><STRONG>This class is not synchronised</STRONG>. If multiple threads
 * access an instance of this class concurrently, they must be synchronised externally.
 *
 * @see java.io.InputStream
 * @see it.unimi.dsi.io.OutputBitStream
 * @author Sebastiano Vigna
 * @since 0.1
 */
#include <vector>
#include <cinttypes>
#include <iostream>
#include <limits.h>
#include <string>

struct InputBitStream
{ 
    bool DEBUG = false;
    /** The default size of the byte buffer in bytes (8Ki). */
    int DEFAULT_BUFFER_SIZE = 8 * 1024;
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
    /** Current position of the first byte in the byte buffer. */
    uint64_t position;

    /** This (non- ) constructor exists just to provide fake initialisation for classes such as {@link DebugInputBitStream}.
     */
    InputBitStream()
    {
        // is = NULL;
        //		noBuffer = true;
        // repositionableStream = NULL;
        // fileChannel = NULL;
        wrapping = false;
    }

    /** Creates a new input bit stream wrapping a given byte array.
     *
     * @param a the byte array to wrap.
     */
    InputBitStream(uint8_t *a)
    {
        // is = NullInputStream.getInstance();
        // repositionableStream = NULL;
        // fileChannel = NULL;

        //		if (a.length > 0) {
        buffer = a;
        avail = 8 * NITEMS;
        wrapping = true;
        pos = 0;
        fill = 0;
        current = 0;
        //			noBuffer = false;
        //		}
        //		else {
        //			// A zero-length buffer is like having no buffer
        //			buffer = NULL;
        //			avail = 0;
        //			wrapping = false;
        //			noBuffer = true;
        //		}
    }

    /** Creates a new input bit stream reading from a file.
     *
     * <p>This constructor invokes directly {@link FileInputStream#getChannel()} to support {@link #position(uint64_t)}.
     *
     * @param name the name of the file.
     * @param bufSize the size in byte of the buffer; it may be 0, denoting no buffering.
     */
    // InputBitStream(string name, int bufSize) throws FileNotFoundException
    // {
    //     this(new FileInputStream(name), bufSize);
    // }

    /** Creates a new input bit stream reading from a file.
     *
     * <p>This constructor invokes directly {@link FileInputStream#getChannel()} to support {@link #position(uint64_t)}.
     *
     * @param name the name of the file.
     */
    // InputBitStream(string name) throws FileNotFoundException
    // {
    //     this(new FileInputStream(name), DEFAULT_BUFFER_SIZE);
    // }

    /** Creates a new input bit stream reading from a file.
     *
     * <p>This constructor invokes directly {@link FileInputStream#getChannel()} to support {@link #position(uint64_t)}.
     *
     * @param file the file.
     */
    // InputBitStream(File file) throws FileNotFoundException
    // {
    //     this(new FileInputStream(file), DEFAULT_BUFFER_SIZE);
    // }

    /** Creates a new input bit stream reading from a file.
     *
     * <p>This constructor invokes directly {@link FileInputStream#getChannel()} to support {@link #position(uint64_t)}.
     *
     * @param file the file.
     * @param bufSize the size in byte of the buffer; it may be 0, denoting no buffering.
     */
    // InputBitStream(File file, int bufSize) throws FileNotFoundException
    // {
    //     this(new FileInputStream(file), bufSize);
    // }

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
            position += pos;
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
        buffer = NULL;
    }

    /** Returns the number of bits that can be read (or skipped over) from this
     * bit stream without blocking by the next caller of a method.
     *
     * @return the number of bits that can be read from this bit stream without blocking.
     */

    // uint64_t available()
    // {
    //     return (is.available() + avail) * 8 + fill;
    // }

    /** Returns the number of bits read from this bit stream.
     *
     * @return the number of bits read so far.
     */
    // uint64_t readBits()
    // {
    //     return readBits;
    // }

    /** Sets the number of bits read from this bit stream.
     *
     * <P>This method is provided so that, for instance, the
     * user can reset via <code>readBits(0)</code> the read-bits count
     * after a {@link #flush()}.
     *
     * @param readBits the new value for the number of bits read so far.
     */
    // void readBits(uint64_t readBits)
    // {
    //     this.readBits = readBits;
    // }

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
        //		if (noBuffer) {
        //			  int t = is.read();
        //			position++;
        //			return t;
        //		}

        // if (avail == 0)
        // {
        //     avail = is.read(buffer);
        //     position += pos;
        //     pos = 0;
        // }

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

        //		current = current << 16 | (buffer[pos++] & 0xFF) << 8 | buffer[pos++] & 0xFF;
        //		return fill += 16;

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
        // is.reset();
    }
};
