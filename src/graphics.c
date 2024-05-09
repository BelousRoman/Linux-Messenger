#include "../hdr/graphics.h"

// #define SUBWND_MIN_H        (elem_h*4)+2+(GFX_ELEM_VOFF*7)
#define SUBWND_MIN_H        (elem_h*6)+4+(GFX_ELEM_VOFF*9)+header_h

// #define SUBWND_MIN_W        ((elem_w/2)+panel_w+2+(GFX_ELEM_HOFF*3))*2
#define SUBWND_MIN_W        (elem_w+2+PANEL_MIN_W+(GFX_ELEM_HOFF*5)) > header_w ? \
                            (elem_w+2+PANEL_MIN_W+(GFX_ELEM_HOFF*5)) : header_w

enum cur_wnd_enum
{
    WND_NONE = 0,
    WND_MAIN_MENU,
    WND_JOIN_SRV,
    WND_CREATE_SRV,
    WND_PREFS
};

/* Global variable, used to store terminal's size in columns and rows */
struct winsize size;

int cur_wnd = WND_NONE;
WINDOW *status_subwnd = NULL;
WINDOW *note_subwnd = NULL;

int wnd_h;
int wnd_w;

int elem_h;
int elem_w;

int panel_h;
int panel_w;

int header_h;
int header_w;

int popup_wnd_h;
int popup_wnd_w;

char *join_note_label = NULL;
char *prefs_note_label = NULL;

int join_note_size = 0;
int prefs_note_size = 0;

/* Function to process signal, called on resizing terminal window */
void sig_winch(int signo)
{
	ioctl(fileno(stdout), TIOCGWINSZ, (char *) &size);
	resizeterm(size.ws_row, size.ws_col);
}

/* Function to handle message when new message in queue occurs */
void handle_msg(union sigval sv)
{
    /*
    * Declare:
    * - sev - sigevent, to repeatedly recall mq_notify;
    * - prio - prioirity of received message;
    * - ret - return value.
    */
	mqd_t *mqd = ((mqd_t *) sv.sival_ptr);
    struct sigevent sev;
    int msg;
    ssize_t ret;
    /*
    * Configure sigevent, set notification method, function called, attributes
    * and pointer to q_handler.
    * */
	sev.sigev_notify = SIGEV_THREAD;
	sev.sigev_notify_function = handle_msg;
	sev.sigev_notify_attributes = NULL;
	sev.sigev_value.sival_ptr = mqd;

    /* Read queue until an error occurs */
	while (ret = mq_receive(*mqd, &msg, sizeof(int),
                        NULL) > 0)
	{
		switch (msg)
        {
        case CONNECT_COMM:
            if (cur_wnd == WND_MAIN_MENU)
            {
                wclear(status_subwnd);
                wmove(status_subwnd, 0, 0);
                switch (connection_flag)
                {
                case STATUS_DISCONNECTED:
                    wattron(status_subwnd, COLOR_PAIR(5));
                    waddch(status_subwnd, ACS_DIAMOND);
                    wattroff(status_subwnd, COLOR_PAIR(5));
                    wprintw(status_subwnd, " Status: ");
                    wattron(status_subwnd, COLOR_PAIR(5));
                    wprintw(status_subwnd, "Connected");
                    wattroff(status_subwnd, COLOR_PAIR(5));

                    wmove(note_subwnd, 0, ((wnd_w-4-(GFX_ELEM_HOFF*2))/2)-strlen(MENU_SCR_NOTE_DISCONNECTED)/2);
                    wprintw(note_subwnd, MENU_SCR_NOTE_DISCONNECTED);
                    break;
                case STATUS_CONNECTED:
                    wattron(status_subwnd, COLOR_PAIR(4));
                    waddch(status_subwnd, ACS_DIAMOND);
                    wattroff(status_subwnd, COLOR_PAIR(4));
                    wprintw(status_subwnd, " Status: ");
                    wattron(status_subwnd, COLOR_PAIR(4));
                    wprintw(status_subwnd, "Disconnected");
                    wattroff(status_subwnd, COLOR_PAIR(4));

                    wmove(note_subwnd, 0, ((wnd_w-4-(GFX_ELEM_HOFF*2))/2)-strlen(MENU_SCR_NOTE_CONNECTED)/2);
                    wprintw(note_subwnd, MENU_SCR_NOTE_CONNECTED);
                    break;
                case STATUS_CONNECTING:
                    wattron(status_subwnd, COLOR_PAIR(3));
                    waddch(status_subwnd, ACS_DIAMOND);
                    wattroff(status_subwnd, COLOR_PAIR(3));
                    wprintw(status_subwnd, " Status: ");
                    wattron(status_subwnd, A_BLINK | COLOR_PAIR(3));
                    wprintw(status_subwnd, "Connecting");
                    wattroff(status_subwnd, A_BLINK | COLOR_PAIR(3));

                    wmove(note_subwnd, 0, ((wnd_w-4-(GFX_ELEM_HOFF*2))/2)-strlen(MENU_SCR_NOTE_DISCONNECTED)/2);
                    wprintw(note_subwnd, MENU_SCR_NOTE_DISCONNECTED);
                    break;
                default:
                    break;
                }
                
            }
            break;
        default:
            break;
        }
	}
    if (ret == -1)
    {
		if (errno != EAGAIN)
		{
			perror("handle_msg mq_receive");
			exit(EXIT_FAILURE);
		}
	}

    /* Register mq_notify */
	while (ret = mq_notify(*mqd, &sev) == -1)
	{
		perror("mq_notify");
		exit(EXIT_FAILURE);
	}
	return EXIT_SUCCESS;
}

int _set_dimensions()
{
    int ret = 0;

    ioctl(fileno(stdout), TIOCGWINSZ, (char *) &size);

    elem_h = ELEM_SET_H > ELEM_MIN_H ? ELEM_SET_H : ELEM_MIN_H;
    elem_w = ELEM_SET_W > ELEM_MIN_W ? ELEM_SET_W : ELEM_MIN_W;

    header_h = WLCM_WND_H_EN;
    header_w = WLCM_WND_W_EN;

    popup_wnd_h = (elem_h*2)+2;
    popup_wnd_w = (elem_w*2)+2+(GFX_ELEM_HOFF*3);

    wnd_h = SUBWND_SET_H > SUBWND_MIN_H ? SUBWND_SET_H : SUBWND_MIN_H;
    wnd_w = SUBWND_SET_W > SUBWND_MIN_W ? SUBWND_SET_W : SUBWND_MIN_W;

    panel_h = wnd_h-2-(header_h+elem_h+GFX_ELEM_VOFF*2);
    panel_w = wnd_w-2-(elem_w+2+GFX_ELEM_HOFF*5);

    if (wnd_h > size.ws_row || wnd_w > size.ws_col)
        ret = 1;

    return ret;
}

