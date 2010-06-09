#ifdef DODEBUG
#include <assert.h>
#define ASSERT(x) assert(x)
#define DPRINTF(fmt,args...) fprintf(stderr, fmt, ##args)
#else
#define ASSERT(x) /* nothing */
#define DPRINTF(fmt,args...) /* nothing */
#endif

#define BUFSIZE		4096

#define MAIN_MENU	"debian-installer/main-menu"
#define MISSING_PROVIDE "debian-installer/missing-provide"
#define ITEM_FAILURE	"debian-installer/main-menu/item-failure"
#define MAIN_MENU_DIR	"/lib/main-menu.d"

#include <debian-installer.h>

#define NEVERDEFAULT 900

enum
{
	EXIT_OK			= 0,
	EXIT_BACKUP		= 10,
};

/* Priority at which the menu is displayed */
#define MENU_PRIORITY	"medium"

/* vim: noexpandtab sw=8
 */
