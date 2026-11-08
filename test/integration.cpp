#include "support.h"

#include <cstdint>
#include <functional>
#include <iostream>
#include <utility>

using testing::check;
namespace fs = std::filesystem;

namespace
{
const std::string SUCCESS = "Выполнение завершено успешно";

void contains(const std::string & output, const std::string & expected)
{
    check(output.find(expected) != std::string::npos,
          "Expected text: " + expected + "\nActual output:\n" + output);
}

void excludes(const std::string & output, const std::string & unexpected)
{
    check(output.find(unexpected) == std::string::npos, "Unexpected text: " + unexpected);
}

// Независимое построение ожидаемого бинарного формата без функций приложения.
template<typename T>
std::string number(T value)
{
    return std::string(reinterpret_cast<const char *>(&value), sizeof(value));
}

std::string binaryString(const std::string & text)
{
    return number<uint16_t>(static_cast<uint16_t>(text.size())) + text;
}

std::string record(uint32_t order_number = 1,
                   const std::string & courier = "Иванов Иван Иванович",
                   const std::string & product = "Книга",
                   const std::string & address = "Москва, ул. Лесная, 1", uint64_t cost = 25050)
{
    return number(order_number) + binaryString(courier) + binaryString(product)
        + binaryString(address) + number(cost);
}

class Fixture
{
    testing::TemporaryDirectory _directory;
    fs::path _binary;
    fs::path _unit_binary;

public:
    Fixture(const fs::path & binary, const fs::path & unit_binary)
        : _binary(binary), _unit_binary(unit_binary) {}

    const fs::path & directory() const { return _directory.path(); }

    std::string run(const std::vector<std::string> & commands, bool is_success = true) const
    {
        std::string input;
        for (const auto & command : commands)
            input += command + '\n';
        auto result = testing::runProcess({_binary.string()}, directory(), input);
        check(result.exit_code == (is_success ? 0 : 1),
              "Unexpected exit code: " + std::to_string(result.exit_code)
              + "\n" + result.output + result.errors);
        check(result.errors.empty(), "Unexpected stderr: " + result.errors);
        if (is_success)
            contains(result.output, SUCCESS);
        else
        {
            excludes(result.output, SUCCESS);
            check(result.output.find("Ошибка") != std::string::npos
                || result.output.find("Недопустимая команда") != std::string::npos,
                "Missing error diagnostic: " + result.output);
        }
        return result.output;
    }

    void runUnitTests() const
    {
        auto result = testing::runProcess({_unit_binary.string(), directory().string()}, directory());
        check(result.exit_code == 0, result.output + result.errors);
        check(result.errors.empty(), result.errors);
        contains(result.output, "C++ unit checks passed:");
        std::cout << "  " << result.output;
    }
};

void assertOrder(const std::string & output, size_t index, uint32_t order_number,
                 const std::string & courier, const std::string & product,
                 const std::string & address, const std::string & cost)
{
    contains(output, "[" + std::to_string(index) + "] Заказ № " + std::to_string(order_number)
        + " | курьер: " + courier + " | товар: " + product + " | адрес: " + address
        + " | доставка: " + cost + " руб.");
}

void testCrudAndPersistence(const Fixture & fixture)
{
    auto output = fixture.run({
        "add 101 \"Иванов Иван Иванович\" \"Ноутбук\" \"Москва, ул. Лесная, 1\" 250.50",
        "add 102 \"Петров Пётр Петрович\" \"Книга\" \"Москва, ул. Бауманская, 2\" 100",
        "update 0 103 \"Сидоров Сидор Сидорович\" \"Планшет\" \"Москва, ул. Новая, 3\" 350.75",
        "remove 1", "view", "save orders.data"});
    assertOrder(output, 0, 103, "Сидоров Сидор Сидорович", "Планшет", "Москва, ул. Новая, 3", "350.75");
    excludes(output, "Петров");
    contains(output, "Количество элементов в коллекции: 1");
    output = fixture.run({"add 999 Temporary Product Address 0", "load orders.data", "view"});
    excludes(output, "Temporary");
    assertOrder(output, 0, 103, "Сидоров Сидор Сидорович", "Планшет", "Москва, ул. Новая, 3", "350.75");
}

void testShortCommands(const Fixture & fixture)
{
    auto output = fixture.run({"a 1 A B C 1", "a 2 D E F 2", "u 0 3 G H I 3.5",
                               "r 1", "r 1", "s", "c", "l", "v", "h"});
    assertOrder(output, 0, 3, "G", "H", "I", "3.50");
    check(fs::is_regular_file(fixture.directory() / "lab.data"), "Default data file not created");
    contains(output, "update");
}

void testCleanAfterRemoval(const Fixture & fixture)
{
    auto output = fixture.run({"add 1 A B C 1", "remove 0", "clean", "add 2 D E F 0",
                               "view", "clean", "save", "load", "view"});
    assertOrder(output, 0, 2, "D", "E", "F", "0.00");
    contains(output, "Количество элементов в коллекции: 0");
    check(testing::readFile(fixture.directory() / "lab.data") == number<uint64_t>(0),
          "Empty collection must contain only the zero count");
}

void testQuotedFields(const Fixture & fixture)
{
    auto output = fixture.run({R"(add 1 "Иванов Иван" "Книга \"C++\"" "Москва \\ Центр" 0.01)",
                               "save \"orders with spaces.data\"", "clean",
                               "load \"orders with spaces.data\"", "view"});
    assertOrder(output, 0, 1, "Иванов Иван", "Книга \"C++\"", "Москва \\ Центр", "0.01");
    check(fs::is_regular_file(fixture.directory() / "orders with spaces.data"), "Quoted filename");
}

