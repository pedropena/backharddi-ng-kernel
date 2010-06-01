/*
 * Debian Installer main menu program.
 *
 * Copyright 2000,2004  Joey Hess <joeyh@debian.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "main-menu.h"
#include <cdebconf/debconfclient.h>
#include <stdlib.h>
#include <search.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

const int RAISE = 1;
const int LOWER = 0;

int last_successful_item = -1;

/* Save default priority, to be able to return to it when we have to lower it */
int default_priority = 1;

/* Save priority set by main-menu to detect priority changes from the user */
int local_priority = -1;

static struct debconfclient *debconf;

static int di_config_package(di_system_package *p,
			     int (*virtfunc)(di_system_package *));

static void modify_debconf_priority (int raise_or_lower);

/*
 * qsort comparison function (sort by menu item values, fall back to
 * lexical sort to resolve ties deterministically).
 */
int package_array_compare (const void *v1, const void *v2) {
	di_system_package *p1, *p2;

	p1 = *(di_system_package **)v1;
	p2 = *(di_system_package **)v2;

	int r = p1->installer_menu_item - p2->installer_menu_item;
	if (r) return r;
	return strcmp(p1->p.package, p2->p.package);
}

int isdefault(di_system_package *p) {
	int check;

	check = di_system_dpkg_package_control_file_exec(&p->p, "menutest", 0, NULL);
	if (check <= 0 || p->p.status == di_package_status_unpacked || p->p.status == di_package_status_half_configured)
		return true;
	return false;
}

bool isinstallable(di_system_package *p) {
	int check;

	check = di_system_dpkg_package_control_file_exec(&p->p, "isinstallable", 0, NULL);
	if (check <= 0)
		return true;
	return false;
}

/* Return nonzero if all of the virtual packages that P provides are
   provided by installed packages.  */
int provides_installed_virtual_packages(di_package *p) {
	di_slist_node *node1, *node2;
	int provides = 0;

	for (node1 = p->depends.head; node1; node1 = node1->next) {
		di_package_dependency *d = node1->data;

		if (d->type == di_package_dependency_type_provides) {
			int installed = 0;

			provides = 1;

			for (node2 = d->ptr->depends.head; node2; node2 = node2->next) {
				d = node2->data;

				if (d->type == di_package_dependency_type_reverse_provides
				    && d->ptr->status == di_package_status_installed) {
					installed = 1;
					break;
				}
			}

			if (!installed)
				return 0;
		}
	}

	return provides;
}

/* Expects a topologically ordered linked list of packages. */
static di_system_package *
get_default_menu_item(di_slist *list)
{
	di_system_package *p;
	di_slist_node *node;

	/* Traverse the list, return the first menu item that isn't installed */
	for (node = list->head; node != NULL; node = node->next) {
		p = node->data;
		//di_log(DI_LOG_LEVEL_DEBUG, "find default; on: %s", p->p.package);
		if (!p->installer_menu_item ||
		    p->p.status == di_package_status_installed ||
		    !isinstallable(p)) {
			//di_log(DI_LOG_LEVEL_DEBUG, "not menu item; or not installed");
			continue;
		}
		if (p->installer_menu_item < last_successful_item &&
		    p->installer_menu_item < NEVERDEFAULT) {
			//di_log(DI_LOG_LEVEL_DEBUG, "not in range to be default");
			continue;
		}
		/* If menutest says this item should be default, make it so */
		if (!isdefault(p)) {
			//di_log(DI_LOG_LEVEL_DEBUG, "isdefalt says no");
			continue;
		}
		/* If all of the virtual packages provided by a
		   package have already been satisfied, do not default
		   to it.  */
		if (!provides_installed_virtual_packages(&p->p)) {
			//di_log(DI_LOG_LEVEL_DEBUG, "success on this one");
			return p;
		}

		//di_log(DI_LOG_LEVEL_DEBUG, "not default");
	}
	/* Severely broken, there are no menu items in the sorted list */
	return NULL;
}

/* Return the text of the menu entry for PACKAGE.  */
static size_t menu_entry(struct debconfclient *debconf, di_system_package *package, char *buf, size_t size)
{
	char question[256];

	snprintf(question, sizeof(question), "debian-installer/%s/title", package->p.package);
	if (!debconf_metaget(debconf, question, "Description")) {
		strncpy(buf, debconf->value, size);
		return strlen (buf);
	}

	/* The following fallback case can go away once all packages
	   have transitioned to the new form.  */
	di_log(DI_LOG_LEVEL_INFO, "Falling back to the package description for %s", package->p.package);
	if (package->p.short_description)
		strncpy(buf, package->p.short_description, size);
	return strlen (buf);
}

