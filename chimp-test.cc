#include <iostream>
#include <chrono>
#include <string>
#include <fstream>
#include "ChimpN.cpp"
#include "CSVReader.cpp"
using namespace std::chrono;
using namespace std;

static int MINIMUM_TOTAL_BLOCKS = 50000;
static string FILENAMES[] = {
    "/city_temperature.csv",
    "/Stocks-Germany-sample.txt",
    "/SSD_HDD_benchmarks.csv"};

void testChimp128()
{
    for (string filename : FILENAMES)
    {
        CSVReader reader(filename);

        uint64_t nlines = 0;

        // std::string rawname = filename.substr(0, filename.find_last_of("."));

        // filename = rawname + std::string(".bin");

        // char *c_nlines = (char *)&nlines;

        // myfile.write(c_nlines, sizeof(uint64_t));
        // myfile.write(c_ncols, sizeof(uint64_t));

        // myfile.close();

        long totalSize = 0;
        float totalBlocks = 0;
        // double[] values;
        long encodingDuration = 0;
        long decodingDuration = 0;
        while (totalBlocks < MINIMUM_TOTAL_BLOCKS)
        {
            std::vector<double> values;
            nlines = 0;
            while (!reader.isEmpty() && nlines < 1000)
            {
                values.push_back(reader.nextLine());
                nlines++;
            }
            if (nlines < 1000)
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

            ChimpNDecompressor d = new ChimpNDecompressor(compressor.getOut(), 128);
            auto uncompresstime = system_clock::now();
            List<Double> uncompressedValues = d.getValues();
            duration<double> uncompressdiff = system_clock::now() - uncompresstime;
            cout << "压缩所耗时间为：" << uncompressdiff.count() * 1e6 << "us" << endl;
            for (int i = 0; i < values.size(); i++)
            {
                assertEquals(values[i], uncompressedValues.get(i).doubleValue(), "Value did not match");
            }
        }
        printf("Chimp128: %s - Bits/value: %.2f, Compression time per block: %.2f, Decompression time per block: %.2f", filename, totalSize / (totalBlocks * 1000), encodingDuration / totalBlocks, decodingDuration / totalBlocks);
    }
}

int main()
{
    testChimp128();
    return 0;
}