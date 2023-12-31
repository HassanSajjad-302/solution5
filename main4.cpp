#include <atomic>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

using std::ifstream, std::string, std::ostringstream, std::format, std::cout, std::vector, std::map,
    std::chrono::high_resolution_clock, std::string_view, std::filesystem::create_directory, std::filesystem::path,
    std::filesystem::current_path, std::set, std::thread, std::mutex, std::atomic, std::lock_guard;

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

string addQuotes(string_view pstr)
{
    return "\"" + string(pstr) + "\"";
}

int main()
{
    size_t count = 1427;
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

    auto getFileNameFromCompileCommand = [](string *compileCommand) -> string_view {
        size_t i = compileCommand->find_last_of(' ');
        i += 1;
        string_view s(&(*compileCommand)[i], compileCommand->size() - i);
        return s;
    };

    path outputDir = current_path() / "main3-output/";
    create_directory(outputDir);

    set<string> uniqueHeaderFiles;
    mutex errorOutputParsingMutex;

    auto scanFile = [&](size_t i, size_t &totalHeaderIncludes, map<size_t, string *> &countOfHeaderIncludesPerFile) {
        string fileName = path(getFileNameFromCompileCommand(compileCommands[i])).filename().string();
        string outputFileName = outputDir.string() + fileName + "_output" + std::to_string(i);
        string errorFileName = outputDir.string() + fileName + "_error" + std::to_string(i);

        string finalCommand = addQuotes(compilerPath) + *compileCommands[i] +
                              " /scanDependencies a.smrules /showIncludes " + "> " + addQuotes(outputFileName) + " 2>" +
                              addQuotes(errorFileName);

        int exitStatus = system(addQuotes(finalCommand).c_str());

        if (exitStatus == EXIT_SUCCESS)
        {
            string commandErrorOutput = fileToString(errorFileName);

            vector<string> outputLines = split(commandErrorOutput, "\n");
            string includeFileNote = "Note: including file:";

            {
                lock_guard<mutex> lk(errorOutputParsingMutex);
                size_t numOfHeaderIncludes = 0;
                for (auto iter = outputLines.begin(); iter != outputLines.end();)
                {
                    if (iter->contains(includeFileNote))
                    {
                        size_t pos = iter->find_first_not_of(includeFileNote);
                        pos = iter->find_first_not_of(" ", pos);

                        for (auto it = iter->begin() + (int)pos; it != iter->end(); ++it)
                        {
                            *it = tolower(*it);
                        }

                        ++numOfHeaderIncludes;
                        ++totalHeaderIncludes;
                        uniqueHeaderFiles.emplace(iter->c_str() + pos, iter->size() - pos);
                    }
                    ++iter;
                }

                countOfHeaderIncludesPerFile.emplace(numOfHeaderIncludes, compileCommands[i]);
                cout << format("{} {}\n", i, fileName);
                fflush(stdout);
            }
        }
        else
        {
            std::cerr << "Failed Executing scanning command\n";
            std::cerr << finalCommand;
            exit(EXIT_FAILURE);
        }
    };

    size_t firstHalfTotalIncludes = 0;
    map<size_t, string *> headerIncludesCountFirstHalf;
    size_t lastHalfTotalIncludes = 0;
    map<size_t, string *> headerIncludesCountLastHalf;

    atomic<size_t> i = 0;
    atomic<size_t> k = compileCommands.size() - count;
    auto parallelScan = [&]() {
        for (size_t j = i.fetch_add(1); j < count;)
        {
            if (!j)
            {
                lock_guard<mutex> lk(errorOutputParsingMutex);
                cout << format("Scanning first {} files\n\n", count);
                fflush(stdout);
            }
            scanFile(j, firstHalfTotalIncludes, headerIncludesCountFirstHalf);
            j = i.fetch_add(1);
        }

        for (size_t l = k.fetch_add(1); l < compileCommands.size();)
        {
            if (l == compileCommands.size() - count)
            {
                lock_guard<mutex> lk(errorOutputParsingMutex);
                cout << format("\n\nScanning last {} files\n\n", count);
                fflush(stdout);
            }
            scanFile(l, lastHalfTotalIncludes, headerIncludesCountLastHalf);
            l = k.fetch_add(1);
        }
    };

    vector<thread *> threads;
    for (size_t m = 0; m < std::thread::hardware_concurrency(); ++m)
    {
        threads.emplace_back(new thread(parallelScan));
    }

    for (thread *t : threads)
    {
        t->join();
    }
    cout << format("\n\n header includes count of first {} files from lowest to highest\n\n", count);

    for (auto &[time, ptr] : headerIncludesCountFirstHalf)
    {
        cout << format("{}    {}\n", time, getFileNameFromCompileCommand(ptr));
    }

    cout << format("\n\n header includes count of last {} files from lowest to highest\n\n", count);

    for (auto &[time, ptr] : headerIncludesCountLastHalf)
    {
        cout << format("{}    {}\n", time, getFileNameFromCompileCommand(ptr));
    }

    cout << "\n\n  Report \n\n";

    double firstHalfAverageIncludes = firstHalfTotalIncludes / count;
    double lastHalfAverageIncludes = lastHalfTotalIncludes / count;
    double averageIncludes = firstHalfAverageIncludes + lastHalfAverageIncludes;
    averageIncludes /= 2;

    cout << format("First-Half Average Includes {}    Second Half Average Includes {}    Average Includes {}\n",
                   firstHalfAverageIncludes, lastHalfAverageIncludes, averageIncludes);

    cout << format("Total Unique Header-Files {}\n", uniqueHeaderFiles.size());
}