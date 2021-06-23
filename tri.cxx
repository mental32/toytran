// A toy language for funsies.

#include <iostream>
#include <string>
#include <map>
#include <cstdio>
#include <termios.h>
#include <unistd.h>

auto stding_getchar_linux()
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

#define TRIE_WORD ssize_t
void eval(const std::string &input)
{
    std::map<TRIE_WORD, TRIE_WORD> tape;
    TRIE_WORD data_ptr = 0;
    TRIE_WORD inst_ptr = 0;

    const auto length = input.length();

    do
    {
        auto ch = input[inst_ptr++];

        switch (ch)
        {
        case '+':
            tape[data_ptr]++;
            break;
        case '-':
            tape[data_ptr]--;
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
                int skip = 0;

                while (inst_ptr <= length)
                {
                    if (input[inst_ptr] == ']')
                    {
                        if (skip == 0)
                        {
                            inst_ptr++;
                            break;
                        }
                        else
                        {
                            skip--;
                            inst_ptr++;
                        }
                    }
                    else if (input[inst_ptr] == '[')
                    {
                        skip++;
                        inst_ptr++;
                    }
                    else
                    {
                        inst_ptr++;
                    }
                }
            }
            break;
        case ']':
            if (tape[data_ptr] != 0)
            {
                int skip = 0;

                inst_ptr--;

                while (--inst_ptr > 0)
                {
                    if (input[inst_ptr] == ']')
                    {
                        skip++;
                    }
                    else if (input[inst_ptr] == '[')
                    {
                        if (skip == 0)
                        {
                            inst_ptr++;
                            break;
                        }
                        else
                        {
                            skip--;
                        }
                    }
                }
            }
            break;
        case '?':
            std::cout << "[" << data_ptr << "]: " << tape[data_ptr] << std::endl;
            break;
        case ',':
            tape[data_ptr] = stding_getchar_linux();
            break;
        case '.':
            std::cout << static_cast<char>(tape[data_ptr]);
            break;
        default:
            break;
        }
    } while (inst_ptr <= length && inst_ptr >= 0);
}

int main(void)
{
    std::string input;

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
