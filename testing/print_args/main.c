/*
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC

 * SPDX-License-Identifier: Apache-2.0

 */

#include <stdio.h>

extern char **environ;

// Application entry point
int main(int argc, char *argv[])
{
    for(int i = 0; i < argc; i++)
    {
        printf("argv[%d]=%s\n", i, argv[i]);
    }

    for(int i = 0; environ[i] != NULL; i++)
    {
        printf("environ[%d]=%s\n", i, environ[i]);
    }

    return 0;
}