/* Displays the main menu via debconf and returns the selected menu item. */
di_system_package *show_main_menu(di_packages *packages, di_packages_allocator *allocator) {
	di_system_package **package_array, *p;
	di_slist *list;
	di_slist_node *node;
	di_system_package *menudefault = NULL, *ret = NULL;
	int i = 0, num = 0;
	char buf[256], *menu, *s;
	int menu_size, menu_used, size;

	for (node = packages->list.head; node; node = node->next) {
		p = node->data;
		if (((di_system_package *)p)->installer_menu_item)
			num++;
	}
	package_array = di_new (di_system_package *, num + 1);
	package_array[num] = NULL;
	for (node = packages->list.head; node; node = node->next) {
		p = node->data;
		if (p->installer_menu_item)
			package_array[i++] = node->data;
	}
	
	/* Sort by menu number. */
	qsort(package_array, num, sizeof (di_system_package *), package_array_compare);

	/* Order menu so depended-upon packages come first. */
	/* The menu number is really only used to break ties. */
	list = di_system_packages_resolve_dependencies_array_permissive (packages, (di_package **) package_array, allocator);

	/*
	 * Generate list of menu choices for debconf.
	 */
	menu = di_malloc(1024);
	menu[0] = '\0';
	menu_size = 1024;
	menu_used = 1;
	for (node = list->head; node != NULL; node = node->next) {
		p = node->data;
		if (!p->installer_menu_item ||
		    !isinstallable(p))
			continue;
		size = menu_entry(debconf, p, buf, sizeof (buf));
		if (menu_used + size + 2 > menu_size)
		{
			menu_size += 1024;
			menu = di_realloc(menu, menu_size);
		}
		if (*menu)
			strcat(menu, ", ");
		strcat(menu, buf);
		menu_used += size + 2;
	}
	menudefault = get_default_menu_item(list);
	di_slist_free(list);

	/* Make debconf show the menu and get the user's choice. */
	debconf_settitle(debconf, "debian-installer/main-menu-title");
	debconf_capb(debconf);
	debconf_subst(debconf, MAIN_MENU, "MENU", menu);
	if (menudefault) {
		menu_entry(debconf, menudefault, buf, sizeof (buf));
		debconf_set(debconf, MAIN_MENU, buf);
	}
	else {
		di_log(DI_LOG_LEVEL_INFO, "no default menu item"); 
		modify_debconf_priority(LOWER);
	}
	debconf_input(debconf, MENU_PRIORITY, MAIN_MENU);
	debconf_go(debconf);
	debconf_get(debconf, MAIN_MENU);
	s = strdup(debconf->value);
	
	/* Figure out which menu item was selected. */
	for (i = 0; i < num; i++) {
		p = package_array[i];
		menu_entry(debconf, p, buf, sizeof (buf));
		if (strcmp(buf, s) == 0) {
			ret = p;
			break;
		}
	}

	if (! ret) {
		/* This could happen because of a debconf protocol problem
		 * (for example, leading whitespace in menu items can
		 * be stripped and confuse the comparisons), or other
		 * problem. */
		di_log(DI_LOG_LEVEL_WARNING, "Internal error! Cannot find \"%s\" in menu.", s);
	}

	free(menu);
	free(package_array);

	return ret;
}

/*
 * Satisfy the dependencies of a virtual package. Its dependencies
 * that actually provide the package are presented in a debconf select
 * question for the user to pick and choose. Other dependencies are
 * just fed recursively through di_config_package.
 */
