/* Программа-заготовка для домашнего задания
*/

#include "hw/l4_InfrastructureLayer.h"

#include <limits>

using namespace std;

string readString(istream& is, size_t max_string_length)
{
    const uint16_t len = readNumber<uint16_t>(is);
    if (len > max_string_length)
        throw runtime_error("Длина строки превышает допустимую");

    string result(len, '\0');
    if (len > 0 && !is.read(&result[0], len))
        throw runtime_error("Данные строки оборваны");

    return result;
}

void writeString(ostream& os, const string& s)
{
    if (s.length() > numeric_limits<uint16_t>::max())
        throw runtime_error("Длина строки не помещается в uint16_t");

    const uint16_t len = static_cast<uint16_t>(s.length());

    writeNumber(os, len);

    if (!os.write(s.data(), len))
        throw runtime_error("Не удалось записать строку в поток");
}


shared_ptr<ICollectable> ACollector::getItem(size_t index) const
{
    if (index >= _items.size())
        throw out_of_range("Индекс элемента вне коллекции");

    return _items[index];
}

bool ACollector::isRemoved(size_t index) const
{
    if (index >= _removed_signs.size())
        throw out_of_range("Индекс элемента вне коллекции");

    return _removed_signs[index];
}

void ACollector::addItem(shared_ptr<ICollectable> item)
{
    if (!item)
        throw invalid_argument("Нельзя добавить пустой элемент");
    if (_items.size() >= MAX_COLLECTION_SIZE)
        throw length_error("Достигнут предел размера коллекции");

    _items.emplace_back(item);
    try
    {
        _removed_signs.emplace_back(false);
    }
    catch (...)
    {
        _items.pop_back();
        throw;
    }

    assert(invariant());
}

void ACollector::removeItem(size_t index)
{
    if (index >= _items.size())
        throw out_of_range("Индекс элемента вне коллекции");

    if (!_removed_signs[index])
    {
        _removed_signs[index] = true;
        _removed_count ++;
    }
}

void ACollector::updateItem(size_t index, const shared_ptr<ICollectable> & item)
{
    if (index >= _items.size() || _removed_signs[index])
        throw out_of_range("Элемент отсутствует или удалён");
    if (!item)
        throw invalid_argument("Нельзя установить пустой элемент");

    _items[index] = item;
}

void ACollector::clean()
{
    _items.clear();
    _removed_signs.clear();
    _removed_count = 0;

    assert(invariant());
}

bool ACollector::loadCollection(const string file_name)
{
    try
    {
        ifstream ifs(file_name, ios_base::binary);
        if (!ifs)
            return false;

        const uint64_t count = readNumber<uint64_t>(ifs);
        if (count > MAX_COLLECTION_SIZE)
            return false;

        vector<shared_ptr<ICollectable>> items;
        vector<bool> removed_signs(static_cast<size_t>(count), false);
        items.reserve(static_cast<size_t>(count));

        for (uint64_t i = 0; i < count; ++i)
        {
            auto item = read(ifs);
            if (!item || !ifs)
                return false;
            items.emplace_back(item);
        }

        if (ifs.peek() != char_traits<char>::eof() || ifs.bad())
            return false;

        // peek() устанавливает eofbit для корректного конца файла.
        ifs.clear();
        ifs.close();
        if (ifs.fail())
            return false;

        _items.swap(items);
        _removed_signs.swap(removed_signs);
        _removed_count = 0;

        assert(invariant());
        return true;
    }
    catch (const exception &)
    {
        return false;
    }
}

bool ACollector::saveCollection(const string file_name) const
{
    assert(invariant());

    try
    {
        ofstream ofs(file_name, ios_base::binary | ios_base::trunc);
        if (!ofs)
            return false;

        const uint64_t count = static_cast<uint64_t>(_items.size() - _removed_count);
        writeNumber(ofs, count);

        bool success = true;
        for (size_t i = 0; i < _items.size(); ++i)
        {
            if (!_removed_signs[i] && (!_items[i]->write(ofs) || !ofs))
            {
                success = false;
                break;
            }
        }

        ofs.close();
        return success && !ofs.fail();
    }
    catch (const exception &)
    {
        return false;
    }
}
