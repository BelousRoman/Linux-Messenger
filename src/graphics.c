#include "../hdr/graphics.h"

// #define SUBWND_MIN_H        (elem_h*4)+2+(GFX_ELEM_VOFF*7)
// #define SUBWND_MIN_H        (elem_h*6)+4+(GFX_ELEM_VOFF*9)+header_h

// // #define SUBWND_MIN_W        ((elem_w/2)+panel_w+2+(GFX_ELEM_HOFF*3))*2
// #define SUBWND_MIN_W        (elem_w+2+PANEL_MIN_W+(GFX_ELEM_HOFF*5)) > header_w ? \
                             (elem_w+2+PANEL_MIN_W+(GFX_ELEM_HOFF*5)) : header_w

char arr[] = {
    '1','2','3','4','5','6','7','8','9','0'//,'q','w','e','r','t','y','u','i','o','p','a','s','d','f','g','h','j','k','l',';'
};

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
// TODO Move wnd var here and call wrefresh(wnd) in handle_msg()
WINDOW *wnd = NULL;
// WINDOW *subwnd = NULL;

WINDOW *note_w = NULL;
WINDOW *note_sw = NULL;

struct mmenu_wnd_t main_menu =
{
    .selection = 0
};

struct join_wnd_t join_srv =
{
    .servers = NULL,
    .servers_count = 0,
    .line = 0,
    .vis_line = 0,
    .selection = 0,
    .mode = 0
};

struct create_wnd_t create_srv =
{
    .line = 0,
    .vis_line = 0,
    .selection = 0
};

struct cfg_wnd_t cfg_wnd = 
{
    .pad = NULL
};

struct global_dims_t global_dims;
struct mmenu_dims_t mmenu_dims;
struct join_dims_t join_dims;
struct create_dims_t create_dims;
struct cfg_dims_t cfg_dims;

struct global_axis_t global_axis;
struct mmenu_axis_t mmenu_axis;
struct join_axis_t join_axis;
struct create_axis_t create_axis;

// int popup_wnd_h;
// int popup_wnd_w;

char *join_note_label = NULL;
char *prefs_note_label = NULL;

int join_note_size = 0;
int prefs_note_size = 0;

