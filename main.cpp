#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using std::ifstream, std::string, std::ostringstream, std::format, std::cout, std::vector, std::map,
    std::chrono::high_resolution_clock, std::string_view;

string fileToString(const string &file_name)
{
    ifstream file_stream{file_name};

    if (file_stream.fail())
    {
        // Error opening file.
        cout << format("Error opening file {}\n", file_name);
        throw std::exception();
    }

    ostringstream str_stream;
    file_stream >> str_stream.rdbuf(); // NOT str_stream << file_stream.rdbuf()

    if (file_stream.fail() && !file_stream.eof())
    {
        // Error reading file.
        cout << format("Error reading file {}\n", file_name);
        throw std::exception();
    }

    return str_stream.str();
}

vector<string> split(string str, const string &token)
{
    vector<string> result;
    while (!str.empty())
    {
        string::size_type index = str.find(token);
        if (index != string::npos)
        {
            result.emplace_back(str.substr(0, index));
            str = str.substr(index + token.size());
            if (str.empty())
            {
                result.emplace_back(str);
            }
        }
        else
        {
            result.emplace_back(str);
            str = "";
        }
    }
    return result;
}

int main()
{
    size_t count = 100;
    string compilerPath =
        R"(C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.37.32822\bin\Hostx64\x64\cl.exe)";
    vector<string *> compileCommands;
    vector<string> splitted = split(fileToString("ninja-nv.txt"), "\n");
    for (string &s : splitted)
    {
        if (size_t i = s.find("cl.exe  /nologo /TP "); i != string::npos)
        {
            i = i + string("cl.exe").size();
            if (i >= s.size())
            {
                cout << "Error Index";
                exit(EXIT_FAILURE);
            }
            s.erase(0, i);
            compileCommands.emplace_back(&s);
        }
    }

    auto scanFile = [&](size_t i, double &totalScanTime, map<double, string *> &scanTimesHalf,
                        map<string *, double> &halfScanTimes) {
        string finalCommand =
            "\"\"" + compilerPath + "\"" + *compileCommands[i] + " /scanDependencies a.smrules" + "\"";

        auto start = high_resolution_clock::now();
        if (system(finalCommand.c_str()) != EXIT_SUCCESS)
        {
            std::cerr << "Failed Executing scanning command\n";
            std::cerr << finalCommand;
            exit(EXIT_FAILURE);
        }
        auto end = high_resolution_clock::now();

        std::chrono::duration<double, std::milli> duration(end - start);

        double scanTime = duration.count();
        totalScanTime += scanTime;
        scanTimesHalf.emplace(scanTime, compileCommands[i]);
        halfScanTimes.emplace(compileCommands[i], scanTime);
    };

    double firstHalfTotalScanTime = 0;
    map<double, string *> scanTimesFirstHalf;
    map<string *, double> firstHalfScanTimes;
    cout << format("Scanning first {} files\n\n", count);
    for (int i = 0; i < count; ++i)
    {
        cout << i;
        scanFile(i, firstHalfTotalScanTime, scanTimesFirstHalf, firstHalfScanTimes);
    }

    double lastHalfTotalScanTime = 0;
    map<double, string *> scanTimesLastHalf;
    map<string *, double> lastHalfScanTimes;
    cout << format("\n\nScanning last {} files\n\n", count);

    for (size_t i = compileCommands.size() - count; i < compileCommands.size(); ++i)
    {
        cout << i;
        scanFile(i, lastHalfTotalScanTime, scanTimesLastHalf, lastHalfScanTimes);
        cout << '\r';
    }

    auto compileFile = [&](size_t i, double &totalCompileTime, map<double, string *> &compileTimes,
                           map<string *, double> &halfScanTimes) {
        string finalCommand = "\"\"" + compilerPath + "\"" + *compileCommands[i] + "\"";

        auto start = high_resolution_clock::now();
        if (system(finalCommand.c_str()) != EXIT_SUCCESS)
        {
            std::cerr << "Failed Executing compile command\n";
            std::cerr << finalCommand;
            exit(EXIT_FAILURE);
        }
        auto end = high_resolution_clock::now();

        std::chrono::duration<double, std::milli> duration(end - start);
        double compileTime = duration.count();
        double scanTime = halfScanTimes.find(compileCommands[i])->second;
        double totalUnitBuildTime = scanTime + compileTime;
        double scanPercentageOfTotalTime = (scanTime / totalUnitBuildTime) * 100;

        totalCompileTime += compileTime;
        compileTimes.emplace(scanPercentageOfTotalTime, compileCommands[i]);
    };

    double firstHalfTotalCompileTime = 0;
    map<double, string *> firstHalfPercentages;
    cout << format("\n\nCompiling first {} files\n\n", count);
    for (int i = 0; i < count; ++i)
    {
        cout << i;
        compileFile(i, firstHalfTotalCompileTime, firstHalfPercentages, firstHalfScanTimes);
        cout << '\r';
    }

    double lastHalfTotalCompileTime = 0;
    map<double, string *> lastHalfPercentages;
    cout << format("\n\nCompiling last {} files\n\n", count);
    for (size_t i = compileCommands.size() - count; i < compileCommands.size(); ++i)
    {
        cout << i;
        compileFile(i, lastHalfTotalCompileTime, lastHalfPercentages, lastHalfScanTimes);
        cout << '\r';
    }

    auto getFileNameFromCompileCommand = [](string *compileCommand) -> string_view {
        size_t i = compileCommand->find_last_of(' ');
        string_view s(&(*compileCommand)[i], compileCommand->size() - i);
        return s;
    };

    cout << format("\n\nscanning time of first {} files from lowest to highest\n\n", count);

    for (auto &[time, ptr] : scanTimesFirstHalf)
    {
        cout << format("{}    {}\n", time, getFileNameFromCompileCommand(ptr));
    }

    cout << format("\n\nscanning time of last {} files from lowest to highest\n\n", count);

    for (auto &[time, ptr] : scanTimesLastHalf)
    {
        cout << format("{}    {}\n", time, getFileNameFromCompileCommand(ptr));
    }

    cout << format(
        "\n\nscanning time percentage of total unit compilation time of first {} files from lowest to highest\n\n",
        count);

    for (auto &[percentage, ptr] : firstHalfPercentages)
    {
        double scanTime = firstHalfScanTimes.find(ptr)->second;
        cout << format("{}    {}   {}    {}\n", scanTime, ((100 * scanTime) / percentage) - scanTime, percentage,
                       getFileNameFromCompileCommand(ptr));
    }

    cout << format(
        "\n\nscanning time percentage of total unit compilation time of last {} files from lowest to highest\n\n",
        count);

    for (auto &[percentage, ptr] : lastHalfPercentages)
    {
        double scanTime = lastHalfScanTimes.find(ptr)->second;
        cout << format("{}    {}   {}    {}\n", scanTime, ((100 * scanTime) / percentage) - scanTime, percentage,
                       getFileNameFromCompileCommand(ptr));
    }

    cout << "\n\n  Report \n\n";

    double firstHalfAverageScanTime = firstHalfTotalScanTime / count;
    double firstHalfAverageCompileTime = firstHalfTotalCompileTime / count;
    cout << format("First Half Average ScanTime, CompileTime, % of ScanTime of Total BuildTime {}    {}    {}\n",
                   firstHalfAverageScanTime, firstHalfAverageCompileTime,
                   (firstHalfAverageScanTime / (firstHalfAverageScanTime + firstHalfAverageCompileTime)) * 100);

    double lastHalfAverageScanTime = lastHalfTotalScanTime / count;
    double lastHalfAverageCompileTime = lastHalfTotalCompileTime / count;
    cout << format("Last Half Average ScanTime, CompileTime, % of ScanTime of Total BuildTime {}    {}    {}\n",
                   lastHalfAverageScanTime, lastHalfAverageCompileTime,
                   (lastHalfAverageScanTime / (lastHalfAverageScanTime + lastHalfAverageCompileTime)) * 100);

    double averageScanTime = firstHalfAverageScanTime + lastHalfAverageScanTime;
    double averageCompileTime = firstHalfAverageCompileTime + lastHalfAverageCompileTime;

    averageScanTime /= 2;
    averageCompileTime /= 2;

    cout << format("Average ScanTime, CompileTime, % of ScanTime of Total BuildTime    {}    {}    {}\n",
                   averageScanTime, averageCompileTime,
                   (averageScanTime / (averageScanTime + averageCompileTime)) * 100);
}