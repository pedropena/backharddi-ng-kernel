/*
 * rfbplaymacro - scriptable control of a VNC session
 * Copyright (C) 2000, 2001, 2002, 2003  Tim Waugh <twaugh@redhat.com>

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * The parsing is awful.  Next time use flex.
 */

/* For getline and strncasecmp.. */
#define _GNU_SOURCE

#include "config.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <getopt.h>
#include <netinet/in.h>
#include <unistd.h>
#include <wordexp.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include "d3des.h"

#define VNC_BASE 5900
#define CHALLENGESIZE 16
/* size of 'buf' used in connect_to_server, please make sure
   it's bigger than CHALLENGESIZE */
#define CTS_BUF_SIZE 20

enum LineType
{
	Version,
	Password,
	Shared,
	Default_Delay,
	Key = 100,
	Press,
	Type,
	Pointer,
	Button,
	Click,
	Cut,
};

struct version_info {
	int major;
	int minor;
};

struct password_info {
	char *password;
};

struct shared_info {
	int shared;
};

struct delay_info {
	int specified;
	struct timeval tv;
};

struct key_info {
	int key;
	int up;
	struct delay_info delay;
};

struct press_info {
	int key;
	struct delay_info delay;
};

struct type_info {
	char *string;
	struct delay_info delay;
};

struct pointer_info {
	int x;
	int y;
	int delta;
	struct delay_info delay;
};

struct button_info {
	int number;
	int up;
	struct delay_info delay;
};

struct click_info {
	int number;
	struct delay_info delay;
};

struct cut_info {
	size_t len;
	char *cut;
};

struct inputline {
	enum LineType type;
	union {
		struct version_info version;
		struct password_info password;
		struct shared_info shared;
		struct delay_info default_delay;
		struct key_info key;
		struct press_info press;
		struct type_info type;
		struct pointer_info pointer;
		struct button_info button;
		struct click_info click;
		struct cut_info cut;
	} u;
};

static int parse_delay (const char *str, struct timeval *tvp)
{
	char *end;
	const char *units = str + strspn (str, "0123456789");
	unsigned long value = strtoul (str, &end, 0);
	if (!strncmp (units, "ms", 2)) {
		tvp->tv_sec = value / 1000;
		tvp->tv_usec = 1000 * (value % 1000);
	} else if (!strncmp (units, "s", 1)) {
		tvp->tv_sec = value;
		tvp->tv_usec = 0;
	} else {
		fprintf (stderr, "Can't parse delay: %s\n", str);
		tvp->tv_sec = 0;
		tvp->tv_usec = 0;
		return 1;
	}
	return 0;
}

static int parse_optional_delay (const char *opt, const char *dly,
				 struct delay_info *delayp)
{
	delayp->specified = 0;
	if (strcasecmp (opt, "delay"))
		return 0;
	parse_delay (dly, &delayp->tv);
	delayp->specified = 1;
	return delayp->specified;
}

static int parse_keysym_char (char sym)
{
	return (int)sym;
}

static int parse_keysym (const char *sym)
{
	/* There's a better way to do this, right? */
	const char *hack = "echo -e \"#include <X11/keysym.h>\\nXK_";
	char buffer[1000];
	const char *p;
	char *q;
	size_t len = 0;
	char *line = NULL;
	FILE *f;
	char *end;
	unsigned long key;

	/* Is it a single character?  That's easy. */
	if (*sym && !sym[1])
		return parse_keysym_char (*sym);

	/* Is it a number key code? */
	key = strtoul (sym, &end, 0);
	if (sym != end)
		return key;

	/* Otherwise it's a keysym.  This code ought to replaced with
	 * something nicer.. */
	strcpy (buffer, hack);
	q = buffer + strlen (buffer);
	for (p = sym; *p && !isspace (*p); p++)
		*q++ = *p;
	strcpy (q, "\"|/lib/cpp|tail -1");
	f = popen (buffer, "r");
	getline (&line, &len, f);
	pclose (f);
	key = strtoul (line, &end, 0);
	free (line);
	if (line == end) {
		fprintf (stderr, "Undefined keysym: %s\n", sym);
		return 0;
	}
	return key;
}

