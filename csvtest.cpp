#include <vector>
#include <cmath>
#include <string>
#include "CSVReader-advanced.cpp"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

// struct CSVReader
// {
//     std::string filename;
//     std::string delimeter;
//     std::ifstream inFile;
//     std::vector<std::string> col_names;
//     std::string line;
//     std::string doublevalue;
//     CSVReader(std::string f, std::string del = ",") : filename(f), delimeter(del)
//     {
//         inFile.open(filename);
//         if (!inFile)
//         {
//             std::cout << "Unable to open file";
//             exit(1); // terminate with error
//         }

//         // if (getline(inFile, line))
//         // {
//         //     // std::string csvline;
//         //     // std::string doublevalue;
//         //     std::stringstream sstream(line);
//         //     getline(sstream, doublevalue, ',');
//         //     getline(sstream, doublevalue, ',');
//         //     getline(sstream, doublevalue, ',');
//         //     // boost::algorithm::split(col_names, line, boost::is_any_of(delimeter));
//         // }
//     };

//     bool isEmpty()
//     {
//         getline(inFile, line);
//         std::stringstream sstream(line);
//         getline(sstream, doublevalue, delimeter[0]);
//         getline(sstream, doublevalue, delimeter[0]);
//         getline(sstream, doublevalue, delimeter[0]);
//         return ((line == "") || (!inFile));
//     }

//     double nextLine()
//     {
//         if (line != "")
//         {

//             // boost::algorithm::split(s, line, boost::is_any_of(delimeter));

//             return double(std::stod(doublevalue));
//         }
//         else
//         {
//             inFile.close();
//             exit(EXIT_FAILURE);
//         }
//     }

//     ~CSVReader()
//     {
//         inFile.close();
//     }
// };

#include <iostream>
#include <vector>
#include <string>
using namespace std;

int main(int argc, char *argv[])
{
    std::string filename(argv[1]);
    // int skip_timestamp = argc > 2;

    CSVReader reader(filename);

    uint64_t nlines = 0;
    for (int i = 0; i < 1; i++)
    {
        std::vector<double> lines;
        int ct = 0;
        while (!reader.isEmpty() && ct < 1000)
        {
            lines.push_back(reader.nextLine());
            ct++;
            nlines++;
        }
        if (ct < 1000)
        {
            break;
        }

        // uint64_t ncols = lines[0].size();

        // std::string rawname = filename.substr(0, filename.find_last_of("."));
        // if (skip_timestamp != 0)
        // {
        //     rawname += "no_ts";
        // }
        for (int i = 0; i < ct; i++)
        {
            std::cout << lines[i] << " ";
        }

        cout << endl;
    }

    // filename = rawname + std::string(".bin");

    // auto myfile = std::ofstream(filename, std::ios::out | std::ios::binary);

    // char *c_nlines = (char *)&nlines;
    // // char *c_ncols = (char *)&ncols;

    // myfile.write(c_nlines, sizeof(uint64_t));
    // // myfile.write(c_ncols, sizeof(uint64_t));

    // myfile.close();

    std::cout << "Lines = " << nlines << " File successfully converted!" << std::endl;
    std::cout << filename << std::endl;
}