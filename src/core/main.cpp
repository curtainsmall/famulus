// Temporary entry point: hello world only. Replaced in M1 by the real daemon
// bootstrap (pipe, serve loop, --foreground, --version).

#include <cstdio>

int main()
{
    std::fputs("hello world\n", stdout);
    return 0;
}
