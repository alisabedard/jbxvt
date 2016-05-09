#include "scr_tab.h"

#include "cursor.h"
#include "global.h"
#include "screen.h"
#include "selection.h"

//  Tab to the next tab_stop.
void scr_tab(void)
{
	home_screen();
	if (screen->col == cwidth - 1)
		return;
	cursor();
	check_selection(screen->row,screen->row);
	if (screen->text[screen->row][screen->col] == 0)
		screen->text[screen->row][screen->col] = '\t';
	screen->col++;
	while (screen->col % 8 != 0 && screen->col < cwidth - 1)
		screen->col++;
	cursor();
}