void testMaximumValues(const Fixture & fixture)
{
    std::string courier(200, 'N'), product(200, 'P'), address(500, 'A');
    auto output = fixture.run({"add 4294967295 " + courier + " " + product + " " + address
                               + " 99999999.99", "save", "clean", "load", "view"});
    assertOrder(output, 0, 4294967295U, courier, product, address, "99999999.99");
}

void testExactMoney(const Fixture & fixture)
{
    const std::vector<std::pair<std::string, std::string>> values = {
        {"0", "0.00"}, {"0.01", "0.01"}, {"1.1", "1.10"}, {"10.05", "10.05"}, {"0001.02", "1.02"}};
    for (const auto & value : values)
    {
        auto output = fixture.run({"add 1 A B C " + value.first, "save", "clean", "load", "view"});
        assertOrder(output, 0, 1, "A", "B", "C", value.second);
    }
}

void testHelpAndWhitespace(const Fixture & fixture)
{
    fixture.run({"\"help\""});
    fixture.run({""});
    fixture.run({"   \t"});
    auto output = fixture.run({"   add 1 A B C 1   ", "view  "});
    assertOrder(output, 0, 1, "A", "B", "C", "1.00");
}

void testArgumentCounts(const Fixture & fixture)
{
    for (const auto & command : {"add", "add 1 A B C", "add 1 A B C 1 extra", "update",
        "update 0 1 A B C", "update 0 1 A B C 1 extra", "remove", "remove 0 extra", "load a b",
        "save a b", "clean extra", "view extra", "help extra"})
        fixture.run({command}, false);
}

void testInvalidOrderNumbers(const Fixture & fixture)
{
    for (const std::string value : {"0", "4294967296", "-1", "+1", "1x", "1.0", "x", "999999999999999999999999"})
        fixture.run({"add " + value + " A B C 0"}, false);
}

void testInvalidCosts(const Fixture & fixture)
{
    for (const std::string value : {"-1", "+1", "1x", "NaN", "inf", "1e2", "1,50", "1.001",
        "1.", ".5", "1.2.3", "100000000", "999999999999999999999999", "\"\""})
        fixture.run({"add 1 A B C " + value}, false);
}

void testInvalidText(const Fixture & fixture)
{
    for (size_t index = 0; index < 3; ++index)
    {
        size_t limit = index == 2 ? 500 : 200;
        for (const std::string & invalid : {std::string("\"\""), std::string("\"   \""),
             std::string("\"A\tB\""), std::string("\"A\177B\""), std::string(limit + 1, 'X')})
        {
            std::vector<std::string> fields = {"A", "B", "C"};
            fields[index] = invalid;
            fixture.run({"add 1 " + fields[0] + " " + fields[1] + " " + fields[2] + " 1"}, false);
        }
    }
}