static int get_input_line (FILE *input, struct inputline *result)
{
	static int state;
	static int ln;
	char *line = NULL;
	size_t len = 0;
	char quoted = '\0';
	int is_comment_line;
	wordexp_t we;
	char *p;
	int ret;

	ret = getline (&line, &len, input);
	if (ret < 0)
		return ret;
	ret = 0;
	ln++;

	if (state == 0) {
		/* There must be a version to start with. */
		unsigned long major, minor;
		char *start, *end;
		if (strncmp (line, "RFM ", 4))
			ret = 1;
		if (!ret) {
			start = line + 4;
			major = strtoul (start, &end, 10);
			if (end != start + 3)
				ret = 1;
			if (*end != '.')
				ret = 1;
		}
		if (!ret) {
			start = start + 4;
			minor = strtoul (start, &end, 10);
			if (end != start + 3)
				ret = 1;
			if (*end != '\n')
				ret = 1;
		}

		if (ret) {
			fprintf (stderr,
				 "Incorrect version in input (line %d)\n", ln);
			exit (1);
		}

		result->type = Version;
		result->u.version.major = major;
		result->u.version.minor = minor;
		state++;
		goto out;
	}

	/* Chop off comments. */
	is_comment_line = 1;
	for (p = line; *p; p++) {
		if (quoted) {
			switch (*p) {
			case '\'':
			case '\"':
				if (quoted == *p)
					quoted = '\0';
				break;
			case '\\':
				p++;
				if (!*p) {
					fprintf (stderr, "Unterminated string "
						 "(line %d)\n", ln);
					ret = 1;
				}
				break;
			}
		} else {
			switch (*p) {
			case '#':
				*p = '\0';
				p--; /* break out */
				break;
			case '\'':
			case '\"':
				quoted = *p;
				break;
			default:
				if (!isspace (*p))
					is_comment_line = 0;
			}
		}
	}

	if (is_comment_line) {
		ret = 1;
		goto out;
	}

	/* Use wordexp to take it apart into words */
	line[strlen (line) - 1] = '\0'; /* chop newline */
	switch ((ret = wordexp (line, &we, WRDE_NOCMD))) {
	case 0:
		break;
	case WRDE_NOSPACE:
		fprintf (stderr, "no free memory for wordexp\n");
		wordfree (&we);
		exit (1);
	default:
		fprintf (stderr, "wordexp returned %d\n", ret);
		exit (1);
	}

	if (we.we_wordc < 1) {
		wordfree (&we);
		goto out;
	}

	/* Lower-case the command keyword */
	for (p = we.we_wordv[0]; *p; p++)
		*p = tolower (*p);

	p = we.we_wordv[0];
	if (!strcmp (p, "password")) {
		if (we.we_wordc != 2) {
			fprintf (stderr, "Command 'password' takes one "
				 "argument (line %d)\n", ln);
			ret = 1;
		}
		if (!ret) {
			result->type = Password;
			result->u.password.password = strdup (we.we_wordv[1]);
		}
	} else if (!strcmp (p, "shared")) {
		if (we.we_wordc != 1) {
			fprintf (stderr, "Command 'shared' does not take any "
				 "arguments (line %d)\n", ln);
			ret = 1;
		}
		if (!ret) {
			result->type = Shared;
			result->u.shared.shared = 1;
		}
	} else if (!strcmp (p, "default")) {
		if (we.we_wordc < 3) {
			fprintf (stderr,
				 "Default requires variable (line %d)\n", ln);
			ret = 1;
		}
		if (!ret) {
			if (!strcasecmp (we.we_wordv[1], "delay")) {
				struct timeval tv;
				if (parse_delay (we.we_wordv[2], &tv))
					ret = 1;
				else {
					result->type = Default_Delay;
					result->u.default_delay.specified = 1;
					result->u.default_delay.tv = tv;
				}
			} else {
				fprintf (stderr,
					 "Unknown variable '%s' (line %d)\n",
					 we.we_wordv[1], ln);
				ret = 1;
			}
		}
	} else if (!strcmp (p, "key")) {
		int up = 0;
		int key;

		if (we.we_wordc < 3) {
			fprintf (stderr, "Command 'key' takes two arguments "
				 "(line %d)\n", ln);
			ret = 1;
		}

		if (!ret) {
			key = parse_keysym (we.we_wordv[1]);
			if (!strcasecmp (we.we_wordv[2], "up"))
				up = 1;
			else if (strcasecmp (we.we_wordv[2], "down")) {
				fprintf (stderr,
					 "Unknown key state (line %d)\n", ln);
				ret = 1;
			}
		}

		if (!ret) {
			result->u.key.delay.specified = 0;
			if (we.we_wordc > 4)
				parse_optional_delay (we.we_wordv[3],
						      we.we_wordv[4],
						      &result->u.key.delay);
			result->type = Key;
			result->u.key.up = up;
			result->u.key.key = key;
		}
	} else if (!strcmp (p, "press")) {
		if (we.we_wordc < 2) {
			fprintf (stderr, "Command 'press' takes one "
				 "argument (line %d)\n", ln);
			ret = 1;
		}

		if (!ret) {
			int key;
			key = parse_keysym (we.we_wordv[1]);
			result->u.press.delay.specified = 0;
			if (we.we_wordc > 3)
				parse_optional_delay (we.we_wordv[2],
						      we.we_wordv[3],
						      &result->u.press.delay);
			result->type = Press;
			result->u.press.key = key;
		}
	} else if (!strcmp (p, "type")) {
		if (we.we_wordc < 2) {
			fprintf (stderr, "Command 'type' takes one argument "
				 "(line %d)\n", ln);
			ret = 1;
		}

		if (!ret) {
			result->u.type.delay.specified = 0;
			if (we.we_wordc > 3)
				parse_optional_delay (we.we_wordv[2],
						      we.we_wordv[3],
						      &result->u.type.delay);
			result->type = Type;
			result->u.type.string = strdup(we.we_wordv[1]);
		}
	} else if (!strcmp (p, "pointer")) {
		int delta = 0;
		char *start, *end;

		if (we.we_wordc < 3) {
			fprintf (stderr, "Command 'pointer' takes two "
				 "arguments (line %d)\n", ln);
			ret = 1;
		}

		if (!ret) {
			if (*we.we_wordv[1] == '+' ||
			    *we.we_wordv[1] == '-' ||
			    *we.we_wordv[2] == '+' ||
			    *we.we_wordv[2] == '-')
				delta = 1;
			if (delta && (isdigit (*we.we_wordv[1]) ||
				      isdigit (*we.we_wordv[2]))) {
				fprintf (stderr, "Either both or none must "
					 "be deltas (line %d)\n", ln);
				ret = 1;
			}
		}

		if (!ret) {
			start = we.we_wordv[1] + delta;
			result->u.pointer.x = strtoul (start, &end, 10);
			if (start == end) {
				fprintf (stderr, "Couldn't understand X coord "
					 "(line %d)\n", ln);
				ret = 1;
			}
		}

		if (!ret) {
			start = we.we_wordv[2] + delta;
			result->u.pointer.y = strtoul (start, &end, 10);
			if (start == end) {
				fprintf (stderr, "Couldn't understand Y coord "
					 "(line %d)\n", ln);
				ret = 1;
			}
		}

		if (!ret) {
			result->u.pointer.delta = delta;

			result->u.pointer.delay.specified = 0;
			if (we.we_wordc > 4)
				parse_optional_delay(we.we_wordv[3],
						     we.we_wordv[4],
						     &result->u.pointer.delay);
			result->type = Pointer;
		}
	} else if (!strcmp (p, "button")) {
		int up = 0;
		int button;

		if (we.we_wordc < 3) {
			fprintf (stderr, "Command 'button' takes two "
				 "arguments (line %d)\n", ln);
			ret = 1;
		}

		if (!ret) {
			char *end;
			button = strtoul (we.we_wordv[1], &end, 10);
			if (we.we_wordv[1] == end) {
				fprintf (stderr, "Unknown button (line %d)\n",
					 ln);
				ret = 1;
			}
		}

		if (!ret) {
			if (!strcasecmp (we.we_wordv[2], "up"))
				up = 1;
			else if (strcasecmp (we.we_wordv[2], "down")) {
				fprintf (stderr,
					 "Unknown button state (line %d)\n",
					 ln);
				ret = 1;
			}
		}

		if (!ret) {
			result->u.button.delay.specified = 0;
			if (we.we_wordc > 4)
				parse_optional_delay (we.we_wordv[3],
						      we.we_wordv[4],
						      &result->u.button.delay);
			result->type = Button;
			result->u.button.up = up;
			result->u.button.number = button;
		}
	} else if (!strcmp (p, "click")) {
		int button;

		if (we.we_wordc < 2) {
			fprintf (stderr, "Command 'click' takes one "
				 "argument (line %d)\n", ln);
			ret = 1;
		}

		if (!ret) {
			char *end;
			button = strtoul (we.we_wordv[1], &end, 10);
			if (we.we_wordv[1] == end) {
				fprintf (stderr, "Unknown button (line %d)\n",
					 ln);
				ret = 1;
			}
		}

		if (!ret) {
			result->u.click.delay.specified = 0;
			if (we.we_wordc > 3)
				parse_optional_delay (we.we_wordv[2],
						      we.we_wordv[3],
						      &result->u.click.delay);
			result->type = Click;
			result->u.click.number = button;
		}
	} else if (!strcmp (p, "cut")) {
		result->type = Cut;
		/* Need to write this */
	} else {
		fprintf (stderr, "Skipping unknown command (line %d)\n", ln);
		ret = 1;
	}

	wordfree (&we);

 out:
	free (line);
	return ret;
}

