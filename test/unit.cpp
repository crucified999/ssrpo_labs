#include "hw/l2_ApplicationLayer.h"

#include <filesystem>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {
size_t checks = 0;

void check(bool condition, const std::string & message)
{
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}

template<class Exception, class Function>
void throws(Function function, const std::string & message)
{
    ++checks;
    try {
        function();
    } catch (const Exception &) {
        return;
    }
    throw std::runtime_error(message);
}

std::shared_ptr<Order> order(const std::string & product = "Book")
{
    return std::make_shared<Order>(1, "Иванов Иван Иванович", product, "Москва, ул. Лесная, 1", 25050);
}

void putFile(const std::filesystem::path & path, const std::string & data)
{
    std::ofstream file(path, std::ios::binary);
    file.write(data.data(), static_cast<std::streamsize>(data.size()));
    check(static_cast<bool>(file), "Cannot create test fixture");
}

class CapturingOutput final : public IOutput {
public:
    mutable std::vector<std::string> messages;
    void Output(std::string message) const override { messages.push_back(std::move(message)); }
};

class FailingItem final : public ICollectable {
    bool _throw;
public:
    explicit FailingItem(bool shouldThrow) : _throw(shouldThrow) {}
    bool write(std::ostream &) override
    {
        if (_throw)
            throw std::runtime_error("Deliberate write failure");
        return false;
    }
};

class FailingCollector final : public ACollector {
    bool _throw;
public:
    explicit FailingCollector(bool shouldThrow) : _throw(shouldThrow) {}
    std::shared_ptr<ICollectable> read(std::istream &) override
    {
        if (_throw)
            throw std::runtime_error("Deliberate read failure");
        return nullptr;
    }
};

void testNumbersAndStrings()
{
    std::stringstream buffer(std::ios::in | std::ios::out | std::ios::binary);
    writeNumber<uint64_t>(buffer, 0x1234567890abcdefULL);
    check(readNumber<uint64_t>(buffer) == 0x1234567890abcdefULL, "Number round trip");
    throws<std::runtime_error>([&] { readNumber<uint16_t>(buffer); }, "Missing number");

    std::istringstream shortNumber(std::string(1, '\0'));
    throws<std::runtime_error>([&] { readNumber<uint16_t>(shortNumber); }, "Truncated number");
    std::ostringstream badOutput;
    badOutput.setstate(std::ios::badbit);
    throws<std::runtime_error>([&] { writeNumber<uint16_t>(badOutput, 1); }, "Failed numeric write");
    throws<std::runtime_error>([&] { writeString(badOutput, "A"); }, "Failed string write");
    throws<std::runtime_error>([&] { writeString(buffer, std::string(65536, 'A')); }, "Oversized string write");

    std::stringstream strings;
    writeString(strings, "");
    writeString(strings, std::string("A\0B", 3));
    writeString(strings, std::string(65535, 'X'));
    check(readString(strings, 0).empty(), "Empty string");
    check(readString(strings, 3) == std::string("A\0B", 3), "String with embedded zero");
    check(readString(strings, 65535).size() == 65535, "Largest serializable string");

    std::stringstream longString;
    writeNumber<uint16_t>(longString, 3);
    throws<std::runtime_error>([&] { readString(longString, 2); }, "String limit");
    std::stringstream shortString;
    writeNumber<uint16_t>(shortString, 4);
    shortString << "AB";
    throws<std::runtime_error>([&] { readString(shortString, 4); }, "Truncated string");
}

