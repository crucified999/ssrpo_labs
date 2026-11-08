/*
MIT License
Copyright (c) 2019 МГТУ им. Н.Э. Баумана, кафедра ИУ-6, Михаил Фетисов
*/

#ifndef HW_L3_DOMAIN_LAYER_H
#define HW_L3_DOMAIN_LAYER_H

#include <cstdint>

#include "hw/l4_InfrastructureLayer.h"

inline constexpr size_t MAX_COURIER_NAME_LENGTH = 200;
inline constexpr size_t MAX_PRODUCT_NAME_LENGTH = 200;
inline constexpr size_t MAX_DELIVERY_ADDRESS_LENGTH = 500;
inline constexpr uint32_t MIN_ORDER_NUMBER = 1;
inline constexpr uint32_t MAX_ORDER_NUMBER = UINT32_MAX;
inline constexpr uint64_t MAX_DELIVERY_COST_KOPECKS = 9999999999ULL;

/** Карточка заказа курьерской службы с проверяемым инвариантом. */
class Order : public ICollectable
{
    uint32_t _order_number;
    std::string _courier_name;
    std::string _product_name;
    std::string _delivery_address;
    uint64_t _delivery_cost_kopecks;

protected:
    bool invariant() const;

public:
    Order() = delete;
    Order(const Order &) = delete;
    Order & operator=(const Order &) = delete;

    Order(uint32_t order_number, const std::string & courier_name,
          const std::string & product_name, const std::string & delivery_address,
          uint64_t delivery_cost_kopecks);

    uint32_t getOrderNumber() const;
    const std::string & getCourierName() const;
    const std::string & getProductName() const;
    const std::string & getDeliveryAddress() const;
    uint64_t getDeliveryCostKopecks() const;

    bool write(std::ostream & os) override;
};

/** Восстанавливает конкретный заказ из общего формата коллекции. */
class ItemCollector : public ACollector
{
public:
    std::shared_ptr<ICollectable> read(std::istream & is) override;
};

#endif // HW_L3_DOMAIN_LAYER_H
