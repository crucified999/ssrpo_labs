/* 
MIT License 

Copyright (c) 2019 МГТУ им. Н.Э. Баумана, кафедра ИУ-6, Михаил Фетисов, 

Программа-заготовка для домашнего задания
*/

#ifndef HW_L4_INFRASTRUCTURE_LAYER_H
#define HW_L4_INFRASTRUCTURE_LAYER_H

#include <string>
#include <vector>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>

#include <fstream>

/** Максимальное количество записей, допустимое в коллекции и её файле. */
inline constexpr size_t MAX_COLLECTION_SIZE = 1000000;

/**
 * Читает бинарное число целиком.
 * @throws std::runtime_error Если поток не содержит всех байтов числа.
 */
template<typename T>
T readNumber(std::istream& is)
{
    T result{};
    if (!is.read(reinterpret_cast<char *>(&result), sizeof(result)))
        throw std::runtime_error("Не удалось прочитать число из потока");

    return result;
}

/**
 * Читает длину uint16_t и ровно столько байтов строки, включая нулевые байты.
 * @throws std::runtime_error Если длина превышает предел или данные оборваны.
 */
std::string readString(std::istream& is, size_t max_string_length);

/** Записывает бинарное число; при ошибке потока бросает std::runtime_error. */
template<typename T>
void writeNumber(std::ostream& os, T i)
{
    if (!os.write(reinterpret_cast<const char *>(&i), sizeof(i)))
        throw std::runtime_error("Не удалось записать число в поток");
}

/**
 * Записывает строку с длиной uint16_t без завершающего нулевого байта.
 * @throws std::runtime_error Если длина не помещается в uint16_t или запись не удалась.
 */
void writeString(std::ostream& os, const std::string& s);

class ICollectable
{
public:
    virtual ~ICollectable() = default;

    virtual bool write(std::ostream& os) = 0;
};

class ACollector
{
    std::vector<std::shared_ptr<ICollectable>> _items;
    std::vector<bool>                          _removed_signs;
    size_t                                     _removed_count = 0;

    bool    invariant() const
    {
        return _items.size() == _removed_signs.size() && _removed_count <= _items.size();
    }

public:
    virtual ~ACollector() = default;

    virtual std::shared_ptr<ICollectable> read(std::istream& is) = 0;

    /** Количество позиций, включая помеченные как удалённые. */
    size_t getSize() const { return _items.size(); }

    /** Возвращает элемент; неверный индекс вызывает std::out_of_range. */
    std::shared_ptr<ICollectable> getItem(size_t index) const;

    /** Возвращает отметку удаления; неверный индекс вызывает std::out_of_range. */
    bool isRemoved(size_t index) const;

    /**
     * Добавляет непустой элемент.
     * @throws std::invalid_argument Для nullptr.
     * @throws std::length_error При достижении MAX_COLLECTION_SIZE.
     */
    void addItem(std::shared_ptr<ICollectable> item);

    /** Помечает существующую позицию удалённой; повторное удаление допустимо. */
    void removeItem(size_t index);

    /**
     * Заменяет активный элемент непустым значением.
     * @throws std::out_of_range Для неверной или удалённой позиции.
     * @throws std::invalid_argument Для nullptr.
     */
    void updateItem(size_t index, const std::shared_ptr<ICollectable> & item);

    /** Полностью очищает позиции и счётчик удалённых элементов. */
    void clean();

    /**
     * Читает количество записей uint64_t и атомарно заменяет текущую коллекцию.
     * При недоступном или некорректном файле возвращает false и сохраняет коллекцию.
     * Лишние байты после последней записи считаются ошибкой формата.
     */
    bool loadCollection(const std::string file_name);

    /** Записывает активные записи; возвращает false при ошибке записи или закрытия. */
    bool saveCollection(const std::string file_name) const;
};

#endif // HW_L4_INFRASTRUCTURE_LAYER_H
