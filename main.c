#include <stdio.h>
#include <stdlib.h>
#include "display.h"

static void
popup(int key)
{
    font_t font = {COLOR_YELLOW, COLOR_BLACK, true};
    panel_t popup;
    panel_init(&popup, 10, 10, 20, 5);
    panel_fill(&popup, font, ' ');
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Unknown: %d", key);
    panel_puts(&popup, 1, 1, font, buffer);
    display_push(&popup);
    display_getch();
    display_pop();
    panel_free(&popup);
    display_refresh();
}

int
main(void)
{
    int w, h;
    device_size(&w, &h);
    if (w < DISPLAY_WIDTH || h < DISPLAY_HEIGHT) {
        printf("Goblin-COM requires a terminal of at least %dx%d characters!\n"
               "Press enter to exit ...", DISPLAY_WIDTH, DISPLAY_HEIGHT);
        getchar();
        exit(EXIT_FAILURE);
    }
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

    /* Main Loop */
    font_t player_font = {COLOR_WHITE, COLOR_BLACK, true};
    char px = (DISPLAY_WIDTH - info_width) / 2;
    char py = DISPLAY_HEIGHT / 2;
    bool running = true;
    int fps = 5;
    while (running) {
        uint64_t uepoch = device_uepoch();
        panel_fill(&world, water_font, ".~"[(uepoch / (1000000 / fps)) % 2]);
        panel_putc(&world, px, py, player_font, '@');
        display_refresh();
        if (device_kbhit(1000000 / fps)) {
            int key = device_getch();
            switch (key) {
            case ARROW_U:
                py--;
                break;
            case ARROW_D:
                py++;
                break;
            case ARROW_L:
                px--;
                break;
            case ARROW_R:
                px++;
                break;
            case 'q':
            case 3:
                running = false;
                break;
            default:
                popup(key);
                break;
            }
        }
    };

    display_pop(); // info
    display_pop(); // world
    panel_free(&info);
    panel_free(&world);
    display_free();
    return 0;
}
