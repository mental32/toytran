// A toy language for funsies.

extern "C"
{
#include <termios.h>
#include <unistd.h>
}

#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <cstdio>
#include <vector>
#include <array>
#include <string_view>
#include <utility>
#include <stack>

namespace tri
{

    int getchar_unbuffered()
    {
        // shamelessly stolen from: https://shtrom.ssji.net/skb/getc.html

        struct termios old_tio, new_tio;

        /* get the terminal settings for stdin */
        tcgetattr(STDIN_FILENO, &old_tio);

        /* we want to keep the old setting to restore them a the end */
        new_tio = old_tio;

        /* disable canonical mode (buffered i/o) and local echo */
        new_tio.c_lflag &= (~ICANON & ~ECHO);

        /* set the new settings immediately */
        tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

        auto c = getchar();

        /* restore the former settings */
        tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);

        return c;
    }

    using TriWord = uint16_t;
    using TriMap = ::std::map<TriWord, TriWord>;

    void log(const TriMap &tape, TriWord inst_ptr, TriWord data_ptr, char ch)
    {
        ::std::cerr << "[" << inst_ptr << ":" << data_ptr << "]: " << tape.at(data_ptr) << ", " << ch << ::std::endl;
    }

    int linear_scan(const ::std::string_view &sub, bool forward, char left, char right)
    {
        int skip = 0;
        int n = 0;

        auto start = forward ? sub.cbegin() : sub.cend();
        auto end = forward ? sub.cend() : sub.cbegin();

        auto target = forward ? right : left;
        auto clobber = forward ? left : right;

        for (auto ptr = start; ptr != end; (forward ? ptr++ : ptr--))
        {
            n++;

            if (*ptr == target && skip-- == 0)
            {
                return n;
            }

            if (*ptr == clobber)
            {
                skip++;
            }
        }

        return n;
    }

    void eval(const std::string &input)
    {
        TriMap tape;
        TriMap loops;

        TriWord data_ptr = 0;
        TriWord inst_ptr = 0;

        const auto length = input.length();
        bool no_comment = false;

        do
        {
            auto ch = input.at(inst_ptr++);

            switch (ch)
            {
            case '+':
                tape[data_ptr]++;
                break;
            case '-':
                tape[data_ptr]--;
                break;
            case 'Z':
                tape[data_ptr] = 0;
                break;
            case '>':
                data_ptr++;
                break;
            case '<':
                data_ptr--;
                break;
            case '[':
                if (tape[data_ptr] == 0)
                {
                    auto search = loops.find(inst_ptr - 1);

                    if (search == loops.cend())
                    {
                        auto delta = linear_scan(input.substr(inst_ptr, input.length()), true, '[', ']');
                        loops.insert({inst_ptr - 1, inst_ptr + delta});
                        inst_ptr += delta;
                    }
                    else
                    {
                        inst_ptr = std::get<1>(*search);
                    }
                }

                break;
            case ']':
                if (tape[data_ptr] != 0)
                {
                    auto search = loops.find(inst_ptr - 1);

                    if (search == loops.cend())
                    {
                        auto delta = linear_scan(input.substr(0, inst_ptr - 2), false, '[', ']');
                        loops.insert({inst_ptr - 1, inst_ptr - delta});
                        inst_ptr -= delta;
                    }
                    else
                    {
                        inst_ptr = std::get<1>(*search);
                    }
                }
                break;
            case '?':
                std::cout << "[" << data_ptr << "]: " << tape[data_ptr] << std::endl;
                break;
            case ',':
                tape[data_ptr] = getchar_unbuffered();
                break;
            case '.':
                std::cout << static_cast<char>(tape[data_ptr]);
                break;
            default:
                if (ch == '/')
                {
                    no_comment = !no_comment;
                    break;
                }

                if (no_comment)
                {
                    switch (ch)
                    {
                    case '$':
                        data_ptr += tape[data_ptr];
                        break;
                    default:
                        std::cout << "unrecognized operator: " << ch << std::endl;
                        break;
                    }
                }

                break;
            }
        } while (inst_ptr < length && inst_ptr >= 0);
    }
}

static auto read_file_to_string(std::ifstream stream) -> std::string
{
    constexpr auto read_size = std::size_t{4096};

    stream.exceptions(std::ios_base::badbit);

    auto out = std::string{};
    auto buf = std::string(read_size, '\0');

    while (stream.read(&buf[0], read_size))
    {
        out.append(buf, 0, static_cast<size_t>(stream.gcount()));
    }

    out.append(buf, 0, static_cast<size_t>(stream.gcount()));

    return out;
}

std::size_t replace_all(std::string &inout, std::string_view what, std::string_view with)
{
    std::size_t count{};
    for (std::string::size_type pos{};
         inout.npos != (pos = inout.find(what.data(), pos, what.length()));
         pos += with.length(), ++count)
    {
        inout.replace(pos, what.length(), with.data(), with.length());
    }
    return count;
}

int main(int argc, char *argv[])
{
    std::string input;

    if (argc == 2)
    {
        input = read_file_to_string(std::ifstream{argv[1]});
        std::cerr << replace_all(input, "[-]", "Z") << std::endl;
        tri::eval(input);
        return 0;
    }

    for (;;)
    {
        std::cout << ">> ";
        std::getline(std::cin, input);

        if (input.length() == 0)
        {
            return 0;
        }
        else
        {
            tri::eval(input);
        }

        input.clear();
    }
}
