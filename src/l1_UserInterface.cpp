#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "hw/l2_ApplicationLayer.h"

using namespace std;

/** Консольная реализация интерфейса вывода прикладного уровня. */
class TerminalOutput : public IOutput
{
public:
    void Output(string text) const override final;
};

void TerminalOutput::Output(string text) const
{
    cout << text << endl;
}

int main()
{
    TerminalOutput out;
    Application app(out);

    for (string line; getline(cin, line); )
    {
        if (line.find_first_not_of(" \t\r\n") == string::npos)
            break;

        istringstream input(line);
        vector<string> args;
        while (input >> ws && !input.eof())
        {
            string argument;
            bool is_parsed = static_cast<bool>(input >> quoted(argument));
            int next = input.peek();
            if (!is_parsed || (next != char_traits<char>::eof()
                && !isspace(static_cast<unsigned char>(next))))
            {
                out.Output("Ошибка: некорректные кавычки или разделители аргументов");
                return 1;
            }
            args.emplace_back(argument);
        }
        if (!app.performCommand(args))
            return 1;
    }

    out.Output("Выполнение завершено успешно");
    return 0;
}
