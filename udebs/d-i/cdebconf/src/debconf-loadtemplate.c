/**
 * @file loadtemplate.c
 * @brief simple utility to load a template file into the 
 *        database
 */
#include "common.h"
#include "configuration.h"
#include "database.h"
#include "question.h"
#include "template.h"
#include "debconfclient.h"

#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <locale.h>

int merge = 0;
static struct option options[] = {
    { "merge", 0, &merge, 'm' },
    { "help", 0, 0, 'h' },
    { 0, 0, 0, 0 },
};

static void usage(char *progname, int rc)
{
    fprintf(stderr, "Usage: %s [--merge] <owner> <template>\n", progname);
    exit(rc);
}

static void parsecmdline(struct configuration *config, int argc, char **argv)
{
    int c;

    while ((c = getopt_long(argc, argv, "hm", options, NULL)) > 0)
    {
        switch(c)
        {
            case 'h':
                usage(argv[0], 0);
                break;
            case 'm':
                break;
            default:
                fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
                exit(-1);
                break;
        }
    }
    if (argc < 2+optind)
        usage(argv[0], -1);
}

void add_questions_debconf(int argc, char **argv)
{
    int i;
    struct debconfclient *client;
    const char *owner;
    client = debconfclient_new ();

    owner = argv[optind];
    for (i = optind + 1; i <= argc; i++)
    {
        if (argv[i])
            client->command (client, "X_LOADTEMPLATEFILE", 
                                         argv[i], owner, NULL);
    }
    debconf_x_save (client);
}

int main(int argc, char **argv)
{
    struct configuration *config = NULL;
    struct question_db *qdb = NULL;
    struct template_db *tdb = NULL;
    struct template *t = NULL;
    struct question *q = NULL;
    struct template *oldt = NULL;
    char *owner;
    int i;

    setlocale(LC_ALL, "");

    config = config_new();
    parsecmdline(config, argc, argv);

    /* always load all translations if running standalone */
    unsetenv("DEBCONF_DROP_TRANSLATIONS");

    /* If debconf is already running, use debconfclient to load
     * the templates;
     * This is a hack until we introduce a standard debconf
     * primitive for doing this.
     */
    if (getenv("DEBIAN_HAS_FRONTEND") != NULL)
    {
         add_questions_debconf(argc, argv);
         exit(0);
    }

    /* parse the configuration info */
    if (config->read(config, DEBCONFCONFIG) == 0)
        DIE("Error reading configuration information");

    /* initialize database modules */
    if ((tdb = template_db_new(config, NULL)) == 0)
        DIE("Cannot initialize DebConf template database");
    if ((qdb = question_db_new(config, tdb, NULL)) == 0)
        DIE("Cannot initialize DebConf config database");

    tdb->methods.load(tdb);
    qdb->methods.load(qdb);

    owner = argv[optind];
    i = optind + 1;
    while (i < argc)
    {
        t = template_load(argv[i++]);
        while (t)
        {
            oldt = tdb->methods.get(tdb, t->tag);
            if (oldt == NULL || merge == 0 || NULL == template_l10nmerge(oldt, t))
            {
                if (tdb->methods.set(tdb, t) != DC_OK)
                    INFO(INFO_ERROR, "Cannot add template %s", t->tag);
            }
            if (oldt)
                template_deref(oldt);

            q = qdb->methods.get(qdb, t->tag);
            if (q == NULL)
            {
                q = question_new(t->tag);
                q->template = t;
                template_ref(t);
            }
            else if (q->template != t)
            {
                template_deref(q->template);
                q->template = t;
                template_ref(t);
            }
            question_owner_add(q, owner);
            if (qdb->methods.set(qdb, q) != DC_OK)
                INFO(INFO_ERROR, "Cannot add config %s", t->tag);
            question_deref(q);
            oldt = t;
            t = t->next;
            template_deref(oldt);
        }
    }

    if (tdb->methods.save(tdb) != DC_OK)
	exit(1);
    if (qdb->methods.save(qdb) != DC_OK)
	exit(1);
    template_db_delete(tdb);
    question_db_delete(qdb);
    config_delete(config);

    return EXIT_SUCCESS;
}
