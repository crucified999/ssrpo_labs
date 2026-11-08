#include <charconv>
#include <limits>
#include <stdexcept>

#include "hw/l2_ApplicationLayer.h"

using namespace std;

namespace
{
    const string DATA_DEFAULT_NAME = "lab.data";

    uint64_t parseUnsigned(const string & text, uint64_t minimum, uint64_t maximum)
    {
        uint64_t value = 0;
        auto result = from_chars(text.data(), text.data() + text.size(), value);
        if (result.ec != errc() || result.ptr != text.data() + text.size()
            || value < minimum || value > maximum)
            throw invalid_argument("недопустимое числовое значение '" + text + "'");
        return value;
    }

    size_t parseIndex(const string & text)
    {
        return static_cast<size_t>(parseUnsigned(text, 0, numeric_limits<size_t>::max()));
    }

    uint64_t parseDeliveryCost(const string & text)
    {
        size_t point = text.find('.');
        string rubles = text.substr(0, point);
        uint64_t value = parseUnsigned(rubles, 0, MAX_DELIVERY_COST_KOPECKS / 100) * 100;
        if (point != string::npos)
        {
            string kopecks = text.substr(point + 1);
            if (kopecks.empty() || kopecks.size() > 2)
                throw invalid_argument("стоимость задаётся в рублях, не более двух цифр после точки");
            value += parseUnsigned(kopecks, 0, 99) * (kopecks.size() == 1 ? 10 : 1);
        }
        return value;
    }

    string formatDeliveryCost(uint64_t kopecks)
    {
        return to_string(kopecks / 100) + "." + (kopecks % 100 < 10 ? "0" : "")
            + to_string(kopecks % 100);
    }

    shared_ptr<Order> createOrder(const vector<string> & args, size_t offset)
    {
        uint32_t order_number = static_cast<uint32_t>(
            parseUnsigned(args[offset], MIN_ORDER_NUMBER, MAX_ORDER_NUMBER));
        uint64_t delivery_cost = parseDeliveryCost(args[offset + 4]);
        return make_shared<Order>(order_number, args[offset + 1], args[offset + 2],
                                  args[offset + 3], delivery_cost);
    }
}

bool Application::performCommand(const vector<string> & args)
{
    if (args.empty())
        return false;

    try
    {
        const string & command = args[0];
        if (command == "l" || command == "load" || command == "s" || command == "save")
        {
            if (args.size() > 2)
                throw invalid_argument("команда load/save принимает не более одного имени файла");
            string file_name = args.size() == 1 ? DATA_DEFAULT_NAME : args[1];
            bool is_load = command == "l" || command == "load";
            bool is_success = is_load ? _col.loadCollection(file_name) : _col.saveCollection(file_name);
            if (!is_success)
                _out.Output(string("Ошибка при ") + (is_load ? "загрузке" : "сохранении")
                    + " файла '" + file_name + "'");
            return is_success;
        }

        if (command == "c" || command == "clean")
        {
            if (args.size() != 1)
                throw invalid_argument("команда clean не принимает аргументов");
            _col.clean();
            return true;
        }

        if (command == "a" || command == "add")
        {
            if (args.size() != 6)
                throw invalid_argument("формат: add NUMBER COURIER PRODUCT ADDRESS COST");
            _col.addItem(createOrder(args, 1));
            return true;
        }

        if (command == "r" || command == "remove")
        {
            if (args.size() != 2)
                throw invalid_argument("формат: remove INDEX");
            _col.removeItem(parseIndex(args[1]));
            return true;
        }

        if (command == "u" || command == "update")
        {
            if (args.size() != 7)
                throw invalid_argument("формат: update INDEX NUMBER COURIER PRODUCT ADDRESS COST");
            size_t index = parseIndex(args[1]);
            auto order = createOrder(args, 2);
            _col.updateItem(index, order);
            return true;
        }

        if (command == "v" || command == "view")
        {
            if (args.size() != 1)
                throw invalid_argument("команда view не принимает аргументов");
            size_t count = 0;
            for (size_t i = 0; i < _col.getSize(); ++i)
            {
                if (_col.isRemoved(i))
                    continue;
                const Order & item = static_cast<const Order &>(*_col.getItem(i));
                _out.Output("[" + to_string(i) + "] Заказ № " + to_string(item.getOrderNumber())
                    + " | курьер: " + item.getCourierName()
                    + " | товар: " + item.getProductName()
                    + " | адрес: " + item.getDeliveryAddress()
                    + " | доставка: " + formatDeliveryCost(item.getDeliveryCostKopecks()) + " руб.");
                ++count;
            }
            _out.Output("Количество элементов в коллекции: " + to_string(count));
            return true;
        }

        if (command == "h" || command == "help")
        {
            if (args.size() != 1)
                throw invalid_argument("команда help не принимает аргументов");
            _out.Output("Курьерская служба. Команды:\n"
                "  add NUMBER COURIER PRODUCT ADDRESS COST (a)\n"
                "  update INDEX NUMBER COURIER PRODUCT ADDRESS COST (u)\n"
                "  remove INDEX (r), view (v), clean (c)\n"
                "  load [FILE] (l), save [FILE] (s), help (h)\n"
                "Текст с пробелами заключайте в двойные кавычки.\n"
                "Номер заказа: 1..4294967295. Стоимость доставки: 0..99999999.99 руб.\n"
                "Стоимость задаётся с точкой, не более двух знаков после неё. Индексы с 0.\n"
                "Пустая строка или конец ввода завершает работу; сохранение выполняется командой save.");
            return true;
        }

        _out.Output("Недопустимая команда '" + command + "'");
        return false;
    }
    catch (const exception & error)
    {
        _out.Output(string("Ошибка: ") + error.what());
        return false;
    }
}