static ssize_t read_exact (int fd, void *buf, size_t len)
{
	ssize_t got;
	ssize_t need = len;
	do {
		got = read (fd, buf, need);
		if (got > 0) {
			buf += got;
			need -= got;
		}
		else return got;
	} while (need > 0);
	return len;
}

static ssize_t write_exact (int fd, const void *buf, size_t len)
{
	ssize_t wrote;
	ssize_t need = len;
	do {
		wrote = write (fd, buf, need);
		if (wrote > 0) {
			buf += wrote;
			need -= wrote;
		}
		else return wrote;
	} while (need > 0);
	return len;
}


/* This function ripped from vnc source as is (vncauth.c) */
void
vncEncryptBytes(unsigned char *bytes, char *passwd)
{
    unsigned char key[8];
    int i;

    /* key is simply password padded with nulls */

    for (i = 0; i < 8; i++) {
        if (i < strlen(passwd)) {
            key[i] = passwd[i];
        } else {
            key[i] = 0;
        }
    }

    deskey(key, EN0);

    for (i = 0; i < CHALLENGESIZE; i += 8) {
        des(bytes+i, bytes+i);
    }
}

static int connect_to_server (const char *display, int shared,
			      const char *password)
{
	char buf[CTS_BUF_SIZE];
	uint32_t bit32;
	struct sockaddr_in sin;
	int s;
	char *server, *p, *end;
	unsigned long port;

	s = socket (PF_INET, SOCK_STREAM, IPPROTO_IP);
	if (s < 0)
		return s;

	/* Parse display as 'server:display' */
	server = strdup (display);
	p = strchr (server, ':');
	if (!p) {
		fprintf (stderr, "Invalid display: \"%s\"\n", display);
		return -1;
	}
	*p++ = '\0';
	port = VNC_BASE + strtoul (p, &end, 10);
	if (p == end) {
		fprintf (stderr, "Invalid display number: %s\n", p);
		return -1;
	}

	sin.sin_family = AF_INET;
	if (!server[0])
		sin.sin_addr.s_addr = htonl (INADDR_ANY);
	else {
		if ((sin.sin_addr.s_addr = inet_addr (server)) == -1) {
			struct hostent *hp;
			hp = gethostbyname (server);
			if (!hp) {
				fprintf (stderr, "unknown host: %s\n", server);
				exit (1);
			}
			memcpy (&sin.sin_addr.s_addr, hp->h_addr_list[0],
				hp->h_length);
		}
	}
	sin.sin_port = htons (port);

	if (connect (s, (struct sockaddr *) &sin, sizeof (sin))) {
		perror ("connect");
		return -1;
	}

	/* ProtocolVersion from server */
	if (read_exact (s, buf, 12) < 12) {
		fprintf (stderr, "Couldn't read ProtocolVersion\n");
		close (s);
		return -1;
	}

	/* ProtocolVersion from client */
	write_exact (s, "RFB 003.003\n", 12);

	if (read_exact (s, &bit32, 4) < 4) {
		fprintf (stderr, "Couldn't read Authentication\n");
		close (s);
		return -1;
	}

	bit32 = ntohl (bit32);
	if (!bit32) {
		fprintf (stderr, "Connection failed\n");
		close (s);
		return -1;
	}

	if (bit32 == 2) {
		if (!password) {
			fprintf (stderr, "Need a password!\n");
			close (s);
			return -1;
		}

		/* Let's try to authenticate */
		if (read_exact (s, buf, CHALLENGESIZE) < CHALLENGESIZE) {
			fprintf (stderr, "Couldn't read DES Challenge\n");
			close(s);
			return -1;
		}

		/* buf now contains the server's 16 byte challenge */
		vncEncryptBytes(buf, (char *)password);
		
		write_exact(s, buf, CHALLENGESIZE);

		if (read_exact(s, &bit32, 4) < 4) {
			fprintf(stderr, "DES Encrypted password sent, unable to read server response\n");
			close(s);
			return -1;
		}

		if (bit32 != 0) {
			fprintf(stderr, "Authentication Failed.\n");
			close(s);
			return -1;
		}

		/* Authentication successful!  wow!  let's set bit32 back to 1 so the next
		   if statement doesn't bomb us out */
		bit32=1;
	}

	if (bit32 != 1) {
		fprintf (stderr, "Unknown authentication scheme: %d\n",
			 bit32);
		close (s);
		return -1;
	}

	/* ClientInitialisation from client */
	buf[0] = shared ? 1 : 0;
	write_exact (s, buf, 1);

	/* ServerInitialisation from server */
	if (read_exact (s, buf, 20) < 20) {
		fprintf (stderr, "Couldn't read ServerInitialisation\n");
		close (s);
		return -1;
	}

	if (read_exact (s, &bit32, 4) < 4) {
		fprintf (stderr, "Couldn't read ServerInitialisation\n");
		close (s);
		return -1;
	}

	bit32 = ntohl (bit32);
	while (bit32) {
		int size = 20;
		if (bit32 < size)
			size = bit32;
		read_exact (s, buf, size);
		bit32 -= size;
	}

	return s;
}

