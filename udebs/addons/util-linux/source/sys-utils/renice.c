/*
 * Copyright (c) 1983, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

 /* 1999-02-22 Arkadiusz Mi�kiewicz <misiek@pld.ORG.PL>
  * - added Native Language Support
  */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <stdio.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "nls.h"

int donice(int,int,int);

void usage(int rc)
{
	printf( _("\nUsage:\n"
		" renice [-n] priority [-p|--pid] pid  [... pid]\n"
		" renice [-n] priority  -g|--pgrp pgrp [... pgrp]\n"
		" renice [-n] priority  -u|--user user [... user]\n"
		" renice -h | --help\n"
		" renice -v | --version\n\n"));

	exit(rc);
}

/*
 * Change the priority (nice) of processes
 * or groups of processes which are already
 * running.
 */
int
main(int argc, char **argv)
{
	int which = PRIO_PROCESS;
	int who = 0, prio, errs = 0;
	char *endptr = NULL;

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	argc--;
	argv++;

	if (argc == 1) {
		if (strcmp(*argv, "-h") == 0 ||
		    strcmp(*argv, "--help") == 0)
			usage(EXIT_SUCCESS);

		if (strcmp(*argv, "-v") == 0 ||
		    strcmp(*argv, "--version") == 0) {
			printf(_("renice from %s\n"), PACKAGE_STRING);
			exit(EXIT_SUCCESS);
		}
	}

	if (argc < 2)
		usage(EXIT_FAILURE);

	if (strcmp(*argv, "-n") == 0 || strcmp(*argv, "--priority") == 0) {
		argc--;
		argv++;
	}

	prio = strtol(*argv, &endptr, 10);
	if (*endptr)
		usage(EXIT_FAILURE);

	argc--;
	argv++;

	for (; argc > 0; argc--, argv++) {
		if (strcmp(*argv, "-g") == 0 || strcmp(*argv, "--pgrp") == 0) {
			which = PRIO_PGRP;
			continue;
		}
		if (strcmp(*argv, "-u") == 0 || strcmp(*argv, "--user") == 0) {
			which = PRIO_USER;
			continue;
		}
		if (strcmp(*argv, "-p") == 0 || strcmp(*argv, "--pid") == 0) {
			which = PRIO_PROCESS;
			continue;
		}
		if (which == PRIO_USER) {
			register struct passwd *pwd = getpwnam(*argv);

			if (pwd == NULL) {
				fprintf(stderr, _("renice: %s: unknown user\n"),
					*argv);
				continue;
			}
			who = pwd->pw_uid;
		} else {
			who = strtol(*argv, &endptr, 10);
			if (who < 0 || *endptr) {
				fprintf(stderr, _("renice: %s: bad value\n"),
					*argv);
				continue;
			}
		}
		errs += donice(which, who, prio);
	}
	return errs != 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

int
donice(int which, int who, int prio) {
	int oldprio, newprio;

	errno = 0;
	oldprio = getpriority(which, who);
	if (oldprio == -1 && errno) {
		fprintf(stderr, "renice: %d: ", who);
		perror(_("getpriority"));
		return 1;
	}
	if (setpriority(which, who, prio) < 0) {
		fprintf(stderr, "renice: %d: ", who);
		perror(_("setpriority"));
		return 1;
	}
	errno = 0;
	newprio = getpriority(which, who);
	if (newprio == -1 && errno) {
		fprintf(stderr, "renice: %d: ", who);
		perror(_("getpriority"));
		return 1;
	}

	printf(_("%d: old priority %d, new priority %d\n"),
	       who, oldprio, newprio);
	return 0;
}