void testDomain()
{
    Order minimum(MIN_ORDER_NUMBER, "A", "B", "C", 0);
    check(minimum.getOrderNumber() == 1, "Order number getter");
    check(minimum.getCourierName() == "A", "Courier getter");
    check(minimum.getProductName() == "B", "Product getter");
    check(minimum.getDeliveryAddress() == "C", "Address getter");
    check(minimum.getDeliveryCostKopecks() == 0, "Free delivery");
    Order maximum(MAX_ORDER_NUMBER, std::string(MAX_COURIER_NAME_LENGTH, 'N'),
                  std::string(MAX_PRODUCT_NAME_LENGTH, 'P'),
                  std::string(MAX_DELIVERY_ADDRESS_LENGTH, 'A'), MAX_DELIVERY_COST_KOPECKS);
    check(maximum.getOrderNumber() == MAX_ORDER_NUMBER, "Maximum order number");
    check(maximum.getCourierName().size() == MAX_COURIER_NAME_LENGTH, "Maximum courier length");
    check(maximum.getProductName().size() == MAX_PRODUCT_NAME_LENGTH, "Maximum product length");
    check(maximum.getDeliveryAddress().size() == MAX_DELIVERY_ADDRESS_LENGTH, "Maximum address length");
    check(maximum.getDeliveryCostKopecks() == MAX_DELIVERY_COST_KOPECKS, "Maximum cost");
    throws<std::invalid_argument>([] { Order(0, "A", "B", "C", 0); }, "Zero order number");
    throws<std::invalid_argument>([] { Order(1, "", "B", "C", 0); }, "Empty courier");
    throws<std::invalid_argument>([] { Order(1, "A", " ", "C", 0); }, "Blank product");
    throws<std::invalid_argument>([] { Order(1, "A", "B", "C\n", 0); }, "Control in address");
    throws<std::invalid_argument>([] { Order(1, "A", "B", std::string("X\0Y", 3), 0); }, "Zero in address");
    throws<std::invalid_argument>([] { Order(1, "A", "B", "C", MAX_DELIVERY_COST_KOPECKS + 1); }, "Cost limit");
    throws<std::invalid_argument>([] { Order(1, std::string(MAX_COURIER_NAME_LENGTH + 1, 'N'), "B", "C", 0); }, "Long courier");
    throws<std::invalid_argument>([] { Order(1, "A", std::string(MAX_PRODUCT_NAME_LENGTH + 1, 'P'), "C", 0); }, "Long product");
    throws<std::invalid_argument>([] { Order(1, "A", "B", std::string(MAX_DELIVERY_ADDRESS_LENGTH + 1, 'A'), 0); }, "Long address");

    std::stringstream bytes;
    check(maximum.write(bytes), "Order serialization");
    ItemCollector reader;
    const auto restored = std::dynamic_pointer_cast<Order>(reader.read(bytes));
    check(restored && restored->getOrderNumber() == maximum.getOrderNumber(), "Order round trip");
    check(restored->getCourierName() == maximum.getCourierName(), "Courier round trip");
    check(restored->getProductName() == maximum.getProductName(), "Product round trip");
    check(restored->getDeliveryAddress() == maximum.getDeliveryAddress(), "Address round trip");
    check(restored->getDeliveryCostKopecks() == MAX_DELIVERY_COST_KOPECKS, "Exact money round trip");
    std::ostringstream failed;
    failed.setstate(std::ios::badbit);
    throws<std::runtime_error>([&] { minimum.write(failed); }, "Order failed output");
}