static int satisfy_virtual(di_system_package *p) {
	di_slist_node *node;
	di_system_package *dep, *defpkg = NULL;
	char buf[256], *menu, *s = NULL;
	size_t menu_size, menu_used, size;
	int is_menu_item = 0;

	menu = di_malloc(1024);
	menu[0] = '\0';
	menu_size = 1024;
	menu_used = 1;
	/* Compile a list of providing package. The default choice will be the
	 * package with highest priority. If we have ties, menu items are
	 * preferred. If we still have ties, the default choice is arbitrary */
	for (node = p->p.depends.head; node; node = node->next) {
		di_package_dependency *d = node->data;
		dep = (di_system_package *)d->ptr;
		if (d->type == di_package_dependency_type_depends) {
			/* Non-providing dependency */
			di_log(DI_LOG_LEVEL_DEBUG, "non-providing dependency from %s to %s", p->p.package, dep->p.package);
			if (dep->p.status != di_package_status_installed) {
				switch (di_config_package(dep, satisfy_virtual)) {
					case -1:
						return -1;
					case EXIT_BACKUP:
						return EXIT_BACKUP;
				}
			}
			continue;
		}
		if (d->type != di_package_dependency_type_reverse_provides)
			continue;
		if (dep->p.status == di_package_status_installed) {
			/* This means that a providing package is already
			 * configure. So we short-circuit. */
			menu_used = 0;
			break;
		}
		if (defpkg == NULL || dep->p.priority > defpkg->p.priority ||
				(dep->p.priority == defpkg->p.priority &&
				 dep->installer_menu_item < defpkg->installer_menu_item))
			defpkg = dep;
		/* This only makes sense if one of the dependencies
		 * is a menu item */
		if (dep->installer_menu_item)
			is_menu_item = 1;

		size = menu_entry(debconf, dep, buf, sizeof (buf));
		if (menu_used + size + 2 > menu_size)
		{
			menu_size += 1024;
			menu = di_realloc(menu, menu_size);
		}

		if (dep == defpkg) {
			/* We want the default to be the first item */
			char *temp;
			temp = di_malloc (menu_size);
			strcpy(temp, menu);
			strcpy(menu, buf);
			if (strlen(temp)) {
				strcat(menu, ", ");
				strcat(menu, temp);
			}
			di_free(temp);
		} else {
			if (strlen(menu)) {
				strcat(menu, ", ");
			}
			strcat(menu, buf);
		}
		menu_used += size + 2;
	}
	if (menu_used >= 2)
		menu[menu_used-2] = '\0';
	if (menu_used > 1) {
		if (is_menu_item) {
			char *priority = "medium";
			/* Only let the user choose if one of them is a menu item */
			if (defpkg != NULL) {
				menu_entry(debconf, defpkg, buf, sizeof(buf));
				debconf_set(debconf, MISSING_PROVIDE, buf);
			} else {
				/* TODO: How to figure out a default? */
				priority = "critical";
			}
			debconf_capb(debconf, "backup");
			debconf_subst(debconf, MISSING_PROVIDE, "CHOICES", menu);
			debconf_input(debconf, priority, MISSING_PROVIDE);
			if (debconf_go(debconf) != 0)
				return 0;
			debconf_capb(debconf);
			debconf_get(debconf, MISSING_PROVIDE);
			s = strdup(debconf->value);
		}
		/* Go through the dependencies again */
		for (node = p->p.depends.head; node; node = node->next) {
			di_package_dependency *d = node->data;
			dep = (di_system_package *)d->ptr;
			menu_entry(debconf, dep, buf, sizeof(buf));
			if (!is_menu_item || strcmp(s, buf) == 0) {
				/* Ick. If we have a menu item it has to match the
				 * debconf choice, otherwise we configure all of
				 * the providing packages */
				switch (di_config_package(dep, satisfy_virtual)) {
					case -1:
						return -1;
					case EXIT_BACKUP:
						return EXIT_BACKUP;
				}
				if (is_menu_item)
					break;
			}
		}
	}
	free(menu);
	free(s);
	/* It doesn't make sense to configure virtual packages,
	 * since they are, well, virtual. */
	p->p.status = di_package_status_installed;
	return 1;
}

static void set_package_title(di_system_package *p) {
	char *title;

	if (!p->installer_menu_item)
		return;
	asprintf(&title, "debian-installer/%s/title", p->p.package);
	if (debconf_settitle(debconf, title))
		di_log(DI_LOG_LEVEL_WARNING, "Unable to set title for %s.", p->p.package);
	free(title);
}

static int do_menu_item(di_system_package *p) {
	di_log(DI_LOG_LEVEL_INFO, "Menu item '%s' selected", p->p.package);

	return di_config_package(p, satisfy_virtual);
}

