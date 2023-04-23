#include <iostream>
#include <chrono>
#include <string>
#include <fstream>
#define NITEMS 3600
#include "ChimpN.cpp"
#include "ChimpNDecompressor.cpp"
#include "CSVReader.cpp"
using namespace std::chrono;
using namespace std;


// static int MINIMUM_TOTAL_BLOCKS = 50000;
#define MINIMUM_TOTAL_BLOCKS 1

static string FILENAMES[] = {
    "city_temperature.csv",
    // "Stocks-Germany-sample.txt",
    // "SSD_HDD_benchmarks.csv"
};

void testChimp128()
{
    for (string filename : FILENAMES)
    {
        CSVReader reader(filename);

        uint64_t nlines = 0;

        uint64_t totalSize = 0;
        int totalBlocks = 0;
        uint64_t encodingDuration = 0;
        uint64_t decodingDuration = 0;
        while (totalBlocks < MINIMUM_TOTAL_BLOCKS)
        {
            std::vector<double> values;
            nlines = 0;
            while (!reader.isEmpty() && nlines < NITEMS)
            {
                values.push_back(reader.nextLine());
                nlines++;
            }
            if (nlines < NITEMS)
            {
                break;
            }
            ChimpN compressor(128);
            auto starttime = system_clock::now();
            for (double value : values)
            {
                compressor.addValue(value);
            }
            compressor.close();
            duration<double> diff = system_clock::now() - starttime;
            cout << "压缩所耗时间为：" << diff.count() * 1e6 << "us" << endl;
            totalSize += compressor.getSize();
            totalBlocks += 1;
            for (int i = 0; i < values.size(); i++)
            {
                cout << values[i] << " ";
                // cout << uncompressedValues[i];
                // if (values[i] != uncompressedValues[i])
                //     cout << "Value did not match" << endl;
            }
            cout << endl;
            cout << endl;
            ChimpNDecompressor d(compressor.getOut(), 128);
            uint8_t *bf = compressor.getOut();

            // uint64_t firstvalue  = 0;
            // for(int i = 0; i < 8; i++) {
            //     firstvalue <<= 8;
            //     firstvalue |=  bf[i];
            // }
            // cout << "First value is " << *((double *)&firstvalue) << endl;

            // Used to test compress , ok

            // for (int i = 0; i < 8 * NITEMS; i++)
            // {
            //     cout << (int)bf[i] << " ";
            // }
            auto uncompresstime = system_clock::now();
            vector<double> uncompressedValues = d.getValues();
            duration<double> uncompressdiff = system_clock::now() - uncompresstime;
            cout << "解压所耗时间为：" << uncompressdiff.count() * 1e6 << "us" << endl;
            int diffvalue = 0;
            for (int i = 0; i < values.size(); i++)
            {
                cout << values[i] << " ";
                cout << uncompressedValues[i] << endl;
                if (values[i] != uncompressedValues[i])
                {
                    diffvalue++;
                    // cout << "Value did not match" << endl;
                }
            }
            cout << "Diff value has " << diffvalue << " num" << endl;
        }
        cout << "compressed_rate: " << totalSize * 1.0 / (totalBlocks * NITEMS * 64) << endl;
        cout << "Chimp128: " << filename;
        printf(" - Bits/value: %.2f, Compression time per block: %.2f, Decompression time per block: %.2f\n", totalSize * 1.0 / (totalBlocks * NITEMS), encodingDuration * 1.0 / totalBlocks, decodingDuration * 1.0 / totalBlocks);
    }
}

int main()
{
    testChimp128();
    return 0;
}