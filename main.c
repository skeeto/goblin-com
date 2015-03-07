#include <stdio.h>
#include "display.h"

static void
popup(void)
{
    font_t font = {COLOR_YELLOW, COLOR_BLACK, true};
    panel_t popup;
    panel_init(&popup, 10, 10, 20, 5);
    panel_fill(&popup, font, ' ');
    panel_puts(&popup, 1, 1, font, "Cool beans!");
    display_push(&popup);
    display_getch();
    display_pop();
    panel_free(&popup);
    display_refresh();
}

int
main(void)
{
    display_init();
    device_title("Goblin-COM");

    int info_width = 20;
    font_t info_font = FONT_DEFAULT;
    font_t water_font = {COLOR_WHITE, COLOR_BLUE, false};

    panel_t world;
    panel_init(&world, 0, 0, DISPLAY_WIDTH - info_width, DISPLAY_HEIGHT);
    panel_fill(&world, water_font, '~');
    display_push(&world);

    panel_t info;
    panel_init(&info, DISPLAY_WIDTH - info_width, 0,
               info_width, DISPLAY_HEIGHT);
    panel_fill(&info, info_font, ' ');
    panel_border(&info, info_font);
    panel_puts(&info, 5, 1, info_font, "Goblin-COM");
    display_push(&info);

    int type = 0;
    do {
        panel_fill(&world, water_font, ".~"[type++ % 2]);
        display_refresh();
    } while (!device_kbhit(1000000 / 5));
    while (device_kbhit(0))
        device_getch();
    popup();

    display_pop(); // info
    display_pop(); // world
    panel_free(&info);
    panel_free(&world);
    display_free();
    return 0;
}