int _set_labels()
{
    char * tmp = NULL;
    char *tmp_label = NULL;
    int index;
    int sec_index;
    int thrd_index;

    tmp_label = (char *)malloc(strlen(PREFS_SCR_NOTE)*sizeof(char)+1);
    if (tmp_label != NULL)
    {
        strncpy(tmp_label, PREFS_SCR_NOTE, strlen(PREFS_SCR_NOTE));
        for (index = 0; index < strlen(PREFS_SCR_NOTE); index++)
        {
            if (tmp_label[index] == '\t')
            {
                for (sec_index = 0; sec_index < sizeof('\t'); sec_index++)
                {
                    prefs_note_size++;
                }
            }
            else
                prefs_note_size++;
        }
        
        prefs_note_label = malloc(prefs_note_size+1);
        if (prefs_note_label != NULL)
        {
            // prefs_note_size
            for (index = 0, sec_index = 0; sec_index < strlen(PREFS_SCR_NOTE); sec_index++)
            {
                if (tmp_label[sec_index] != '\t')
                {
                    prefs_note_label[index] = tmp_label[sec_index];
                    index++;
                }
                else
                {
                    for (thrd_index = 0; thrd_index < sizeof('\t'); ++thrd_index)
                    {
                        prefs_note_label[index] = '\x20';
                        index++;
                    }
                    
                }

                // For some reason commenting this line will cause "malloc(): corrupted top size" error
                // printf(" ");
            }
        }
        // free(tmp_label);
        
        // printf(" ");
    }

    tmp = (char *)realloc(tmp_label, strlen(JOIN_SCR_NOTE)*sizeof(char)+1);
    if (tmp != NULL)
    {
        tmp_label = tmp;
        strncpy(tmp_label, JOIN_SCR_NOTE, strlen(JOIN_SCR_NOTE));
        for (index = 0; index < strlen(JOIN_SCR_NOTE); index++)
        {
            if (tmp_label[index] == '\t')
            {
                for (sec_index = 0; sec_index < sizeof('\t'); sec_index++)
                {
                    join_note_size++;
                }
            }
            else
                join_note_size++;
        }
        
        join_note_label = malloc(join_note_size+1);
        if (join_note_label != NULL)
        {
            // join_note_size
            for (index = 0, sec_index = 0; sec_index < strlen(JOIN_SCR_NOTE); sec_index++)
            {
                if (tmp_label[sec_index] != '\t')
                {
                    join_note_label[index] = tmp_label[sec_index];
                    index++;
                }
                else
                {
                    for (thrd_index = 0; thrd_index < sizeof('\t'); ++thrd_index)
                    {
                        join_note_label[index] = '\x20';
                        index++;
                    }
                    
                }

                // For some reason commenting this line will cause "malloc(): corrupted top size" error
                // printf(" ");
            }
        }
        free(tmp_label);
    }

    return 0;
}

void init_graphics()
{
    _set_dimensions();

    _set_labels();

    initscr(); /* Init ncurses lib */
    fflush(stdout);
	signal(SIGWINCH, sig_winch); /* Set signal handler for resizing terminal window */
	ioctl(fileno(stdout), TIOCGWINSZ, (char *) &size); /* Get current size of a terminal */

	cbreak(); /* */

	noecho(); /* Disable write of user input on a screen */
	curs_set(0); /* Set cursor invisible */
	start_color(); /* */
	init_pair(1, COLOR_WHITE, COLOR_BLACK); /* Init color pair for a usual color palette */
	init_pair(2, COLOR_BLACK, COLOR_WHITE); /* Init color pair for a 'selected' color palette */
    init_pair(3, COLOR_YELLOW, COLOR_BLACK); /* Init color pair for a 'selected' color palette */
    init_pair(4, COLOR_RED, COLOR_BLACK); /* Init color pair for a 'selected' color palette */
    init_pair(5, COLOR_GREEN, COLOR_BLACK); /* Init color pair for a 'selected' color palette */
    
    //init_pair(3, COLOR_BLUE, COLOR_MAGENTA); /* Init color pair for a 'selected' color palette */
	bkgd(COLOR_PAIR(1)); /* Set background color to usual color palette */
	refresh(); /* Update stdscr */
}

void deinit_graphics()
{
    endwin();

    if (prefs_note_label != NULL)
    {
        free(prefs_note_label);
    }
    if (join_note_label != NULL)
    {
        free(join_note_label);
    }
}

int wait_wnd(char *str, int type)
{
    WINDOW *wnd;
    WINDOW *subwnd;

    int main_wnd_w;
    int main_wnd_h;
    int subwnd_w;
    int subwnd_h;
    int str_len = 0;
    int ret = EXIT_SUCCESS;

    if (str != NULL)
    {
        str_len = strlen(str);
    }
    else
    {
        return EXIT_FAILURE;
    }

    subwnd_w = str_len;
    subwnd_h = 3;
    if (subwnd_w > (size.ws_col-12))
    {
        int tmp = (size.ws_col-12) / subwnd_w;
        subwnd_w /= tmp;
        subwnd_h += tmp;
    }

    main_wnd_w = subwnd_w + 12;
    main_wnd_h = subwnd_h + 2;

    wnd = newwin(main_wnd_h, main_wnd_w, (size.ws_row/2)-(main_wnd_h/2), (size.ws_col/2)-(main_wnd_w/2));
    subwnd = derwin(wnd, subwnd_h, subwnd_w, 1, 6);

    box(wnd, ACS_VLINE, ACS_HLINE);

    wprintw(subwnd, "%s", str);

    wmove(subwnd, subwnd_h-1, (subwnd_w/2)-1);
    wattron(subwnd, A_BLINK);
    waddch(subwnd, ACS_DIAMOND);
    wattroff(subwnd, A_BLINK);

    wrefresh(subwnd);
    wgetch(wnd);

    delwin(subwnd);
    delwin(wnd);

    return ret;
}