static int do_pointer_event (int buttons, int x, int y, int socket)
{
	char packet[20];
	uint16_t xx, yy;

	packet[0] = '\5';
	packet[1] = buttons;
	xx = htons (x);
	yy = htons (y);
	memcpy (packet + 2, &xx, 2);
	memcpy (packet + 4, &yy, 2);
	write_exact (socket, packet, 6);
	return 0;
}

void usage (const char *progname)
{
	fprintf(stderr,
		"usage: cat filename | %s [OPTIONS]\n"
		"where OPTIONS are:\n\n"
		" --passwd=password             Password for authentication\n"
		" --server=[server]:display     VNC server to connect to\n"
		" --version                     Display rfbproxy version\n"
		" --help                        Display this help\n",
		progname);
}

int main (int argc, char *argv[])
{
	const char *display = ":1";
	int connected = 0;
	struct timeval defaultdelay;
	struct timeval delay;
	char packet[20];
	uint32_t bit32;
	struct inputline line;
	int shared = 0;
	char *password = NULL;
	int socket = -1;
	int do_delay;
	int x, y;
	int buttons = 0;
	int i;

	x = y = 0;

	for (;;) {
		static struct option long_options[] = {
			{"passwd", 1, 0, 'p'},
			{"server", 1, 0, 's'},
			{"version", 0, 0, 'v'},
			{"help", 0, 0, 'h'}
		};
		int c = getopt_long (argc, argv, "ps",
				long_options, NULL);
		if (c == -1)
			break;
			
		switch (c) {
			char *p;
		case 'v':
			fprintf (stderr, "%s\n", PACKAGE " " VERSION);
			exit (0);
		case 'h':
			usage (argv[0]);
			exit (0);
		case 's':
			display = optarg;
			break;
		case 'p':
			password = optarg;
			break;
		default:
			usage (argv[0]);
			exit (1);
		}
	}

	if (!display) {
		fprintf(stderr, "Invalid vnc server specified\n");
		exit(0);
	}

	defaultdelay.tv_sec = 0;
	defaultdelay.tv_usec = 0;
	while (!feof (stdin)) {
		if (get_input_line (stdin, &line))
			continue;

		if (line.type >= 100 && socket < 0) {
			/* First command.  Connect. */
			socket = connect_to_server (display, shared, password);
			if (socket < 0)
				break;

			connected = 1;
		}

		do_delay = 0;
		switch (line.type) {
		case Version:
			if (line.u.version.major != 1) {
				fprintf (stderr, "Incorrect version\n");
				exit (1);
			}
			break;

		case Shared:
			shared = 1;
			break;

		case Default_Delay:
			defaultdelay = line.u.default_delay.tv;
			break;

		case Password:
			password = line.u.password.password;
			break;

		case Key:
			if (!connected)
				goto not_connected;

			packet[0] = '\4';
			packet[1] = !line.u.key.up;
			packet[2] = packet[3] = '\0';
			bit32 = htonl (line.u.key.key);
			memcpy (packet + 4, &bit32, 4);
			write_exact (socket, packet, 8);
			do_delay = 1;
			if (line.u.key.delay.specified)
				delay = line.u.key.delay.tv;
			else
				delay = defaultdelay;
			break;

		case Press:
			if (!connected)
				goto not_connected;

			packet[0] = '\4';
			packet[1] = '\1';
			packet[2] = packet[3] = '\0';
			bit32 = htonl (line.u.press.key);
			memcpy (packet + 4, &bit32, 4);
			write_exact (socket, packet, 8);
			packet[1] = '\0';
			write_exact (socket, packet, 8);
			do_delay = 1;
			if (line.u.press.delay.specified)
				delay = line.u.press.delay.tv;
			else
				delay = defaultdelay;
			break;

		case Type:
		{
			unsigned char *p = line.u.type.string;
			if (!connected)
				goto not_connected;

			packet[0] = '\4';
			packet[2] = packet[3] = '\0';
			while (*p) {
				bit32 = htonl (*p);
				memcpy (packet + 4, &bit32, 4);
				packet[1] = '\1';
				write_exact (socket, packet, 8);
				packet[1] = '\0';
				write_exact (socket, packet, 8);
				p++;
			}

			do_delay = 1;
			if (line.u.type.delay.specified)
				delay = line.u.type.delay.tv;
			else
				delay = defaultdelay;
			break;
		}

		case Pointer:
			if (!connected)
				goto not_connected;

			if (line.u.pointer.delta) {
				x += line.u.pointer.x;
				y += line.u.pointer.y;
				if (x < 0) x = 0;
				if (y < 0) y = 0;
				/* Should cap high values too */
			} else {
				x = line.u.pointer.x;
				y = line.u.pointer.y;
			}

			do_pointer_event (buttons, x, y, socket);
			do_delay = 1;
			if (line.u.pointer.delay.specified)
				delay = line.u.pointer.delay.tv;
			else
				delay = defaultdelay;
			break;
			
		case Button:
			if (!connected)
				goto not_connected;

			if (line.u.button.number > 7) {
				fprintf (stderr, "Button %d ignored\n",
					 line.u.button.number);
				break;
			}

			buttons &= ~(1<<line.u.button.number);
			if (!line.u.button.up)
				buttons |= 1<<line.u.button.number;

			do_pointer_event (buttons, x, y, socket);
			do_delay = 1;
			if (line.u.button.delay.specified)
				delay = line.u.button.delay.tv;
			else
				delay = defaultdelay;
			break;

		case Click:
			if (!connected) {
			not_connected:
				fprintf (stderr, "Not connected yet!\n");
				break;
			}

			if (line.u.button.number > 7) {
				fprintf (stderr, "Button %d ignored\n",
					 line.u.button.number);
				break;
			}

			buttons |= 1<<line.u.button.number;
			do_pointer_event (buttons, x, y, socket);

			buttons &= ~(1<<line.u.button.number);
			do_pointer_event (buttons, x, y, socket);
			do_delay = 1;
			if (line.u.button.delay.specified)
				delay = line.u.button.delay.tv;
			else
				delay = defaultdelay;
			break;

		case Cut:
		default:
			fprintf (stderr, "(not implemented)\n");
			break;
		}

		if (do_delay)
			select (0, NULL, NULL, NULL, &delay);
	}

	if (socket >= 0)
		close (socket);

	return 0;
}
