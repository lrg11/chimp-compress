#include <vector>
#include <cmath>
#include <string>
// #include <boost/algorithm/string.hpp>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

class CSVReader
{
    std::string filename;
    std::string delimeter;
    std::ifstream inFile;
    std::vector<std::string> col_names;
    std::string line;
    std::string doublevalue;

public:
    CSVReader(std::string f, std::string del = ",") : filename(f), delimeter(del)
    {
        inFile.open(filename);
        if (!inFile)
        {
            std::cout << "Unable to open file";
            exit(1); // terminate with error
        }

        // if (getline(inFile, line))
        // {
        //     // std::string csvline;
        //     // std::string doublevalue;
        //     std::stringstream sstream(line);
        //     getline(sstream, doublevalue, ',');
        //     getline(sstream, doublevalue, ',');
        //     getline(sstream, doublevalue, ',');
        //     // boost::algorithm::split(col_names, line, boost::is_any_of(delimeter));
        // }
    };

    bool isEmpty()
    {
        getline(inFile, line);
        std::stringstream sstream(line);
        getline(sstream, doublevalue, ',');
        getline(sstream, doublevalue, ',');
        getline(sstream, doublevalue, ',');
        return ((line == "") || (!inFile));
    }

    double nextLine()
    {
        if (line != "")
        {

            // boost::algorithm::split(s, line, boost::is_any_of(delimeter));

            return double(std::stod(doublevalue));
        }
        else
        {
            inFile.close();
            exit(EXIT_FAILURE);
        }
    }

    std::vector<std::string> nextLineString()
    {
        if (getline(inFile, line))
        {
            std::vector<std::string> s;
            // boost::algorithm::split(s, line, boost::is_any_of(delimeter));
            return s;
        }
        else
        {
            inFile.close();
            exit(EXIT_FAILURE);
        }
    }

    ~CSVReader()
    {
        inFile.close();
    }
};