int menu_wnd(int *option)
{
    WINDOW *subwnd;
    WINDOW *header_wnd;
    WINDOW *status_wnd;
    WINDOW *btns_border;
    WINDOW *btns[4];
    WINDOW *note_wnd;
    WINDOW *panel_wnd;

    char *labels[4];
    int symbol;
    int index;
    int selection = *option-1;
    int ret = 0;

    cur_wnd = WND_MAIN_MENU;

    int border_h = (elem_h*4)+2+(GFX_ELEM_VOFF*5);
    int border_w = elem_w+2+(GFX_ELEM_HOFF*2);

    labels[0] = malloc(strlen(MENU_SCR_CLT_BTN_LABEL)+1);
    labels[1] = malloc(strlen(MENU_SCR_SRV_BTN_LABEL)+1);
    labels[2] = malloc(strlen(MENU_SCR_CFG_BTN_LABEL)+1);
    labels[3] = malloc(strlen(MENU_SCR_QUIT_BTN_LABEL)+1);

    strcpy(labels[0], MENU_SCR_CLT_BTN_LABEL);
    strcpy(labels[1], MENU_SCR_SRV_BTN_LABEL);
    strcpy(labels[2], MENU_SCR_CFG_BTN_LABEL);
    strcpy(labels[3], MENU_SCR_QUIT_BTN_LABEL);
// stdscr
	/*
	* Create wnd, l_wnd and r_wnd, create borders, add line between main window
	* and cwd window.
	*/
	subwnd = newwin(wnd_h, wnd_w, (size.ws_row/2)-(wnd_h/2), (size.ws_col/2)-(wnd_w/2));
    header_wnd = derwin(subwnd, header_h, header_w, 1, 1);
    status_wnd = derwin(subwnd, elem_h, border_w, wnd_h-(1+border_h+(elem_h*2)+(GFX_ELEM_VOFF*2)), 1+GFX_ELEM_HOFF);
    status_subwnd = derwin(status_wnd, 1, border_w-2, 1, 1);
	btns_border = derwin(subwnd, border_h, border_w, wnd_h-1-border_h-elem_h-(GFX_ELEM_VOFF*2), 1+GFX_ELEM_HOFF);
    btns[0] = derwin(btns_border, elem_h, elem_w, 1+GFX_ELEM_VOFF, 1+GFX_ELEM_HOFF);
    btns[1] = derwin(btns_border, elem_h, elem_w, 1+elem_h+(GFX_ELEM_VOFF*2), 1+GFX_ELEM_HOFF);
    btns[2] = derwin(btns_border, elem_h, elem_w, 1+(elem_h*2)+(GFX_ELEM_VOFF*3), 1+GFX_ELEM_HOFF);
    btns[3] = derwin(btns_border, elem_h, elem_w, 1+(elem_h*3)+(GFX_ELEM_VOFF*4), 1+GFX_ELEM_HOFF);
    note_wnd = derwin(subwnd, elem_h, wnd_w-2-(GFX_ELEM_HOFF*2), wnd_h-1-elem_h, 1+GFX_ELEM_HOFF);
    note_subwnd = derwin(note_wnd, 1, wnd_w-4-(GFX_ELEM_HOFF*2), 1, 1);
    panel_wnd = derwin(subwnd, panel_h, panel_w, 1+header_h+GFX_ELEM_VOFF, 1+border_w+(GFX_ELEM_HOFF*2));

    //box(stdscr, NULL, NULL);
    box(subwnd, ACS_VLINE, ACS_HLINE);
    box(status_wnd, ' ', ' ');
    box(btns_border, ACS_VLINE, ACS_HLINE);
    box(btns[0], ACS_VLINE, ACS_HLINE);
    box(btns[1], ACS_VLINE, ACS_HLINE);
    box(btns[2], ACS_VLINE, ACS_HLINE);
    box(btns[3], ACS_VLINE, ACS_HLINE);
    box(note_wnd, ' ', ' ');

    wmove(subwnd, 0, (wnd_w/2)-strlen(MENU_SCR_LABEL)/2);
    wmove(btns[0], 1, (elem_w/2)-strlen(labels[0])/2);
    wmove(btns[1], 1, (elem_w/2)-strlen(labels[1])/2);
    wmove(btns[2], 1, (elem_w/2)-strlen(labels[2])/2);
    wmove(btns[3], 1, (elem_w/2)-strlen(labels[3])/2);

    wprintw(subwnd, MENU_SCR_LABEL);
    wprintw(header_wnd,
            "__          __  _\n" \
            "\\ \\        / / | |\n" \
            " \\ \\  /\\  / /__| | ___ ___  _ __ ___   ___ \n" \
            "  \\ \\/  \\/ / _ \\ |/ __/ _ \\| '_ ` _ \\ / _ \\\n" \
            "   \\  /\\  /  __/ | (_| (_) | | | | | |  __/\n" \
            "    \\/  \\/ \\___|_|\\___\\___/|_| |_| |_|\\___|\n");

    switch (connection_flag)
    {
    case STATUS_DISCONNECTED:
        wattron(status_subwnd, COLOR_PAIR(4));
        waddch(status_subwnd, ACS_DIAMOND);
        wattroff(status_subwnd, COLOR_PAIR(4));
        wprintw(status_subwnd, " Status: ");
        wattron(status_subwnd, COLOR_PAIR(4));
        wprintw(status_subwnd, "Disconnected");
        wattroff(status_subwnd, COLOR_PAIR(4));

        wmove(note_subwnd, 0, ((wnd_w-4-(GFX_ELEM_HOFF*2))/2)-strlen(MENU_SCR_NOTE_DISCONNECTED)/2);
        wprintw(note_subwnd, MENU_SCR_NOTE_DISCONNECTED);
        break;
    case STATUS_CONNECTED:
        wattron(status_subwnd, COLOR_PAIR(5));
        waddch(status_subwnd, ACS_DIAMOND);
        wattroff(status_subwnd, COLOR_PAIR(5));
        wprintw(status_subwnd, " Status: ");
        wattron(status_subwnd, COLOR_PAIR(5));
        wprintw(status_subwnd, "Connected");
        wattroff(status_subwnd, COLOR_PAIR(5));

        wmove(note_subwnd, 0, ((wnd_w-4-(GFX_ELEM_HOFF*2))/2)-strlen(MENU_SCR_NOTE_CONNECTED)/2);
        wprintw(note_subwnd, MENU_SCR_NOTE_CONNECTED);
        break;
    case STATUS_CONNECTING:
        wattron(status_subwnd, COLOR_PAIR(3));
        waddch(status_subwnd, ACS_DIAMOND);
        wattroff(status_subwnd, COLOR_PAIR(3));
        wprintw(status_subwnd, " Status: ");
        wattron(status_subwnd, A_BLINK | COLOR_PAIR(3));
        wprintw(status_subwnd, "Connecting");
        wattroff(status_subwnd, A_BLINK | COLOR_PAIR(3));

        wmove(note_subwnd, 0, ((wnd_w-4-(GFX_ELEM_HOFF*2))/2)-strlen(MENU_SCR_NOTE_DISCONNECTED)/2);
        wprintw(note_subwnd, MENU_SCR_NOTE_DISCONNECTED);
        break;
    default:
        break;
    }

    wprintw(panel_wnd, MENU_SCR_NOTE_LABEL);
    // for (int i = 0; i < 43;i++)
    // {
        // waddch(note_wnd, arr[i]);
        // waddch(note_wnd, ' ');
    // }
    // wbkgd(panel_wnd, COLOR_PAIR(2));
    {
    wprintw(panel_wnd, "\n1: ");
    waddch(panel_wnd, ACS_BBSS);
    wprintw(panel_wnd, " 2: ");
    waddch(panel_wnd, ACS_BLOCK);
    wprintw(panel_wnd, " 3: ");
    waddch(panel_wnd, ACS_BOARD);
    wprintw(panel_wnd, " 4: ");
    waddch(panel_wnd, ACS_BSBS);
    wprintw(panel_wnd, " 5: ");
    waddch(panel_wnd, ACS_BSSB);
    wprintw(panel_wnd, "\n6: ");
    waddch(panel_wnd, ACS_BSSS);
    wprintw(panel_wnd, " 7: ");
    waddch(panel_wnd, ACS_BTEE);
    wprintw(panel_wnd, " 8: ");
    waddch(panel_wnd, ACS_BULLET);
    wprintw(panel_wnd, " 9: ");
    waddch(panel_wnd, ACS_CKBOARD);
    wprintw(panel_wnd, " 10: ");
    waddch(panel_wnd, ACS_DARROW);
    wprintw(panel_wnd, "\n11: ");
    waddch(panel_wnd, ACS_DEGREE);
    wprintw(panel_wnd, " 12: ");
    waddch(panel_wnd, ACS_DIAMOND);
    wprintw(panel_wnd, " 13: ");
    waddch(panel_wnd, ACS_GEQUAL);
    wprintw(panel_wnd, " 14: ");
    waddch(panel_wnd, ACS_HLINE);
    wprintw(panel_wnd, " 15: ");
    waddch(panel_wnd, ACS_LANTERN);
    wprintw(panel_wnd, "\n16: ");
    waddch(panel_wnd, ACS_LARROW);
    wprintw(panel_wnd, " 17: ");
    waddch(panel_wnd, ACS_LEQUAL);
    wprintw(panel_wnd, " 18: ");
    waddch(panel_wnd, ACS_LLCORNER);
    wprintw(panel_wnd, " 19: ");
    waddch(panel_wnd, ACS_LRCORNER);
    wprintw(panel_wnd, " 20: ");
    waddch(panel_wnd, ACS_LTEE);
    wprintw(panel_wnd, "\n21: ");
    waddch(panel_wnd, ACS_NEQUAL);
    wprintw(panel_wnd, " 22: ");
    waddch(panel_wnd, ACS_PI);
    wprintw(panel_wnd, " 23: ");
    waddch(panel_wnd, ACS_PLMINUS);
    wprintw(panel_wnd, " 24: ");
    waddch(panel_wnd, ACS_PLUS);
    wprintw(panel_wnd, " 25: ");
    waddch(panel_wnd, ACS_RARROW);
    wprintw(panel_wnd, "\n26: ");
    waddch(panel_wnd, ACS_RTEE);
    wprintw(panel_wnd, " 27: ");
    waddch(panel_wnd, ACS_S1);
    wprintw(panel_wnd, " 28: ");
    waddch(panel_wnd, ACS_S3);
    wprintw(panel_wnd, " 29: ");
    waddch(panel_wnd, ACS_S7);
    wprintw(panel_wnd, " 30: ");
    waddch(panel_wnd, ACS_S9);
    wprintw(panel_wnd, "\n31: ");
    waddch(panel_wnd, ACS_SBBS);
    wprintw(panel_wnd, " 32: ");
    waddch(panel_wnd, ACS_SBSB);
    wprintw(panel_wnd, " 33: ");
    waddch(panel_wnd, ACS_SBSS);
    wprintw(panel_wnd, " 34: ");
    waddch(panel_wnd, ACS_SSBB);
    wprintw(panel_wnd, " 35: ");
    waddch(panel_wnd, ACS_SSBS);
    wprintw(panel_wnd, "\n36: ");
    waddch(panel_wnd, ACS_SSSB);
    wprintw(panel_wnd, " 37: ");
    waddch(panel_wnd, ACS_SSSS);
    wprintw(panel_wnd, " 38: ");
    waddch(panel_wnd, ACS_STERLING);
    wprintw(panel_wnd, " 39: ");
    waddch(panel_wnd, ACS_TTEE);
    wprintw(panel_wnd, " 40: ");
    waddch(panel_wnd, ACS_UARROW);
    wprintw(panel_wnd, "\n41: ");
    waddch(panel_wnd, ACS_ULCORNER);
    wprintw(panel_wnd, " 42: ");
    waddch(panel_wnd, ACS_URCORNER);
    wprintw(panel_wnd, " 43: ");
    waddch(panel_wnd, ACS_VLINE);
    }

                    // for (index = 0; index < 4; ++index)
                    // {
// Writing buttons will go there
                    // }
    for (index = 0; index < 4; ++index)
    {
        if (index == (*option-1))
        {
            wattron(btns[index], A_BOLD | A_UNDERLINE);
            wprintw(btns[index], "%s", labels[index]);
            wattroff(btns[index], A_BOLD | A_UNDERLINE);
        }
        else
            wprintw(btns[index], "%s", labels[index]);
    }

    // wbkgd(panel_wnd, COLOR_PAIR(2));
    // wbkgd(note_subwnd, COLOR_PAIR(2));
    
    refresh();
    wrefresh(subwnd);

    keypad(subwnd, true);
    
	while(ret != -1)
	{
		symbol = wgetch(subwnd);

        if ('\t' == symbol)
		{
            wmove(btns[selection], 1, (elem_w/2)-strlen(labels[selection])/2);
            wprintw(btns[selection], "%s", labels[selection]);
            wrefresh(btns[selection]);

			if (selection == 3)
                selection = 0;
            else
                selection++;

            wattron(btns[selection], A_BOLD | A_UNDERLINE);
            wmove(btns[selection], 1, (elem_w/2)-strlen(labels[selection])/2);
            wprintw(btns[selection], "%s", labels[selection]);
            wattroff(btns[selection], A_BOLD | A_UNDERLINE);
            wrefresh(btns[selection]);
		}
		/* if ARROW_KEY_UP is pressed -> navigate in the directory */
		else if (KEY_UP == symbol)
		{
			if (selection > 0)
            {
                wmove(btns[selection], 1, (elem_w/2)-strlen(labels[selection])/2);
                wprintw(btns[selection], "%s", labels[selection]);
                wrefresh(btns[selection]);

                selection--;

                wattron(btns[selection], A_BOLD | A_UNDERLINE);
                wmove(btns[selection], 1, (elem_w/2)-strlen(labels[selection])/2);
                wprintw(btns[selection], "%s", labels[selection]);
                wattroff(btns[selection], A_BOLD | A_UNDERLINE);
                wrefresh(btns[selection]);
            }
		}
		/* if ARROW_KEY_DOWN is pressed -> navigate in the directory */
		else if (KEY_DOWN == symbol)
		{
			if (selection < 3)
            {
                wmove(btns[selection], 1, (elem_w/2)-strlen(labels[selection])/2);
                wprintw(btns[selection], "%s", labels[selection]);
                wrefresh(btns[selection]);

                selection++;

                wattron(btns[selection], A_BOLD | A_UNDERLINE);
                wmove(btns[selection], 1, (elem_w/2)-strlen(labels[selection])/2);
                wprintw(btns[selection], "%s", labels[selection]);
                wattroff(btns[selection], A_BOLD | A_UNDERLINE);
                wrefresh(btns[selection]);
            }
		}
		/*
		* if ENTER is pressed -> try to open current selected item and change
		* current working directory.
		*/
		else if ('\n' == symbol)
		{
            switch (selection)
            {
            case 0:
            case 1:
            case 2:
                *option = selection+1;
                break;
            case 3:
                *option = 0;
                break;
            default:
                break;
            }

            break;
		}
		/* If F3 is pressed -> exit application */
		else if (KEY_F(4) == symbol)
		{
            *option = 0;
			break;
		}
	}

    cur_wnd = WND_NONE;

    for (index = 0; index < 4; ++index)
    {
        if (labels[index] != NULL)
            free(labels[index]);
        delwin(btns[index]);
    }
    delwin(status_subwnd);
    delwin(status_wnd);
    delwin(btns_border);
    delwin(note_subwnd);
    delwin(note_wnd);
    delwin(panel_wnd);
    delwin(header_wnd);
    delwin(subwnd);

    status_subwnd = NULL;
    note_subwnd = NULL;

    return ret;
}