static char *debconf_priorities[] =
  {
    "low",
    "medium",
    "high",
    "critical"
  };

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
static void modify_debconf_priority (int raise_or_lower) {
	int pri, i;
	static int menu_pri = -1;
	const char *template = "debconf/priority";
	debconf_get(debconf, template);
	
	if (menu_pri == -1) {
		for (i = 0; (size_t)i < ARRAY_SIZE(debconf_priorities); ++i) {
			if (0 == strcmp(MENU_PRIORITY,
					debconf_priorities[i]) ) {
				menu_pri = i;
				break;
			}
		}
	}

	pri = 1;
	if ( debconf->value ) {
		for (i = 0; (size_t)i < ARRAY_SIZE(debconf_priorities); ++i) {
			if (0 == strcmp(debconf->value,
					debconf_priorities[i]) ) {
				pri = i;
				break;
			}
		}
	}
	if (raise_or_lower == LOWER) {
		--pri;
		/* Make sure the menu is always displayed after a single
		 * backing up.
		 */
		if (menu_pri != -1 && pri > menu_pri)
			pri = menu_pri;
	}
	else if (raise_or_lower == RAISE)
		++pri;
	if (0 > pri)
		pri = 0;
	if (pri > default_priority)
		pri = default_priority;
	if (local_priority != pri) {
		if (local_priority > -1)
			di_log(DI_LOG_LEVEL_INFO, "Modifying debconf priority limit from '%s' to '%s'",
				debconf->value ? debconf->value : "(null)",
				debconf_priorities[pri] ? debconf_priorities[pri] : "(null)");
		local_priority = pri;
	    
		debconf_set(debconf, template, debconf_priorities[pri]);
	}
}
	
static void adjust_default_priority (void) {
	int pri, i;
	const char *template = "debconf/priority";
	debconf_get(debconf,  template);

	pri = 1;
	if ( debconf->value ) {
		for (i = 0; (size_t)i < ARRAY_SIZE(debconf_priorities); ++i) {
			if (0 == strcmp(debconf->value,
					debconf_priorities[i]) ) {
				pri = i;
				break;
			}
		}
	}
	
	if ( pri != local_priority) {
		if (local_priority > -1)
			di_log(DI_LOG_LEVEL_INFO, "Priority changed externally, setting main-menu default to '%s' (%s)",
				debconf_priorities[pri] ? debconf_priorities[pri] : "(null)", debconf->value);
		local_priority = pri;
		default_priority = pri;
	}
}

void notify_user_of_failure (di_system_package *p) {
	char buf[256];
	
	debconf_capb(debconf);
	menu_entry(debconf, p, buf, sizeof (buf));
	debconf_subst(debconf, ITEM_FAILURE, "ITEM", buf);
	debconf_input(debconf, "critical", ITEM_FAILURE);
	debconf_go(debconf);
	debconf_capb(debconf, "backup");
}

/* Cheap-and-cheerful run-parts-a-like for /lib/main-menu.d. Allows packages
 * to register scripts to be run at main-menu startup that need to share
 * main-menu's debconf frontend but that don't merit a menu item, such as
 * setting an info message.
 */
static void menu_startup (void) {
	struct dirent **namelist;
	int entries, i;

	/* scandir() isn't POSIX, but it makes things easy. */
	entries = scandir(MAIN_MENU_DIR, &namelist, NULL, alphasort);
	if (entries < 0)
		return;

	for (i = 0; i < entries; ++i) {
		size_t len;
		char *filename;
		struct stat st;
		int ret;

		if (strcmp(namelist[i]->d_name, ".") == 0 || strcmp(namelist[i]->d_name, "..") == 0)
			continue;

		/* sizeof(MAIN_MENU_DIR) includes trailing \0 */
		len = sizeof(MAIN_MENU_DIR) + 1 + strlen(namelist[i]->d_name);
		filename = di_new(char, len);
		snprintf(filename, len, "%s/%s", MAIN_MENU_DIR, namelist[i]->d_name);

		if (stat(filename, &st) != 0) {
			di_log(DI_LOG_LEVEL_WARNING, "Can't stat %s (%s)", filename, strerror(errno));
			di_free(filename);
			continue;
		}
		if (!S_ISREG(st.st_mode)) {
			di_log(DI_LOG_LEVEL_WARNING, "%s is not a regular file", filename);
			di_free(filename);
			continue;
		}
		if (access(filename, X_OK) != 0) {
			di_log(DI_LOG_LEVEL_WARNING, "%s is not executable", filename);
			di_free(filename);
			continue;
		}

		//di_log(DI_LOG_LEVEL_DEBUG, "Executing %s", filename);
		ret = system(filename);
		if (ret != 0)
			di_log(DI_LOG_LEVEL_WARNING, "%s exited with status %d", filename, ret);

		di_free(filename);
	}
}

