/*
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC

 * SPDX-License-Identifier: Apache-2.0

 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define BUF_SIZE 4096

int main(int argc, char *argv[])
{
	int use_stderr = 0;
	int out_fd = STDOUT_FILENO;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-e") == 0) {
			use_stderr = 1;
			out_fd = STDERR_FILENO;
		}
	}

	char buf[BUF_SIZE];
	ssize_t n;

	while ((n = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
		ssize_t written = 0;
		while (written < n) {
			ssize_t w = write(out_fd, buf + written, n - written);
			if (w < 0) {
				return 1;
			}
			written += w;
		}
	}

	if (n < 0) {
		return 1;
	}

	return 0;
}