int join_srv_wnd()
{
    WINDOW *subwnd;
    WINDOW* serv_name_wnd;
    WINDOW* serv_address_wnd;
    WINDOW* serv_users_wnd;
    WINDOW *mainpad;
    WINDOW *border_wnd;
    WINDOW *address_tb;
    WINDOW *note_wnd;
    WINDOW *note_subwnd;
    WINDOW *btns[3];
    char *labels[3];
    
    int symbol;
    int index;
    int ret = 0;

    labels[0] = malloc(strlen(JOIN_SCR_JOIN_BTN_LABEL)+1);
    labels[1] = malloc(strlen(JOIN_SCR_REFRESH_BTN_LABEL)+1);
    labels[2] = malloc(strlen(JOIN_SCR_CLEAR_BTN_LABEL)+1);

    strcpy(labels[0], JOIN_SCR_JOIN_BTN_LABEL);
    strcpy(labels[1], JOIN_SCR_REFRESH_BTN_LABEL);
    strcpy(labels[2], JOIN_SCR_CLEAR_BTN_LABEL);

    char arr[] = {
        '1','2','3','4','5','6','7','8','9','0'//,'q','w','e','r','t','y','u','i','o','p','a','s','d','f','g','h','j','k','l',';'
    };
    int line = 0;
    int vis_line = 0;

    int border_h = wnd_h-2-elem_h-(GFX_ELEM_VOFF*2);
    int border_w = wnd_w-2-(GFX_ELEM_HOFF*2);

    int pad_h = sizeof(arr) > border_h-(elem_h*2) ? sizeof(arr) : border_h-(elem_h*2);
    int pad_w = border_w-2;

    int address_tb_width = 40;
    int name_field_width = 1+strlen(JOIN_SCR_MAN_ADDR_LABEL)+address_tb_width;
    int field_width = ((border_w-2) - (name_field_width))/3 -1 > 10 ? 
                        ((border_w-2) - (name_field_width))/3 -1  : 10;
    // int users_field_width = ((border_w-4)/5) > 7 ? (((border_w-2)/5)-2) : 7;
    // int address_field_width = ((border_w-4)/2 - users_field_width) > 20 ? ((border_w-4)/2 - users_field_width) : 20;
    int address_field_width = 1+(field_width*2);
    
    // int name_field_width = (border_w-4) - (users_field_width + address_field_width);
    

    
    
    subwnd = newwin(wnd_h, wnd_w, (size.ws_row/2)-(wnd_h/2), (size.ws_col/2)-(wnd_w/2));
    border_wnd = derwin(subwnd, border_h, border_w, 1+GFX_ELEM_VOFF, 1+GFX_ELEM_HOFF);
    serv_name_wnd = derwin(border_wnd, 1, name_field_width, 1, 1);
    serv_address_wnd = derwin(border_wnd, 1, address_field_width, 1, border_w-1-field_width-1-address_field_width);
    serv_users_wnd = derwin(border_wnd, 1, field_width, 1, border_w-1-field_width);
    address_tb = derwin(border_wnd, 1, address_tb_width, (border_h-2), 1+strlen(JOIN_SCR_MAN_ADDR_LABEL)+1);
    // btns[0] = derwin(border_wnd, 1, (elem_w-2), (border_h-2), 1+GFX_ELEM_HOFF);
    // btns[1] = derwin(border_wnd, 1, (elem_w-2), (border_h-2), 1+GFX_ELEM_HOFF);
    btns[0] = derwin(border_wnd, 1, field_width, (border_h-2), border_w-1-(field_width*3)-2);
    btns[1] = derwin(border_wnd, 1, field_width, (border_h-2), border_w-1-(field_width*2)-1);
    btns[2] = derwin(border_wnd, 1, field_width, (border_h-2), border_w-1-field_width);
    note_wnd = derwin(subwnd, elem_h, border_w, wnd_h-1-elem_h, 1+GFX_ELEM_HOFF);
    note_subwnd = derwin(note_wnd, 1, border_w-4, 1, 1);
    // JOIN_SCR_MAN_ADDR_LABEL
    
    mainpad = newpad(pad_h, pad_w);

    box(subwnd, ACS_VLINE, ACS_HLINE);
    // box(serv_name_wnd, NULL, NULL);
    // box(serv_address_wnd, NULL, NULL);
    // box(serv_users_wnd, NULL, NULL);
    box(border_wnd, ACS_VLINE, ACS_HLINE);
    box(note_wnd, ' ', ' ');
    
    wmove(subwnd, 0, (wnd_w/2)-strlen(JOIN_SCR_LABEL)/2);
    wprintw(subwnd, JOIN_SCR_LABEL);
    // wmove(serv_name_wnd, 1, (name_field_width/2)-strlen(JOIN_SCR_SRV_NAME_LABEL)/2);
    wmove(serv_name_wnd, 1, 1);
    wprintw(serv_name_wnd, JOIN_SCR_SRV_NAME_LABEL);
    // wmove(serv_address_wnd, 1, (address_field_width/2)-strlen(JOIN_SCR_SRV_ADDR_LABEL)/2);
    wmove(serv_address_wnd, 1, 1);
    wprintw(serv_address_wnd, JOIN_SCR_SRV_ADDR_LABEL);
    // wmove(serv_users_wnd, 1, (users_field_width/2)-strlen(JOIN_SCR_CONN_USERS_LABEL)/2);
    wmove(serv_users_wnd, 1, 1);
    wprintw(serv_users_wnd, JOIN_SCR_CONN_USERS_LABEL);

    wattron(mainpad, COLOR_PAIR(2));
    wmove(mainpad, line, 0);
    whline(mainpad, ' ', pad_w);
    wmove(mainpad, line, 0);
    wprintw(mainpad, "%c", arr[line]);
    wmove(mainpad, line, (name_field_width));
    waddch(mainpad, ACS_VLINE);
    wmove(mainpad, line, (name_field_width+1+address_field_width));
    waddch(mainpad, ACS_VLINE);
    wattroff(mainpad, COLOR_PAIR(2));

    // wmove(serv_address_wnd, 1, 1);
    // wprintw(serv_address_wnd, JOIN_SCR_SRV_ADDR_LABEL);
    for (index = 0; index < 3; index++)
    {
        wmove(btns[index], 0, (field_width/2)-(strlen(labels[index])/2));
        wprintw(btns[index], "%s", labels[index]);
    }

    wmove(note_subwnd, 0, ((wnd_w-4-(GFX_ELEM_HOFF*2))/2)-join_note_size/2);
    wprintw(note_subwnd, "%s", join_note_label);

    wmove(border_wnd, 2, 1);
    whline(border_wnd, ACS_HLINE, (border_w-2));

    wmove(border_wnd, 0, (1+name_field_width));
    waddch(border_wnd, ACS_BSSS);
    wmove(border_wnd, 0, (1+name_field_width+1+address_field_width));
    waddch(border_wnd, ACS_BSSS);

    wmove(border_wnd, 2, 0);
    waddch(border_wnd, ACS_LTEE);
    wmove(border_wnd, 2, border_w-1);
    waddch(border_wnd, ACS_RTEE);
    
    wmove(border_wnd, 1, (1+name_field_width));
    waddch(border_wnd, ACS_VLINE);
    // wvline(border_wnd, ACS_VLINE, 1);
    wmove(border_wnd, 1, (1+name_field_width+1+address_field_width));
    waddch(border_wnd, ACS_VLINE);
    // wvline(border_wnd, ACS_VLINE, 1);

    wmove(border_wnd, 2, (1+name_field_width));
    waddch(border_wnd, ACS_PLUS);
    wmove(border_wnd, 2, (1+name_field_width+1+address_field_width));
    waddch(border_wnd, ACS_PLUS);

    wmove(border_wnd, (border_h-3), 1);
    whline(border_wnd, ACS_HLINE, (border_w-2));

    wmove(border_wnd, (border_h-3), 0);
    waddch(border_wnd, ACS_LTEE);
    wmove(border_wnd, (border_h-3), border_w-1);
    waddch(border_wnd, ACS_RTEE);

    wmove(border_wnd, (border_h-3), 1+strlen(JOIN_SCR_MAN_ADDR_LABEL));
    waddch(border_wnd, ACS_BSSS);

    wmove(border_wnd, (border_h-1), 1+strlen(JOIN_SCR_MAN_ADDR_LABEL));
    waddch(border_wnd, ACS_BTEE);
    wmove(border_wnd, (border_h-2), 1+strlen(JOIN_SCR_MAN_ADDR_LABEL));
    wvline(border_wnd, ACS_VLINE, 1);

    wmove(border_wnd, (border_h-3), 1+strlen(JOIN_SCR_MAN_ADDR_LABEL)+1+address_tb_width);
    waddch(border_wnd, ACS_BSSS);
    wmove(border_wnd, (border_h-1), 1+strlen(JOIN_SCR_MAN_ADDR_LABEL)+1+address_tb_width);
    waddch(border_wnd, ACS_BTEE);
    wmove(border_wnd, (border_h-2), 1+strlen(JOIN_SCR_MAN_ADDR_LABEL)+1+address_tb_width);
    wvline(border_wnd, ACS_VLINE, 1);

    wmove(border_wnd, (border_h-3), border_w-1-field_width-1);
    waddch(border_wnd, ACS_BSSS);
    wmove(border_wnd, (border_h-1), border_w-1-field_width-1);
    waddch(border_wnd, ACS_BTEE);
    wmove(border_wnd, (border_h-2), border_w-1-field_width-1);
    wvline(border_wnd, ACS_VLINE, 1);

    wmove(border_wnd, (border_h-3), border_w-1-(field_width*2)-2);
    waddch(border_wnd, ACS_BSSS);
    wmove(border_wnd, (border_h-1), border_w-1-(field_width*2)-2);
    waddch(border_wnd, ACS_BTEE);
    wmove(border_wnd, (border_h-2), border_w-1-(field_width*2)-2);
    wvline(border_wnd, ACS_VLINE, 1);

    wmove(border_wnd, (border_h-3), (name_field_width+1));
    waddch(border_wnd, ACS_PLUS);
    wmove(border_wnd, (border_h-3), (name_field_width+1+address_field_width+1));
    waddch(border_wnd, ACS_PLUS);

    for (index = 1; index < sizeof(arr); index++)
    {
        // wmove(mainpad, i, 0);
        // wprintw(mainpad, "%c", arr[i]);
        wmove(mainpad, index, 0);
        whline(mainpad, ' ', pad_w);
        wmove(mainpad, index, 0);
        wprintw(mainpad, "%c", arr[index]);
        wmove(mainpad, index, (name_field_width));
        waddch(mainpad, ACS_VLINE);
        wmove(mainpad, index, (name_field_width+1+address_field_width));
        waddch(mainpad, ACS_VLINE);
    }
    for (; index < pad_h; index++)
    {
        wmove(mainpad, index, (name_field_width));
        waddch(mainpad, ACS_VLINE);
        wmove(mainpad, index, (name_field_width+1+address_field_width));
        waddch(mainpad, ACS_VLINE);
    }

    wmove(border_wnd, (border_h-2), 1);
    wprintw(border_wnd, JOIN_SCR_MAN_ADDR_LABEL);

    // wbkgd(serv_name_wnd, COLOR_PAIR(2));
    // wbkgd(serv_address_wnd, COLOR_PAIR(2));
    // wbkgd(serv_users_wnd, COLOR_PAIR(2));
    // wbkgd(address_tb, COLOR_PAIR(2));
    // wbkgd(mainpad, COLOR_PAIR(2));
    // wbkgd(btns[0], COLOR_PAIR(2));
    // wbkgd(btns[1], COLOR_PAIR(2));
    // wbkgd(btns[2], COLOR_PAIR(2));

    wrefresh(subwnd);
    // wrefresh(border_wnd);
    prefresh(mainpad, vis_line, 0, (size.ws_row/2)-(wnd_h/2)+elem_h+1, (size.ws_col/2)-(wnd_w/2)+3, (size.ws_row/2)+(wnd_h/2)-6, size.ws_col);
    
    scrollok(mainpad, true);
    keypad(subwnd, true);
    while(1)
    {
        symbol = wgetch(subwnd);
        if ('\n' == symbol) 
        {
            WINDOW * sub_pad = newwin(10,40, wnd_h/2, wnd_w/2);
            box(sub_pad, ACS_VLINE, ACS_HLINE);
            wmove(sub_pad, 1, 1);
            wbkgd(sub_pad, COLOR_PAIR(2));
            wprintw(sub_pad, "%c", arr[line]);
            wrefresh(sub_pad);
            wgetch(subwnd);
            // wclear(sub_pad);
            delwin(sub_pad);
            prefresh(mainpad, vis_line, 0, (size.ws_row/2)-(wnd_h/2)+elem_h+1, (size.ws_col/2)-(wnd_w/2)+3, (size.ws_row/2)+(wnd_h/2)-6, size.ws_col);
        }
        else if (KEY_F(3) == symbol)
		{
            ret = 3;
			break;
		}
        else if (KEY_F(4) == symbol)
        {
            ret = 0;
            break;
        }
        else if (KEY_UP == symbol)
        {
            if (line > 0)
            {
                // wmove(mainpad, line, 0);
                // wprintw(mainpad, "%c\n", arr[line]);
                wmove(mainpad, line, 0);
                whline(mainpad, ' ', pad_w);
                wmove(mainpad, line, 0);
                wprintw(mainpad, "%c", arr[line]);
                wmove(mainpad, line, (name_field_width));
                waddch(mainpad, ACS_VLINE);
                wmove(mainpad, line, (name_field_width+1+address_field_width));
                waddch(mainpad, ACS_VLINE);

                line--;

                wattron(mainpad, COLOR_PAIR(2));
                wmove(mainpad, line, 0);
                whline(mainpad, ' ', pad_w);
                // for (int i = 0; i < pad_w; i++)
                //     wprintw(mainpad, " ");
                // wattroff(mainpad, COLOR_PAIR(2));
                // wattron(mainpad, COLOR_PAIR(2));
                wmove(mainpad, line, 0);
                wprintw(mainpad, "%c", arr[line]);

                wmove(mainpad, line, (name_field_width));
                waddch(mainpad, ACS_VLINE);
                wmove(mainpad, line, (name_field_width+1+address_field_width));
                waddch(mainpad, ACS_VLINE);
                wattroff(mainpad, COLOR_PAIR(2));

                if (line <= vis_line)
                    vis_line--;

                prefresh(mainpad, vis_line, 0, (size.ws_row/2)-(wnd_h/2)+elem_h+1, (size.ws_col/2)-(wnd_w/2)+3, (size.ws_row/2)+(wnd_h/2)-6, size.ws_col);
            }
        }
            /* if ARROW_KEY_DOWN is pressed -> navigate in the directory */
        else if (KEY_DOWN == symbol)
        {
            if (line < (sizeof(arr))-1)
            {
                    
                    
                // wmove(mainpad, line, 0);
                // wprintw(mainpad, "%c\n", arr[line]);
                wmove(mainpad, line, 0);
                whline(mainpad, ' ', pad_w);
                wmove(mainpad, line, 0);
                wprintw(mainpad, "%c", arr[line]);
                wmove(mainpad, line, (name_field_width));
                waddch(mainpad, ACS_VLINE);
                wmove(mainpad, line, (name_field_width+1+address_field_width));
                waddch(mainpad, ACS_VLINE);

                line++;

                wattron(mainpad, COLOR_PAIR(2));
                wmove(mainpad, line, 0);
                whline(mainpad, ' ', pad_w);
                wmove(mainpad, line, 0);
                wprintw(mainpad, "%c", arr[line]);
                wmove(mainpad, line, (name_field_width));
                waddch(mainpad, ACS_VLINE);
                wmove(mainpad, line, (name_field_width+1+address_field_width));
                waddch(mainpad, ACS_VLINE);
                wattroff(mainpad, COLOR_PAIR(2));

                if ((line + pad_h) <= sizeof(arr))
                    vis_line = line;

                prefresh(mainpad, vis_line, 0, (size.ws_row/2)-(wnd_h/2)+elem_h+1, (size.ws_col/2)-(wnd_w/2)+3, (size.ws_row/2)+(wnd_h/2)-6, size.ws_col);
            }
        }
        else
        {
                // wprintw(mainpad, "%c\n", symbol);
                // prefresh(mainpad, vis_line, 0, (size.ws_row/2)-(SUBWND_H/2)+2, (size.ws_col/2)-(SUBWND_W/2)+2, (size.ws_row/2)+(SUBWND_H/2)-5, size.ws_col);
        }
    }

    for (index = 0; index < 3; ++index)
    {
        if (labels[index] != NULL)
            free(labels[index]);
        delwin(btns[index]);
    }

    delwin(subwnd);
    delwin(border_wnd);
    delwin(serv_name_wnd);
    delwin(serv_address_wnd);
    delwin(serv_users_wnd);
    delwin(mainpad);
    delwin(address_tb);
    delwin(note_wnd);
    delwin(note_subwnd);
    delwin(btns[2]);

    return ret;
}

