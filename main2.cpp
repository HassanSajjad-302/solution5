#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using std::ifstream, std::string, std::ostringstream, std::format, std::cout, std::vector, std::map,
    std::chrono::high_resolution_clock, std::string_view, std::filesystem::recursive_directory_iterator;

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

int main()
{
    string compilerPath =
        R"(C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.37.32822\bin\Hostx64\x64\cl.exe)";

    vector<string> smrulesResponseFiles;
    for (auto &f : recursive_directory_iterator(std::filesystem::current_path()))
    {
        if (f.is_regular_file())
        {
            if (string str = f.path().string(); str.ends_with(".smrules.response"))
            {
                smrulesResponseFiles.emplace_back(str);
                string s = fileToString(str);
                if (size_t i = s.find("/showIncludes"); i != string::npos)
                {
                    s.erase(i, string("/showIncludes").size());
                }
                std::ofstream(str) << s;
            }
        }
    }

    size_t count = smrulesResponseFiles.size();

    vector<string> huResponseFiles;

    string smrulesResponse = ".smrules.response";
    string response = ".response";
    for (string f : smrulesResponseFiles)
    {
        f.replace(f.size() - smrulesResponse.size(), smrulesResponse.size(), response);
        huResponseFiles.emplace_back(std::move(f));
    }

    double totalScanTime = 0;
    map<double, string *> scanTimesPaths;
    map<string *, double> pathsScanTimes;

    cout << format("Scanning {} files\n\n", count);
    for (int i = 0; i < count; ++i)
    {
        cout << i;
        string finalCommand = "\"\"" + compilerPath + "\" \"@" + smrulesResponseFiles[i] + "\"" + "\"";

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
        scanTimesPaths.emplace(scanTime, &(smrulesResponseFiles[i]));
        pathsScanTimes.emplace(&(smrulesResponseFiles[i]), scanTime);
    }

    double totalCompileTime = 0;
    map<double, string *> percentages;

    cout << format("\n\nCompiling {} files\n\n", count);
    for (int i = 0; i < count; ++i)
    {
        cout << i;
        string finalCommand = "\"\"" + compilerPath + "\" \"@" + huResponseFiles[i] + "\"" + "\"";

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
        double scanTime = pathsScanTimes.find(&(smrulesResponseFiles[i]))->second;
        double totalUnitBuildTime = scanTime + compileTime;
        double scanPercentageOfTotalTime = (scanTime / totalUnitBuildTime) * 100;

        totalCompileTime += compileTime;
        percentages.emplace(scanPercentageOfTotalTime, &(smrulesResponseFiles[i]));
    }

    auto getFileNameFromCompileCommand = [](string *compileCommand) -> string_view {
        size_t i = compileCommand->find_last_of('\\');
        string_view s(&(*compileCommand)[i], compileCommand->size() - i);
        return s;
    };

    cout << format("\n\nscanning time of {} files from lowest to highest\n\n", count);

    for (auto &[time, ptr] : scanTimesPaths)
    {
        cout << format("{}    {}\n", time, getFileNameFromCompileCommand(ptr));
    }

    cout << format("\n\nscanning time percentage of total unit compilation time of {} files from lowest to highest\n\n",
                   count);

    for (auto &[percentage, ptr] : percentages)
    {
        double scanTime = pathsScanTimes.find(ptr)->second;
        cout << format("{}    {}   {}    {}\n", scanTime, ((100 * scanTime) / percentage) - scanTime, percentage,
                       getFileNameFromCompileCommand(ptr));
    }

    cout << "\n\n  Report \n\n";

    double averageScanTime = totalScanTime / count;
    double averageCompileTime = totalCompileTime / count;
    cout << format("Average ScanTime, CompileTime, % of ScanTime of Total BuildTime {}    {}    {}\n",
                   averageScanTime, averageCompileTime,
                   (averageScanTime / (averageScanTime + averageCompileTime)) * 100);
}
