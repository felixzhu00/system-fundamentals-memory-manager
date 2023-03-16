#include <stdio.h>
#include "sfmm.h"


#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>
#include "debug.h"
int main(int argc, char const *argv[]) {
    sf_malloc(16336);
    sf_show_heap();
    return EXIT_SUCCESS;
}