int create_srv_wnd()
{
    WINDOW *subwnd;
    WINDOW *border_wnd;

    // WINDOW *serv_name_wnd;
    // WINDOW *serv_name_tb;

    // WINDOW *serv_type_wnd;
    // WINDOW *serv_type_local;

    WINDOW *note_wnd;
    WINDOW *note_subwnd;
    
    int symbol;
    // int index;
    int ret = 0;

    int border_h = wnd_h-2-elem_h-(GFX_ELEM_VOFF*2);
    int border_w = wnd_w-2-(GFX_ELEM_HOFF*2);
    
    subwnd = newwin(wnd_h, wnd_w, (size.ws_row/2)-(wnd_h/2), (size.ws_col/2)-(wnd_w/2));
    border_wnd = derwin(subwnd, border_h, border_w, 1+GFX_ELEM_VOFF, 1+GFX_ELEM_HOFF);
    // btns[0] = derwin(border_wnd, 1, (elem_w-2), (border_h-2), 1+GFX_ELEM_HOFF);
    // btns[1] = derwin(border_wnd, 1, (elem_w-2), (border_h-2), 1+GFX_ELEM_HOFF);
    note_wnd = derwin(subwnd, elem_h, border_w, wnd_h-1-elem_h, 1+GFX_ELEM_HOFF);
    note_subwnd = derwin(note_wnd, 1, border_w-4, 1, 1);

    box(subwnd, ACS_VLINE, ACS_HLINE);
    box(border_wnd, ACS_VLINE, ACS_HLINE);
    box(note_wnd, ' ', ' ');
    
    wmove(subwnd, 0, (wnd_w/2)-strlen(CREATE_SCR_LABEL)/2);
    wprintw(subwnd, CREATE_SCR_LABEL);

    wrefresh(subwnd);

    keypad(subwnd, true);
    while(1)
    {
        symbol = wgetch(subwnd);
        if ('\n' == symbol) 
        {
            
        }
        else if (KEY_F(3) == symbol)
		{
            ret = 3;
			break;
		}
        else if (KEY_F(4) == symbol)
        {
            ret = 0;
            break;
        }
        else if (KEY_UP == symbol)
        {
            
        }
            /* if ARROW_KEY_DOWN is pressed -> navigate in the directory */
        else if (KEY_DOWN == symbol)
        {
            
        }
        else
        {
                // wprintw(mainpad, "%c\n", symbol);
                // prefresh(mainpad, vis_line, 0, (size.ws_row/2)-(SUBWND_H/2)+2, (size.ws_col/2)-(SUBWND_W/2)+2, (size.ws_row/2)+(SUBWND_H/2)-5, size.ws_col);
        }
    }

    delwin(subwnd);
    delwin(border_wnd);
    delwin(note_wnd);
    delwin(note_subwnd);

    return ret;
}

