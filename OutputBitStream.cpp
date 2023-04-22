#include <vector>
#include <cinttypes>
#include <iostream>
#include <limits.h>

struct OutputBitStream
{
	int MAX_PRECOMPUTED = 4096;
	bool DEBUG = false;

	int DEFAULT_BUFFER_SIZE = 16 * 1024;
	/** The underlying {@link std::ostream}. */

	// std::ostream os;
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
	/** Current position of the underlying output stream. */

	uint64_t position;
	/** Current number of bytes available in the uint8_t buffer. */

	int avail;
	/** Size of the small buffer for temporary usage. */
	int TEMP_BUFFER_SIZE = 128;

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
		// if (os != NULL)
		// {
		// 	if (buffer != NULL)
		// 	{
		// 		os.write(buffer, 0, pos);
		// 		position += pos;
		// 		pos = 0;
		// 		avail = buffer.size();
		// 	}
		// 	os.flush();
		// }
	}

	/* * Closes the bit stream. All resources associated with the stream are released.
	 */

	void close()
	{
		flush();
		// if (os != NULL && os != System.out && os != System.err)
		// 	os.close();
		buffer = NULL;
	}

	/** Returns the number of bits written to this bit stream.
	 *
	 * @return the number of bits written so far.
	 */
	// uint64_t writtenBits()
	// {
	// 	return writtenBits;
	// }

	// void writtenBits(uint64_t writtenBits)
	// {
	// 	this->writtenBits = writtenBits;
	// }

	void write(int b)
	{
		--avail;
		// if (avail-- == 0)
		// {
		// 	if (buffer == NULL)
		// 	{
		// 		os.write(b);
		// 		position++;
		// 		avail = 0;
		// 		return;
		// 	}
		// 	os.write(buffer);
		// 	position += buffer.size();
		// 	avail = buffer.size() - 1;
		// 	pos = 0;
		// }

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

	/** Writes a sequence of bits emitted by a boolean iterator.
	 *
	 * <P>If the iterator throws an exception, it is catched,
	 * and the return value is given by the number of bits written
	 * increased by one and with the sign changed.
	 *
	 * @param i a boolean iterator.
	 * @return if <code>i</code> did not throw a runtime exception,
	 * the number of bits written; otherwise, the number of bits written,
	 * plus one, with the sign changed.
	 */

	// int write(BooleanIterator i)
	// {
	// 	int count = 0;
	// 	bool bit;
	// 	while (i.hasNext())
	// 	{
	// 		try
	// 		{
	// 			bit = i.nextBoolean();
	// 		}
	// 		catch (RuntimeException hide)
	// 		{
	// 			return -count - 1;
	// 		}

	// 		writeBit(bit);
	// 		count++;
	// 	}
	// 	return count;
	// }

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
