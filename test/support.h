#ifndef COURIER_TEST_SUPPORT_H
#define COURIER_TEST_SUPPORT_H

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

namespace testing
{
namespace fs = std::filesystem;

inline void check(bool condition, const std::string & message)
{
    if (!condition)
        throw std::runtime_error(message);
}

inline std::string readFile(const fs::path & path)
{
    std::ifstream stream(path, std::ios::binary);
    check(static_cast<bool>(stream), "Cannot read " + path.string());
    return std::string(std::istreambuf_iterator<char>(stream), {});
}

inline void writeFile(const fs::path & path, const std::string & data)
{
    std::ofstream stream(path, std::ios::binary);
    stream.write(data.data(), static_cast<std::streamsize>(data.size()));
    stream.close();
    check(static_cast<bool>(stream), "Cannot write " + path.string());
}

/** Изолированный каталог для данных теста; удаляется даже при исключении. */
class TemporaryDirectory
{
    fs::path _path;

public:
    TemporaryDirectory()
    {
        auto pattern = (fs::temp_directory_path() / "courier-tests-XXXXXX").string();
        std::vector<char> buffer(pattern.begin(), pattern.end());
        buffer.push_back('\0');
        check(mkdtemp(buffer.data()) != nullptr, "Cannot create temporary directory");
        _path = buffer.data();
    }

    TemporaryDirectory(const TemporaryDirectory &) = delete;
    TemporaryDirectory & operator=(const TemporaryDirectory &) = delete;

    ~TemporaryDirectory()
    {
        std::error_code error;
        fs::remove_all(_path, error);
    }

    const fs::path & path() const { return _path; }
};

struct ProcessResult
{
    int exit_code;
    std::string output;
    std::string errors;
};

/**
 * Запуск отдельного процесса в Linux/macOS без разбора команды оболочкой.
 * Файлы вместо каналов позволяют безопасно собирать большой stdout/stderr.
 */
inline ProcessResult runProcess(const std::vector<std::string> & command,
                                const fs::path & directory, const std::string & input = "",
                                std::chrono::seconds timeout = std::chrono::seconds(10))
{
    check(!command.empty(), "Empty process command");
    TemporaryDirectory streams;
    auto input_path = streams.path() / "stdin";
    auto output_path = streams.path() / "stdout";
    auto errors_path = streams.path() / "stderr";
    writeFile(input_path, input);

    // execvp требует нуль-терминированный массив указателей на аргументы.
    std::vector<char *> arguments;
    for (const auto & argument : command)
        arguments.push_back(const_cast<char *>(argument.c_str()));
    arguments.push_back(nullptr);

    pid_t child = fork();
    check(child >= 0, "Cannot fork test process");
    if (child == 0)
    {
        int in = open(input_path.c_str(), O_RDONLY);
        int out = open(output_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        int err = open(errors_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (in < 0 || out < 0 || err < 0 || chdir(directory.c_str()) != 0
            || dup2(in, STDIN_FILENO) < 0 || dup2(out, STDOUT_FILENO) < 0
            || dup2(err, STDERR_FILENO) < 0)
            std::_Exit(126);
        close(in);
        close(out);
        close(err);
        execvp(arguments[0], arguments.data());
        std::_Exit(127);
    }

    auto deadline = std::chrono::steady_clock::now() + timeout;
    int child_status = 0;
    for (;;)
    {
        pid_t result = waitpid(child, &child_status, WNOHANG);
        if (result == child)
            break;
        if (result < 0 && errno != EINTR)
            throw std::runtime_error("Cannot wait for test process");
        if (std::chrono::steady_clock::now() >= deadline)
        {
            kill(child, SIGKILL);
            while (waitpid(child, &child_status, 0) < 0 && errno == EINTR) {}
            throw std::runtime_error("Process timed out: " + command[0]);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    int exit_code = WIFEXITED(child_status) ? WEXITSTATUS(child_status) : 128 + WTERMSIG(child_status);
    return {exit_code, readFile(output_path), readFile(errors_path)};
}
}

#endif // COURIER_TEST_SUPPORT_H