int main (int argc __attribute__ ((unused)), char **argv) {
	di_system_package *p;
	di_packages *packages;
	di_packages_allocator *allocator;
	int ret;

	debconf = debconfclient_new();
	di_system_init(basename(argv[0]));
	
	/* Tell udpkg to shut up. */
	setenv("UDPKG_QUIET", "y", 1);

	/* Make cdebconf honour currently set language */
	const char *template = "debconf/language";
	if (debconf_get(debconf, template) == CMD_SUCCESS && 
	    debconf->value && *debconf->value)
		debconf_set(debconf, template, debconf->value);

	menu_startup();

	allocator = di_system_packages_allocator_alloc ();
	packages = di_system_packages_status_read_file(DI_SYSTEM_DPKG_STATUSFILE, allocator);
	while ((p=show_main_menu(packages, allocator))) {
		ret = do_menu_item(p);
		adjust_default_priority();
		switch (ret) {
			case EXIT_OK:
				/* Success */
				if (p->installer_menu_item < NEVERDEFAULT) {
					last_successful_item = p->installer_menu_item;
					modify_debconf_priority(RAISE);
					//di_log(DI_LOG_LEVEL_DEBUG, "Installed package '%s', raising last_successful_item to %d", p->p.package, p->installer_menu_item);
				}
				else {
					// di_log(DI_LOG_LEVEL_DEBUG, "Installed package '%s' but no raise since %d >= %i", p->p.package, p->installer_menu_item, NEVERDEFAULT);
				}
				break;
			case EXIT_BACKUP:
				di_log(DI_LOG_LEVEL_INFO, "Menu item '%s' succeeded but requested to be left unconfigured.", p->p.package); 
				modify_debconf_priority(LOWER);
				break;
			default:
				di_log(DI_LOG_LEVEL_WARNING, "Menu item '%s' failed.", p->p.package);
				notify_user_of_failure(p);
				modify_debconf_priority(LOWER);
		}
		
		di_packages_free (packages);
		di_packages_allocator_free (allocator);
		allocator = di_system_packages_allocator_alloc ();
		packages = di_system_packages_status_read_file(DI_SYSTEM_DPKG_STATUSFILE, allocator);

		/* tell cdebconf to save the database */
		debconf->command(debconf, "X_SAVE", NULL);
	}
	
	return EXIT_FAILURE;
}

/*
 * Configure all dependencies, special case for virtual packages.
 * This is done depth-first.
 */
static int di_config_package(di_system_package *p,
			     int (*virtfunc)(di_system_package *)) {
	char *configcommand;
	int ret;
	di_slist_node *node;
	di_system_package *dep;

	//di_log(DI_LOG_LEVEL_DEBUG, "configure %s, status: %d\n", p->p.package, p->p.status);

	if (p->p.type == di_package_type_virtual_package) {
		//di_log(DI_LOG_LEVEL_DEBUG, "virtual package %s\n", p->p.package);
		if (virtfunc)
			return virtfunc(p);
		else
			return -1;
	}
	else if (p->p.type == di_package_type_non_existent)
		return 0;

	for (node = p->p.depends.head; node; node = node->next) {
		di_package_dependency *d = node->data;
		dep = (di_system_package *)d->ptr;
		if (dep->p.status == di_package_status_installed)
			continue;
		if (d->type != di_package_dependency_type_depends)
			continue;
		/* Recursively configure this package */
		switch (di_config_package(dep, virtfunc)) {
			case -1:
				return -1;
			case EXIT_BACKUP:
				return EXIT_BACKUP;
		}
	}

	set_package_title(p);
	if (asprintf(&configcommand, "exec udpkg --configure --force-configure %s", p->p.package) == -1)
		return -1;

	ret = di_exec_shell_log(configcommand);
	ret = di_exec_mangle_status(ret);
	free(configcommand);
	switch (ret) {
		case EXIT_OK:
			p->p.status = di_package_status_installed;
			break;
		default:
			di_log(DI_LOG_LEVEL_WARNING, "Configuring '%s' failed with error code %d", p->p.package, ret);
			ret = -1;
		case EXIT_BACKUP:
			p->p.status = di_package_status_half_configured;
			break;
	}
	return ret;
}

/* vim: noexpandtab sw=8
 */
