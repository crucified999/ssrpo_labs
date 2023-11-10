#include "hw/l2_ApplicationLayer.h"

#include <algorithm>

using namespace std;

inline const string DATA_DEFAULT_NAME = "lab.data";

bool Application::performCommand(const vector<string> & args)
{
    if (args.empty())
        return false;

    // load [имя_файла]
    if (args[0] == "l" || args[0] == "load")
    {
        string filename = (args.size() == 1) ? DATA_DEFAULT_NAME : args[1];

        if (!_col.loadCollection(filename))
        {
            _out.Output("Ошибка при загрузке файла '" + filename + "'");
            return false;
        }

        return true;
    }

    // save [имя_файла]
    if (args[0] == "s" || args[0] == "save")
    {
        string filename = (args.size() == 1) ? DATA_DEFAULT_NAME : args[1];

        if (!_col.saveCollection(filename))
        {
            _out.Output("Ошибка при сохранении файла '" + filename + "'");
            return false;
        }

        return true;
    }

    // clean
    if (args[0] == "c" || args[0] == "clean")
    {
        if (args.size() != 1)
        {
            _out.Output("Некорректное количество аргументов команды clean");
            return false;
        }

        _col.clean();

        return true;
    }

    // add alias
    if (args[0] == "a" || args[0] == "add")
    {
        if (args.size() != 2)
        {
            _out.Output("Некорректное количество аргументов команды add");
            return false;
        }

        _col.addItem(make_shared<Person>(args[1].c_str()));
        return true;
    }

    // add_visit person_no year month day
    if (args[0] == "av" || args[0] == "add_visit")
    {
        if (args.size() != 5)
        {
            _out.Output("Некорректное количество аргументов команды add_visit");
            return false;
        }

        Person & p = _col.getPersonRef(stoul(args[1]));

        vector<Visit> v = p.getVisists();
        v.push_back(Visit(stod(args[2]),stod(args[3]),stod(args[4])));
        p.setVisits(v);

        return true;
    }

    // remove person_no
    if (args[0] == "r" || args[0] == "remove")
    {
        if (args.size() != 2)
        {
            _out.Output("Некорректное количество аргументов команды remove");
            return false;
        }

        _col.removeItem(stoul(args[1]));
        return true;
    }

    // update person_no alias
    if (args[0] == "u" || args[0] == "update")
    {
        if (args.size() != 3)
        {
            _out.Output("Некорректное количество аргументов команды update");
            return false;
        }

        _col.updateItem(stoul(args[1]), make_shared<Person>(args[2].c_str()));
        return true;
    }

    // view
    if (args[0] == "v" || args[0] == "view")
    {
        if (args.size() != 1)
        {
            _out.Output("Некорректное количество аргументов команды view");
            return false;
        }

        size_t count = 0;
        for(size_t i=0; i < _col.getSize(); ++i)
        {
            if (!_col.isRemoved(i))
            {
                const Person & item = _col.getPersonRef(i);

                _out.Output("[" + to_string(i) + "] "
                        + item.getAlias() + " "
                        );

                for(const Visit & v : item.getVisists())
                    _out.Output("\t" + to_string(v.getDay()) + "." + to_string(v.getMonth()) + "." + to_string(v.getYear()));

                count ++;
            }
        }

        _out.Output("Количество элементов в коллекции: " + to_string(count));
        return true;
    }

    // report
    if (args[0] == "rp" || args[0] == "report")
    {
        if (args.size() != 1)
        {
            _out.Output("Некорректное количество аргументов команды view");
            return false;
        }

        vector<pair<size_t,string>> report_set;

        for(size_t i=0; i < _col.getSize(); ++i)
        {
            if (!_col.isRemoved(i))
            {
                const Person & p = _col.getPersonRef(i);

                if (p.getVisists().size() > 0)
                    report_set.push_back({p.getVisists().size(),p.getAlias()});
            }
        }

        sort(report_set.begin(),report_set.end(),[](pair<size_t,string> p1, pair<size_t,string> p2){
            return p1.first > p2.first;
        });

        for(const auto & [quantity,alias] : report_set)
            _out.Output(alias + " " + to_string(quantity));

        return true;
    }

    _out.Output("Недопустимая команда '" + args[0] + "'");
    return false;
}
