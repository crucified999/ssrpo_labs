#ifndef HW_L3_DOMAIN_LAYER_H
#define HW_L3_DOMAIN_LAYER_H

#include "hw/l4_InfrastructureLayer.h"

const size_t MAX_NAME_LENGTH    = 50;
const size_t MIN_YEAR_OF_BIRTH  = 1900;
const size_t MAX_YEAR_OF_BIRTH  = 2019;

class Visit
{
    int _year, _month, _day;

public:
    Visit() = delete;
    Visit(int year, int month, int day)
        : _year(year), _month(month), _day(day)
    {}

    int getYear() const { return _year; }
    int getMonth() const { return _month; }
    int getDay() const { return _day; }
};

class Person : public ICollectable
{
    std::string        _alias;
    std::vector<Visit> _visits;

protected:
    bool invariant() const;

public:
    Person() = delete;
    Person(const Person & p) = delete;

    Person & operator = (const Person & p) = delete;

    Person(const std::string & alias);
    Person(const std::string & alias, std::vector<Visit> visits);

    const std::string & getAlias() const;

    void setVisits(const std::vector<Visit> & visits);
    const std::vector<Visit> &getVisists() const;

    virtual bool   write(std::ostream& os) override;
};


class ItemCollector: public ACollector
{
public:
    virtual std::shared_ptr<ICollectable> read(std::istream& is) override;

    Person & getPersonRef(size_t index);
};

#endif // HW_L3_DOMAIN_LAYER_H
