// A toy language for funsies.

extern "C"
{
#include <termios.h>
#include <unistd.h>
#include <cassert>
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

void log(TriMap &tape, TriWord inst_ptr, TriWord data_ptr, char ch)
{
    ::std::cerr << "[" << inst_ptr << ":" << data_ptr << "]: " << tape[data_ptr] << ", " << ch << ::std::endl;
}

int linear_scan(const auto &start, const auto &end, char target, char clobber, int delta)
{
    int skip = 0;
    int n = 0;

    auto ptr = start;

    for (; ptr != end; ptr += delta)
    {
        n++;

        if (ptr->op == target && skip-- == 0)
        {
            return n;
        }

        if (ptr->op == clobber)
        {
            skip++;
        }
    }

    return n;
}

struct Instr
{
    char op;
    TriWord arg;
    TriWord arg2;
};

std::vector<Instr> parse(const std::string &input)
{
    std::vector<Instr> program;

    for (auto ch : input)
    {
        program.emplace_back(Instr{ch, 1});
    }

    return program;
}

std::vector<Instr> reduce_rle(std::vector<Instr> program)
{
    std::vector<Instr> rle_program;
    rle_program.reserve(program.size());

    for (auto &inst : program)
    {
        if (rle_program.size() != 0)
        {
            Instr &top = rle_program.back();

            if (inst.op == top.op && inst.op != '[' && inst.op != ']')
            {
                top.arg += inst.arg;
                continue;
            }
        }

        if (inst.op == '[' || inst.op == ']' || inst.op == '+' || inst.op == '-' || inst.op == '<' || inst.op == '>' || inst.op == '.' || inst.op == ',')
        {
            rle_program.push_back(Instr{inst.op, inst.arg});
        }
    }

    return rle_program;
}

void optimize(std::vector<Instr> &rle_program)
{

    uint16_t offset = 0;
    for (auto &instr : rle_program)
    {
        switch (instr.op)
        {
        case '[':
            instr.arg = linear_scan(rle_program.cbegin() + (offset + 1), rle_program.end(), ']', '[', 1);
            break;

        case ']':
        {
            instr.arg = linear_scan(rle_program.cbegin() + (offset - 1), rle_program.cbegin(), '[', ']', -1);
            int start = (offset + 0) - instr.arg;

            // std::cout << "\n";

            // for (auto i = start - 1; i != offset; i++)
            // {
            //     std::cout << offset << " " << rle_program[i].op << std::endl;
            // }

            assert(rle_program[start].op == '[');
            assert(rle_program[start + rle_program[start].arg].op == ']');

            if (instr.arg == 2 && rle_program[offset - 1].op == '-')
            {
                // std::cout << "Detected Clear loop @ " << offset << std::endl;
                rle_program[offset - 0] = Instr{'Z', 0};
                rle_program[offset - 1] = Instr{'Z', 0};
                rle_program[offset - 2] = Instr{'Z', 3};
            }
            else if (instr.arg == 2 && rle_program[offset - 1].op == '<')
            {
                // std::cout << "Detected Left Scan loop @ " << offset << " " << rle_program[offset - 1].arg << std::endl;
                TriWord end = offset + 1;
                rle_program[offset - 2] = Instr{'L', rle_program[offset - 1].arg, end};
                rle_program[offset - 0] = Instr{'P', 1};
                rle_program[offset - 1] = Instr{'P', 0};
            }
            else if (instr.arg == 2 && rle_program[offset - 1].op == '>')
            {
                // std::cout << "Detected Right Scan loop @ " << offset << " " << rle_program[offset - 1].arg << std::endl;
                TriWord end = offset + 1;
                rle_program[offset - 2] = Instr{'R', rle_program[offset - 1].arg, end};
                rle_program[offset - 0] = Instr{'P', 1};
                rle_program[offset - 1] = Instr{'P', 0};
            }
            else if (rle_program[start + 1].op == '-'                            // NOLINT
                     && rle_program[start + 2].op == '>'                         // NOLINT
                     && rle_program[start + 3].op == '+'                         // NOLINT
                     && rle_program[start + 4].op == '<'                         // NOLINT
                     && rle_program[start + 2].arg == rle_program[start + 4].arg // NOLINT
                     && rle_program[start + 1].arg == rle_program[start + 3].arg // NOLINT
                     && rle_program[start + 5].op == ']'                         // NOLINT
                     && (start + 5) == offset)                                   // NOLINT
            {
                // std::cout << "Detected Copy loop @ " << offset << " " << instr.arg << std::endl;

                TriWord cell_delta = rle_program[start + 2].arg;
                TriWord jump = rle_program[start].arg;

                rle_program[start] = Instr{'C', cell_delta, jump};
                rle_program[start + 1] = Instr{'[', jump};
            }
            else if (rle_program[start + 1].op == '-'                            // NOLINT
                     && rle_program[start + 2].op == '<'                         // NOLINT
                     && rle_program[start + 3].op == '+'                         // NOLINT
                     && rle_program[start + 4].op == '>'                         // NOLINT
                     && rle_program[start + 2].arg == rle_program[start + 4].arg // NOLINT
                     && rle_program[start + 1].arg == rle_program[start + 3].arg // NOLINT
                     && rle_program[start + 5].op == ']'                         // NOLINT
                     && (start + 5) == offset)                                   // NOLINT
            {
                // std::cout << "Detected Copy loop @ " << offset << " " << instr.arg << std::endl;

                TriWord cell_delta = rle_program[start + 2].arg;
                TriWord jump = rle_program[start].arg;

                rle_program[start] = Instr{'c', cell_delta, jump};
                rle_program[start + 1] = Instr{'[', jump};
            }

            break;
        }

        default:
            break;
        }

        offset++;
    }
}

void eval(const std::string &input)
{
    std::vector<Instr> raw_program = parse(std::move(input));
    std::vector<Instr> rle_program = reduce_rle(std::move(raw_program));

    optimize(rle_program);

    TriMap tape;

    TriWord data_ptr = 0;
    TriWord inst_ptr = 0;

    for (; inst_ptr < rle_program.size();)
    {
        Instr &instr = rle_program.at(inst_ptr);

        // log(tape, inst_ptr, data_ptr, instr.op);

        switch (instr.op)
        {
        case '+':
            tape[data_ptr] += instr.arg;
            inst_ptr++;
            break;

        case '-':
            tape[data_ptr] -= instr.arg;
            inst_ptr++;
            break;

        case '>':
            data_ptr += instr.arg;
            inst_ptr++;
            break;

        case '<':
            data_ptr -= instr.arg;
            inst_ptr++;
            break;

        case 'Z':
            tape[data_ptr] = 0;
            inst_ptr += instr.arg;
            break;

        case 'C':
            // std::cout << "Copy loop @ " << inst_ptr << " " << instr.arg << std::endl;
            tape[data_ptr + instr.arg] += tape[data_ptr];
            tape[data_ptr] = 0;
            inst_ptr++;
            break;

        case 'c':
            // std::cout << "Copy loop @ " << inst_ptr << " " << instr.arg << std::endl;
            tape[data_ptr - instr.arg] += tape[data_ptr];
            tape[data_ptr] = 0;
            inst_ptr++;
            break;

        case 'L':
            if (tape[data_ptr] == 0)
            {
                inst_ptr = instr.arg2;
                break;
            }

            while (data_ptr > 0 && tape[data_ptr] != 0)
            {
                data_ptr -= instr.arg;
            }

            inst_ptr = instr.arg2;
            break;

        case 'R':
            if (tape[data_ptr] == 0)
            {
                inst_ptr = instr.arg2;
                break;
            }

            while (data_ptr > 0 && tape[data_ptr] != 0)
            {
                data_ptr += instr.arg;
            }

            inst_ptr = instr.arg2;
            break;

        case 'P':
            std::cerr << "Panic! reached UB area! ix=" << inst_ptr << " " << instr.op << ":" << instr.arg << std::endl;
            return;

        case '.':
            for (int i = 0; i < instr.arg; i++)
            {
                std::cout << static_cast<char>(tape[data_ptr]);
            }

            fflush(stdout);

            inst_ptr++;
            break;

        case ',':
            tape[data_ptr] = getchar_unbuffered();
            inst_ptr++;
            break;

        case '[':
            if (tape[data_ptr] == 0)
            {
                inst_ptr += instr.arg;
            }
            else
            {
                inst_ptr++;
            }

            break;

        case ']':
            if (tape[data_ptr] != 0)
            {
                inst_ptr -= instr.arg;
            }
            else
            {
                inst_ptr++;
            }

            break;

        default:
            inst_ptr++;
        }
    }

    // std::cerr << std::endl;
    // std::cerr << inst_ptr;
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

int main(int argc, char *argv[])
{
    std::string input;

    if (argc == 2)
    {
        input = read_file_to_string(std::ifstream{argv[1]});
        eval(input);
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
            eval(input);
        }

        input.clear();
    }
}
