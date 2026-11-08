#include <algorithm>
#include <stdexcept>

#include "hw/l3_DomainLayer.h"

using namespace std;

namespace
{
    bool isValidText(const string & text, size_t max_length)
    {
        return !text.empty() && text.size() <= max_length
            && text.find_first_not_of(' ') != string::npos
            && none_of(text.begin(), text.end(), [](unsigned char ch)
                { return ch < ' ' || ch == 127; });
    }
}

bool Order::invariant() const
{
    return _order_number >= MIN_ORDER_NUMBER
        && isValidText(_courier_name, MAX_COURIER_NAME_LENGTH)
        && isValidText(_product_name, MAX_PRODUCT_NAME_LENGTH)
        && isValidText(_delivery_address, MAX_DELIVERY_ADDRESS_LENGTH)
        && _delivery_cost_kopecks <= MAX_DELIVERY_COST_KOPECKS;
}

Order::Order(uint32_t order_number, const string & courier_name,
             const string & product_name, const string & delivery_address,
             uint64_t delivery_cost_kopecks)
    : _order_number(order_number), _courier_name(courier_name),
      _product_name(product_name), _delivery_address(delivery_address),
      _delivery_cost_kopecks(delivery_cost_kopecks)
{
    if (!invariant())
        throw invalid_argument("некорректные данные заказа: проверьте номер, текстовые поля и стоимость");
}

uint32_t Order::getOrderNumber() const
{
    return _order_number;
}

const string & Order::getCourierName() const
{
    return _courier_name;
}

const string & Order::getProductName() const
{
    return _product_name;
}

const string & Order::getDeliveryAddress() const
{
    return _delivery_address;
}

uint64_t Order::getDeliveryCostKopecks() const
{
    return _delivery_cost_kopecks;
}

bool Order::write(ostream & os)
{
    writeNumber(os, _order_number);
    writeString(os, _courier_name);
    writeString(os, _product_name);
    writeString(os, _delivery_address);
    writeNumber(os, _delivery_cost_kopecks);
    return os.good();
}

shared_ptr<ICollectable> ItemCollector::read(istream & is)
{
    uint32_t order_number = readNumber<uint32_t>(is);
    string courier_name = readString(is, MAX_COURIER_NAME_LENGTH);
    string product_name = readString(is, MAX_PRODUCT_NAME_LENGTH);
    string delivery_address = readString(is, MAX_DELIVERY_ADDRESS_LENGTH);
    uint64_t delivery_cost_kopecks = readNumber<uint64_t>(is);
    return make_shared<Order>(order_number, courier_name, product_name,
                              delivery_address, delivery_cost_kopecks);
}
