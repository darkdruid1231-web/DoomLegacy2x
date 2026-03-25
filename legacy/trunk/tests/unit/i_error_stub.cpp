// Minimal I_Error stub for test_menu_draw.
// Provides I_Error without pulling in the full interface layer (SDL, WinMain, etc.)

#include <cstdio>
#include <cstdlib>

void I_Error(const char *error, ...)
{
    std::fputs("I_Error: ", stderr);
    // Print only the format string (va_args not formatted - this is a stub)
    if (error) {
        // Print up to first newline or null
        const char *p = error;
        while (*p && *p != '\n') {
            std::putc(*p++, stderr);
        }
    }
    std::fputs("\n", stderr);
    std::abort();
}