void testInvalidIndexes(const Fixture & fixture)
{
    for (const std::string command : {"remove", "update"})
    {
        std::string suffix = command == "update" ? " 1 A B C 0" : "";
        for (const std::string index : {"0", "1", "-1", "+1", "1x", "x", "999999999999999999999999"})
            fixture.run({command + " " + index + suffix}, false);
    }
    fixture.run({"add 1 A B C 0", "remove 0", "update 0 2 D E F 1"}, false);
}

void testUnknownCommand(const Fixture & fixture)
{
    auto output = fixture.run({"nonsense", "add 1 A B C 0", "view"}, false);
    excludes(output, "[0]");
}

void testMalformedQuotes(const Fixture & fixture)
{
    fixture.run({"add 1 \"unclosed A B 0"}, false);
    fixture.run({"add 1 \"A\"B C D 0"}, false);
}

void testFileErrors(const Fixture & fixture)
{
    fixture.run({"load missing.data"}, false);
    fixture.run({"save missing-directory/orders.data"}, false);
    fixture.run({"save ."}, false);
}

void testCorruptFiles(const Fixture & fixture)
{
    std::string valid = record();
    const std::vector<std::string> cases = {
        "", "\1", number<uint64_t>(1000001), number<uint64_t>(1),
        number<uint64_t>(1) + valid.substr(0, valid.size() - 1),
        number<uint64_t>(1) + number<uint32_t>(1) + number<uint16_t>(201),
        number<uint64_t>(1) + record(1, "Иванов Иван Иванович", ""),
        number<uint64_t>(1) + record(0),
        number<uint64_t>(1) + record(1, "A", "B", "C", 10000000000ULL),
        number<uint64_t>(1) + valid + "X"};
    for (const auto & data : cases)
    {
        testing::writeFile(fixture.directory() / "corrupt.data", data);
        fixture.run({"load corrupt.data"}, false);
    }
}

void testBinaryLayout(const Fixture & fixture)
{
    fixture.run({"add 305419896 \"Иванов Иван Иванович\" Книга \"Москва, ул. Лесная, 1\" 250.50",
                 "save layout.data"});
    check(testing::readFile(fixture.directory() / "layout.data")
        == number<uint64_t>(1) + record(305419896), "Binary order layout differs from the specification");
}
}

int main(int argc, char ** argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: integration_tests LAB_BINARY UNIT_BINARY\n";
        return 1;
    }
    const auto binary = fs::absolute(argv[1]);
    const auto unit_binary = fs::absolute(argv[2]);
    const std::vector<std::pair<std::string, std::function<void(const Fixture &)>>> cases = {
        {"CRUD and persistence", testCrudAndPersistence}, {"Short commands", testShortCommands},
        {"Clean after removal", testCleanAfterRemoval}, {"Quoted fields and filename", testQuotedFields},
        {"Maximum values", testMaximumValues}, {"Exact money", testExactMoney},
        {"Help and whitespace", testHelpAndWhitespace}, {"Argument counts", testArgumentCounts},
        {"Invalid order numbers", testInvalidOrderNumbers}, {"Invalid costs", testInvalidCosts},
        {"Invalid text fields", testInvalidText}, {"Invalid indexes", testInvalidIndexes},
        {"Unknown command", testUnknownCommand}, {"Malformed quotes", testMalformedQuotes},
        {"File errors", testFileErrors}, {"Corrupt files", testCorruptFiles},
        {"Binary record layout", testBinaryLayout},
        {"Domain and infrastructure", [](const Fixture & fixture) { fixture.runUnitTests(); }}};
    size_t failures = 0;
    for (const auto & item : cases)
    {
        try
        {
            Fixture fixture(binary, unit_binary);
            item.second(fixture);
            std::cout << "[PASS] " << item.first << '\n';
        }
        catch (const std::exception & error)
        {
            ++failures;
            std::cerr << "[FAIL] " << item.first << ": " << error.what() << '\n';
        }
    }
    std::cout << "Tests passed: " << cases.size() - failures << '/' << cases.size() << '\n';
    return failures == 0 ? 0 : 1;
}
