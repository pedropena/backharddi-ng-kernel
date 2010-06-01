/**
 * @file frontend.h
 * @brief debconf frontend interface
 */
#ifndef _FRONTEND_H_
#define _FRONTEND_H_

#include <stdbool.h>

#include "constants.h"

#undef _
#define _(x) (x)

struct configuration;
struct template_db;
struct question_db;
struct question;
struct frontend;

#define DCF_CAPB_BACKUP		(1UL << 0)
#define DCF_CAPB_PROGRESSCANCEL	(1UL << 1)

struct frontend_module {
    int (*initialize)(struct frontend *, struct configuration *);
    int (*shutdown)(struct frontend *);
    unsigned long (*query_capability)(struct frontend *);
    void (*set_title)(struct frontend *, const char *title);
    void (*info)(struct frontend *, struct question *);
    int (*add)(struct frontend *, struct question *q);
    int (*go)(struct frontend *);
    void (*clear)(struct frontend *);
    bool (*can_go_back)(struct frontend *, struct question *);
    bool (*can_go_forward)(struct frontend *, struct question *);
    bool (*can_cancel_progress)(struct frontend *);

    void (*progress_start)(struct frontend *fe, int min, int max, const char *title);
    int (*progress_set) (struct frontend *fe, int val);
    /* You do not have to implement _step, it will call _set by default */
    int (*progress_step)(struct frontend *fe, int step);
    int (*progress_info)(struct frontend *fe, const char *info);
    void (*progress_stop)(struct frontend *fe);
};

struct frontend {
    /* module name */
    char *name;
	/* module handle */
	void *handle;
	/* configuration data */
	struct configuration *config;
    /* config path - base of instance configuration */
    char configpath[DEBCONF_MAX_CONFIGPATH_LEN];
	/* database handle for templates and config */
	struct template_db *tdb;
	struct question_db *qdb;
	/* frontend capabilities */
	unsigned long capability;
	/* private data */
	void *data;

	/* class variables */
	struct question *questions;
	int interactive;
	char *capb;
	char *title;
    struct question *info;
	char *progress_title;
    int progress_min, progress_max, progress_cur;
	
	/* methods */
    struct frontend_module methods;
    /* path to plugins */
    char *plugin_path;
};

struct frontend *frontend_new(struct configuration *, struct template_db *, struct question_db *);
void frontend_delete(struct frontend *);

#endif
