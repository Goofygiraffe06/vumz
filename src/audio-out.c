#include "audio-out.h"
#include <ncurses.h>
#include <stdlib.h>

#define VUMETER_GREEN_THRESHOLD_DB -20.0f
#define VUMETER_YELLOW_THRESHOLD_DB -6.0f

// 100%, 87.5%, 75%, 62.5%, 50%, 37.5%, 25%, 12.5%, 0%
static char* fill_percentage[8] = {"█", "▇", "▆", "▅", "▄", "▃", "▂", "▁"};

static double db_to_vu_height(float db, int vu_height)
{
    // height = 39 when my terminal is using the whole height
    // db = 0 to test
    if (db <= -60.0f)
    {
        db = -60.0f;
    }

    if (db >= 0.0f)
    {
        db = 0.0f;
    }

    return (double)(((db + 60.0f) / 60.0f) * vu_height);
}

static int get_fill_percentage_index(double percentage) {
    int fill_index;
    if (percentage >= 87.5) {
        // 100 >= x >= 87.5 -> █
        fill_index = 0;

    } else if (percentage >= 75) {
        // 87.5 > x >= 75 -> ▇
        fill_index = 1;

    } else if (percentage >= 62.5) {
        // 75 > x >= 62.5 -> ▆
        fill_index = 2;

    } else if (percentage >= 50) {
        // 62.5 > x >= 50 -> ▅
        fill_index = 3;

    } else if (percentage >= 37.5) {
        // 50 > x >= 37.5 -> ▄
        fill_index = 4;

    } else if (percentage >= 25) {
        // 37.5 > x >= 25 -> ▃
        fill_index = 5;

    } else if (percentage >= 12.5) {
        // 25 > x >= 12.5 -> ▂
        fill_index = 6;
    } else {
        // 12.5 > x > 0 -> ▁
        fill_index = 7;
    }

    return fill_index;
}

double get_fill_percentage(double input) {
    // Calculate the fractional part of a number (e,g,. 8.623 -> 62.3)
    double percentage = (input - (int)input) * 100;

    return percentage;
}

void init_ncurses()
{
    initscr();              /* Start curses mode */
    raw();                  /* Line buffering disabled */
    keypad(stdscr, TRUE);   /* Enable keypad */
    noecho();               /* Don't echo user input */
    curs_set(0);            /* Make the cursor invisible */
    timeout(0);             /* Non blocking mode */
    cbreak();               /* Don't wait for new lines to read input */
    start_color();          /* Start color */
    nodelay(stdscr, TRUE);  /* Set getch() to be non-blocking */

    if (has_colors())
    {
        use_default_colors(); // Allow using the default terminal colors
        init_pair(1, COLOR_GREEN, -1); // First color pair, this will be used for the lower volume levels
        init_pair(2, COLOR_YELLOW, -1); // Second color pair, this will be used for the medium volume levels
        init_pair(3, COLOR_RED, -1); // Third color pair, this will be used for the higher volume levels
        init_pair(4, COLOR_MAGENTA, -1); // Fourth color pair, this will be used for the debug window
        init_pair(5, COLOR_BLUE, -1); // Fifth color pair, this will be used for the debug window
    }

    refresh(); // Refresh the screen
}

//FIX: Updat this with the new buffer
void draw_vumeter_data(const float* audio_out_buffer, float* audio_out_buffer_prev, int n_channels) {

    // -- Get terminal dimmensions and calculate color threshold levels --
    int terminal_height, terminal_width;
    getmaxyx(stdscr, terminal_height, terminal_width);
    int green_threshold_height = db_to_vu_height(VUMETER_GREEN_THRESHOLD_DB, terminal_height);
    int yellow_threshold_height = db_to_vu_height(VUMETER_YELLOW_THRESHOLD_DB, terminal_height);

    // -- Calculate starting positions and other shenanigans --
    int vu_bar_width = 4;
    int startx_left = (terminal_width / 4) - (vu_bar_width / 2) + 7;
    int startx_right = (3 * terminal_width / 4) - (vu_bar_width / 2) - 7;

    // -- Get the current buffer data and split it into left and right channels --
    // Current left channel data
    float left_channel_dbs = audio_out_buffer[0];
    double current_left_height = db_to_vu_height(left_channel_dbs, terminal_height);
    double left_percentage = get_fill_percentage(current_left_height);
    // Current right channel data
    float right_channel_dbs = audio_out_buffer[1];
    double current_right_height = db_to_vu_height(right_channel_dbs, terminal_height);
    double right_percentage = get_fill_percentage(current_right_height);

    // -- Get the previous buffer data and split it into left and right channels --
    // Previous left channel data
    float prev_left_channel_dbs = audio_out_buffer_prev[0];
    // Previous right channel data
    float prev_right_channel_dbs = audio_out_buffer_prev[1];

    // Main render
    for (int i = 0; i < terminal_height; i++)
    {
        // Apply color according to db
        if (terminal_height - i < green_threshold_height) {
            //green
            attron(COLOR_PAIR(1));
        }
        else if (terminal_height - i < yellow_threshold_height) {
            // yellow
            attron(COLOR_PAIR(2));
        }
        else {
            // red
            attron(COLOR_PAIR(3));
        }

        // Render left channel
        if (terminal_height - i <= current_left_height) {
            // Draw a colored block;
            for (int j = 0; j < vu_bar_width; j++)
            {
                // check how tall the block should be
                mvprintw(0 + i, startx_left + j, "█");
            }
        }
        else if (terminal_height - i > current_left_height && terminal_height - i < current_left_height + 1) {
            // If it is the top block
            // Check the percentage
            int fill_index = get_fill_percentage_index(left_percentage);

            for (int j = 0; j < vu_bar_width; j++)
            {
                mvprintw(0 + i, startx_left + j, "%s", fill_percentage[fill_index]);
            }
        }
        else {
            // If it's none of the above
            for (int j = 0; j < vu_bar_width; j++)
            {
                mvprintw(0 + i, startx_left + j, " ");
            }
        }

        // Render right channel
        if (terminal_height - i <= current_right_height) {
            // Draw a colored block;
            for (int j = 0; j < vu_bar_width; j++)
            {
                // check how tall the block should be
                mvprintw(0 + i, startx_right + j, "█");
            }

        }
        else if (terminal_height - i > current_right_height && terminal_height - i < current_right_height + 1) {
            // If it is the top block
            // Check the percentage
            int fill_index = get_fill_percentage_index(right_percentage);

            for (int j = 0; j < vu_bar_width; j++)
            {
                mvprintw(0 + i, startx_right + j, "%s", fill_percentage[fill_index]);
            }
        }
        else {
            // If it's none of the above
            for (int j = 0; j < vu_bar_width; j++)
            {
                mvprintw(0 + i, startx_right + j, " ");
            }
        }

        // Deactivate color attributes
        attroff(A_COLOR);
    }

    // Print to debug
    mvprintw(terminal_height - 2, 0, "Current Left: %.2f | Previous Left: %.2f", audio_out_buffer[0], audio_out_buffer_prev[0]);
    mvprintw(terminal_height - 3, 0, "Current Right: %.2f | Previous Right: %.2f", audio_out_buffer[1], audio_out_buffer_prev[1]);
    // -- Copy the current buffer data to the previous buffer data --
    audio_out_buffer_prev[0] = audio_out_buffer[0]; // Left
    audio_out_buffer_prev[1] = audio_out_buffer[1]; // Right
    refresh();
}

void cleanup_ncurses()
{
    endwin();
    system("clear");
}
