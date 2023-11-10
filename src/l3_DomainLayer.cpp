#include "hw/l3_DomainLayer.h"

using namespace std;


bool Person::invariant() const
{
    return !_alias.empty();
}

Person::Person(const std::string & alias)
    : _alias(alias)
{
    assert(invariant());
}

Person::Person(const string &alias, std::vector<Visit> visits)
    : _alias(alias)
    , _visits(visits)
{
    assert(invariant());
}

const string &Person::getAlias() const
{
    return _alias;
}

void Person::setVisits(const std::vector<Visit> &visits)
{
    _visits = visits;
}

const std::vector<Visit> &Person::getVisists() const
{
    return _visits;
}

bool   Person::write(ostream& os)
{
    writeString(os, _alias);

    size_t visits_qount = _visits.size();
    writeNumber(os, visits_qount);

    for(const Visit & v : _visits)
    {
        writeNumber(os, v.getYear());
        writeNumber(os, v.getMonth());
        writeNumber(os, v.getDay());
    }

    return os.good();
}



shared_ptr<ICollectable> ItemCollector::read(istream& is)
{
    string alias        = readString(is, MAX_NAME_LENGTH);
    size_t visits_qount = readNumber<size_t>(is);

    vector<Visit> v;

    v.reserve(visits_qount);
    for(size_t i=0; i < visits_qount; ++i)
    {
        int year = readNumber<int>(is);
        int month = readNumber<int>(is);
        int day = readNumber<int>(is);

        v.push_back(Visit(year, month, day));
    }

    shared_ptr<ICollectable> p = std::make_shared<Person>(alias, v);

    return p;
}

Person &ItemCollector::getPersonRef(size_t index)
{
    Person & p = *static_cast<Person *>(getItem(index).get());

    return p;
}