int _draw_window(int);
int _update_window(void);
int _delete_window(void);

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
	mqd_t mqd = ((mqd_t) sv.sival_int);
    struct sigevent sev;
    int msg;
    ssize_t ret;
    char ch[21];
    /*
    * Configure sigevent, set notification method, function called, attributes
    * and pointer to q_handler.
    * */
	sev.sigev_notify = SIGEV_THREAD;
	sev.sigev_notify_function = handle_msg;
	sev.sigev_notify_attributes = NULL;
	sev.sigev_value.sival_ptr = mqd;

    popup_wnd("handle msg", POPUP_W_WAIT);

    /* Read queue until an error occurs */
	while (ret = mq_receive(mqd, &msg, sizeof(int),
                        NULL) > 0)
	{
        snprintf(ch, 20, "handle %d|%d|%d\0", mqd, msg, cur_wnd);
        popup_wnd(ch, POPUP_W_WAIT);
        if (wnd == NULL || main_menu.status_sw == NULL || note_sw == NULL)
            return;

		switch (msg)
        {
        case CONNECT_COMM:
            if (cur_wnd == WND_MAIN_MENU)
            {
                wclear(main_menu.status_sw);
                wmove(main_menu.status_sw, 0, 0);
                switch (connection_flag)
                {
                case STATUS_DISCONNECTED:
                    wattron(main_menu.status_sw, COLOR_PAIR(5));
                    waddch(main_menu.status_sw, ACS_DIAMOND);
                    wattroff(main_menu.status_sw, COLOR_PAIR(5));
                    wprintw(main_menu.status_sw, " Status: ");
                    wattron(main_menu.status_sw, COLOR_PAIR(5));
                    wprintw(main_menu.status_sw, "Connected");
                    wattroff(main_menu.status_sw, COLOR_PAIR(5));

                    wmove(note_sw, 0, ((global_dims.wnd_w-4-(GFX_ELEM_HOFF*2))/2)-strlen(MENU_SCR_NOTE_DISCONNECTED)/2);
                    wprintw(note_sw, MENU_SCR_NOTE_DISCONNECTED);
                    break;
                case STATUS_CONNECTED:
                    wattron(main_menu.status_sw, COLOR_PAIR(4));
                    waddch(main_menu.status_sw, ACS_DIAMOND);
                    wattroff(main_menu.status_sw, COLOR_PAIR(4));
                    wprintw(main_menu.status_sw, " Status: ");
                    wattron(main_menu.status_sw, COLOR_PAIR(4));
                    wprintw(main_menu.status_sw, "Disconnected");
                    wattroff(main_menu.status_sw, COLOR_PAIR(4));

                    wmove(note_sw, 0, ((global_dims.wnd_w-4-(GFX_ELEM_HOFF*2))/2)-strlen(MENU_SCR_NOTE_CONNECTED)/2);
                    wprintw(note_sw, MENU_SCR_NOTE_CONNECTED);
                    break;
                case STATUS_CONNECTING:
                    wattron(main_menu.status_sw, COLOR_PAIR(3));
                    waddch(main_menu.status_sw, ACS_DIAMOND);
                    wattroff(main_menu.status_sw, COLOR_PAIR(3));
                    wprintw(main_menu.status_sw, " Status: ");
                    wattron(main_menu.status_sw, A_BLINK | COLOR_PAIR(3));
                    wprintw(main_menu.status_sw, "Connecting");
                    wattroff(main_menu.status_sw, A_BLINK | COLOR_PAIR(3));

                    wmove(note_sw, 0, ((global_dims.wnd_w-4-(GFX_ELEM_HOFF*2))/2)-strlen(MENU_SCR_NOTE_DISCONNECTED)/2);
                    wprintw(note_sw, MENU_SCR_NOTE_DISCONNECTED);
                    break;
                default:
                    break;
                }
                
            }
            wrefresh(wnd);
            wrefresh(main_menu.status_sw);
            wrefresh(note_sw);
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
	while (ret = mq_notify(mqd, &sev) == -1)
	{
		perror("mq_notify");
		exit(EXIT_FAILURE);
	}
	return EXIT_SUCCESS;
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

int _set_dimensions()
{
    int main_menu_wnd_min_h;
    int main_menu_wnd_min_w;
    // int tmp = 0;
    int ret = EXIT_SUCCESS;

    ioctl(fileno(stdout), TIOCGWINSZ, (char *) &size);

    /* Set dimensions that are global across all windows */
    {
        global_dims.elem_h = ELEM_SET_H > ELEM_MIN_H ? ELEM_SET_H : ELEM_MIN_H;
        global_dims.elem_w = ELEM_SET_W > ELEM_MIN_W ? ELEM_SET_W : ELEM_MIN_W;
        
        global_dims.wnd_h = SUBWND_SET_H - 2 - (GFX_ELEM_VOFF*3) - global_dims.elem_h;
        global_dims.wnd_w = SUBWND_SET_W - 2 - (GFX_ELEM_HOFF*2);

        // global_dims.subwnd_h = global_dims.wnd_h - 2 - (GFX_ELEM_VOFF*2);
        // global_dims.subwnd_w = global_dims.wnd_w - 2 - (GFX_ELEM_HOFF*2);

        global_dims.note_h = global_dims.elem_h;
        global_dims.note_w = SUBWND_SET_W - 2 - (GFX_ELEM_HOFF*2);
    }

    /* Set dimensions that are used in main menu window */
    {
        mmenu_dims.header_h = WLCM_WND_H_EN;
        mmenu_dims.header_w = WLCM_WND_W_EN;

        mmenu_dims.status_h = global_dims.elem_h;
        mmenu_dims.status_w = 2 + global_dims.elem_w + (GFX_ELEM_HOFF*2);

        mmenu_dims.btns_h = global_dims.elem_h;
        mmenu_dims.btns_w = global_dims.elem_w;

        mmenu_dims.btns_border_h = 2 + (global_dims.elem_h*4) + (GFX_ELEM_VOFF*5);
        mmenu_dims.btns_border_w = mmenu_dims.status_w;

        mmenu_dims.panel_h = global_dims.wnd_h - 2 - mmenu_dims.header_h - GFX_ELEM_VOFF;
        mmenu_dims.panel_w = global_dims.wnd_w - 2 - mmenu_dims.status_w - (GFX_ELEM_HOFF*3);
    }

    /* Set dimensions that are used in join server window */
    {
        join_dims.sinfo_h = global_dims.elem_h;
        join_dims.sinfo_w = global_dims.wnd_w - 2 - (GFX_ELEM_HOFF*2);

        join_dims.susers_h = join_dims.sinfo_h - 2;
        join_dims.susers_w = (join_dims.sinfo_w - 4)/6;
        join_dims.susers_w = join_dims.susers_w > 7 ? join_dims.susers_w : 7;

        join_dims.saddr_h = join_dims.sinfo_h - 2;
        join_dims.saddr_w = join_dims.susers_w * 2;
        join_dims.saddr_w = join_dims.saddr_w > IP_ADDR_LEN + 6 ? join_dims.saddr_w : IP_ADDR_LEN + 6;

        join_dims.sname_h = join_dims.sinfo_h - 2;
        join_dims.sname_w = join_dims.sinfo_w - 4 - join_dims.saddr_w - join_dims.susers_w;

        join_dims.btns_border_h = global_dims.elem_h;
        join_dims.btns_border_w = 4 + (global_dims.elem_w*3);

        join_dims.btns_h = join_dims.btns_border_h - 2;
        join_dims.btns_w = global_dims.elem_w;

        join_dims.lbl_h = global_dims.elem_h - 2;
        join_dims.lbl_w = strlen(JOIN_SCR_MAN_ADDR_LABEL);

        join_dims.tb_h = global_dims.elem_h - 2;
        join_dims.tb_w = join_dims.sname_w - 1 - join_dims.lbl_w;
        if ((2 + join_dims.lbl_w + join_dims.tb_w + join_dims.btns_border_w) > (global_dims.wnd_w - 2 - (GFX_ELEM_HOFF*2)))
            // join_dims.tb_w = IP_ADDR_LEN + 6;
            join_dims.tb_w += (global_dims.wnd_w - 2 - (GFX_ELEM_HOFF*2)) - (2 + join_dims.lbl_w + join_dims.tb_w + join_dims.btns_border_w);
        else
            join_dims.tb_w = join_dims.tb_w > IP_ADDR_LEN + 6 ? join_dims.tb_w : IP_ADDR_LEN + 6;

        join_dims.caddr_h = global_dims.elem_h;
        join_dims.caddr_w = 3 + join_dims.lbl_w + join_dims.tb_w;

        join_dims.pad_border_h = global_dims.wnd_h - join_dims.sinfo_h - join_dims.caddr_h - GFX_ELEM_VOFF;
        join_dims.pad_border_w = global_dims.wnd_w - 2 - (GFX_ELEM_HOFF*2);

        join_dims.vis_pad_h = join_dims.pad_border_h - 2;
        join_dims.vis_pad_w = join_dims.pad_border_w - 2;

        // TODO: Add calibration for these dimensions
        join_dims.pad_h = join_dims.vis_pad_h;
        join_dims.pad_w = join_dims.vis_pad_w;

        if (join_dims.sname_w < NAME_LEN)
            ret = EXIT_FAILURE;
    }

    /* Set dimensions that are used in create server window */
    {
        create_dims.subwnd_h = global_dims.wnd_h - 2 - (GFX_ELEM_VOFF*2);

        create_dims.sname_h = global_dims.elem_h - 2;
        create_dims.sname_w = strlen(CREATE_SCR_SRV_NAME_LABEL)+1+NAME_LEN;

        create_dims.musers_h = global_dims.elem_h - 2;
        create_dims.musers_w = strlen(CREATE_SCR_CONN_USERS_LABEL)+4;

        create_dims.rusers_h = global_dims.elem_h - 2;
        create_dims.rusers_w = strlen(CREATE_SCR_RESTR_USERS_LABEL)+2;

        create_dims.saddr_h = global_dims.elem_h - 2;
        create_dims.saddr_w = strlen(CREATE_SCR_SRV_ADDR_LABEL) + IP_ADDR_LEN + 1;

        create_dims.sport_h = global_dims.elem_h - 2;
        create_dims.sport_w = strlen(CREATE_SCR_SRV_PORT_LABEL) + 6;

        create_dims.lcl_addr_h = global_dims.elem_h - 2;
        create_dims.lcl_addr_w = strlen(CREATE_SCR_LCL_ADDR_LABEL)+2;

        create_dims.auto_port_h = global_dims.elem_h - 2;
        create_dims.auto_port_w = strlen(CREATE_SCR_AUTO_PORT_LABEL)+2;

        create_dims.btns_h = global_dims.elem_h;
        create_dims.btns_w = global_dims.elem_w;

        create_dims.pad_elem_h = 1;
        create_dims.pad_elem_w = (create_dims.btns_w*3) - 4;
        create_dims.pad_elem_w = create_dims.pad_elem_w > create_dims.sname_w ? create_dims.pad_elem_w : create_dims.sname_w;
        create_dims.pad_elem_w = create_dims.pad_elem_w > create_dims.musers_w ? create_dims.pad_elem_w : create_dims.musers_w;
        create_dims.pad_elem_w = create_dims.pad_elem_w > create_dims.rusers_w ? create_dims.pad_elem_w : create_dims.rusers_w;
        create_dims.pad_elem_w = create_dims.pad_elem_w > create_dims.saddr_w ? create_dims.pad_elem_w : create_dims.saddr_w;
        create_dims.pad_elem_w = create_dims.pad_elem_w > create_dims.sport_w ? create_dims.pad_elem_w : create_dims.sport_w;
        create_dims.pad_elem_w = create_dims.pad_elem_w > create_dims.lcl_addr_w ? create_dims.pad_elem_w : create_dims.lcl_addr_w;
        create_dims.pad_elem_w = create_dims.pad_elem_w > create_dims.auto_port_w ? create_dims.pad_elem_w : create_dims.auto_port_w;

        create_dims.srv_info_h = global_dims.wnd_h - 2 - (GFX_ELEM_VOFF*2);
        create_dims.srv_info_sw_h = 4;
        create_dims.srv_info_w = create_dims.pad_elem_w + 4 + (GFX_ELEM_HOFF*2);

        // create_dims.pad_border_h = global_dims.wnd_h - 2 - (GFX_ELEM_VOFF*3) - create_dims.btns_h;
        

        create_dims.pad_h = (create_dims.pad_elem_h*7) + 6;
        create_dims.pad_w = create_dims.pad_elem_w;

        create_dims.vis_pad_h = create_dims.srv_info_h - 2 - (GFX_ELEM_VOFF*2) - create_dims.srv_info_sw_h - create_dims.btns_h - 2 - (GFX_ELEM_VOFF*2);
        create_dims.vis_pad_h = create_dims.vis_pad_h > create_dims.pad_h ? create_dims.pad_h : create_dims.vis_pad_h;
        create_dims.vis_pad_w = create_dims.pad_w;

        // create_dims.pad_border_h = create_dims.srv_info_h - 2 - (GFX_ELEM_VOFF*2) - create_dims.srv_info_sw_h - create_dims.btns_h - 2 - (GFX_ELEM_VOFF*2);
        create_dims.pad_border_h = create_dims.vis_pad_h + 2;
        create_dims.pad_border_w = create_dims.pad_elem_w + 2;

        create_dims.subwnd_w = global_dims.wnd_w - create_dims.pad_border_w - 2 - (GFX_ELEM_HOFF*3);
    }

    /* Set dimensions that are used in preferences window */
    {
        cfg_dims.pad_h = global_dims.wnd_h - 2;
        cfg_dims.pad_w = global_dims.wnd_w - 2;
    }

    return ret;
}

int _set_axis()
{
    int ret = EXIT_SUCCESS;

    {
        global_axis.wnd_y = 1+GFX_ELEM_VOFF;
        global_axis.wnd_x = 1+GFX_ELEM_HOFF;
        // global_axis.subwnd_y = 0;
        // global_axis.subwnd_x = 0;
        global_axis.note_y = global_axis.wnd_y+global_dims.wnd_h+(GFX_ELEM_VOFF*2);
        global_axis.note_x = 1+GFX_ELEM_HOFF;
    }

    {
        mmenu_axis.header_y = 1+GFX_ELEM_VOFF;
        mmenu_axis.header_x = 1+GFX_ELEM_HOFF;
        mmenu_axis.status_y = 1+mmenu_dims.header_h+(GFX_ELEM_VOFF*2);
        mmenu_axis.status_x = 1+GFX_ELEM_HOFF;
        mmenu_axis.btns_border_y = 1+mmenu_dims.header_h+mmenu_dims.status_h+(GFX_ELEM_VOFF*3);
        mmenu_axis.btns_border_x = 1+GFX_ELEM_HOFF;
        mmenu_axis.panel_y = 1+mmenu_dims.header_h+(GFX_ELEM_VOFF*2);
        mmenu_axis.panel_x = 1+mmenu_dims.status_w+(GFX_ELEM_HOFF*2);
        mmenu_axis.btns_y[0] = 1+GFX_ELEM_VOFF;
        mmenu_axis.btns_x[0] = 1+GFX_ELEM_HOFF;
        mmenu_axis.btns_y[1] = 1+global_dims.elem_h+(GFX_ELEM_VOFF*2);
        mmenu_axis.btns_x[1] = 1+GFX_ELEM_HOFF;
        mmenu_axis.btns_y[2] = 1+(global_dims.elem_h*2)+(GFX_ELEM_VOFF*3);
        mmenu_axis.btns_x[2] = 1+GFX_ELEM_HOFF;
        mmenu_axis.btns_y[3] = 1+(global_dims.elem_h*3)+(GFX_ELEM_VOFF*4);
        mmenu_axis.btns_x[3] = 1+GFX_ELEM_HOFF;
    }

    {
        join_axis.top_panel_y = 1+GFX_ELEM_VOFF;
        join_axis.top_panel_x = 1+GFX_ELEM_HOFF;
        join_axis.sname_y = 1;
        join_axis.sname_x = 1;
        join_axis.saddr_y = 1;
        join_axis.saddr_x = join_axis.sname_x + join_dims.sname_w + 1;
        join_axis.susers_y = 1;
        join_axis.susers_x = join_axis.saddr_x + join_dims.saddr_w + 1;
        join_axis.pad_border_y = join_axis.top_panel_y + join_dims.sinfo_h - 1;
        join_axis.pad_border_x = 1+GFX_ELEM_HOFF;

        join_axis.vis_pad_ys = join_axis.pad_border_y + 2 + GFX_ELEM_VOFF;
        join_axis.vis_pad_xs = join_axis.pad_border_x + 2 + GFX_ELEM_HOFF;
        join_axis.vis_pad_ye = join_axis.pad_border_y + join_dims.pad_border_h;
        join_axis.vis_pad_xe = join_axis.pad_border_x + join_dims.pad_border_w;

        join_axis.pad_ys = 0;
        join_axis.pad_xs = 0;
        join_axis.pad_ye;
        join_axis.pad_xe;

        join_axis.caddr_y = join_axis.pad_border_y + join_dims.pad_border_h + GFX_ELEM_VOFF - 1;
        join_axis.caddr_x = 1+GFX_ELEM_HOFF;
        join_axis.lbl_y = 1;
        join_axis.lbl_x = 1;
        join_axis.tb_y = 1;
        join_axis.tb_x = join_axis.lbl_x + join_dims.lbl_w + 1;
        join_axis.btns_border_y = join_axis.pad_border_y + join_dims.pad_border_h + GFX_ELEM_VOFF - 1;
        join_axis.btns_border_x = global_dims.wnd_w - join_dims.btns_border_w - 1 - GFX_ELEM_HOFF;
        join_axis.btns_y[0] = 1;
        join_axis.btns_x[0] = 1;
        join_axis.btns_y[1] = 1;
        join_axis.btns_x[1] = 2 + join_dims.btns_w;
        join_axis.btns_y[2] = 1;
        join_axis.btns_x[2] = 3 + (join_dims.btns_w*2);
    }

    {
        create_axis.srv_info_y = 1+GFX_ELEM_VOFF;
        create_axis.srv_info_x = (global_dims.wnd_w - create_dims.srv_info_w)/2 + 1 + GFX_ELEM_HOFF;

        create_axis.srv_info_sw_y = 2+GFX_ELEM_VOFF;
        create_axis.srv_info_sw_x = 2+GFX_ELEM_HOFF;

        create_axis.pad_border_y = create_axis.srv_info_sw_y + create_dims.srv_info_sw_h + 1 + GFX_ELEM_VOFF;
        create_axis.pad_border_x = 1+GFX_ELEM_HOFF;

        create_axis.vis_pad_ys = global_axis.wnd_y + create_axis.srv_info_y + create_axis.pad_border_y + 1;
        create_axis.vis_pad_xs = global_axis.wnd_x + create_axis.srv_info_x + create_axis.pad_border_x + 1;
        create_axis.vis_pad_ye = create_axis.vis_pad_ys + create_dims.vis_pad_h - 1;
        create_axis.vis_pad_xe = create_axis.vis_pad_xs + create_dims.vis_pad_w - 1;

        // create_axis.vis_pad_ye = create_axis.vis_pad_ys + create_dims.pad_h - 1;
        // create_axis.vis_pad_xe = create_axis.vis_pad_xs + create_dims.vis_pad_w - 1;

        create_axis.pad_ys;
        create_axis.pad_xs;
        create_axis.pad_ye;
        create_axis.pad_xe;

        create_axis.sname_y = 0;
        create_axis.sname_x = 0;

        create_axis.musers_y = create_axis.sname_y + 2;
        create_axis.musers_x = 0;
        create_axis.rusers_y = create_axis.musers_y + 2;
        create_axis.rusers_x = 0;
        create_axis.saddr_y = create_axis.rusers_y + 2;
        create_axis.saddr_x = 0;
        create_axis.sport_y = create_axis.saddr_y + 2;
        create_axis.sport_x = 0;
        create_axis.lcl_addr_y = create_axis.sport_y + 2;
        create_axis.lcl_addr_x = 0;
        create_axis.auto_port_y = create_axis.lcl_addr_y + 2;
        create_axis.auto_port_x = 0;

        create_axis.btns_y[0] = create_dims.srv_info_h - 1 - create_dims.btns_h + GFX_ELEM_VOFF;
        create_axis.btns_x[0] = 1 + GFX_ELEM_HOFF;
        create_axis.btns_y[1] = create_dims.srv_info_h - 1 - create_dims.btns_h + GFX_ELEM_VOFF;
        create_axis.btns_x[1] = create_axis.btns_x[0] + create_dims.btns_w - 1;
        create_axis.btns_y[2] = create_dims.srv_info_h - 1 - create_dims.btns_h + GFX_ELEM_VOFF;
        create_axis.btns_x[2] = create_axis.btns_x[1] + create_dims.btns_w - 1;

        create_axis.pad_vdelim = strlen(CREATE_SCR_SRV_NAME_LABEL);
        create_axis.pad_vdelim = create_axis.pad_vdelim > strlen(CREATE_SCR_CONN_USERS_LABEL) ? create_axis.pad_vdelim : strlen(CREATE_SCR_CONN_USERS_LABEL);
        create_axis.pad_vdelim = create_axis.pad_vdelim > strlen(CREATE_SCR_RESTR_USERS_LABEL) ? create_axis.pad_vdelim : strlen(CREATE_SCR_RESTR_USERS_LABEL);
        create_axis.pad_vdelim = create_axis.pad_vdelim > strlen(CREATE_SCR_SRV_ADDR_LABEL) ? create_axis.pad_vdelim : strlen(CREATE_SCR_SRV_ADDR_LABEL);
        create_axis.pad_vdelim = create_axis.pad_vdelim > strlen(CREATE_SCR_SRV_PORT_LABEL) ? create_axis.pad_vdelim : strlen(CREATE_SCR_SRV_PORT_LABEL);
        create_axis.pad_vdelim = create_axis.pad_vdelim > strlen(CREATE_SCR_LCL_ADDR_LABEL) ? create_axis.pad_vdelim : strlen(CREATE_SCR_LCL_ADDR_LABEL);
        create_axis.pad_vdelim = create_axis.pad_vdelim > strlen(CREATE_SCR_AUTO_PORT_LABEL) ? create_axis.pad_vdelim : strlen(CREATE_SCR_AUTO_PORT_LABEL);
        create_axis.pad_vdelim = create_axis.pad_vdelim > create_axis.btns_x[1] - 1 ? create_axis.pad_vdelim : create_axis.btns_x[1] - 2 - GFX_ELEM_HOFF;
    }

    return ret;
}

void init_graphics()
{
    _set_labels();

    _set_dimensions();

    _set_axis();

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

int _draw_window(int wnd_type)
{
    int index;
    int tmp = 0;
    int ret = EXIT_SUCCESS;

    switch(wnd_type)
    {
        case WND_NONE:
            ret = EXIT_FAILURE;
            break;
        case WND_MAIN_MENU:
            {
                if (wnd_type == cur_wnd)
                {
                    delwin(note_sw);
                    delwin(note_w);
                    delwin(main_menu.panel_sw);
                    delwin(main_menu.panel_w);
                    for (index = 0; index < 4; ++index)
                    {
                        delwin(main_menu.btns[index].wnd);
                    }
                    delwin(main_menu.btns_border);
                    delwin(main_menu.status_sw);
                    delwin(main_menu.status_w);
                    delwin(main_menu.header_w);
                    delwin(wnd);

                    wnd = NULL;
                    main_menu.header_w = NULL;
                    main_menu.status_w = NULL;
                    main_menu.status_sw = NULL;
                    main_menu.btns_border = NULL;
                    for (index = 0; index < 4; ++index)
                    {
                        main_menu.btns[index].wnd = NULL;
                    }
                    main_menu.panel_w = NULL;
                    main_menu.panel_sw = NULL;
                    note_w = NULL;
                    note_sw = NULL;
                }
                else
                {
                    main_menu.btns[0].lbl = malloc(strlen(MENU_SCR_CLT_BTN_LABEL)+1);
                    main_menu.btns[1].lbl = malloc(strlen(MENU_SCR_SRV_BTN_LABEL)+1);
                    main_menu.btns[2].lbl = malloc(strlen(MENU_SCR_CFG_BTN_LABEL)+1);
                    main_menu.btns[3].lbl = malloc(strlen(MENU_SCR_QUIT_BTN_LABEL)+1);

                    strcpy(main_menu.btns[0].lbl, MENU_SCR_CLT_BTN_LABEL);
                    strcpy(main_menu.btns[1].lbl, MENU_SCR_SRV_BTN_LABEL);
                    strcpy(main_menu.btns[2].lbl, MENU_SCR_CFG_BTN_LABEL);
                    strcpy(main_menu.btns[3].lbl, MENU_SCR_QUIT_BTN_LABEL);

                    cur_wnd = WND_MAIN_MENU;
                }

                wnd = newwin(global_dims.wnd_h, global_dims.wnd_w, global_axis.wnd_y, global_axis.wnd_x);

                main_menu.header_w = derwin(wnd, mmenu_dims.header_h, mmenu_dims.header_w, mmenu_axis.header_y, mmenu_axis.header_x);
                main_menu.status_w = derwin(wnd, mmenu_dims.status_h, mmenu_dims.status_w, mmenu_axis.status_y, mmenu_axis.status_x);
                main_menu.btns_border = derwin(wnd, mmenu_dims.btns_border_h, mmenu_dims.btns_border_w, mmenu_axis.btns_border_y, mmenu_axis.btns_border_x);
                main_menu.panel_w = derwin(wnd, mmenu_dims.panel_h, mmenu_dims.panel_w, mmenu_axis.panel_y, mmenu_axis.panel_x);
                main_menu.panel_sw = derwin(main_menu.panel_w, mmenu_dims.panel_h-2, mmenu_dims.panel_w-2, 1, 1);
                
                main_menu.status_sw = derwin(main_menu.status_w, mmenu_dims.status_h - 2, mmenu_dims.status_w - 2, 1, 1);
                main_menu.btns[0].wnd = derwin(main_menu.btns_border, global_dims.elem_h, global_dims.elem_w, mmenu_axis.btns_y[0], mmenu_axis.btns_x[0]);
                main_menu.btns[1].wnd = derwin(main_menu.btns_border, global_dims.elem_h, global_dims.elem_w, mmenu_axis.btns_y[1], mmenu_axis.btns_x[1]);
                main_menu.btns[2].wnd = derwin(main_menu.btns_border, global_dims.elem_h, global_dims.elem_w, mmenu_axis.btns_y[2], mmenu_axis.btns_x[2]);
                main_menu.btns[3].wnd = derwin(main_menu.btns_border, global_dims.elem_h, global_dims.elem_w, mmenu_axis.btns_y[3], mmenu_axis.btns_x[3]);

                note_w = newwin(global_dims.note_h, global_dims.note_w, global_axis.note_y, global_axis.note_x);
                note_sw = derwin(note_w, global_dims.note_h-2, global_dims.note_w-2, 1, 1);

                box(wnd, ACS_VLINE, ACS_HLINE);
                box(main_menu.status_w, ' ', ' ');
                box(main_menu.btns_border, ACS_VLINE, ACS_HLINE);
                box(main_menu.btns[0].wnd, ACS_VLINE, ACS_HLINE);
                box(main_menu.btns[1].wnd, ACS_VLINE, ACS_HLINE);
                box(main_menu.btns[2].wnd, ACS_VLINE, ACS_HLINE);
                box(main_menu.btns[3].wnd, ACS_VLINE, ACS_HLINE);
                box(main_menu.panel_w, ACS_VLINE, ACS_HLINE);
                box(note_w, ' ', ' ');

                wmove(wnd, 0, (global_dims.wnd_w/2)-strlen(MENU_SCR_LABEL)/2);
                wprintw(wnd, MENU_SCR_LABEL);

                wprintw(main_menu.header_w,
                        "__          __  _\n" \
                        "\\ \\        / / | |\n" \
                        " \\ \\  /\\  / /__| | ___ ___  _ __ ___   ___ \n" \
                        "  \\ \\/  \\/ / _ \\ |/ __/ _ \\| '_ ` _ \\ / _ \\\n" \
                        "   \\  /\\  /  __/ | (_| (_) | | | | | |  __/\n" \
                        "    \\/  \\/ \\___|_|\\___\\___/|_| |_| |_|\\___|");

                switch (connection_flag)
                {
                case STATUS_DISCONNECTED:
                    wattron(main_menu.status_sw, COLOR_PAIR(4));
                    waddch(main_menu.status_sw, ACS_DIAMOND);
                    wattroff(main_menu.status_sw, COLOR_PAIR(4));
                    wprintw(main_menu.status_sw, " Status: ");
                    wattron(main_menu.status_sw, COLOR_PAIR(4));
                    wprintw(main_menu.status_sw, "Disconnected");
                    wattroff(main_menu.status_sw, COLOR_PAIR(4));

                    wmove(note_sw, 0, ((global_dims.note_w-2)/2)-strlen(MENU_SCR_NOTE_DISCONNECTED)/2);
                    wprintw(note_sw, MENU_SCR_NOTE_DISCONNECTED);
                    break;
                case STATUS_CONNECTED:
                    wattron(main_menu.status_sw, COLOR_PAIR(5));
                    waddch(main_menu.status_sw, ACS_DIAMOND);
                    wattroff(main_menu.status_sw, COLOR_PAIR(5));
                    wprintw(main_menu.status_sw, " Status: ");
                    wattron(main_menu.status_sw, COLOR_PAIR(5));
                    wprintw(main_menu.status_sw, "Connected");
                    wattroff(main_menu.status_sw, COLOR_PAIR(5));

                    wmove(note_sw, 0, ((global_dims.note_w-2)/2)-strlen(MENU_SCR_NOTE_DISCONNECTED)/2);
                    wprintw(note_sw, MENU_SCR_NOTE_CONNECTED);
                    break;
                case STATUS_CONNECTING:
                    wattron(main_menu.status_sw, COLOR_PAIR(3));
                    waddch(main_menu.status_sw, ACS_DIAMOND);
                    wattroff(main_menu.status_sw, COLOR_PAIR(3));
                    wprintw(main_menu.status_sw, " Status: ");
                    wattron(main_menu.status_sw, A_BLINK | COLOR_PAIR(3));
                    wprintw(main_menu.status_sw, "Connecting");
                    wattroff(main_menu.status_sw, A_BLINK | COLOR_PAIR(3));

                    wmove(note_sw, 0, ((global_dims.note_w-2)/2)-strlen(MENU_SCR_NOTE_DISCONNECTED)/2);
                    wprintw(note_sw, MENU_SCR_NOTE_DISCONNECTED);
                    break;
                default:
                    break;
                }

                wprintw(main_menu.panel_sw, MENU_SCR_NOTE_LABEL"\n");
                {
                wprintw(main_menu.panel_sw, "1: ");
                waddch(main_menu.panel_sw, ACS_BBSS);
                wprintw(main_menu.panel_sw, " 2: ");
                waddch(main_menu.panel_sw, ACS_BLOCK);
                wprintw(main_menu.panel_sw, " 3: ");
                waddch(main_menu.panel_sw, ACS_BOARD);
                wprintw(main_menu.panel_sw, " 4: ");
                waddch(main_menu.panel_sw, ACS_BSBS);
                wprintw(main_menu.panel_sw, " 5: ");
                waddch(main_menu.panel_sw, ACS_BSSB);
                wprintw(main_menu.panel_sw, "\n6: ");
                waddch(main_menu.panel_sw, ACS_BSSS);
                wprintw(main_menu.panel_sw, " 7: ");
                waddch(main_menu.panel_sw, ACS_BTEE);
                wprintw(main_menu.panel_sw, " 8: ");
                waddch(main_menu.panel_sw, ACS_BULLET);
                wprintw(main_menu.panel_sw, " 9: ");
                waddch(main_menu.panel_sw, ACS_CKBOARD);
                wprintw(main_menu.panel_sw, " 10: ");
                waddch(main_menu.panel_sw, ACS_DARROW);
                wprintw(main_menu.panel_sw, "\n11: ");
                waddch(main_menu.panel_sw, ACS_DEGREE);
                wprintw(main_menu.panel_sw, " 12: ");
                waddch(main_menu.panel_sw, ACS_DIAMOND);
                wprintw(main_menu.panel_sw, " 13: ");
                waddch(main_menu.panel_sw, ACS_GEQUAL);
                wprintw(main_menu.panel_sw, " 14: ");
                waddch(main_menu.panel_sw, ACS_HLINE);
                wprintw(main_menu.panel_sw, " 15: ");
                waddch(main_menu.panel_sw, ACS_LANTERN);
                wprintw(main_menu.panel_sw, "\n16: ");
                waddch(main_menu.panel_sw, ACS_LARROW);
                wprintw(main_menu.panel_sw, " 17: ");
                waddch(main_menu.panel_sw, ACS_LEQUAL);
                wprintw(main_menu.panel_sw, " 18: ");
                waddch(main_menu.panel_sw, ACS_LLCORNER);
                wprintw(main_menu.panel_sw, " 19: ");
                waddch(main_menu.panel_sw, ACS_LRCORNER);
                wprintw(main_menu.panel_sw, " 20: ");
                waddch(main_menu.panel_sw, ACS_LTEE);
                wprintw(main_menu.panel_sw, "\n21: ");
                waddch(main_menu.panel_sw, ACS_NEQUAL);
                wprintw(main_menu.panel_sw, " 22: ");
                waddch(main_menu.panel_sw, ACS_PI);
                wprintw(main_menu.panel_sw, " 23: ");
                waddch(main_menu.panel_sw, ACS_PLMINUS);
                wprintw(main_menu.panel_sw, " 24: ");
                waddch(main_menu.panel_sw, ACS_PLUS);
                wprintw(main_menu.panel_sw, " 25: ");
                waddch(main_menu.panel_sw, ACS_RARROW);
                wprintw(main_menu.panel_sw, "\n26: ");
                waddch(main_menu.panel_sw, ACS_RTEE);
                wprintw(main_menu.panel_sw, " 27: ");
                waddch(main_menu.panel_sw, ACS_S1);
                wprintw(main_menu.panel_sw, " 28: ");
                waddch(main_menu.panel_sw, ACS_S3);
                wprintw(main_menu.panel_sw, " 29: ");
                waddch(main_menu.panel_sw, ACS_S7);
                wprintw(main_menu.panel_sw, " 30: ");
                waddch(main_menu.panel_sw, ACS_S9);
                wprintw(main_menu.panel_sw, "\n31: ");
                waddch(main_menu.panel_sw, ACS_SBBS);
                wprintw(main_menu.panel_sw, " 32: ");
                waddch(main_menu.panel_sw, ACS_SBSB);
                wprintw(main_menu.panel_sw, " 33: ");
                waddch(main_menu.panel_sw, ACS_SBSS);
                wprintw(main_menu.panel_sw, " 34: ");
                waddch(main_menu.panel_sw, ACS_SSBB);
                wprintw(main_menu.panel_sw, " 35: ");
                waddch(main_menu.panel_sw, ACS_SSBS);
                wprintw(main_menu.panel_sw, "\n36: ");
                waddch(main_menu.panel_sw, ACS_SSSB);
                wprintw(main_menu.panel_sw, " 37: ");
                waddch(main_menu.panel_sw, ACS_SSSS);
                wprintw(main_menu.panel_sw, " 38: ");
                waddch(main_menu.panel_sw, ACS_STERLING);
                wprintw(main_menu.panel_sw, " 39: ");
                waddch(main_menu.panel_sw, ACS_TTEE);
                wprintw(main_menu.panel_sw, " 40: ");
                waddch(main_menu.panel_sw, ACS_UARROW);
                wprintw(main_menu.panel_sw, "\n41: ");
                waddch(main_menu.panel_sw, ACS_ULCORNER);
                wprintw(main_menu.panel_sw, " 42: ");
                waddch(main_menu.panel_sw, ACS_URCORNER);
                wprintw(main_menu.panel_sw, " 43: ");
                waddch(main_menu.panel_sw, ACS_VLINE);
                }

                for (index = 0; index < 4; ++index)
                {
                    wmove(main_menu.btns[index].wnd, 1, (mmenu_dims.btns_w/2)-strlen(main_menu.btns[index].lbl)/2);
                    if (index == main_menu.selection)
                    {
                        wattron(main_menu.btns[index].wnd, A_BOLD | A_UNDERLINE);
                        wprintw(main_menu.btns[index].wnd, "%s", main_menu.btns[index].lbl);
                        wattroff(main_menu.btns[index].wnd, A_BOLD | A_UNDERLINE);
                    }
                    else
                        wprintw(main_menu.btns[index].wnd, "%s", main_menu.btns[index].lbl);
                }

                _update_window();
            }
            break;
        case WND_JOIN_SRV:
            {
                if (wnd_type == cur_wnd)
                {
                    delwin(note_sw);
                    delwin(note_w);
                    for (index = 0; index < 3; ++index)
                    {
                        delwin(join_srv.btns[index].wnd);
                    }
                    delwin(join_srv.btns_border);
                    delwin(join_srv.tb_w);
                    delwin(join_srv.label_w);
                    delwin(join_srv.caddr_w);
                    delwin(join_srv.servers_pad);
                    delwin(join_srv.pad_border);
                    delwin(join_srv.susers_w);
                    delwin(join_srv.saddr_w);
                    delwin(join_srv.sname_w);
                    delwin(join_srv.top_panel);
                    delwin(wnd);

                    wnd = NULL;
                    join_srv.top_panel = NULL;
                    join_srv.saddr_w = NULL;
                    join_srv.susers_w = NULL;
                    join_srv.pad_border = NULL;
                    join_srv.servers_pad = NULL;
                    join_srv.caddr_w = NULL;
                    join_srv.label_w = NULL;
                    join_srv.tb_w = NULL;
                    join_srv.btns_border = NULL;
                    for (index = 0; index < 4; ++index)
                    {
                        join_srv.btns[index].wnd = NULL;
                    }
                    note_w = NULL;
                    note_sw = NULL;
                }
                else
                {
                    join_srv.btns[0].lbl = malloc(strlen(JOIN_SCR_JOIN_BTN_LABEL)+1);
                    join_srv.btns[1].lbl = malloc(strlen(JOIN_SCR_REFRESH_BTN_LABEL)+1);
                    join_srv.btns[2].lbl = malloc(strlen(JOIN_SCR_CLEAR_BTN_LABEL)+1);

                    strcpy(join_srv.btns[0].lbl, JOIN_SCR_JOIN_BTN_LABEL);
                    strcpy(join_srv.btns[1].lbl, JOIN_SCR_REFRESH_BTN_LABEL);
                    strcpy(join_srv.btns[2].lbl, JOIN_SCR_CLEAR_BTN_LABEL);

                    cur_wnd = WND_JOIN_SRV;
                }

                wnd = newwin(global_dims.wnd_h, global_dims.wnd_w, global_axis.wnd_y, global_axis.wnd_x);

                join_srv.top_panel = derwin(wnd, join_dims.sinfo_h, join_dims.sinfo_w, join_axis.top_panel_y, join_axis.top_panel_x);
                join_srv.pad_border = derwin(wnd, join_dims.pad_border_h, join_dims.pad_border_w, join_axis.pad_border_y, join_axis.pad_border_x);
                join_srv.caddr_w = derwin(wnd, join_dims.caddr_h, join_dims.caddr_w, join_axis.caddr_y, join_axis.caddr_x);
                join_srv.btns_border = derwin(wnd, join_dims.btns_border_h, join_dims.btns_border_w, join_axis.btns_border_y, join_axis.btns_border_x);

                join_srv.sname_w = derwin(join_srv.top_panel, join_dims.sname_h, join_dims.sname_w, join_axis.sname_y, join_axis.sname_x);
                join_srv.saddr_w = derwin(join_srv.top_panel, join_dims.saddr_h, join_dims.saddr_w, join_axis.saddr_y, join_axis.saddr_x);
                join_srv.susers_w = derwin(join_srv.top_panel, join_dims.susers_h, join_dims.susers_w, join_axis.susers_y, join_axis.susers_x);
                join_srv.servers_pad = newpad(join_dims.pad_h, join_dims.pad_w);
                join_srv.label_w = derwin(join_srv.caddr_w, join_dims.lbl_h, join_dims.lbl_w, join_axis.lbl_y, join_axis.lbl_x);
                join_srv.tb_w = derwin(join_srv.caddr_w, join_dims.tb_h, join_dims.tb_w, join_axis.tb_y, join_axis.tb_x);
                join_srv.btns[0].wnd = derwin(join_srv.btns_border, join_dims.btns_h, join_dims.btns_w, join_axis.btns_y[0], join_axis.btns_x[0]);
                join_srv.btns[1].wnd = derwin(join_srv.btns_border, join_dims.btns_h, join_dims.btns_w, join_axis.btns_y[1], join_axis.btns_x[1]);
                join_srv.btns[2].wnd = derwin(join_srv.btns_border, join_dims.btns_h, join_dims.btns_w, join_axis.btns_y[2], join_axis.btns_x[2]);

                note_w = newwin(global_dims.note_h, global_dims.note_w, global_axis.note_y, global_axis.note_x);
                note_sw = derwin(note_w, global_dims.note_h-2, global_dims.note_w-2, 1, 1);

                box(wnd, ACS_VLINE, ACS_HLINE);
                box(join_srv.top_panel, ACS_VLINE, ACS_HLINE);
                box(join_srv.pad_border, ACS_VLINE, ACS_HLINE);
                box(join_srv.caddr_w, ACS_VLINE, ACS_HLINE);
                box(join_srv.btns_border, ACS_VLINE, ACS_HLINE);
                box(note_w, ' ', ' ');

                wmove(wnd, 0, (global_dims.wnd_w/2)-strlen(JOIN_SCR_LABEL)/2);
                wprintw(wnd, JOIN_SCR_LABEL);
                wprintw(join_srv.sname_w, JOIN_SCR_SRV_NAME_LABEL);
                wprintw(join_srv.saddr_w, JOIN_SCR_SRV_ADDR_LABEL);
                wprintw(join_srv.susers_w, JOIN_SCR_CONN_USERS_LABEL);
                wprintw(join_srv.label_w, JOIN_SCR_MAN_ADDR_LABEL);
                for (index = 0; index < 3; ++index)
                {
                    wmove(join_srv.btns[index].wnd, 0, (join_dims.btns_w/2)-strlen(join_srv.btns[index].lbl)/2);
                    if (index == join_srv.selection)
                    {
                        wattron(join_srv.btns[index].wnd, A_BOLD | A_UNDERLINE);
                        wprintw(join_srv.btns[index].wnd, "%s", join_srv.btns[index].lbl);
                        wattroff(join_srv.btns[index].wnd, A_BOLD | A_UNDERLINE);
                    }
                    else
                        wprintw(join_srv.btns[index].wnd, "%s", join_srv.btns[index].lbl);
                }
                wmove(note_sw, 0, ((global_dims.note_w-2)/2)-join_note_size/2);
                wprintw(note_sw, join_note_label);


                wmove(join_srv.top_panel, 0, join_axis.saddr_x-1);
                waddch(join_srv.top_panel, ACS_BSSS);
                wmove(join_srv.top_panel, 0, join_axis.susers_x-1);
                waddch(join_srv.top_panel, ACS_BSSS);

                wmove(join_srv.top_panel, 1, join_axis.saddr_x-1);
                wvline(join_srv.top_panel, ACS_VLINE, join_dims.sname_h);
                wmove(join_srv.top_panel, 1, join_axis.susers_x-1);
                wvline(join_srv.top_panel, ACS_VLINE, join_dims.sname_h);

                wmove(join_srv.pad_border, 0, 0);
                waddch(join_srv.pad_border, ACS_LTEE);
                wmove(join_srv.pad_border, 0, join_axis.saddr_x-1);
                waddch(join_srv.pad_border, ACS_PLUS);
                wmove(join_srv.pad_border, 0, join_axis.susers_x-1);
                waddch(join_srv.pad_border, ACS_PLUS);
                wmove(join_srv.pad_border, 0, join_dims.pad_border_w-1);
                waddch(join_srv.pad_border, ACS_RTEE);

                wmove(join_srv.pad_border, join_dims.pad_border_h-1, 0);
                waddch(join_srv.pad_border, ACS_LTEE);

                wmove(join_srv.caddr_w, 0, join_axis.tb_x-1);
                waddch(join_srv.caddr_w, ACS_BSSS);
                wmove(join_srv.caddr_w, 0, join_dims.caddr_w-1);
                waddch(join_srv.caddr_w, ACS_BSSS);
                wmove(join_srv.btns_border, 0, join_axis.btns_x[1]-1);
                waddch(join_srv.btns_border, ACS_BSSS);
                wmove(join_srv.btns_border, 0, join_axis.btns_x[2]-1);
                waddch(join_srv.btns_border, ACS_BSSS);
                wmove(join_srv.btns_border, 0, 0);
                waddch(join_srv.btns_border, ACS_BSSS);


                wmove(join_srv.servers_pad, 0, join_axis.saddr_x-2);
                wvline(join_srv.servers_pad, ACS_VLINE, join_dims.pad_h);
                wmove(join_srv.servers_pad, 0, join_axis.susers_x-2);
                wvline(join_srv.servers_pad, ACS_VLINE, join_dims.pad_h);
                wattron(join_srv.servers_pad, COLOR_PAIR(2));
                wmove(join_srv.servers_pad, join_srv.line, 0);
                whline(join_srv.servers_pad, ' ', join_dims.pad_w);
                wmove(join_srv.servers_pad, join_srv.line, 0);
                wprintw(join_srv.servers_pad, "%c", arr[join_srv.line]);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.susers_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);
                wattroff(join_srv.servers_pad, COLOR_PAIR(2));
                for (index = 1; index < sizeof(arr); index++)
                {
                    wmove(join_srv.servers_pad, index, 0);
                    whline(join_srv.servers_pad, ' ', join_dims.pad_w);
                    wmove(join_srv.servers_pad, index, 0);
                    wprintw(join_srv.servers_pad, "%c", arr[index]);
                    wmove(join_srv.servers_pad, index, join_axis.saddr_x-2);
                    waddch(join_srv.servers_pad, ACS_VLINE);
                    wmove(join_srv.servers_pad, index, join_axis.susers_x-2);
                    waddch(join_srv.servers_pad, ACS_VLINE);
                }


                tmp = join_axis.pad_border_x+join_axis.saddr_x-1;
                if (tmp == join_axis.caddr_x+join_dims.caddr_w-1)
                {
                    wmove(join_srv.pad_border, join_dims.pad_border_h-1, join_axis.saddr_x-1);
                    waddch(join_srv.pad_border, ACS_PLUS);
                }
                else
                {
                    
                    if (tmp == join_axis.btns_border_x)
                    {
                        wmove(join_srv.pad_border, join_dims.pad_border_h-1, join_axis.saddr_x-1);
                        waddch(join_srv.pad_border, ACS_PLUS);
                    }
                    else
                    {
                        if (tmp == join_axis.btns_border_x + join_axis.btns_x[1]-1)
                        {
                            wmove(join_srv.pad_border, join_dims.pad_border_h-1, join_axis.saddr_x-1);
                            waddch(join_srv.pad_border, ACS_PLUS);
                        }
                        else
                        {
                            if (tmp == join_axis.btns_border_x + join_axis.btns_x[2]-1)
                            {
                                wmove(join_srv.pad_border, join_dims.pad_border_h-1, join_axis.saddr_x-1);
                                waddch(join_srv.pad_border, ACS_PLUS);
                            }
                            else
                            {
                                wmove(join_srv.pad_border, join_dims.pad_border_h-1, join_axis.saddr_x-1);
                                waddch(join_srv.pad_border, ACS_BTEE);
                            }
                        }
                    }
                }

                tmp = (join_axis.pad_border_x+join_axis.susers_x-1);
                if ((join_axis.pad_border_x+join_axis.susers_x-1) == join_axis.btns_border_x)
                {
                    wmove(join_srv.pad_border, join_dims.pad_border_h-1, join_axis.susers_x-1);
                    waddch(join_srv.pad_border, ACS_PLUS);
                }
                else
                {
                    wmove(join_srv.pad_border, join_dims.pad_border_h-1, join_axis.susers_x-1);
                    waddch(join_srv.pad_border, ACS_BTEE);
                }

                wmove(join_srv.caddr_w, 0, join_dims.lbl_w+1);
                waddch(join_srv.caddr_w, ACS_BSSS);

                wmove(join_srv.pad_border, join_dims.pad_border_h-1, join_dims.pad_border_w-1);
                waddch(join_srv.pad_border, ACS_RTEE);

                wmove(join_srv.caddr_w, 1, join_dims.lbl_w+1);
                wvline(join_srv.caddr_w, ACS_VLINE, join_dims.tb_h);
                wmove(join_srv.btns_border, 1, join_axis.btns_x[1]-1);
                wvline(join_srv.btns_border, ACS_VLINE, join_dims.btns_h);
                wmove(join_srv.btns_border, 1, join_axis.btns_x[2]-1);
                wvline(join_srv.btns_border, ACS_VLINE, join_dims.btns_h);

                wmove(join_srv.caddr_w, join_dims.caddr_h-1, join_dims.lbl_w+1);
                waddch(join_srv.caddr_w, ACS_BTEE);
                if (join_axis.caddr_x+join_dims.caddr_w-1 == join_axis.btns_border_x)
                {
                    wmove(join_srv.btns_border, join_dims.btns_border_h-1, 0);
                    waddch(join_srv.btns_border, ACS_BTEE);
                }
                wmove(join_srv.btns_border, join_dims.btns_border_h-1, join_axis.btns_x[1]-1);
                waddch(join_srv.btns_border, ACS_BTEE);
                wmove(join_srv.btns_border, join_dims.btns_border_h-1, join_axis.btns_x[2]-1);
                waddch(join_srv.btns_border, ACS_BTEE);

                _update_window();
            }
            break;
        case WND_CREATE_SRV:
            {
                if (wnd_type == cur_wnd)
                {
                    delwin(note_sw);
                    delwin(note_w);

                    delwin(create_srv.auto_port_w);
                    delwin(create_srv.lcl_addr_w);
                    delwin(create_srv.port_w);
                    delwin(create_srv.addr_w);
                    delwin(create_srv.rusers_w);
                    delwin(create_srv.musers_w);
                    delwin(create_srv.sname_w);

                    delwin(create_srv.srv_info_sw);
                    for (index = 0; index < 3; ++index)
                    {
                        delwin(create_srv.btns[index].wnd);
                    }
                    delwin(create_srv.pad);

                    delwin(create_srv.srv_info_w);
                    delwin(create_srv.pad_border);
                    delwin(wnd);

                    wnd = NULL;

                    create_srv.pad_border = NULL;
                    create_srv.srv_info_w = NULL;
                    create_srv.pad = NULL;
                    
                    create_srv.sname_w = NULL;
                    create_srv.musers_w = NULL;
                    create_srv.rusers_w = NULL;
                    create_srv.addr_w = NULL;
                    create_srv.port_w = NULL;
                    create_srv.lcl_addr_w = NULL;
                    create_srv.auto_port_w = NULL;

                    for (index = 0; index < 3; ++index)
                    {
                        create_srv.btns[index].wnd = NULL;
                    }

                    note_w = NULL;
                    note_sw = NULL;
                }
                else
                {
                    create_srv.btns[0].lbl = malloc(strlen(CREATE_SCR_CREATE_BTN_LABEL)+1);
                    create_srv.btns[1].lbl = malloc(strlen(CREATE_SCR_DEFAULT_BTN_LABEL)+1);
                    create_srv.btns[2].lbl = malloc(strlen(CREATE_SCR_CLEAR_BTN_LABEL)+1);

                    strcpy(create_srv.btns[0].lbl, CREATE_SCR_CREATE_BTN_LABEL);
                    strcpy(create_srv.btns[1].lbl, CREATE_SCR_DEFAULT_BTN_LABEL);
                    strcpy(create_srv.btns[2].lbl, CREATE_SCR_CLEAR_BTN_LABEL);

                    cur_wnd = WND_CREATE_SRV;
                }

                wnd = newwin(global_dims.wnd_h, global_dims.wnd_w, global_axis.wnd_y, global_axis.wnd_x);
                
                create_srv.srv_info_w = derwin(wnd, create_dims.srv_info_h, create_dims.srv_info_w, create_axis.srv_info_y, create_axis.srv_info_x);

                create_srv.srv_info_sw = derwin(create_srv.srv_info_w, create_dims.srv_info_sw_h, create_dims.pad_elem_w, create_axis.srv_info_sw_y, create_axis.srv_info_sw_x);
                create_srv.pad_border = derwin(create_srv.srv_info_w, create_dims.pad_border_h, create_dims.pad_border_w, create_axis.pad_border_y, create_axis.pad_border_x);
                create_srv.btns[0].wnd = derwin(create_srv.srv_info_w, create_dims.btns_h, create_dims.btns_w, create_axis.btns_y[0], create_axis.btns_x[0]);
                create_srv.btns[1].wnd = derwin(create_srv.srv_info_w, create_dims.btns_h, create_dims.btns_w, create_axis.btns_y[1], create_axis.btns_x[1]);
                create_srv.btns[2].wnd = derwin(create_srv.srv_info_w, create_dims.btns_h, create_dims.btns_w, create_axis.btns_y[2], create_axis.btns_x[2]);

                create_srv.pad = newpad(create_dims.pad_h, create_dims.pad_w);

                create_srv.sname_w = subpad(create_srv.pad, create_dims.sname_h, create_dims.sname_w, create_axis.sname_y, create_axis.sname_x);
                create_srv.musers_w = subpad(create_srv.pad, create_dims.musers_h, create_dims.musers_w, create_axis.musers_y, create_axis.musers_x);
                create_srv.rusers_w = subpad(create_srv.pad, create_dims.rusers_h, create_dims.rusers_w, create_axis.rusers_y, create_axis.rusers_x);
                create_srv.addr_w = subpad(create_srv.pad, create_dims.saddr_h, create_dims.saddr_w, create_axis.saddr_y, create_axis.saddr_x);
                create_srv.port_w = subpad(create_srv.pad, create_dims.sport_h, create_dims.sport_w, create_axis.sport_y, create_axis.sport_x);
                create_srv.lcl_addr_w = subpad(create_srv.pad, create_dims.lcl_addr_h, create_dims.lcl_addr_w, create_axis.lcl_addr_y, create_axis.lcl_addr_x);
                create_srv.auto_port_w = subpad(create_srv.pad, create_dims.auto_port_h, create_dims.auto_port_w, create_axis.auto_port_y, create_axis.auto_port_x);

                note_w = newwin(global_dims.note_h, global_dims.note_w, global_axis.note_y, global_axis.note_x);
                note_sw = derwin(note_w, global_dims.note_h-2, global_dims.note_w-2, 1, 1);

                box(wnd, ACS_VLINE, ACS_HLINE);
                box(create_srv.srv_info_w, ACS_VLINE, ACS_HLINE);
                box(create_srv.pad_border, ACS_VLINE, ACS_HLINE);
                box(create_srv.btns[0].wnd, ACS_VLINE, ACS_HLINE);
                box(create_srv.btns[1].wnd, ACS_VLINE, ACS_HLINE);
                box(create_srv.btns[2].wnd, ACS_VLINE, ACS_HLINE);
                box(note_w, ' ', ' ');

                // wbkgd(create_srv.srv_info_w, COLOR_PAIR(2));
                // wbkgd(create_srv.srv_info_sw, COLOR_PAIR(2));
                // wbkgd(create_srv.pad_border, COLOR_PAIR(3));
                // wbkgd(create_srv.pad, COLOR_PAIR(2));

                wmove(wnd, 0, (global_dims.wnd_w/2)-strlen(CREATE_SCR_LABEL)/2);
                wprintw(wnd, CREATE_SCR_LABEL);
                wprintw(create_srv.srv_info_sw, CREATE_SCR_SRV_INFO_TEMPLATE, "Default name", "Not restricted", "127.0.0.1:27015");
                wprintw(create_srv.sname_w, CREATE_SCR_SRV_NAME_LABEL);
                wprintw(create_srv.musers_w, CREATE_SCR_CONN_USERS_LABEL);
                wprintw(create_srv.rusers_w, CREATE_SCR_RESTR_USERS_LABEL);
                wprintw(create_srv.addr_w, CREATE_SCR_SRV_ADDR_LABEL);
                wprintw(create_srv.port_w, CREATE_SCR_SRV_PORT_LABEL);
                wprintw(create_srv.lcl_addr_w, CREATE_SCR_LCL_ADDR_LABEL);
                wprintw(create_srv.auto_port_w, CREATE_SCR_AUTO_PORT_LABEL);
                for (index = 0; index < 3; ++index)
                {
                    wmove(create_srv.btns[index].wnd, 1, (create_dims.btns_w/2)-strlen(create_srv.btns[index].lbl)/2);
                    if (index == create_srv.selection)
                    {
                        wattron(create_srv.btns[index].wnd, A_BOLD | A_UNDERLINE);
                        wprintw(create_srv.btns[index].wnd, "%s", create_srv.btns[index].lbl);
                        wattroff(create_srv.btns[index].wnd, A_BOLD | A_UNDERLINE);
                    }
                    else
                        wprintw(create_srv.btns[index].wnd, "%s", create_srv.btns[index].lbl);
                }
                wmove(create_srv.btns[1].wnd, 0, 0);
                waddch(create_srv.btns[1].wnd, ACS_BSSS);
                wmove(create_srv.btns[1].wnd, create_dims.btns_h-1, 0);
                waddch(create_srv.btns[1].wnd, ACS_BTEE);
                wmove(create_srv.btns[2].wnd, 0, 0);
                waddch(create_srv.btns[2].wnd, ACS_BSSS);
                wmove(create_srv.btns[2].wnd, create_dims.btns_h-1, 0);
                waddch(create_srv.btns[2].wnd, ACS_BTEE);

                wmove(create_srv.pad_border, 0, create_axis.pad_vdelim+1);
                waddch(create_srv.pad_border, ACS_BSSS);
                wmove(create_srv.pad, 0, create_axis.pad_vdelim);
                wvline(create_srv.pad, ACS_VLINE, create_dims.pad_h);
                // for(index = create_dims.pad_elem_h; index < create_dims.pad_border_h; index+=2)
                // {
                //     wmove(create_srv.pad_border, index+create_dims.pad_elem_h, 0);
                //     waddch(create_srv.pad_border, ACS_LTEE);
                //     wmove(create_srv.pad_border, index+create_dims.pad_elem_h, create_dims.pad_border_w-1);
                //     waddch(create_srv.pad_border, ACS_RTEE);
                // }
                for(index = create_dims.pad_elem_h; index < create_dims.pad_h; index+=2)
                {
                    if (index < create_dims.pad_border_h-1)
                    {
                        wmove(create_srv.pad_border, index+create_dims.pad_elem_h, 0);
                        waddch(create_srv.pad_border, ACS_LTEE);
                    }
                    
                    wmove(create_srv.pad, index, 0);
                    whline(create_srv.pad, ACS_HLINE, create_dims.pad_w);
                    wmove(create_srv.pad, index, create_axis.pad_vdelim);
                    waddch(create_srv.pad,ACS_PLUS);
                    if (index < create_dims.pad_border_h-1)
                    {
                        wmove(create_srv.pad_border, index+create_dims.pad_elem_h, create_dims.pad_border_w-1);
                        waddch(create_srv.pad_border, ACS_RTEE);
                    }
                    
                }
                wmove(create_srv.pad_border, create_dims.pad_border_h-1, create_axis.pad_vdelim+1);
                waddch(create_srv.pad_border, ACS_BTEE);

                wmove(note_sw, 0, ((global_dims.note_w-2)/2)-strlen(CREATE_SCR_NOTE)/2);
                wprintw(note_sw, CREATE_SCR_NOTE);

                _update_window();
            }
            break;
        case WND_PREFS:
            break;
        default:
            ret = EXIT_FAILURE;
            break;
    }

    return ret;
}

int _update_window(void)
{
    int ret = EXIT_SUCCESS;

    switch (cur_wnd)
    {
        case WND_NONE:
            ret = EXIT_FAILURE;
            break;
        case WND_MAIN_MENU:
            {
                wnoutrefresh(wnd);
                wnoutrefresh(main_menu.header_w);
                wnoutrefresh(main_menu.status_w);
                wnoutrefresh(main_menu.btns_border);
                wnoutrefresh(main_menu.panel_w);
                wnoutrefresh(main_menu.panel_sw);
                wnoutrefresh(main_menu.status_sw);
                wnoutrefresh(main_menu.btns[0].wnd);
                wnoutrefresh(main_menu.btns[1].wnd);
                wnoutrefresh(main_menu.btns[2].wnd);
                wnoutrefresh(main_menu.btns[3].wnd);
                wnoutrefresh(note_w);
                wnoutrefresh(note_sw);
                wnoutrefresh(stdscr);
            }
            break;
        case WND_JOIN_SRV:
            {
                wnoutrefresh(wnd);
                wnoutrefresh(join_srv.top_panel);
                wnoutrefresh(join_srv.pad_border);
                wnoutrefresh(join_srv.caddr_w);
                wnoutrefresh(join_srv.sname_w);
                wnoutrefresh(join_srv.saddr_w);
                wnoutrefresh(join_srv.susers_w);
                wnoutrefresh(join_srv.label_w);
                wnoutrefresh(join_srv.tb_w);
                wnoutrefresh(join_srv.btns[0].wnd);
                wnoutrefresh(join_srv.btns[1].wnd);
                wnoutrefresh(join_srv.btns[2].wnd);
                wnoutrefresh(note_w);
                wnoutrefresh(note_sw);
                pnoutrefresh(join_srv.servers_pad, join_srv.vis_line, 0, join_axis.vis_pad_ys, join_axis.vis_pad_xs, join_axis.vis_pad_ye, join_axis.vis_pad_xe);
                wnoutrefresh(stdscr);
            }
            break;
        case WND_CREATE_SRV:
            {
                wnoutrefresh(wnd);
                wnoutrefresh(create_srv.srv_info_w);
                wnoutrefresh(create_srv.srv_info_sw);
                wnoutrefresh(create_srv.pad);
                wnoutrefresh(create_srv.sname_w);
                wnoutrefresh(create_srv.musers_w);
                wnoutrefresh(create_srv.rusers_w);
                wnoutrefresh(create_srv.addr_w);
                wnoutrefresh(create_srv.port_w);
                wnoutrefresh(create_srv.lcl_addr_w);
                wnoutrefresh(create_srv.auto_port_w);
                wnoutrefresh(create_srv.btns[0].wnd);
                wnoutrefresh(create_srv.btns[1].wnd);
                wnoutrefresh(create_srv.btns[2].wnd);
                wnoutrefresh(note_w);
                wnoutrefresh(note_sw);
                pnoutrefresh(create_srv.pad, create_srv.vis_line, 0, create_axis.vis_pad_ys, create_axis.vis_pad_xs, create_axis.vis_pad_ye, create_axis.vis_pad_xe);
                wnoutrefresh(stdscr);
            }
            break;
        case WND_PREFS:
            break;
        default:
            ret = EXIT_FAILURE;
            break;
    }
    
    doupdate();

    return ret;
}

int _delete_window(void)
{
    int index;
    int ret = EXIT_SUCCESS;

    switch (cur_wnd)
    {
        case WND_NONE:
            ret = EXIT_FAILURE;
            break;
        case WND_MAIN_MENU:
            {
                delwin(note_sw);
                delwin(note_w);
                delwin(main_menu.panel_sw);
                delwin(main_menu.panel_w);
                for (index = 0; index < 4; ++index)
                {
                    if (main_menu.btns[index].lbl != NULL)
                        free(main_menu.btns[index].lbl);
                    delwin(main_menu.btns[index].wnd);
                }
                delwin(main_menu.btns_border);
                delwin(main_menu.status_sw);
                delwin(main_menu.status_w);
                delwin(main_menu.header_w);
                delwin(wnd);

                wnd = NULL;
                main_menu.header_w = NULL;
                main_menu.status_w = NULL;
                main_menu.status_sw = NULL;
                main_menu.btns_border = NULL;
                for (index = 0; index < 4; ++index)
                {
                    main_menu.btns[index].wnd = NULL;
                    main_menu.btns[index].lbl = NULL;
                }
                main_menu.panel_w = NULL;
                main_menu.panel_sw = NULL;
                note_w = NULL;
                note_sw = NULL;

                main_menu.selection = 0;

                cur_wnd = WND_NONE;
            }
            break;
        case WND_JOIN_SRV:
            {
                delwin(note_sw);
                delwin(note_w);
                for (index = 0; index < 3; ++index)
                {
                    if (join_srv.btns[index].lbl != NULL)
                        free(join_srv.btns[index].lbl);
                    delwin(join_srv.btns[index].wnd);
                }
                delwin(join_srv.btns_border);
                delwin(join_srv.tb_w);
                delwin(join_srv.label_w);
                delwin(join_srv.caddr_w);
                delwin(join_srv.servers_pad);
                delwin(join_srv.pad_border);
                delwin(join_srv.susers_w);
                delwin(join_srv.saddr_w);
                delwin(join_srv.sname_w);
                delwin(join_srv.top_panel);
                delwin(wnd);

                wnd = NULL;
                join_srv.top_panel = NULL;
                join_srv.saddr_w = NULL;
                join_srv.susers_w = NULL;
                join_srv.pad_border = NULL;
                join_srv.servers_pad = NULL;
                join_srv.pad_border = NULL;
                join_srv.caddr_w = NULL;
                join_srv.label_w = NULL;
                join_srv.tb_w = NULL;
                join_srv.btns_border = NULL;
                for (index = 0; index < 4; ++index)
                {
                    join_srv.btns[index].wnd = NULL;
                    join_srv.btns[index].lbl = NULL;
                }
                note_w = NULL;
                note_sw = NULL;

                join_srv.vis_line = 0;
                join_srv.line = 0;
                join_srv.selection = 0;
                join_srv.mode = 0;

                cur_wnd = WND_NONE;
            }
            break;
        case WND_CREATE_SRV:
            {
                delwin(note_sw);
                delwin(note_w);

                delwin(create_srv.auto_port_w);
                delwin(create_srv.lcl_addr_w);
                delwin(create_srv.port_w);
                delwin(create_srv.addr_w);
                delwin(create_srv.rusers_w);
                delwin(create_srv.musers_w);
                delwin(create_srv.sname_w);

                delwin(create_srv.srv_info_sw);
                for (index = 0; index < 3; ++index)
                {
                    if (create_srv.btns[index].lbl != NULL)
                        free(create_srv.btns[index].lbl);
                    delwin(create_srv.btns[index].wnd);
                }
                delwin(create_srv.pad);

                delwin(create_srv.srv_info_w);
                delwin(create_srv.pad_border);
                delwin(wnd);

                wnd = NULL;

                create_srv.pad_border = NULL;
                create_srv.srv_info_w = NULL;
                create_srv.pad = NULL;
                        
                create_srv.sname_w = NULL;
                create_srv.musers_w = NULL;
                create_srv.rusers_w = NULL;
                create_srv.addr_w = NULL;
                create_srv.port_w = NULL;
                create_srv.lcl_addr_w = NULL;
                create_srv.auto_port_w = NULL;

                for (index = 0; index < 3; ++index)
                {
                    create_srv.btns[index].wnd = NULL;
                    create_srv.btns[index].lbl = NULL;
                }

                note_w = NULL;
                note_sw = NULL;

                create_srv.selection = 0;

                cur_wnd = WND_NONE;
            }
            break;
        case WND_PREFS:
            break;
        default:
            ret = EXIT_FAILURE;
            break;
    }

    return ret;
}

int popup_wnd(char *str, int type, ...)
{
    WINDOW *popup_w;
    WINDOW *popup_sw;

    int popup_w_w;
    int popup_w_h;
    int popup_sw_w;
    int popup_sw_h;
    int str_len = 0;
    va_list ap;
    int ret = EXIT_SUCCESS;

    if (str != NULL)
        str_len = strlen(str);
    else
        return EXIT_FAILURE;

    popup_sw_w = str_len + 12;
    popup_sw_h = 3;
    if (popup_sw_w > (size.ws_col-12))
    {
        int tmp = (size.ws_col-12) / popup_sw_w;
        popup_sw_w /= tmp;
        popup_sw_h += tmp;
    }
    if (popup_sw_h > (size.ws_row-2))
        return EXIT_FAILURE;

    popup_w_w = popup_sw_w + 2;
    popup_w_h = popup_sw_h + 2;

    va_start(ap, type);

    popup_w = newwin(popup_w_h, popup_w_w, (size.ws_row/2)-(popup_w_h/2), (size.ws_col/2)-(popup_w_w/2));
    popup_sw = derwin(popup_w, popup_sw_h, popup_sw_w, 1, 1);

    box(popup_w, ACS_VLINE, ACS_HLINE);

    if (popup_sw_h == 3)
        wmove(popup_sw, 0, ((popup_sw_w)-strlen(str))/2);
    wprintw(popup_sw, "%s", str);
    // wprintw(popup_sw, "%d|%d", popup_sw_h, popup_sw_w);

    wmove(popup_sw, popup_sw_h-1, ((popup_sw_w)/2));
    wattron(popup_sw, A_BLINK);
    waddch(popup_sw, ACS_DIAMOND);
    wattroff(popup_sw, A_BLINK);

    wnoutrefresh(popup_w);
    wnoutrefresh(popup_sw);
    doupdate();

    switch (type)
    {
        case POPUP_W_BLOCK:
            break;
        case POPUP_W_WAIT:
            sleep(1);
            break;
        case POPUP_W_CONFIRM:
            break;
        case POPUP_W_INFO:
            break;
        default:
            ret = EXIT_FAILURE;
            break;
    }

    delwin(popup_sw);
    delwin(popup_w);

    return ret;
}

int menu_wnd(int *option)
{
    int symbol;
    int index;
    int ret = 0;
    main_menu.selection = *option-1;

    _draw_window(WND_MAIN_MENU);

    keypad(wnd, true);
	while(ret != -1)
	{
		symbol = wgetch(wnd);

        if ('\t' == symbol)
		{
            wmove(main_menu.btns[main_menu.selection].wnd, 1, (mmenu_dims.btns_w/2)-strlen(main_menu.btns[main_menu.selection].lbl)/2);
            wprintw(main_menu.btns[main_menu.selection].wnd, "%s", main_menu.btns[main_menu.selection].lbl);

			if (main_menu.selection == 3)
                main_menu.selection = 0;
            else
                main_menu.selection++;

            wattron(main_menu.btns[main_menu.selection].wnd, A_BOLD | A_UNDERLINE);
            wmove(main_menu.btns[main_menu.selection].wnd, 1, (mmenu_dims.btns_w/2)-strlen(main_menu.btns[main_menu.selection].lbl)/2);
            wprintw(main_menu.btns[main_menu.selection].wnd, "%s", main_menu.btns[main_menu.selection].lbl);
            wattroff(main_menu.btns[main_menu.selection].wnd, A_BOLD | A_UNDERLINE);

            _update_window();
		}
		else if (KEY_UP == symbol)
		{
			if (main_menu.selection > 0)
            {
                wmove(main_menu.btns[main_menu.selection].wnd, 1, (mmenu_dims.btns_w/2)-strlen(main_menu.btns[main_menu.selection].lbl)/2);
                wprintw(main_menu.btns[main_menu.selection].wnd, "%s", main_menu.btns[main_menu.selection].lbl);

                main_menu.selection--;

                wattron(main_menu.btns[main_menu.selection].wnd, A_BOLD | A_UNDERLINE);
                wmove(main_menu.btns[main_menu.selection].wnd, 1, (mmenu_dims.btns_w/2)-strlen(main_menu.btns[main_menu.selection].lbl)/2);
                wprintw(main_menu.btns[main_menu.selection].wnd, "%s", main_menu.btns[main_menu.selection].lbl);
                wattroff(main_menu.btns[main_menu.selection].wnd, A_BOLD | A_UNDERLINE);

                _update_window();
            }
		}
		else if (KEY_DOWN == symbol)
		{
			if (main_menu.selection < 3)
            {
                wmove(main_menu.btns[main_menu.selection].wnd, 1, (mmenu_dims.btns_w/2)-strlen(main_menu.btns[main_menu.selection].lbl)/2);
                wprintw(main_menu.btns[main_menu.selection].wnd, "%s", main_menu.btns[main_menu.selection].lbl);

                main_menu.selection++;

                wattron(main_menu.btns[main_menu.selection].wnd, A_BOLD | A_UNDERLINE);
                wmove(main_menu.btns[main_menu.selection].wnd, 1, (mmenu_dims.btns_w/2)-strlen(main_menu.btns[main_menu.selection].lbl)/2);
                wprintw(main_menu.btns[main_menu.selection].wnd, "%s", main_menu.btns[main_menu.selection].lbl);
                wattroff(main_menu.btns[main_menu.selection].wnd, A_BOLD | A_UNDERLINE);

                _update_window();
            }
		}
		else if ('\n' == symbol)
		{
            switch (main_menu.selection)
            {
            case 0:
            case 1:
            case 2:
                *option = main_menu.selection+1;
                break;
            case 3:
                *option = 0;
                break;
            default:
                break;
            }

            break;
		}
        else if (KEY_F(2) == symbol)
		{
            if (connection_flag == STATUS_CONNECTED)
            {
                disconnect_from_main_server();
                connect_to_main_server();
                client_send(CONNECT_COMM, WAIT_TRUE, RECV_TIMEOUT);
            }
            else
            {
                connect_to_main_server();
                client_send(CONNECT_COMM, WAIT_TRUE, RECV_TIMEOUT);
            }
		}
        else if (KEY_F(3) == symbol) // TODO: Fix ENOTSOCK from recv() call in net_thread after pressing F3
		{
            if (connection_flag == STATUS_CONNECTED)
            {
                disconnect_from_main_server();
            }
            
		}
		/* If F4 is pressed -> exit application */
		else if (KEY_F(4) == symbol)
		{
            *option = 0;
			break;
		}
	}
    _delete_window();

    return ret;
}

int join_srv_wnd(int *option)
{
    int symbol;
    int index;
    int ret = 0;
    join_srv.mode = MODE_LIST;
    join_srv.selection = *option-1;

    _draw_window(WND_JOIN_SRV);
    
// scrollok(mainpad, true);
                
    keypad(wnd, true);
    while(1)
    {
        symbol = wgetch(wnd);
        if ('\n' == symbol) 
        {
            // WINDOW * sub_pad = newwin(10,40, global_dims.wnd_h/2, global_dims.wnd_w/2);
            // box(sub_pad, ACS_VLINE, ACS_HLINE);
            // wmove(sub_pad, 1, 1);
            // wbkgd(sub_pad, COLOR_PAIR(2));
            // wprintw(sub_pad, "%c", arr[join_srv.line]);
            
            // wrefresh(sub_pad);
            // wgetch(wnd);
            // delwin(sub_pad);
            char ch = arr[join_srv.line];
            popup_wnd(&ch, POPUP_W_WAIT);
            _update_window();
        }
        else if ('\t' == symbol)
        {
            switch (join_srv.mode)
            {
                case MODE_LIST:
                    wmove(join_srv.servers_pad, join_srv.line, 0);
                    whline(join_srv.servers_pad, ' ', join_dims.pad_w);
                    wmove(join_srv.servers_pad, join_srv.line, 0);
                    wprintw(join_srv.servers_pad, "%c", arr[join_srv.line]);
                    wmove(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-2);
                    waddch(join_srv.servers_pad, ACS_VLINE);
                    wmove(join_srv.servers_pad, join_srv.line, join_axis.susers_x-2);
                    waddch(join_srv.servers_pad, ACS_VLINE);

                    join_srv.mode = MODE_TEXTBOX;
                    
                    move(global_axis.wnd_y+join_axis.caddr_y+join_axis.tb_y, global_axis.wnd_x+join_axis.caddr_x+join_axis.tb_x);
                    curs_set(1);
                    _update_window();
                    break;
                case MODE_TEXTBOX:
                    curs_set(0);

                    join_srv.mode = MODE_LIST;

                    wattron(join_srv.servers_pad, COLOR_PAIR(2));
                    wmove(join_srv.servers_pad, join_srv.line, 0);
                    whline(join_srv.servers_pad, ' ', join_dims.pad_w);
                    wmove(join_srv.servers_pad, join_srv.line, 0);
                    wprintw(join_srv.servers_pad, "%c", arr[join_srv.line]);
                    wmove(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-2);
                    waddch(join_srv.servers_pad, ACS_VLINE);
                    wmove(join_srv.servers_pad, join_srv.line, join_axis.susers_x-2);
                    waddch(join_srv.servers_pad, ACS_VLINE);
                    wattroff(join_srv.servers_pad, COLOR_PAIR(2));
                    _update_window();
                    break;
                default:
                    break;
            }
            
        }
        else if (KEY_UP == symbol)
        {
            if (join_srv.line > 0)
            {
                wmove(join_srv.servers_pad, join_srv.line, 0);
                whline(join_srv.servers_pad, ' ', join_dims.pad_w);
                wmove(join_srv.servers_pad, join_srv.line, 0);
                wprintw(join_srv.servers_pad, "%c", arr[join_srv.line]);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.susers_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);

                join_srv.line--;

                wattron(join_srv.servers_pad, COLOR_PAIR(2));
                wmove(join_srv.servers_pad, join_srv.line, 0);
                whline(join_srv.servers_pad, ' ', join_dims.pad_w);
                wmove(join_srv.servers_pad, join_srv.line, 0);
                wprintw(join_srv.servers_pad, "%c", arr[join_srv.line]);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.susers_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);
                wattroff(join_srv.servers_pad, COLOR_PAIR(2));

                if (join_srv.line <= join_srv.vis_line)
                    join_srv.vis_line--;

                _update_window();
            }
        }
        else if (KEY_LEFT == symbol)
		{
			if (join_srv.selection > 0)
            {
                wmove(join_srv.btns[join_srv.selection].wnd, 0, (join_dims.btns_w/2)-strlen(join_srv.btns[join_srv.selection].lbl)/2);
                wprintw(join_srv.btns[join_srv.selection].wnd, "%s", join_srv.btns[join_srv.selection].lbl);

                join_srv.selection--;

                wattron(join_srv.btns[join_srv.selection].wnd, A_BOLD | A_UNDERLINE);
                wmove(join_srv.btns[join_srv.selection].wnd, 0, (join_dims.btns_w/2)-strlen(join_srv.btns[join_srv.selection].lbl)/2);
                wprintw(join_srv.btns[join_srv.selection].wnd, "%s", join_srv.btns[join_srv.selection].lbl);
                wattroff(join_srv.btns[join_srv.selection].wnd, A_BOLD | A_UNDERLINE);

                _update_window();
            }
		}
		else if (KEY_RIGHT == symbol)
		{
			if (join_srv.selection < 2)
            {
                wmove(join_srv.btns[join_srv.selection].wnd, 0, (join_dims.btns_w/2)-strlen(join_srv.btns[join_srv.selection].lbl)/2);
                wprintw(join_srv.btns[join_srv.selection].wnd, "%s", join_srv.btns[join_srv.selection].lbl);

                join_srv.selection++;

                wattron(join_srv.btns[join_srv.selection].wnd, A_BOLD | A_UNDERLINE);
                wmove(join_srv.btns[join_srv.selection].wnd, 0, (join_dims.btns_w/2)-strlen(join_srv.btns[join_srv.selection].lbl)/2);
                wprintw(join_srv.btns[join_srv.selection].wnd, "%s", join_srv.btns[join_srv.selection].lbl);
                wattroff(join_srv.btns[join_srv.selection].wnd, A_BOLD | A_UNDERLINE);

                _update_window();
            }
		}
        else if (KEY_DOWN == symbol)
        {
            if (join_srv.line < (sizeof(arr))-1)
            {
                wmove(join_srv.servers_pad, join_srv.line, 0);
                whline(join_srv.servers_pad, ' ', join_dims.pad_w);
                wmove(join_srv.servers_pad, join_srv.line, 0);
                wprintw(join_srv.servers_pad, "%c", arr[join_srv.line]);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.susers_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);

                join_srv.line++;

                wattron(join_srv.servers_pad, COLOR_PAIR(2));
                wmove(join_srv.servers_pad, join_srv.line, 0);
                whline(join_srv.servers_pad, ' ', join_dims.pad_w);
                wmove(join_srv.servers_pad, join_srv.line, 0);
                wprintw(join_srv.servers_pad, "%c", arr[join_srv.line]);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.susers_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);
                wattroff(join_srv.servers_pad, COLOR_PAIR(2));

                if ((join_srv.line + join_dims.pad_h) <= sizeof(arr))
                    join_srv.vis_line = join_srv.line;

                _update_window();
            }
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
        else
        {}
    }

    _delete_window();

    return ret;
}

