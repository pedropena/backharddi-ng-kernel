
#ifndef _CDEBCONF_NEWT_H_
#define _CDEBCONF_NEWT_H_

/*  Horizontal offset between buttons and text box */
#define TEXT_PADDING 1
/*  Horizontal offset between text box and borders */
#define BUTTON_PADDING 4

#define cdebconf_newt_create_form(scrollbar)          newtForm((scrollbar), NULL, 0)

void cdebconf_newt_create_window(const int width, const int height, const char *title, const char *priority);

int
cdebconf_newt_get_text_height(const char *text, int win_width);

int
cdebconf_newt_get_text_width(const char *text);

#endif /* _CDEBCONF_NEWT_H_ */
