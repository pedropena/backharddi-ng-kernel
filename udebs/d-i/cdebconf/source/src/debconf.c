/**
 * @file debconf.c
 * @brief Configuration module interface
 */
#include "confmodule.h"
#include "configuration.h"
#include "question.h"
#include "frontend.h"
#include "database.h"

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <debian-installer.h>

static struct configuration *config = NULL;
static struct frontend *frontend = NULL;
static struct confmodule *confmodule = NULL;
static struct question_db *questions = NULL;
static struct template_db *templates = NULL;
static char *owner;

static struct option options[] = {
    { "owner", 1, 0, 'o' },
    { "frontend", 1, 0, 'f' },
    { "priority", 1, 0, 'p' },
    { 0, 0, 0, 0 },
};

static int save(void)
{
	return confmodule->save(confmodule);
}

static void cleanup()
{
	if (confmodule->frontend != NULL)
		frontend_delete(confmodule->frontend);
	if (confmodule->questions != NULL)
		question_db_delete(confmodule->questions);
	if (confmodule->templates != NULL)
		template_db_delete(confmodule->templates);
	if (confmodule->config != NULL)
		config_delete(confmodule->config);
}

void sighandler(int sig)
{
	int status = 1;
	if (sig == SIGCHLD)
	{
		while (waitpid(0, &status, WNOHANG) > 0)
			sigchld_status = status;
		signal_received = sig;
		return;
	}
	save();
	/*
	 * SIGUSR1 used to reconfigure the language. Now it
	 * only saves the database.
	 */
	if (sig == SIGUSR1)
		return;
	cleanup();
	exit(status);
}

void help(const char *exename)
{
    fprintf(stderr, "%s [-ffrontend] [-ppriority] [-oowner] <config script>\n", exename);
    fprintf(stderr, "%s [--frontend=frontend] [--priority=priority] [--owner=owner] <config script>\n", exename);
    exit(-1);
}

void parsecmdline(struct configuration *config, int argc, char **argv)
{
    int c;

    while ((c = getopt_long(argc, argv, "+o:p:f:", options, NULL)) > 0)
    {
        switch (c)
        {
            case 'o':
                owner = optarg;
                break;
            case 'p':
                break;
            case 'f':
                config->set(config, "_cmdline::frontend", optarg);
                break;
            default:
                break;
        }
    }

    if (optind >= argc)
    {
        help(argv[0]);
    }
}

int main(int argc, char **argv)
{
	struct sigaction sa;

	sa.sa_handler = &sighandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_NOCLDSTOP | SA_RESTART;
	sigaction(SIGCHLD, &sa, NULL);

	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);
	signal(SIGUSR1, sighandler);
	setlocale (LC_ALL, "");

	config = config_new();
	if (!config) {
	  DIE("Cannot read new config");
	}
	parsecmdline(config, argc, argv);

	/* parse the configuration info */
	if (config->read(config, DEBCONFCONFIG) == 0)
		DIE("Error reading configuration information");

	/* initialize database and frontend modules */
	if ((templates = template_db_new(config, NULL)) == 0)
        	DIE("Cannot initialize debconf template database");
    	if (templates->methods.load(templates) != DC_OK)
        	DIE("Cannot initialize debconf template database");
	if ((questions = question_db_new(config, templates, NULL)) == 0)
		DIE("Cannot initialize debconf configuration database");
	if (questions->methods.load(questions) != DC_OK)
		DIE("Cannot initialize debconf configuration database");
	if ((frontend = frontend_new(config, templates, questions)) == 0)
		DIE("Cannot initialize debconf frontend");
	/* set title */
	{
		char buf[100], pkg[100];
		char *slash = strrchr(argv[optind], '/');
		if (slash == NULL) slash = argv[optind]; else slash++;
		snprintf(pkg, sizeof(pkg), "%s", slash);
		if (strlen(pkg) >= 7 
			&& strcmp(pkg + strlen(pkg) - 7, ".config") == 0)
		{
			pkg[strlen(pkg) - 7] = '\0';
		}
		snprintf(buf, sizeof(buf), "Configuring %s", pkg);
		frontend->methods.set_title(frontend, buf);
	}

	/* startup the confmodule; run the config script and talk to it */
	confmodule = confmodule_new(config, templates, questions, frontend);
        if (owner == NULL)
          owner = "unknown";
        confmodule->owner = owner;
	confmodule->run(confmodule, argc - optind + 1, argv + optind - 1);
	confmodule->communicate(confmodule);
        confmodule->shutdown(confmodule);

	/* shutting down .... sync the database and shutdown the modules */
	save();
	cleanup();

	return confmodule->exitcode;
}