int create_srv_wnd()
{
    int symbol;
    int index;
    int ret = EXIT_SUCCESS;
    create_srv.selection = 0;

    _draw_window(WND_CREATE_SRV);

    keypad(wnd, true);
    // TODO: Write the correct algorithm to process keypress on create server screen, add ys,xs,ye,xe for pad window for correct scrolling
    while(1)
    {
        symbol = wgetch(wnd);
        if ('\n' == symbol) 
        {
            _update_window();
        }
        else if (KEY_UP == symbol)
        {
            continue;
            if (join_srv.line > 0)
            {
                wmove(join_srv.servers_pad, join_srv.line, 0);
                whline(join_srv.servers_pad, ' ', join_dims.pad_w);
                wmove(join_srv.servers_pad, join_srv.line, 0);
                wprintw(join_srv.servers_pad, "%c", arr[join_srv.line]);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.susers_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);

                join_srv.line--;

                wattron(join_srv.servers_pad, COLOR_PAIR(2));
                wmove(join_srv.servers_pad, join_srv.line, 0);
                whline(join_srv.servers_pad, ' ', join_dims.pad_w);
                wmove(join_srv.servers_pad, join_srv.line, 0);
                wprintw(join_srv.servers_pad, "%c", arr[join_srv.line]);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.susers_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);
                wattroff(join_srv.servers_pad, COLOR_PAIR(2));

                if (join_srv.line <= join_srv.vis_line)
                    join_srv.vis_line--;

                _update_window();
            }
        }
        else if (KEY_LEFT == symbol)
		{
			if (create_srv.selection > 0)
            {
                wmove(create_srv.btns[create_srv.selection].wnd, 1, (create_dims.btns_w/2)-strlen(create_srv.btns[create_srv.selection].lbl)/2);
                wprintw(create_srv.btns[create_srv.selection].wnd, "%s", create_srv.btns[create_srv.selection].lbl);

                create_srv.selection--;

                wattron(create_srv.btns[create_srv.selection].wnd, A_BOLD | A_UNDERLINE);
                wmove(create_srv.btns[create_srv.selection].wnd, 1, (create_dims.btns_w/2)-strlen(create_srv.btns[create_srv.selection].lbl)/2);
                wprintw(create_srv.btns[create_srv.selection].wnd, "%s", create_srv.btns[create_srv.selection].lbl);
                wattroff(create_srv.btns[create_srv.selection].wnd, A_BOLD | A_UNDERLINE);

                _update_window();
            }
		}
		else if (KEY_RIGHT == symbol)
		{
			if (create_srv.selection < 2)
            {
                wmove(create_srv.btns[create_srv.selection].wnd, 1, (create_dims.btns_w/2)-strlen(create_srv.btns[create_srv.selection].lbl)/2);
                wprintw(create_srv.btns[create_srv.selection].wnd, "%s", create_srv.btns[create_srv.selection].lbl);

                create_srv.selection++;

                wattron(create_srv.btns[create_srv.selection].wnd, A_BOLD | A_UNDERLINE);
                wmove(create_srv.btns[create_srv.selection].wnd, 1, (create_dims.btns_w/2)-strlen(create_srv.btns[create_srv.selection].lbl)/2);
                wprintw(create_srv.btns[create_srv.selection].wnd, "%s", create_srv.btns[create_srv.selection].lbl);
                wattroff(create_srv.btns[create_srv.selection].wnd, A_BOLD | A_UNDERLINE);

                _update_window();
            }
		}
        else if (KEY_DOWN == symbol)
        {
            continue;
            if (join_srv.line < (sizeof(arr))-1)
            {
                wmove(join_srv.servers_pad, join_srv.line, 0);
                whline(join_srv.servers_pad, ' ', join_dims.pad_w);
                wmove(join_srv.servers_pad, join_srv.line, 0);
                wprintw(join_srv.servers_pad, "%c", arr[join_srv.line]);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.susers_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);

                join_srv.line++;

                wattron(join_srv.servers_pad, COLOR_PAIR(2));
                wmove(join_srv.servers_pad, join_srv.line, 0);
                whline(join_srv.servers_pad, ' ', join_dims.pad_w);
                wmove(join_srv.servers_pad, join_srv.line, 0);
                wprintw(join_srv.servers_pad, "%c", arr[join_srv.line]);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.susers_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);
                wattroff(join_srv.servers_pad, COLOR_PAIR(2));

                if ((join_srv.line + join_dims.pad_h) <= sizeof(arr))
                    join_srv.vis_line = join_srv.line;

                _update_window();
            }
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
        else
        {}
    }

    _delete_window();

    return ret;
}

