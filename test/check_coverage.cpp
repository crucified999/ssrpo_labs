#include "support.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

namespace fs = std::filesystem;
using testing::check;

namespace
{
using Coverage = std::map<std::string, std::map<size_t, bool>>;

std::string trim(const std::string & text)
{
    auto first = text.find_first_not_of(" \t\r\n");
    return first == std::string::npos ? "" : text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}

bool isDigits(const std::string & text)
{
    return !text.empty() && std::all_of(text.begin(), text.end(),
        [](unsigned char ch) { return std::isdigit(ch); });
}

std::string quoteJson(const std::string & text)
{
    std::ostringstream output;
    output << '"';
    for (unsigned char ch : text)
    {
        if (ch == '"' || ch == '\\')
            output << '\\' << ch;
        else if (ch < 32)
            output << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(ch);
        else
            output << ch;
    }
    return output.str() + '"';
}

void readReport(const fs::path & path, const fs::path & root, const fs::path & report_directory,
                Coverage & coverage)
{
    std::string content = testing::readFile(path);
    std::istringstream input(content);
    std::string key;
    for (std::string row; std::getline(input, row); )
    {
        size_t first = row.find(':');
        size_t second = first == std::string::npos ? first : row.find(':', first + 1);
        if (second == std::string::npos)
            continue;
        std::string count = trim(row.substr(0, first));
        std::string line_number = trim(row.substr(first + 1, second - first - 1));
        if (!isDigits(line_number))
            continue;
        size_t line = std::stoull(line_number);
        if (line == 0 && row.compare(second + 1, 7, "Source:") == 0)
        {
            fs::path source = row.substr(second + 8);
            if (!source.is_absolute())
                source = root / source;
            fs::path relative = fs::relative(source, root);
            auto first_part = relative.begin();
            if (first_part != relative.end() && (*first_part == "src" || *first_part == "include"))
            {
                key = relative.generic_string();
                coverage.try_emplace(key);
            }
            continue;
        }
        if (key.empty() || line == 0 || count == "-")
            continue;
        count.erase(std::remove(count.begin(), count.end(), ','), count.end());
        if (!count.empty() && count.back() == '*')
            count.pop_back();
        bool covered = isDigits(count) && std::stoull(count) > 0;
        coverage[key][line] = coverage[key][line] || covered;
    }
    if (!key.empty())
    {
        std::string name = key;
        std::replace(name.begin(), name.end(), '/', '_');
        testing::writeFile(report_directory / (name + ".gcov"), content);
    }
}

std::vector<std::string> gcovCommand()
{
    const char * configured = std::getenv("GCOV");
    std::istringstream input(configured ? configured : "gcov");
    std::vector<std::string> command;
    for (std::string word; input >> std::quoted(word); )
        command.push_back(word);
    check(!command.empty(), "GCOV is empty; use gcov or GCOV='llvm-cov gcov'");
    return command;
}

int collectCoverage(const fs::path & root, double minimum)
{
    fs::path report_directory = root / "coverage";
    fs::create_directories(report_directory);
    for (const auto & entry : fs::directory_iterator(report_directory))
        if (entry.path().extension() == ".gcov" || entry.path().filename() == "summary.json")
            fs::remove(entry.path());

    std::vector<fs::path> notes;
    for (const auto & entry : fs::directory_iterator(root / "build"))
    {
        std::string name = entry.path().filename().string();
        if (entry.path().extension() == ".gcno" && name.size() > 3 && name[0] == 'l'
            && name[1] >= '1' && name[1] <= '4' && name[2] == '_')
            notes.push_back(entry.path());
    }
    check(!notes.empty(), "No coverage notes found; rebuild with --coverage");
    std::sort(notes.begin(), notes.end());

    testing::TemporaryDirectory temporary;
    // Clang может хранить в .gcno относительные пути к исходникам проекта.
    fs::create_directory_symlink(root / "src", temporary.path() / "src");
    fs::create_directory_symlink(root / "include", temporary.path() / "include");
    const auto command = gcovCommand();
    Coverage coverage;
    for (const auto & note : notes)
    {
        auto arguments = command;
        arguments.push_back(note.string());
        auto result = testing::runProcess(arguments, temporary.path());
        check(result.exit_code == 0, "gcov failed (check compiler compatibility):\n" + result.output + result.errors);
        for (const auto & entry : fs::directory_iterator(temporary.path()))
        {
            if (entry.path().extension() != ".gcov")
                continue;
            readReport(entry.path(), root, report_directory, coverage);
            fs::remove(entry.path());
        }
    }
    for (const auto & entry : fs::directory_iterator(root / "src"))
        if (entry.path().extension() == ".cpp")
            check(coverage.count("src/" + entry.path().filename().string()) > 0,
                  "Missing coverage data for " + entry.path().string());

    size_t total_covered = 0, total_executable = 0;
    std::ostringstream summary;
    summary << std::fixed << std::setprecision(6) << "{\n";
    std::cout << std::fixed << std::setprecision(2);
    for (const auto & file : coverage)
    {
        size_t executable = file.second.size();
        size_t covered = static_cast<size_t>(std::count_if(file.second.begin(), file.second.end(),
            [](const auto & entry) { return entry.second; }));
        double percent = executable ? covered * 100.0 / executable : 100.0;
        total_covered += covered;
        total_executable += executable;
        std::cout << file.first << ": " << covered << '/' << executable << " lines (" << percent << "%)\n";
        summary << "  " << quoteJson(file.first) << ": {\"covered\": " << covered
                << ", \"executable\": " << executable << ", \"percent\": " << percent << ", \"uncovered\": [";
        bool first = true;
        for (const auto & line : file.second)
        {
            if (line.second)
                continue;
            summary << (first ? "" : ", ") << line.first;
            first = false;
        }
        summary << "]},\n";
    }
    check(total_executable > 0, "gcov did not report executable source lines");
    double percent = total_covered * 100.0 / total_executable;
    summary << "  \"total\": {\"covered\": " << total_covered << ", \"executable\": " << total_executable
            << ", \"percent\": " << percent << "}\n}\n";
    testing::writeFile(report_directory / "summary.json", summary.str());
    std::cout << "TOTAL line coverage: " << percent << "% (" << total_covered << '/' << total_executable << ")\n";
    if (percent + 1e-9 < minimum)
    {
        std::cerr << "Required coverage: " << minimum << "%\n";
        return 1;
    }
    return 0;
}
}

int main(int argc, char ** argv)
{
    try
    {
        double minimum = 95.0;
        bool reset = false;
        for (int i = 1; i < argc; ++i)
        {
            std::string argument = argv[i];
            if (argument == "--reset")
                reset = true;
            else if (argument == "--minimum" && i + 1 < argc)
            {
                std::string text = argv[++i];
                size_t consumed = 0;
                minimum = std::stod(text, &consumed);
                check(consumed == text.size() && std::isfinite(minimum) && minimum >= 0 && minimum <= 100,
                      "Coverage threshold must be between 0 and 100");
            }
            else
                throw std::runtime_error("Usage: check_coverage [--reset] [--minimum PERCENT]");
        }
        fs::path root = fs::canonical(fs::current_path());
        if (reset)
        {
            for (const auto & entry : fs::directory_iterator(root / "build"))
                if (entry.path().extension() == ".gcda")
                    fs::remove(entry.path());
            return 0;
        }
        return collectCoverage(root, minimum);
    }
    catch (const std::exception & error)
    {
        std::cerr << "Coverage error: " << error.what() << '\n';
        return 1;
    }
}