int prefs_wnd()
{
    WINDOW *subwnd;
    WINDOW *mainpad;
    WINDOW *pad_border;
    WINDOW *note_wnd;
    WINDOW *note_subwnd;
    int symbol;
    int index;
    int ret = 0;

    char arr[] = {
        '1','2','3','4','5','6','7','8','9','0'//,'q','w','e','r','t','y','u','i','o','p','a','s','d','f','g','h','j','k','l',';'
    };

    int border_h = wnd_h-2-elem_h;
    int border_w = wnd_w-2-(GFX_ELEM_HOFF*2);

    int pad_h = border_h-2;
    int pad_w = border_w-2;
    
    int line = 0;
    int vis_line = 0;

    subwnd = newwin(wnd_h, wnd_w, (size.ws_row/2)-(wnd_h/2), (size.ws_col/2)-(wnd_w/2));
    pad_border = derwin(subwnd, border_h, border_w, 1+GFX_ELEM_VOFF, 1+GFX_ELEM_HOFF);
    mainpad = newpad((sizeof(arr)+1) > pad_h ? (sizeof(arr)+1) : pad_h, pad_w);
    note_wnd = derwin(subwnd, elem_h, border_w, wnd_h-1-elem_h-GFX_ELEM_VOFF, 1+GFX_ELEM_HOFF);
    note_subwnd = derwin(note_wnd, 1, border_w-4, 1, 1);
    

    box(subwnd, ACS_VLINE, ACS_HLINE);
    box(pad_border, ACS_VLINE, ACS_HLINE);
    box(note_wnd, ' ', ' ');

    wmove(subwnd, 0, (wnd_w/2)-strlen(PREFS_SCR_LABEL)/2);
    wprintw(subwnd, PREFS_SCR_LABEL);
    // wmove(subwnd, subwnd_h-3, (wnd_w/2)-strlen(PREFS_SCR_NOTE)/2);
    // wprintw(subwnd, PREFS_SCR_NOTE);
    wmove(note_subwnd, 0, ((border_w-4)/2)-(prefs_note_size/2));
    wprintw(note_subwnd, "%s", prefs_note_label);

    wmove(pad_border, 0, pad_w-pad_w/3);
    waddch(pad_border, ACS_BSSS);
    wmove(pad_border, (border_h-1), pad_w-pad_w/3);
    waddch(pad_border, ACS_BTEE);

    wattron(mainpad, COLOR_PAIR(2));
    wmove(mainpad, 0, 0);
    whline(mainpad, ' ', pad_w-pad_w/3-1);
    wmove(mainpad, 0, 0);
    wprintw(mainpad, "%c", arr[0]);
    wattroff(mainpad, COLOR_PAIR(2));
    wmove(mainpad, 0, pad_w-pad_w/3-1);
    waddch(mainpad, ACS_VLINE);
    
    // wattron(mainpad, A_STANDOUT);
    // wprintw(mainpad, "%c", arr[0]);
    // wattroff(mainpad, A_STANDOUT);
    // wmove(mainpad, 0, pad_w-pad_w/3-1);
    // wprintw(mainpad, "|");

    for (index = 1; index < sizeof(arr); index++)
    {
        // wmove(mainpad, i, 0);
        // wprintw(mainpad, "%c", arr[i]);
        wmove(mainpad, index, 0);
        whline(mainpad, ' ', pad_w-pad_w/3-1);
        wmove(mainpad, index, 0);
        wprintw(mainpad, "%c", arr[index]);
        wmove(mainpad, index, pad_w-pad_w/3-1);
        waddch(mainpad, ACS_VLINE);
    }
    for (; index < pad_h; index++)
    {
        wmove(mainpad, index, pad_w-pad_w/3-1);
        waddch(mainpad, ACS_VLINE);
    }

    // for (index = 1; index < sizeof(arr); index++)
    // {
    //     wmove(mainpad, index, 0);
    //     wprintw(mainpad, "%c", arr[index]);

    //     wmove(mainpad, i, pad_w-pad_w/3-1);
    //     wprintw(mainpad, "|");
    // }

    // wbkgd(mainpad, COLOR_PAIR(2));

    wrefresh(subwnd);
    prefresh(mainpad, vis_line, 0, (size.ws_row/2)-(wnd_h/2)+2, (size.ws_col/2)-(wnd_w/2)+2+GFX_ELEM_HOFF, (size.ws_row/2)+(wnd_h/2)-5, size.ws_col);

    scrollok(mainpad, true);
    keypad(subwnd, true);
    
    while(1)
    {
        symbol = wgetch(subwnd);
        if ('\n' == symbol) 
        {
            WINDOW * sub_pad = newwin(10,40, wnd_h/2, wnd_w/2);
            box(sub_pad, ACS_VLINE, ACS_HLINE);
            wmove(sub_pad, 1, 1);
            wbkgd(sub_pad, COLOR_PAIR(2));
            wprintw(sub_pad, "%c", arr[line]);
            wrefresh(sub_pad);
            wgetch(subwnd);
            delwin(sub_pad);
            wrefresh(subwnd);
            prefresh(mainpad, vis_line, 0, (size.ws_row/2)-(wnd_h/2)+2, (size.ws_col/2)-(wnd_w/2)+2+GFX_ELEM_HOFF, (size.ws_row/2)+(wnd_h/2)-6, size.ws_col);
        }
        else if (KEY_F(1) == symbol)
		{
            ret = 1;
			break;
		}
        else if (KEY_F(2) == symbol)
		{
            ret = 2;
			break;
		}
        else if (KEY_F(3) == symbol)
		{
            ret = 3;
			break;
		}
        else if (KEY_F(4) == symbol)
		{
            ret = 0;
			break;
		}
        else if (KEY_UP == symbol)
		{
            if (line > 0)
            {
                wmove(mainpad, line, 0);
                whline(mainpad, ' ', pad_w-pad_w/3-1);
                wmove(mainpad, line, 0);
                wprintw(mainpad, "%c", arr[line]);
                wmove(mainpad, line, pad_w-pad_w/3-1);
                waddch(mainpad, ACS_VLINE);

                line--;

                wattron(mainpad, COLOR_PAIR(2));
                wmove(mainpad, line, 0);
                whline(mainpad, ' ', pad_w-pad_w/3-1);
                wmove(mainpad, line, 0);
                wprintw(mainpad, "%c", arr[line]);
                wattroff(mainpad, COLOR_PAIR(2));
                wmove(mainpad, line, pad_w-pad_w/3-1);
                waddch(mainpad, ACS_VLINE);

                if (line <= vis_line)
                    vis_line--;

                prefresh(mainpad, vis_line, 0, (size.ws_row/2)-(wnd_h/2)+2, (size.ws_col/2)-(wnd_w/2)+2+GFX_ELEM_HOFF, (size.ws_row/2)+(wnd_h/2)-6, size.ws_col);
            }
		}
		/* if ARROW_KEY_DOWN is pressed -> navigate in the directory */
		else if (KEY_DOWN == symbol)
		{
            if (line < (sizeof(arr))-1)
            {
                wmove(mainpad, line, 0);
                whline(mainpad, ' ', pad_w-pad_w/3-1);
                wmove(mainpad, line, 0);
                wprintw(mainpad, "%c", arr[line]);
                wmove(mainpad, line, pad_w-pad_w/3-1);
                waddch(mainpad, ACS_VLINE);

                line++;

                wattron(mainpad, COLOR_PAIR(2));
                wmove(mainpad, line, 0);
                whline(mainpad, ' ', pad_w-pad_w/3-1);
                wmove(mainpad, line, 0);
                wprintw(mainpad, "%c", arr[line]);
                wattroff(mainpad, COLOR_PAIR(2));
                wmove(mainpad, line, pad_w-pad_w/3-1);
                waddch(mainpad, ACS_VLINE);

                if ((line + pad_h) <= sizeof(arr))
                    vis_line = line;
                
                prefresh(mainpad, vis_line, 0, (size.ws_row/2)-(wnd_h/2)+2, (size.ws_col/2)-(wnd_w/2)+2+GFX_ELEM_HOFF, (size.ws_row/2)+(wnd_h/2)-6, size.ws_col);
            }
		}
    }
    delwin(mainpad);
    delwin(subwnd);

    return ret;
}