void testCollection(const std::filesystem::path & directory)
{
    ItemCollector collection;
    check(collection.getSize() == 0, "Empty collection");
    throws<std::out_of_range>([&] { collection.getItem(0); }, "Get out of range");
    throws<std::out_of_range>([&] { collection.isRemoved(0); }, "Removal flag out of range");
    throws<std::out_of_range>([&] { collection.removeItem(0); }, "Remove out of range");
    throws<std::out_of_range>([&] { collection.updateItem(0, order()); }, "Update out of range");
    throws<std::invalid_argument>([&] { collection.addItem(nullptr); }, "Add nullptr");
    collection.addItem(order());
    throws<std::invalid_argument>([&] { collection.updateItem(0, nullptr); }, "Update nullptr");
    collection.updateItem(0, order("Updated"));
    check(std::dynamic_pointer_cast<Order>(collection.getItem(0))->getProductName() == "Updated", "Update record");
    check(!collection.isRemoved(0), "New item active");
    collection.removeItem(0);
    collection.removeItem(0);
    check(collection.isRemoved(0) && collection.getItem(0), "Idempotent removal");
    throws<std::out_of_range>([&] { collection.updateItem(0, order()); }, "Update removed record");
    collection.clean();
    collection.clean();
    collection.addItem(order("Survivor"));
    check(collection.getSize() == 1 && !collection.isRemoved(0), "Clean resets tombstones");

    const auto valid = directory / "unit-valid.data";
    check(collection.saveCollection(valid.string()), "Save valid collection");
    ItemCollector restored;
    restored.addItem(order("Old"));
    restored.addItem(order("Old2"));
    check(restored.loadCollection(valid.string()), "Load valid collection");
    check(restored.getSize() == 1 && std::dynamic_pointer_cast<Order>(restored.getItem(0))->getProductName() == "Survivor",
          "Load replaces collection");
    check(!restored.loadCollection((directory / "missing.data").string()), "Missing file");
    check(!restored.saveCollection((directory / "missing" / "output.data").string()), "Missing output directory");
    check(!restored.saveCollection(directory.string()), "Directory as output file");

    std::vector<std::string> invalid;
    invalid.emplace_back();
    invalid.emplace_back(2, '\0');
    std::ostringstream count;
    writeNumber<uint64_t>(count, MAX_COLLECTION_SIZE + 1);
    invalid.push_back(count.str());
    std::ostringstream truncated;
    writeNumber<uint64_t>(truncated, 1);
    invalid.push_back(truncated.str());
    std::ifstream file(valid, std::ios::binary);
    const std::string validBytes((std::istreambuf_iterator<char>(file)), {});
    invalid.push_back(validBytes + "X");
    invalid.push_back(validBytes.substr(0, validBytes.size() - 1));
    // Failure after reading one valid record must also preserve the old state.
    std::ostringstream partial;
    writeNumber<uint64_t>(partial, 2);
    order("First")->write(partial);
    invalid.push_back(partial.str());
    const auto bad = directory / "unit-invalid.data";
    for (const auto & data : invalid) {
        putFile(bad, data);
        check(!restored.loadCollection(bad.string()), "Malformed collection rejected");
        check(restored.getSize() == 1 && std::dynamic_pointer_cast<Order>(restored.getItem(0))->getProductName() == "Survivor",
              "Failed load is atomic");
    }

    for (bool shouldThrow : {false, true}) {
        FailingCollector failures(shouldThrow);
        failures.addItem(order());
        check(!failures.loadCollection(valid.string()), "Null or throwing deserializer");
        check(failures.getSize() == 1, "Deserializer failure preserves data");
        failures.addItem(std::make_shared<FailingItem>(shouldThrow));
        check(!failures.saveCollection((directory / "unit-failed-save.data").string()), "False or throwing serializer");
    }

    restored.removeItem(0);
    check(restored.saveCollection(valid.string()), "Save tombstones");
    check(collection.loadCollection(valid.string()) && collection.getSize() == 0, "Saved tombstones omitted");
    if (std::filesystem::exists("/dev/full"))
        check(!restored.saveCollection("/dev/full"), "Buffered output failure");
}

void testApplication()
{
    CapturingOutput output;
    Application application(output);
    check(!application.performCommand({}), "Empty argument vector");
    check(application.performCommand({"add", "1", "Иванов Иван Иванович", "Книга", "Москва, ул. Лесная, 1", "250.50"}), "Application add");
    check(application.performCommand({"view"}), "Application view");
    check(!output.messages.empty(), "Application output callback");
    check(!application.performCommand({"update", "0", "0", "A", "B", "C", "0"}), "Invalid update value");
    output.messages.clear();
    check(application.performCommand({"view"}), "Snapshot before failed update");
    const auto before = output.messages;
    check(!application.performCommand({"update", "99", "2", "A", "B", "C", "1.00"}), "Invalid update index");
    check(!application.performCommand({"update", "0", "2", "", "B", "C", "1.00"}), "Invalid update text");
    output.messages.clear();
    check(application.performCommand({"view"}), "Snapshot after failed updates");
    check(output.messages == before, "Failed updates preserve the entire original order");
    check(application.performCommand({"remove", "0"}), "Application remove");
    check(application.performCommand({"view"}), "View skips tombstones");
}
} // namespace

int main(int argc, char ** argv)
{
    try {
        if (argc != 2)
            throw std::runtime_error("Expected a temporary fixture directory");
        testNumbersAndStrings();
        testDomain();
        testCollection(argv[1]);
        testApplication();
        std::cout << "C++ unit checks passed: " << checks << '\n';
        return 0;
    } catch (const std::exception & error) {
        std::cerr << "Unit test failure: " << error.what() << '\n';
        return 1;
    }
}