int prefs_wnd()
{
    int symbol;
    int index;
    int ret = EXIT_SUCCESS;
    // create_srv.selection = 0;

    _draw_window(WND_PREFS);

    keypad(wnd, true);
    while(1)
    {
        symbol = wgetch(wnd);
        if ('\n' == symbol) 
        {
            _update_window();
        }
        else if (KEY_UP == symbol)
        {
            continue;
            if (join_srv.line > 0)
            {
                wmove(join_srv.servers_pad, join_srv.line, 0);
                whline(join_srv.servers_pad, ' ', join_dims.pad_w);
                wmove(join_srv.servers_pad, join_srv.line, 0);
                wprintw(join_srv.servers_pad, "%c", arr[join_srv.line]);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.susers_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);

                join_srv.line--;

                wattron(join_srv.servers_pad, COLOR_PAIR(2));
                wmove(join_srv.servers_pad, join_srv.line, 0);
                whline(join_srv.servers_pad, ' ', join_dims.pad_w);
                wmove(join_srv.servers_pad, join_srv.line, 0);
                wprintw(join_srv.servers_pad, "%c", arr[join_srv.line]);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.susers_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);
                wattroff(join_srv.servers_pad, COLOR_PAIR(2));

                if (join_srv.line <= join_srv.vis_line)
                    join_srv.vis_line--;

                _update_window();
            }
        }
        else if (KEY_LEFT == symbol)
		{
            continue;
			if (create_srv.selection > 0)
            {
                wmove(create_srv.btns[create_srv.selection].wnd, 1, (create_dims.btns_w/2)-strlen(create_srv.btns[create_srv.selection].lbl)/2);
                wprintw(create_srv.btns[create_srv.selection].wnd, "%s", create_srv.btns[create_srv.selection].lbl);

                create_srv.selection--;

                wattron(create_srv.btns[create_srv.selection].wnd, A_BOLD | A_UNDERLINE);
                wmove(create_srv.btns[create_srv.selection].wnd, 1, (create_dims.btns_w/2)-strlen(create_srv.btns[create_srv.selection].lbl)/2);
                wprintw(create_srv.btns[create_srv.selection].wnd, "%s", create_srv.btns[create_srv.selection].lbl);
                wattroff(create_srv.btns[create_srv.selection].wnd, A_BOLD | A_UNDERLINE);

                _update_window();
            }
		}
		else if (KEY_RIGHT == symbol)
		{
            continue;
			if (create_srv.selection < 2)
            {
                wmove(create_srv.btns[create_srv.selection].wnd, 1, (create_dims.btns_w/2)-strlen(create_srv.btns[create_srv.selection].lbl)/2);
                wprintw(create_srv.btns[create_srv.selection].wnd, "%s", create_srv.btns[create_srv.selection].lbl);

                create_srv.selection++;

                wattron(create_srv.btns[create_srv.selection].wnd, A_BOLD | A_UNDERLINE);
                wmove(create_srv.btns[create_srv.selection].wnd, 1, (create_dims.btns_w/2)-strlen(create_srv.btns[create_srv.selection].lbl)/2);
                wprintw(create_srv.btns[create_srv.selection].wnd, "%s", create_srv.btns[create_srv.selection].lbl);
                wattroff(create_srv.btns[create_srv.selection].wnd, A_BOLD | A_UNDERLINE);

                _update_window();
            }
		}
        else if (KEY_DOWN == symbol)
        {
            continue;
            if (join_srv.line < (sizeof(arr))-1)
            {
                wmove(join_srv.servers_pad, join_srv.line, 0);
                whline(join_srv.servers_pad, ' ', join_dims.pad_w);
                wmove(join_srv.servers_pad, join_srv.line, 0);
                wprintw(join_srv.servers_pad, "%c", arr[join_srv.line]);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.susers_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);

                join_srv.line++;

                wattron(join_srv.servers_pad, COLOR_PAIR(2));
                wmove(join_srv.servers_pad, join_srv.line, 0);
                whline(join_srv.servers_pad, ' ', join_dims.pad_w);
                wmove(join_srv.servers_pad, join_srv.line, 0);
                wprintw(join_srv.servers_pad, "%c", arr[join_srv.line]);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);
                wmove(join_srv.servers_pad, join_srv.line, join_axis.susers_x-2);
                waddch(join_srv.servers_pad, ACS_VLINE);
                wattroff(join_srv.servers_pad, COLOR_PAIR(2));

                if ((join_srv.line + join_dims.pad_h) <= sizeof(arr))
                    join_srv.vis_line = join_srv.line;

                _update_window();
            }
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
        else
        {}
    }

    _delete_window();

    return ret;
    // WINDOW *mainpad;
    // WINDOW *pad_border;
    // WINDOW *note_wnd;
    // WINDOW *note_subwnd;
    // int symbol;
    // int index;
    // int ret = 0;

    // char arr[] = {
    //     '1','2','3','4','5','6','7','8','9','0'//,'q','w','e','r','t','y','u','i','o','p','a','s','d','f','g','h','j','k','l',';'
    // };

    // int border_h = wnd_h-2-elem_h;
    // int border_w = wnd_w-2-(GFX_ELEM_HOFF*2);

    // int pad_h = border_h-2;
    // int pad_w = border_w-2;
    
    // int line = 0;
    // int vis_line = 0;

    // wnd = newwin(wnd_h, wnd_w, (size.ws_row/2)-(wnd_h/2), (size.ws_col/2)-(wnd_w/2));
    // pad_border = derwin(wnd, border_h, border_w, 1+GFX_ELEM_VOFF, 1+GFX_ELEM_HOFF);
    // mainpad = newpad((sizeof(arr)+1) > pad_h ? (sizeof(arr)+1) : pad_h, pad_w);
    // note_wnd = derwin(wnd, elem_h, border_w, wnd_h-1-elem_h-GFX_ELEM_VOFF, 1+GFX_ELEM_HOFF);
    // note_subwnd = derwin(note_wnd, 1, border_w-4, 1, 1);
    

    // box(wnd, ACS_VLINE, ACS_HLINE);
    // box(pad_border, ACS_VLINE, ACS_HLINE);
    // box(note_wnd, ' ', ' ');

    // wmove(wnd, 0, (wnd_w/2)-strlen(PREFS_SCR_LABEL)/2);
    // wprintw(wnd, PREFS_SCR_LABEL);
    // // wmove(wnd, subwnd_h-3, (wnd_w/2)-strlen(PREFS_SCR_NOTE)/2);
    // // wprintw(wnd, PREFS_SCR_NOTE);
    // wmove(note_subwnd, 0, ((border_w-4)/2)-(prefs_note_size/2));
    // wprintw(note_subwnd, "%s", prefs_note_label);

    // wmove(pad_border, 0, pad_w-pad_w/3);
    // waddch(pad_border, ACS_BSSS);
    // wmove(pad_border, (border_h-1), pad_w-pad_w/3);
    // waddch(pad_border, ACS_BTEE);

    // wattron(mainpad, COLOR_PAIR(2));
    // wmove(mainpad, 0, 0);
    // whline(mainpad, ' ', pad_w-pad_w/3-1);
    // wmove(mainpad, 0, 0);
    // wprintw(mainpad, "%c", arr[0]);
    // wattroff(mainpad, COLOR_PAIR(2));
    // wmove(mainpad, 0, pad_w-pad_w/3-1);
    // waddch(mainpad, ACS_VLINE);
    
    // // wattron(mainpad, A_STANDOUT);
    // // wprintw(mainpad, "%c", arr[0]);
    // // wattroff(mainpad, A_STANDOUT);
    // // wmove(mainpad, 0, pad_w-pad_w/3-1);
    // // wprintw(mainpad, "|");

    // for (index = 1; index < sizeof(arr); index++)
    // {
    //     // wmove(mainpad, i, 0);
    //     // wprintw(mainpad, "%c", arr[i]);
    //     wmove(mainpad, index, 0);
    //     whline(mainpad, ' ', pad_w-pad_w/3-1);
    //     wmove(mainpad, index, 0);
    //     wprintw(mainpad, "%c", arr[index]);
    //     wmove(mainpad, index, pad_w-pad_w/3-1);
    //     waddch(mainpad, ACS_VLINE);
    // }
    // for (; index < pad_h; index++)
    // {
    //     wmove(mainpad, index, pad_w-pad_w/3-1);
    //     waddch(mainpad, ACS_VLINE);
    // }

    // // for (index = 1; index < sizeof(arr); index++)
    // // {
    // //     wmove(mainpad, index, 0);
    // //     wprintw(mainpad, "%c", arr[index]);

    // //     wmove(mainpad, i, pad_w-pad_w/3-1);
    // //     wprintw(mainpad, "|");
    // // }

    // // wbkgd(mainpad, COLOR_PAIR(2));

    // wrefresh(wnd);
    // prefresh(mainpad, vis_line, 0, (size.ws_row/2)-(wnd_h/2)+2, (size.ws_col/2)-(wnd_w/2)+2+GFX_ELEM_HOFF, (size.ws_row/2)+(wnd_h/2)-5, size.ws_col);

    // scrollok(mainpad, true);
    // keypad(wnd, true);
    
    // while(1)
    // {
    //     symbol = wgetch(wnd);
    //     if ('\n' == symbol) 
    //     {
    //         WINDOW * sub_pad = newwin(10,40, wnd_h/2, wnd_w/2);
    //         box(sub_pad, ACS_VLINE, ACS_HLINE);
    //         wmove(sub_pad, 1, 1);
    //         wbkgd(sub_pad, COLOR_PAIR(2));
    //         wprintw(sub_pad, "%c", arr[line]);
    //         wrefresh(sub_pad);
    //         wgetch(wnd);
    //         delwin(sub_pad);
    //         wrefresh(wnd);
    //         prefresh(mainpad, vis_line, 0, (size.ws_row/2)-(wnd_h/2)+2, (size.ws_col/2)-(wnd_w/2)+2+GFX_ELEM_HOFF, (size.ws_row/2)+(wnd_h/2)-6, size.ws_col);
    //     }
    //     else if (KEY_F(1) == symbol)
	// 	{
    //         ret = 1;
	// 		break;
	// 	}
    //     else if (KEY_F(2) == symbol)
	// 	{
    //         ret = 2;
	// 		break;
	// 	}
    //     else if (KEY_F(3) == symbol)
	// 	{
    //         ret = 3;
	// 		break;
	// 	}
    //     else if (KEY_F(4) == symbol)
	// 	{
    //         ret = 0;
	// 		break;
	// 	}
    //     else if (KEY_UP == symbol)
	// 	{
    //         if (line > 0)
    //         {
    //             wmove(mainpad, line, 0);
    //             whline(mainpad, ' ', pad_w-pad_w/3-1);
    //             wmove(mainpad, line, 0);
    //             wprintw(mainpad, "%c", arr[line]);
    //             wmove(mainpad, line, pad_w-pad_w/3-1);
    //             waddch(mainpad, ACS_VLINE);

    //             line--;

    //             wattron(mainpad, COLOR_PAIR(2));
    //             wmove(mainpad, line, 0);
    //             whline(mainpad, ' ', pad_w-pad_w/3-1);
    //             wmove(mainpad, line, 0);
    //             wprintw(mainpad, "%c", arr[line]);
    //             wattroff(mainpad, COLOR_PAIR(2));
    //             wmove(mainpad, line, pad_w-pad_w/3-1);
    //             waddch(mainpad, ACS_VLINE);

    //             if (line <= vis_line)
    //                 vis_line--;

    //             prefresh(mainpad, vis_line, 0, (size.ws_row/2)-(wnd_h/2)+2, (size.ws_col/2)-(wnd_w/2)+2+GFX_ELEM_HOFF, (size.ws_row/2)+(wnd_h/2)-6, size.ws_col);
    //         }
	// 	}
	// 	/* if ARROW_KEY_DOWN is pressed -> navigate in the directory */
	// 	else if (KEY_DOWN == symbol)
	// 	{
    //         if (line < (sizeof(arr))-1)
    //         {
    //             wmove(mainpad, line, 0);
    //             whline(mainpad, ' ', pad_w-pad_w/3-1);
    //             wmove(mainpad, line, 0);
    //             wprintw(mainpad, "%c", arr[line]);
    //             wmove(mainpad, line, pad_w-pad_w/3-1);
    //             waddch(mainpad, ACS_VLINE);

    //             line++;

    //             wattron(mainpad, COLOR_PAIR(2));
    //             wmove(mainpad, line, 0);
    //             whline(mainpad, ' ', pad_w-pad_w/3-1);
    //             wmove(mainpad, line, 0);
    //             wprintw(mainpad, "%c", arr[line]);
    //             wattroff(mainpad, COLOR_PAIR(2));
    //             wmove(mainpad, line, pad_w-pad_w/3-1);
    //             waddch(mainpad, ACS_VLINE);

    //             if ((line + pad_h) <= sizeof(arr))
    //                 vis_line = line;
                
    //             prefresh(mainpad, vis_line, 0, (size.ws_row/2)-(wnd_h/2)+2, (size.ws_col/2)-(wnd_w/2)+2+GFX_ELEM_HOFF, (size.ws_row/2)+(wnd_h/2)-6, size.ws_col);
    //         }
	// 	}
    // }
    // delwin(mainpad);
    // delwin(wnd);

    // wnd = NULL;

    // return ret;
    return 0;
}
