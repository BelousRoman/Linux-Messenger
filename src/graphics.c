#include "../hdr/graphics.h"

struct server_info_t servers_info[] = {
    {"Sample server 1", 0, "192.168.0.1", 27015, 10, 5},
    {"Sample server 2", 0, "8.8.8.8", 7890, 100, 19},
    {"Sample local server", 0, "127.0.0.1", 9999, 0, 0}
};

// char arr[] = {
//     '1','2','3','4','5','6','7','8','9','0','q','w','e','r','t','y','u','i','o','p','a','s','d','f','g','h','j','k','l',';'
// };

/* Global variable, used to store terminal's size in columns and rows */
struct winsize size;

int cur_wnd = WND_NONE;

struct global_dims_t global_dims;
struct mmenu_dims_t mmenu_dims;
struct join_dims_t join_dims;
struct create_dims_t create_dims;
struct cfg_dims_t cfg_dims;

struct global_axis_t global_axis;
struct mmenu_axis_t mmenu_axis;
struct join_axis_t join_axis;
struct create_axis_t create_axis;
struct cfg_axis_t cfg_axis;

struct global_wnds_t global_wnds;
struct mmenu_wnd_t main_menu;
struct join_wnd_t join_srv;
struct create_wnd_t create_srv;
struct cfg_wnd_t cfg_wnd;

struct note_labels_t note_labels;

int _draw_window(int);
int _update_window(void);
int _delete_window(void);

/* Function to process signal, called on resizing terminal window */
void sig_winch(int signo)
{
	ioctl(fileno(stdout), TIOCGWINSZ, (char *) &size);
	resize_term(size.ws_row, size.ws_col);
    _set_dimensions();
    _set_axis();
    // wclear(curscr);
    // _delete_window();
    _draw_window(cur_wnd);
    // _update_window();
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

    // popup_wnd("handle msg", POPUP_W_WAIT);

    /* Read queue until an error occurs */
	while (ret = mq_receive(mqd, &msg, sizeof(int),
                        NULL) > 0)
	{
        // snprintf(ch, 20, "handle %d|%d|%d\0", mqd, msg, cur_wnd);
        snprintf(ch, 20, "connection flag: %d\0", connection_flag);
        popup_wnd(ch, POPUP_W_WAIT);
        if (global_wnds.wnd == NULL || main_menu.status_sw == NULL || global_wnds.note_sw == NULL)
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
                    wattron(main_menu.status_sw, COLOR_PAIR(4));
                    waddch(main_menu.status_sw, ACS_DIAMOND);
                    wattroff(main_menu.status_sw, COLOR_PAIR(4));
                    wprintw(main_menu.status_sw, " Status: ");
                    wattron(main_menu.status_sw, COLOR_PAIR(4));
                    wprintw(main_menu.status_sw, "Disconnected");
                    wattroff(main_menu.status_sw, COLOR_PAIR(4));

                    mvwprintw(global_wnds.note_sw, 0, (((global_dims.note_w-2)-note_labels.mmenu_disconnected.size)/2), note_labels.mmenu_disconnected.text);
                    break;
                case STATUS_CONNECTED:
                    wattron(main_menu.status_sw, COLOR_PAIR(5));
                    waddch(main_menu.status_sw, ACS_DIAMOND);
                    wattroff(main_menu.status_sw, COLOR_PAIR(5));
                    wprintw(main_menu.status_sw, " Status: ");
                    wattron(main_menu.status_sw, COLOR_PAIR(5));
                    wprintw(main_menu.status_sw, "Connected");
                    wattroff(main_menu.status_sw, COLOR_PAIR(5));

                    mvwprintw(global_wnds.note_sw, 0, (((global_dims.note_w-2)-note_labels.mmenu_connected.size)/2), note_labels.mmenu_connected.text);
                    break;
                case STATUS_CONNECTING:
                    wattron(main_menu.status_sw, COLOR_PAIR(3));
                    waddch(main_menu.status_sw, ACS_DIAMOND);
                    wattroff(main_menu.status_sw, COLOR_PAIR(3));
                    wprintw(main_menu.status_sw, " Status: ");
                    wattron(main_menu.status_sw, A_BLINK | COLOR_PAIR(3));
                    wprintw(main_menu.status_sw, "Connecting");
                    wattroff(main_menu.status_sw, A_BLINK | COLOR_PAIR(3));

                    mvwprintw(global_wnds.note_sw, 0, (((global_dims.note_w-2)-note_labels.mmenu_connecting.size)/2), note_labels.mmenu_connecting.text);
                    break;
                default:
                    break;
                }
                
            }
            _update_window();
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

int _set_label(struct label_t *lbl, char *str)
{
    char *tmp_label = NULL;
    int tmp_size = 0;
    int index;
    int sec_index;

    int thrd_index;

    int ret = EXIT_SUCCESS;

    tmp_size = strlen(str);

    tmp_label = (char *)malloc(tmp_size*sizeof(char)+1);
    if (tmp_label == NULL)
        return EXIT_FAILURE;

    strncpy(tmp_label, str, tmp_size);
    for (index = 0; index < tmp_size; index++)
    {
        if (tmp_label[index] == '\t')
        {
            lbl->size+=sizeof('\t');
        }
        else
            lbl->size++;
    }
        
    lbl->text = malloc(lbl->size+1);

    if (lbl->text == NULL)
        return EXIT_FAILURE;

    memset(lbl->text, '\x20', lbl->size);
    for (index = 0, sec_index = 0; sec_index < tmp_size; sec_index++)
    {
        if (tmp_label[sec_index] == '\t')
        {
            index += sizeof('\t');
        }
        else
        {
            lbl->text[index] = tmp_label[sec_index];
            index++;
        }
    }

    free(tmp_label);

    return ret;
}

int _set_labels()
{
    int ret = EXIT_SUCCESS;

    ret += _set_label(&note_labels.mmenu_connected, MENU_SCR_NOTE_CONNECTED);
    ret += _set_label(&note_labels.mmenu_disconnected, MENU_SCR_NOTE_DISCONNECTED);
    ret += _set_label(&note_labels.mmenu_connecting, MENU_SCR_NOTE_CONNECTING);
    ret += _set_label(&note_labels.join_srv, JOIN_SCR_NOTE);
    ret += _set_label(&note_labels.create_srv, CREATE_SCR_NOTE);
    ret += _set_label(&note_labels.prefs_wnd, PREFS_SCR_NOTE);

    return ret;
}

// TODO: Add incorrect dimensions handling by sending EXIT_FAILURE
int _set_dimensions()
{
    int main_menu_wnd_min_h;
    int main_menu_wnd_min_w;
    int ret = EXIT_SUCCESS;

    ioctl(fileno(stdout), TIOCGWINSZ, (char *) &size);

    /* Set dimensions that are global across all windows */
    {
        global_dims.elem_h = ELEM_SET_H > ELEM_MIN_H ? ELEM_SET_H : ELEM_MIN_H;
        global_dims.elem_w = ELEM_SET_W > ELEM_MIN_W ? ELEM_SET_W : ELEM_MIN_W;
        
        global_dims.wnd_h = SUBWND_SET_H - 2 - (GFX_ELEM_VOFF*3) - global_dims.elem_h;
        global_dims.wnd_w = SUBWND_SET_W - 2 - (GFX_ELEM_HOFF*2);

        global_dims.note_h = global_dims.elem_h;
        global_dims.note_w = global_dims.wnd_w;
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
            join_dims.tb_w += (global_dims.wnd_w - 2 - (GFX_ELEM_HOFF*2)) - (2 + join_dims.lbl_w + join_dims.tb_w + join_dims.btns_border_w);
        else
            join_dims.tb_w = join_dims.tb_w > IP_ADDR_LEN + 6 ? join_dims.tb_w : IP_ADDR_LEN + 6;

        join_dims.caddr_h = global_dims.elem_h;
        join_dims.caddr_w = 3 + join_dims.lbl_w + join_dims.tb_w;

        join_dims.pad_border_h = global_dims.wnd_h - join_dims.sinfo_h - join_dims.caddr_h - GFX_ELEM_VOFF;
        join_dims.pad_border_w = global_dims.wnd_w - 2 - (GFX_ELEM_HOFF*2);

        join_dims.vis_pad_h = join_dims.pad_border_h - 2;
        join_dims.vis_pad_w = join_dims.pad_border_w - 2;

        join_dims.pad_w = join_dims.vis_pad_w;

        if (join_dims.sname_w < NAME_LEN)
            ret = EXIT_FAILURE;
    }

    /* Set dimensions that are used in create server window */
    {
        create_dims.subwnd_h = global_dims.wnd_h - 2 - (GFX_ELEM_VOFF*2);

        create_dims.pad_elem_h = 1;

        create_dims.sname_h = create_dims.pad_elem_h;
        create_dims.sname_w = strlen(CREATE_SCR_SRV_NAME_LABEL)+1+NAME_LEN;

        create_dims.musers_h = create_dims.pad_elem_h;
        create_dims.musers_w = strlen(CREATE_SCR_CONN_USERS_LABEL)+4;

        create_dims.rusers_h = create_dims.pad_elem_h;
        create_dims.rusers_w = strlen(CREATE_SCR_RESTR_USERS_LABEL)+2;

        create_dims.saddr_h = create_dims.pad_elem_h;
        create_dims.saddr_w = strlen(CREATE_SCR_SRV_ADDR_LABEL) + IP_ADDR_LEN + 1;

        create_dims.sport_h = create_dims.pad_elem_h;
        create_dims.sport_w = strlen(CREATE_SCR_SRV_PORT_LABEL) + 6;

        create_dims.lcl_addr_h = create_dims.pad_elem_h;
        create_dims.lcl_addr_w = strlen(CREATE_SCR_LCL_ADDR_LABEL)+2;

        create_dims.auto_port_h = create_dims.pad_elem_h;
        create_dims.auto_port_w = strlen(CREATE_SCR_AUTO_PORT_LABEL)+2;

        create_dims.btns_h = global_dims.elem_h;
        create_dims.btns_w = global_dims.elem_w;

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

        create_dims.pad_h = (create_dims.pad_elem_h*7) + 6;
        create_dims.pad_w = create_dims.pad_elem_w;

        create_dims.vis_pad_h = create_dims.srv_info_h - 2 - (GFX_ELEM_VOFF*2) - create_dims.srv_info_sw_h - create_dims.btns_h - 2 - (GFX_ELEM_VOFF*2);
        create_dims.vis_pad_h = create_dims.vis_pad_h > create_dims.pad_h ? create_dims.pad_h : create_dims.vis_pad_h;
        create_dims.vis_pad_w = create_dims.pad_w;

        create_dims.pad_border_h = create_dims.vis_pad_h + 2;
        create_dims.pad_border_w = create_dims.pad_elem_w + 2;

        create_dims.subwnd_w = global_dims.wnd_w - create_dims.pad_border_w - 2 - (GFX_ELEM_HOFF*3);
    }

    /* Set dimensions that are used in preferences window */
    {
        cfg_dims.entry_hspacer = 4;

        cfg_dims.pad_border_h = global_dims.wnd_h - 2 - (GFX_ELEM_VOFF*2);
        // cfg_dims.pad_border_w = global_dims.wnd_w - 2 - (GFX_ELEM_HOFF*2);
        cfg_dims.pad_border_w = LABEL_LEN*2+3+cfg_dims.entry_hspacer*3;

        cfg_dims.vis_pad_h = cfg_dims.pad_border_h - 2;
        cfg_dims.vis_pad_w = cfg_dims.pad_border_w - 2;

        cfg_dims.pad_h = cfg_dims.vis_pad_h;
        cfg_dims.pad_w = cfg_dims.vis_pad_w;

        cfg_dims.entry_h = 1;
        cfg_dims.entry_w = LABEL_LEN*2+cfg_dims.entry_hspacer+2;
    }

    return ret;
}

int _set_axis()
{
    int ret = EXIT_SUCCESS;

    /* Set axis that are global across all windows */
    {
        global_axis.wnd_y = 1+GFX_ELEM_VOFF;
        global_axis.wnd_x = 1+GFX_ELEM_HOFF;
        // global_axis.wnd_y = 0;
        // global_axis.wnd_x = 0;
        // global_axis.subwnd_y = 0;
        // global_axis.subwnd_x = 0;
        global_axis.note_y = global_axis.wnd_y+global_dims.wnd_h+(GFX_ELEM_VOFF*2);
        global_axis.note_x = 1+GFX_ELEM_HOFF;
        // global_axis.note_y = global_axis.wnd_y+global_dims.wnd_h+(GFX_ELEM_VOFF*1);
        // global_axis.note_x = 0;
    }

    /* Set axis that are used in main menu window */
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

    /* Set axis that are used in join server window */
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

        join_axis.vis_pad_ys = global_axis.wnd_y + join_axis.pad_border_y + 1;
        join_axis.vis_pad_xs = global_axis.wnd_x + join_axis.pad_border_x + 1;
        join_axis.vis_pad_ye = join_axis.pad_border_y + join_dims.pad_border_h - 1;
        join_axis.vis_pad_xe = join_axis.pad_border_x + join_dims.pad_border_w;

        // join_axis.pad_ys = 0;
        // join_axis.pad_xs = 0;
        // join_axis.pad_ye = join_axis.pad_ys + join_dims.pad_h - 1;
        // join_axis.pad_xe = join_axis.pad_xs + join_dims.pad_w - 1;

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

    /* Set axis that are used in create server window */
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

        // create_axis.sname_y = 0;
        // create_axis.sname_x = 0;

        create_axis.pad_elems_y[0] = 0;
        create_axis.pad_elems_x[0] = 0;
        create_axis.pad_elems_y[1] = create_axis.pad_elems_y[0] + 2;
        create_axis.pad_elems_x[1] = 0;
        create_axis.pad_elems_y[2] = create_axis.pad_elems_y[1] + 2;
        create_axis.pad_elems_x[2] = 0;
        create_axis.pad_elems_y[3] = create_axis.pad_elems_y[2] + 2;
        create_axis.pad_elems_x[3] = 0;
        create_axis.pad_elems_y[4] = create_axis.pad_elems_y[3] + 2;
        create_axis.pad_elems_x[4] = 0;
        create_axis.pad_elems_y[5] = create_axis.pad_elems_y[4] + 2;
        create_axis.pad_elems_x[5] = 0;
        create_axis.pad_elems_y[6] = create_axis.pad_elems_y[5] + 2;
        create_axis.pad_elems_x[6] = 0;

        // create_axis.sport_y = create_axis.saddr_y + 2;
        // create_axis.sport_x = 0;
        // create_axis.lcl_addr_y = create_axis.sport_y + 2;
        // create_axis.lcl_addr_x = 0;
        // create_axis.auto_port_y = create_axis.lcl_addr_y + 2;
        // create_axis.auto_port_x = 0;

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

    /* Set axis that are used in preferences window */
    {
        cfg_axis.pad_border_y = 1 + GFX_ELEM_VOFF;
        // cfg_axis.pad_border_x = 1 + GFX_ELEM_HOFF;
        cfg_axis.pad_border_x = (global_dims.wnd_w-cfg_dims.pad_border_w)/2;
        cfg_axis.vis_pad_ys = global_axis.wnd_y + cfg_axis.pad_border_y + 1;
        cfg_axis.vis_pad_xs = global_axis.wnd_x + cfg_axis.pad_border_x + 1;
        cfg_axis.vis_pad_ye = cfg_axis.vis_pad_ys + cfg_dims.vis_pad_h - 1;
        cfg_axis.vis_pad_xe = cfg_axis.vis_pad_xs + cfg_dims.vis_pad_w - 1;

        cfg_axis.vdelim_x = LABEL_LEN;

        cfg_axis.entries_y[0] = 0;
        cfg_axis.entries_x[0] = 4;
        cfg_axis.fields_x[0] = cfg_axis.vdelim_x + 1 + cfg_dims.entry_hspacer;
        cfg_axis.entries_y[1] = cfg_axis.entries_y[0] + 2;;
        cfg_axis.entries_x[1] = cfg_axis.entries_x[0];
        cfg_axis.fields_x[1] = cfg_axis.vdelim_x + 1 + cfg_dims.entry_hspacer;
        cfg_axis.entries_y[2] = cfg_axis.entries_y[1] + 2;;
        cfg_axis.entries_x[2] = cfg_axis.entries_x[0];
        cfg_axis.fields_x[2] = cfg_axis.vdelim_x + 1 + cfg_dims.entry_hspacer;
        cfg_axis.entries_y[3] = cfg_axis.entries_y[2] + 2;;
        cfg_axis.entries_x[3] = cfg_axis.entries_x[0];
        cfg_axis.fields_x[3] = cfg_axis.vdelim_x + 1 + cfg_dims.entry_hspacer;
    }

    return ret;
}

int init_graphics()
{
    int ret = EXIT_SUCCESS;

    // _set_labels();
    ret += _set_label(&note_labels.mmenu_connected, MENU_SCR_NOTE_CONNECTED);
    ret += _set_label(&note_labels.mmenu_disconnected, MENU_SCR_NOTE_DISCONNECTED);
    ret += _set_label(&note_labels.mmenu_connecting, MENU_SCR_NOTE_CONNECTING);
    ret += _set_label(&note_labels.join_srv, JOIN_SCR_NOTE);
    ret += _set_label(&note_labels.create_srv, CREATE_SCR_NOTE);
    ret += _set_label(&note_labels.prefs_wnd, PREFS_SCR_NOTE);

    ret += _set_dimensions();

    ret += _set_axis();

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

    return ret;
}

void deinit_graphics()
{
    endwin();

    if (note_labels.mmenu_connected.text != NULL)
    {
        free(note_labels.mmenu_connected.text);
    }
    if (note_labels.mmenu_disconnected.text != NULL)
    {
        free(note_labels.mmenu_disconnected.text);
    }
    if (note_labels.mmenu_connecting.text != NULL)
    {
        free(note_labels.mmenu_connecting.text);
    }
    if (note_labels.join_srv.text != NULL)
    {
        free(note_labels.join_srv.text);
    }
    if (note_labels.create_srv.text != NULL)
    {
        free(note_labels.create_srv.text);
    }
    if (note_labels.prefs_wnd.text != NULL)
    {
        free(note_labels.prefs_wnd.text);
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
                    wclear(stdscr);
                    refresh();

                    delwin(global_wnds.note_sw);
                    delwin(global_wnds.note_w);
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
                    delwin(global_wnds.wnd);

                    global_wnds.wnd = NULL;
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
                    global_wnds.note_w = NULL;
                    global_wnds.note_sw = NULL;
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
                }

                global_wnds.wnd = newwin(global_dims.wnd_h, global_dims.wnd_w, global_axis.wnd_y, global_axis.wnd_x);

                main_menu.header_w = derwin(global_wnds.wnd, mmenu_dims.header_h, mmenu_dims.header_w, mmenu_axis.header_y, mmenu_axis.header_x);
                main_menu.status_w = derwin(global_wnds.wnd, mmenu_dims.status_h, mmenu_dims.status_w, mmenu_axis.status_y, mmenu_axis.status_x);
                main_menu.btns_border = derwin(global_wnds.wnd, mmenu_dims.btns_border_h, mmenu_dims.btns_border_w, mmenu_axis.btns_border_y, mmenu_axis.btns_border_x);
                main_menu.panel_w = derwin(global_wnds.wnd, mmenu_dims.panel_h, mmenu_dims.panel_w, mmenu_axis.panel_y, mmenu_axis.panel_x);
                main_menu.panel_sw = derwin(main_menu.panel_w, mmenu_dims.panel_h-2, mmenu_dims.panel_w-2, 1, 1);
                
                main_menu.status_sw = derwin(main_menu.status_w, mmenu_dims.status_h - 2, mmenu_dims.status_w - 2, 1, 1);
                main_menu.btns[0].wnd = derwin(main_menu.btns_border, global_dims.elem_h, global_dims.elem_w, mmenu_axis.btns_y[0], mmenu_axis.btns_x[0]);
                main_menu.btns[1].wnd = derwin(main_menu.btns_border, global_dims.elem_h, global_dims.elem_w, mmenu_axis.btns_y[1], mmenu_axis.btns_x[1]);
                main_menu.btns[2].wnd = derwin(main_menu.btns_border, global_dims.elem_h, global_dims.elem_w, mmenu_axis.btns_y[2], mmenu_axis.btns_x[2]);
                main_menu.btns[3].wnd = derwin(main_menu.btns_border, global_dims.elem_h, global_dims.elem_w, mmenu_axis.btns_y[3], mmenu_axis.btns_x[3]);

                global_wnds.note_w = newwin(global_dims.note_h, global_dims.note_w, global_axis.note_y, global_axis.note_x);
                global_wnds.note_sw = derwin(global_wnds.note_w, global_dims.note_h-2, global_dims.note_w-2, 1, 1);

                box(global_wnds.wnd, ACS_VLINE, ACS_HLINE);
                box(main_menu.status_w, ' ', ' ');
                box(main_menu.btns_border, ACS_VLINE, ACS_HLINE);
                box(main_menu.btns[0].wnd, ACS_VLINE, ACS_HLINE);
                box(main_menu.btns[1].wnd, ACS_VLINE, ACS_HLINE);
                box(main_menu.btns[2].wnd, ACS_VLINE, ACS_HLINE);
                box(main_menu.btns[3].wnd, ACS_VLINE, ACS_HLINE);
                box(main_menu.panel_w, ACS_VLINE, ACS_HLINE);
                box(global_wnds.note_w, ' ', ' ');

                mvwprintw(global_wnds.wnd, 0, (global_dims.wnd_w-strlen(MENU_SCR_LABEL))/2, MENU_SCR_LABEL);
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

                    mvwprintw(global_wnds.note_sw, 0, (((global_dims.note_w-2)-note_labels.mmenu_disconnected.size)/2), note_labels.mmenu_disconnected.text);
                    break;
                case STATUS_CONNECTED:
                    wattron(main_menu.status_sw, COLOR_PAIR(5));
                    waddch(main_menu.status_sw, ACS_DIAMOND);
                    wattroff(main_menu.status_sw, COLOR_PAIR(5));
                    wprintw(main_menu.status_sw, " Status: ");
                    wattron(main_menu.status_sw, COLOR_PAIR(5));
                    wprintw(main_menu.status_sw, "Connected");
                    wattroff(main_menu.status_sw, COLOR_PAIR(5));

                    mvwprintw(global_wnds.note_sw, 0, (((global_dims.note_w-2)-note_labels.mmenu_connected.size)/2), note_labels.mmenu_connected.text);
                    break;
                case STATUS_CONNECTING:
                    wattron(main_menu.status_sw, COLOR_PAIR(3));
                    waddch(main_menu.status_sw, ACS_DIAMOND);
                    wattroff(main_menu.status_sw, COLOR_PAIR(3));
                    wprintw(main_menu.status_sw, " Status: ");
                    wattron(main_menu.status_sw, A_BLINK | COLOR_PAIR(3));
                    wprintw(main_menu.status_sw, "Connecting");
                    wattroff(main_menu.status_sw, A_BLINK | COLOR_PAIR(3));

                    mvwprintw(global_wnds.note_sw, 0, (((global_dims.note_w-2)-note_labels.mmenu_connecting.size)/2), note_labels.mmenu_connecting.text);
                    break;
                default:
                    break;
                }

                wprintw(main_menu.panel_sw, MENU_SCR_NOTE_LABEL"\n");
                {
                wprintw(main_menu.panel_sw, "ACS_BBSS: ");
                waddch(main_menu.panel_sw, ACS_BBSS);
                wprintw(main_menu.panel_sw, "\tACS_BLOCK: ");
                waddch(main_menu.panel_sw, ACS_BLOCK);
                wprintw(main_menu.panel_sw, "\tACS_BOARD: ");
                waddch(main_menu.panel_sw, ACS_BOARD);
                wprintw(main_menu.panel_sw, "\tACS_BSBS: ");
                waddch(main_menu.panel_sw, ACS_BSBS);
                wprintw(main_menu.panel_sw, "\tACS_BSSB: ");
                waddch(main_menu.panel_sw, ACS_BSSB);
                wprintw(main_menu.panel_sw, "\nACS_BSSS: ");
                waddch(main_menu.panel_sw, ACS_BSSS);
                wprintw(main_menu.panel_sw, "\tACS_BTEE: ");
                waddch(main_menu.panel_sw, ACS_BTEE);
                wprintw(main_menu.panel_sw, "\tACS_BULLET: ");
                waddch(main_menu.panel_sw, ACS_BULLET);
                wprintw(main_menu.panel_sw, "\tACS_CKBOARD: ");
                waddch(main_menu.panel_sw, ACS_CKBOARD);
                wprintw(main_menu.panel_sw, "\tACS_DARROW: ");
                waddch(main_menu.panel_sw, ACS_DARROW);
                wprintw(main_menu.panel_sw, "\nACS_DEGREE: ");
                waddch(main_menu.panel_sw, ACS_DEGREE);
                wprintw(main_menu.panel_sw, "\tACS_DIAMOND: ");
                waddch(main_menu.panel_sw, ACS_DIAMOND);
                wprintw(main_menu.panel_sw, "\tACS_GEQUAL: ");
                waddch(main_menu.panel_sw, ACS_GEQUAL);
                wprintw(main_menu.panel_sw, "\tACS_HLINE: ");
                waddch(main_menu.panel_sw, ACS_HLINE);
                wprintw(main_menu.panel_sw, "\tACS_LANTERN: ");
                waddch(main_menu.panel_sw, ACS_LANTERN);
                wprintw(main_menu.panel_sw, "\nACS_LARROW: ");
                waddch(main_menu.panel_sw, ACS_LARROW);
                wprintw(main_menu.panel_sw, "\tACS_LEQUAL: ");
                waddch(main_menu.panel_sw, ACS_LEQUAL);
                wprintw(main_menu.panel_sw, "\tACS_LLCORNER: ");
                waddch(main_menu.panel_sw, ACS_LLCORNER);
                wprintw(main_menu.panel_sw, "\tACS_LRCORNER: ");
                waddch(main_menu.panel_sw, ACS_LRCORNER);
                wprintw(main_menu.panel_sw, "\tACS_LTEE: ");
                waddch(main_menu.panel_sw, ACS_LTEE);
                wprintw(main_menu.panel_sw, "\nACS_NEQUAL: ");
                waddch(main_menu.panel_sw, ACS_NEQUAL);
                wprintw(main_menu.panel_sw, "\tACS_PI: ");
                waddch(main_menu.panel_sw, ACS_PI);
                wprintw(main_menu.panel_sw, "\tACS_PLMINUS: ");
                waddch(main_menu.panel_sw, ACS_PLMINUS);
                wprintw(main_menu.panel_sw, "\tACS_PLUS: ");
                waddch(main_menu.panel_sw, ACS_PLUS);
                wprintw(main_menu.panel_sw, "\tACS_RARROW: ");
                waddch(main_menu.panel_sw, ACS_RARROW);
                wprintw(main_menu.panel_sw, "\nACS_RTEE: ");
                waddch(main_menu.panel_sw, ACS_RTEE);
                wprintw(main_menu.panel_sw, "\tACS_S1: ");
                waddch(main_menu.panel_sw, ACS_S1);
                wprintw(main_menu.panel_sw, "\tACS_S3: ");
                waddch(main_menu.panel_sw, ACS_S3);
                wprintw(main_menu.panel_sw, "\tACS_S7: ");
                waddch(main_menu.panel_sw, ACS_S7);
                wprintw(main_menu.panel_sw, "\tACS_S9: ");
                waddch(main_menu.panel_sw, ACS_S9);
                wprintw(main_menu.panel_sw, "\nACS_SBBS: ");
                waddch(main_menu.panel_sw, ACS_SBBS);
                wprintw(main_menu.panel_sw, "\tACS_SBSB: ");
                waddch(main_menu.panel_sw, ACS_SBSB);
                wprintw(main_menu.panel_sw, "\tACS_SBSS: ");
                waddch(main_menu.panel_sw, ACS_SBSS);
                wprintw(main_menu.panel_sw, "\tACS_SSBB: ");
                waddch(main_menu.panel_sw, ACS_SSBB);
                wprintw(main_menu.panel_sw, "\tACS_SSBS: ");
                waddch(main_menu.panel_sw, ACS_SSBS);
                wprintw(main_menu.panel_sw, "\nACS_SSSB: ");
                waddch(main_menu.panel_sw, ACS_SSSB);
                wprintw(main_menu.panel_sw, "\tACS_SSSS: ");
                waddch(main_menu.panel_sw, ACS_SSSS);
                wprintw(main_menu.panel_sw, "\tACS_STERLING: ");
                waddch(main_menu.panel_sw, ACS_STERLING);
                wprintw(main_menu.panel_sw, "\tACS_TTEE: ");
                waddch(main_menu.panel_sw, ACS_TTEE);
                wprintw(main_menu.panel_sw, "\tACS_UARROW: ");
                waddch(main_menu.panel_sw, ACS_UARROW);
                wprintw(main_menu.panel_sw, "\nACS_ULCORNER: ");
                waddch(main_menu.panel_sw, ACS_ULCORNER);
                wprintw(main_menu.panel_sw, "\tACS_URCORNER: ");
                waddch(main_menu.panel_sw, ACS_URCORNER);
                wprintw(main_menu.panel_sw, "\tACS_VLINE: ");
                waddch(main_menu.panel_sw, ACS_VLINE);
                }

                for (index = 0; index < 4; ++index)
                {
                    mvwprintw(main_menu.btns[index].wnd, 1, (mmenu_dims.btns_w-strlen(main_menu.btns[index].lbl))/2, main_menu.btns[index].lbl);
                }

                wattron(main_menu.btns[main_menu.selection].wnd, A_BOLD | A_UNDERLINE);
                mvwprintw(main_menu.btns[main_menu.selection].wnd, 1, (mmenu_dims.btns_w-strlen(main_menu.btns[main_menu.selection].lbl))/2, main_menu.btns[main_menu.selection].lbl);
                wattroff(main_menu.btns[main_menu.selection].wnd, A_BOLD | A_UNDERLINE);
            }
            break;
        case WND_JOIN_SRV:
            {
                if (wnd_type == cur_wnd)
                {
                    wclear(stdscr);
                    refresh();

                    delwin(global_wnds.note_sw);
                    delwin(global_wnds.note_w);
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
                    delwin(global_wnds.wnd);

                    global_wnds.wnd = NULL;
                    join_srv.top_panel = NULL;
                    join_srv.sname_w = NULL;
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
                    global_wnds.note_w = NULL;
                    global_wnds.note_sw = NULL;
                }
                else
                {
                    join_srv.btns[0].lbl = malloc(strlen(JOIN_SCR_JOIN_BTN_LABEL)+1);
                    join_srv.btns[1].lbl = malloc(strlen(JOIN_SCR_REFRESH_BTN_LABEL)+1);
                    join_srv.btns[2].lbl = malloc(strlen(JOIN_SCR_CLEAR_BTN_LABEL)+1);

                    strcpy(join_srv.btns[0].lbl, JOIN_SCR_JOIN_BTN_LABEL);
                    strcpy(join_srv.btns[1].lbl, JOIN_SCR_REFRESH_BTN_LABEL);
                    strcpy(join_srv.btns[2].lbl, JOIN_SCR_CLEAR_BTN_LABEL);
                }

                global_wnds.wnd = newwin(global_dims.wnd_h, global_dims.wnd_w, global_axis.wnd_y, global_axis.wnd_x);

                join_srv.top_panel = derwin(global_wnds.wnd, join_dims.sinfo_h, join_dims.sinfo_w, join_axis.top_panel_y, join_axis.top_panel_x);
                join_srv.pad_border = derwin(global_wnds.wnd, join_dims.pad_border_h, join_dims.pad_border_w, join_axis.pad_border_y, join_axis.pad_border_x);
                join_srv.caddr_w = derwin(global_wnds.wnd, join_dims.caddr_h, join_dims.caddr_w, join_axis.caddr_y, join_axis.caddr_x);
                join_srv.btns_border = derwin(global_wnds.wnd, join_dims.btns_border_h, join_dims.btns_border_w, join_axis.btns_border_y, join_axis.btns_border_x);

                join_srv.sname_w = derwin(join_srv.top_panel, join_dims.sname_h, join_dims.sname_w, join_axis.sname_y, join_axis.sname_x);
                join_srv.saddr_w = derwin(join_srv.top_panel, join_dims.saddr_h, join_dims.saddr_w, join_axis.saddr_y, join_axis.saddr_x);
                join_srv.susers_w = derwin(join_srv.top_panel, join_dims.susers_h, join_dims.susers_w, join_axis.susers_y, join_axis.susers_x);
                join_srv.servers_pad = newpad(join_dims.pad_h, join_dims.pad_w);
                join_srv.label_w = derwin(join_srv.caddr_w, join_dims.lbl_h, join_dims.lbl_w, join_axis.lbl_y, join_axis.lbl_x);
                join_srv.tb_w = derwin(join_srv.caddr_w, join_dims.tb_h, join_dims.tb_w, join_axis.tb_y, join_axis.tb_x);
                join_srv.btns[0].wnd = derwin(join_srv.btns_border, join_dims.btns_h, join_dims.btns_w, join_axis.btns_y[0], join_axis.btns_x[0]);
                join_srv.btns[1].wnd = derwin(join_srv.btns_border, join_dims.btns_h, join_dims.btns_w, join_axis.btns_y[1], join_axis.btns_x[1]);
                join_srv.btns[2].wnd = derwin(join_srv.btns_border, join_dims.btns_h, join_dims.btns_w, join_axis.btns_y[2], join_axis.btns_x[2]);

                global_wnds.note_w = newwin(global_dims.note_h, global_dims.note_w, global_axis.note_y, global_axis.note_x);
                global_wnds.note_sw = derwin(global_wnds.note_w, global_dims.note_h-2, global_dims.note_w-2, 1, 1);

                box(global_wnds.wnd, ACS_VLINE, ACS_HLINE);
                box(join_srv.top_panel, ACS_VLINE, ACS_HLINE);
                box(join_srv.pad_border, ACS_VLINE, ACS_HLINE);
                box(join_srv.caddr_w, ACS_VLINE, ACS_HLINE);
                box(join_srv.btns_border, ACS_VLINE, ACS_HLINE);
                box(global_wnds.note_w, ' ', ' ');

                mvwprintw(global_wnds.wnd, 0, (global_dims.wnd_w-strlen(JOIN_SCR_LABEL))/2, JOIN_SCR_LABEL);
                wprintw(join_srv.sname_w, JOIN_SCR_SRV_NAME_LABEL);
                wprintw(join_srv.saddr_w, JOIN_SCR_SRV_ADDR_LABEL);
                wprintw(join_srv.susers_w, JOIN_SCR_CONN_USERS_LABEL);
                wprintw(join_srv.label_w, JOIN_SCR_MAN_ADDR_LABEL);
                for (index = 0; index < 3; ++index)
                {
                    mvwprintw(join_srv.btns[index].wnd, 0, (join_dims.btns_w-strlen(join_srv.btns[index].lbl))/2, join_srv.btns[index].lbl);
                }

                wattron(join_srv.btns[join_srv.selection].wnd, A_BOLD | A_UNDERLINE);
                mvwprintw(join_srv.btns[join_srv.selection].wnd, 0, (join_dims.btns_w-strlen(join_srv.btns[join_srv.selection].lbl))/2, join_srv.btns[join_srv.selection].lbl);
                wattroff(join_srv.btns[join_srv.selection].wnd, A_BOLD | A_UNDERLINE);

                mvwprintw(global_wnds.note_sw, 0, (((global_dims.note_w-2)-note_labels.join_srv.size)/2), note_labels.join_srv.text);

                mvwaddch(join_srv.top_panel, 0, join_axis.saddr_x-1, ACS_BSSS);
                mvwaddch(join_srv.top_panel, 0, join_axis.susers_x-1, ACS_BSSS);

                mvwvline(join_srv.top_panel, 1, join_axis.saddr_x-1, ACS_VLINE, join_dims.sname_h);
                mvwvline(join_srv.top_panel, 1, join_axis.susers_x-1, ACS_VLINE, join_dims.sname_h);

                mvwaddch(join_srv.pad_border, 0, 0, ACS_LTEE);
                mvwaddch(join_srv.pad_border, 0, join_axis.saddr_x-1, ACS_PLUS);
                mvwaddch(join_srv.pad_border, 0, join_axis.susers_x-1, ACS_PLUS);
                mvwaddch(join_srv.pad_border, 0, join_dims.pad_border_w-1, ACS_RTEE);

                mvwaddch(join_srv.pad_border, join_dims.pad_border_h-1, 0, ACS_LTEE);

                mvwaddch(join_srv.caddr_w, 0, join_axis.tb_x-1, ACS_BSSS);
                mvwaddch(join_srv.caddr_w, 0, join_dims.caddr_w-1, ACS_BSSS);
                mvwaddch(join_srv.btns_border, 0, join_axis.btns_x[1]-1, ACS_BSSS);
                mvwaddch(join_srv.btns_border, 0, join_axis.btns_x[2]-1, ACS_BSSS);
                mvwaddch(join_srv.btns_border, 0, 0, ACS_BSSS);


                mvwvline(join_srv.pad_border, 1, join_axis.saddr_x-1, ACS_VLINE, join_dims.pad_border_h-2);
                mvwvline(join_srv.pad_border, 1, join_axis.susers_x-1, ACS_VLINE, join_dims.pad_border_h-2);
                
                for (index = 0; index < sizeof(servers_info)/sizeof(struct server_info_t); index++)
                {
                    mvwhline(join_srv.servers_pad, index, 0, ' ', join_dims.pad_w);
                    mvwprintw(join_srv.servers_pad, index, 0, "%s", servers_info[index].server_name);
                    mvwprintw(join_srv.servers_pad, index, join_axis.saddr_x-1, "%s:%d", servers_info[index].ip, servers_info[index].port);
                    if (servers_info[index].max_users != 0)
                        mvwprintw(join_srv.servers_pad, index, join_axis.susers_x-1, "%d/%d", servers_info[index].cur_users, servers_info[index].max_users);
                    else
                        mvwprintw(join_srv.servers_pad, index, join_axis.susers_x-1, "%d", servers_info[index].cur_users);
                    mvwaddch(join_srv.servers_pad, index, join_axis.saddr_x-2, ACS_VLINE);
                    mvwaddch(join_srv.servers_pad, index, join_axis.susers_x-2, ACS_VLINE);
                }
                wattron(join_srv.servers_pad, COLOR_PAIR(2));
                mvwhline(join_srv.servers_pad, join_srv.line, 0, ' ', join_dims.pad_w);
                mvwprintw(join_srv.servers_pad, join_srv.line, 0, "%s", servers_info[join_srv.line].server_name);
                mvwprintw(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-1, "%s:%d", servers_info[join_srv.line].ip, servers_info[join_srv.line].port);
                if (servers_info[join_srv.line].max_users != 0)
                    mvwprintw(join_srv.servers_pad, join_srv.line, join_axis.susers_x-1, "%d/%d", servers_info[join_srv.line].cur_users, servers_info[join_srv.line].max_users);
                else
                    mvwprintw(join_srv.servers_pad, join_srv.line, join_axis.susers_x-1, "%d", servers_info[join_srv.line].cur_users);
                mvwaddch(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-2, ACS_VLINE);
                mvwaddch(join_srv.servers_pad, join_srv.line, join_axis.susers_x-2, ACS_VLINE);
                wattroff(join_srv.servers_pad, COLOR_PAIR(2));

                tmp = join_axis.pad_border_x+join_axis.saddr_x-1;
                if (tmp == join_axis.caddr_x+join_dims.caddr_w-1)
                {
                    mvwaddch(join_srv.pad_border, join_dims.pad_border_h-1, join_axis.saddr_x-1, ACS_PLUS);
                }
                else
                {
                    
                    if (tmp == join_axis.btns_border_x)
                    {
                        mvwaddch(join_srv.pad_border, join_dims.pad_border_h-1, join_axis.saddr_x-1, ACS_PLUS);
                    }
                    else
                    {
                        if (tmp == join_axis.btns_border_x + join_axis.btns_x[1]-1)
                        {
                            mvwaddch(join_srv.pad_border, join_dims.pad_border_h-1, join_axis.saddr_x-1, ACS_PLUS);
                        }
                        else
                        {
                            if (tmp == join_axis.btns_border_x + join_axis.btns_x[2]-1)
                            {
                                mvwaddch(join_srv.pad_border, join_dims.pad_border_h-1, join_axis.saddr_x-1, ACS_PLUS);
                            }
                            else
                            {
                                mvwaddch(join_srv.pad_border, join_dims.pad_border_h-1, join_axis.saddr_x-1, ACS_BTEE);
                            }
                        }
                    }
                }

                tmp = (join_axis.pad_border_x+join_axis.susers_x-1);
                if ((join_axis.pad_border_x+join_axis.susers_x-1) == join_axis.btns_border_x)
                {
                    mvwaddch(join_srv.pad_border, join_dims.pad_border_h-1, join_axis.susers_x-1, ACS_PLUS);
                }
                else
                {
                    mvwaddch(join_srv.pad_border, join_dims.pad_border_h-1, join_axis.susers_x-1, ACS_BTEE);
                }

                mvwaddch(join_srv.caddr_w, 0, join_dims.lbl_w+1, ACS_BSSS);
                mvwaddch(join_srv.pad_border, join_dims.pad_border_h-1, join_dims.pad_border_w-1, ACS_RTEE);
                mvwvline(join_srv.caddr_w, 1, join_dims.lbl_w+1, ACS_VLINE, join_dims.tb_h);
                mvwvline(join_srv.btns_border, 1, join_axis.btns_x[1]-1, ACS_VLINE, join_dims.btns_h);
                mvwvline(join_srv.btns_border, 1, join_axis.btns_x[2]-1, ACS_VLINE, join_dims.btns_h);

                mvwaddch(join_srv.caddr_w, join_dims.caddr_h-1, join_dims.lbl_w+1, ACS_BTEE);
                if (join_axis.caddr_x+join_dims.caddr_w-1 == join_axis.btns_border_x)
                {
                    mvwaddch(join_srv.btns_border, join_dims.btns_border_h-1, 0, ACS_BTEE);
                }
                mvwaddch(join_srv.btns_border, join_dims.btns_border_h-1, join_axis.btns_x[1]-1, ACS_BTEE);
                mvwaddch(join_srv.btns_border, join_dims.btns_border_h-1, join_axis.btns_x[2]-1, ACS_BTEE);
            }
            break;
        case WND_CREATE_SRV:
            {
                if (wnd_type == cur_wnd)
                {
                    wclear(stdscr);
                    refresh();

                    delwin(global_wnds.note_sw);
                    delwin(global_wnds.note_w);

                    delwin(create_srv.auto_port_w);
                    delwin(create_srv.lcl_addr_w);
                    delwin(create_srv.port_w);
                    delwin(create_srv.addr_w);
                    delwin(create_srv.rusers_w);
                    delwin(create_srv.musers_w);
                    delwin(create_srv.sname_w);

                    for (index = 0; index < 7; ++index)
                    {
                        delwin(create_srv.pad_elems[index].wnd);
                    }
                    delwin(create_srv.srv_info_sw);
                    for (index = 0; index < 3; ++index)
                    {
                        delwin(create_srv.btns[index].wnd);
                    }
                    delwin(create_srv.pad);

                    delwin(create_srv.srv_info_w);
                    delwin(create_srv.pad_border);
                    delwin(global_wnds.wnd);

                    global_wnds.wnd = NULL;

                    create_srv.pad_border = NULL;
                    create_srv.srv_info_w = NULL;
                    create_srv.pad = NULL;

                    for (index = 0; index < 7; ++index)
                    {
                        create_srv.pad_elems[index].wnd = NULL;
                    }
                    for (index = 0; index < 3; ++index)
                    {
                        create_srv.btns[index].wnd = NULL;
                    }

                    global_wnds.note_w = NULL;
                    global_wnds.note_sw = NULL;
                }
                else
                {
                    create_srv.pad_elems[0].lbl = malloc(strlen(CREATE_SCR_SRV_NAME_LABEL)+1);
                    create_srv.pad_elems[1].lbl = malloc(strlen(CREATE_SCR_CONN_USERS_LABEL)+1);
                    create_srv.pad_elems[2].lbl = malloc(strlen(CREATE_SCR_RESTR_USERS_LABEL)+1);
                    create_srv.pad_elems[3].lbl = malloc(strlen(CREATE_SCR_SRV_ADDR_LABEL)+1);
                    create_srv.pad_elems[4].lbl = malloc(strlen(CREATE_SCR_SRV_PORT_LABEL)+1);
                    create_srv.pad_elems[5].lbl = malloc(strlen(CREATE_SCR_LCL_ADDR_LABEL)+1);
                    create_srv.pad_elems[6].lbl = malloc(strlen(CREATE_SCR_AUTO_PORT_LABEL)+1);

                    strcpy(create_srv.pad_elems[0].lbl, CREATE_SCR_SRV_NAME_LABEL);
                    strcpy(create_srv.pad_elems[1].lbl, CREATE_SCR_CONN_USERS_LABEL);
                    strcpy(create_srv.pad_elems[2].lbl, CREATE_SCR_RESTR_USERS_LABEL);
                    strcpy(create_srv.pad_elems[3].lbl, CREATE_SCR_SRV_ADDR_LABEL);
                    strcpy(create_srv.pad_elems[4].lbl, CREATE_SCR_SRV_PORT_LABEL);
                    strcpy(create_srv.pad_elems[5].lbl, CREATE_SCR_LCL_ADDR_LABEL);
                    strcpy(create_srv.pad_elems[6].lbl, CREATE_SCR_AUTO_PORT_LABEL);

                    create_srv.btns[0].lbl = malloc(strlen(CREATE_SCR_CREATE_BTN_LABEL)+1);
                    create_srv.btns[1].lbl = malloc(strlen(CREATE_SCR_DEFAULT_BTN_LABEL)+1);
                    create_srv.btns[2].lbl = malloc(strlen(CREATE_SCR_CLEAR_BTN_LABEL)+1);

                    strcpy(create_srv.btns[0].lbl, CREATE_SCR_CREATE_BTN_LABEL);
                    strcpy(create_srv.btns[1].lbl, CREATE_SCR_DEFAULT_BTN_LABEL);
                    strcpy(create_srv.btns[2].lbl, CREATE_SCR_CLEAR_BTN_LABEL);
                }

                global_wnds.wnd = newwin(global_dims.wnd_h, global_dims.wnd_w, global_axis.wnd_y, global_axis.wnd_x);
                
                create_srv.srv_info_w = derwin(global_wnds.wnd, create_dims.srv_info_h, create_dims.srv_info_w, create_axis.srv_info_y, create_axis.srv_info_x);

                create_srv.srv_info_sw = derwin(create_srv.srv_info_w, create_dims.srv_info_sw_h, create_dims.pad_elem_w, create_axis.srv_info_sw_y, create_axis.srv_info_sw_x);
                create_srv.pad_border = derwin(create_srv.srv_info_w, create_dims.pad_border_h, create_dims.pad_border_w, create_axis.pad_border_y, create_axis.pad_border_x);
                create_srv.btns[0].wnd = derwin(create_srv.srv_info_w, create_dims.btns_h, create_dims.btns_w, create_axis.btns_y[0], create_axis.btns_x[0]);
                create_srv.btns[1].wnd = derwin(create_srv.srv_info_w, create_dims.btns_h, create_dims.btns_w, create_axis.btns_y[1], create_axis.btns_x[1]);
                create_srv.btns[2].wnd = derwin(create_srv.srv_info_w, create_dims.btns_h, create_dims.btns_w, create_axis.btns_y[2], create_axis.btns_x[2]);

                create_srv.pad = newpad(create_dims.pad_h, create_dims.pad_w);

                for (index = 0; index < 7; ++index)
                {
                    create_srv.pad_elems[index].wnd = subpad(create_srv.pad, create_dims.pad_elem_h, create_dims.pad_elem_w, create_axis.pad_elems_y[index], create_axis.pad_elems_x[index]);
                }

                global_wnds.note_w = newwin(global_dims.note_h, global_dims.note_w, global_axis.note_y, global_axis.note_x);
                global_wnds.note_sw = derwin(global_wnds.note_w, global_dims.note_h-2, global_dims.note_w-2, 1, 1);

                box(global_wnds.wnd, ACS_VLINE, ACS_HLINE);
                box(create_srv.srv_info_w, ACS_VLINE, ACS_HLINE);
                box(create_srv.pad_border, ACS_VLINE, ACS_HLINE);
                box(create_srv.btns[0].wnd, ACS_VLINE, ACS_HLINE);
                box(create_srv.btns[1].wnd, ACS_VLINE, ACS_HLINE);
                box(create_srv.btns[2].wnd, ACS_VLINE, ACS_HLINE);
                box(global_wnds.note_w, ' ', ' ');

                mvwprintw(global_wnds.wnd, 0, ((global_dims.wnd_w-strlen(CREATE_SCR_LABEL))/2), CREATE_SCR_LABEL);

                wprintw(create_srv.srv_info_sw, CREATE_SCR_SRV_INFO_TEMPLATE, "Default name", "Not restricted", "127.0.0.1:27015");
                for (index = 0; index < 7; ++index)
                {
                    mvwprintw(create_srv.pad_elems[index].wnd, 0, 0, create_srv.pad_elems[index].lbl);
                }
                for (index = 0; index < 3; ++index)
                {
                    mvwprintw(create_srv.btns[index].wnd, 1, (create_dims.btns_w-strlen(create_srv.btns[index].lbl))/2, create_srv.btns[index].lbl);
                }
                switch (create_srv.mode)
                {
                    case MODE_PAD:
                        wattron(create_srv.pad_elems[create_srv.line].wnd, A_UNDERLINE | A_BOLD);
                        mvwprintw(create_srv.pad_elems[create_srv.line].wnd, 0, 0, create_srv.pad_elems[create_srv.line].lbl);
                        wattroff(create_srv.pad_elems[create_srv.line].wnd, A_UNDERLINE | A_BOLD);
                        // mvwaddch(create_srv.pad_elems[create_srv.line].wnd, 0, create_axis.pad_vdelim, ACS_VLINE);
                        break;
                    case MODE_BUTTONS:
                        wattron(create_srv.btns[create_srv.selection].wnd, A_BOLD | A_UNDERLINE);
                        mvwprintw(create_srv.btns[create_srv.selection].wnd, 1, (create_dims.btns_w-strlen(create_srv.btns[create_srv.selection].lbl))/2, create_srv.btns[create_srv.selection].lbl);
                        wattroff(create_srv.btns[create_srv.selection].wnd, A_BOLD | A_UNDERLINE);
                        break;
                    default:
                        break;
                }

                mvwaddch(create_srv.btns[1].wnd, 0, 0, ACS_BSSS);
                mvwaddch(create_srv.btns[1].wnd, create_dims.btns_h-1, 0, ACS_BTEE);
                mvwaddch(create_srv.btns[2].wnd, 0, 0, ACS_BSSS);
                mvwaddch(create_srv.btns[2].wnd, create_dims.btns_h-1, 0, ACS_BTEE);
                mvwaddch(create_srv.pad_border, 0, create_axis.pad_vdelim+1, ACS_BSSS);
                mvwvline(create_srv.pad, 0, create_axis.pad_vdelim, ACS_VLINE, create_dims.pad_h);
                for(index = create_dims.pad_elem_h; index < create_dims.pad_h; index+=2)
                {
                    if (index < create_dims.pad_border_h-1)
                    {
                        mvwaddch(create_srv.pad_border, index+create_dims.pad_elem_h, 0, ACS_LTEE);
                    }
                    mvwhline(create_srv.pad, index, 0, ACS_HLINE, create_dims.pad_w);
                    mvwaddch(create_srv.pad, index, create_axis.pad_vdelim, ACS_PLUS);
                    if (index < create_dims.pad_border_h-1)
                    {
                        mvwaddch(create_srv.pad_border, index+create_dims.pad_elem_h, create_dims.pad_border_w-1, ACS_RTEE);
                    }
                    
                }
                mvwaddch(create_srv.pad_border, create_dims.pad_border_h-1, create_axis.pad_vdelim+1, ACS_BTEE);

                mvwprintw(global_wnds.note_sw, 0, (((global_dims.note_w-2)-note_labels.create_srv.size)/2), note_labels.create_srv.text);
            }
            break;
        case WND_PREFS:
            {
                if (wnd_type == cur_wnd)
                {
                    wclear(stdscr);
                    refresh();

                    delwin(global_wnds.note_sw);
                    delwin(global_wnds.note_w);

                    delwin(cfg_wnd.pad);
                    delwin(cfg_wnd.pad_border);
                    
                    delwin(global_wnds.wnd);

                    global_wnds.wnd = NULL;

                    cfg_wnd.pad_border = NULL;
                    cfg_wnd.pad = NULL;

                    global_wnds.note_w = NULL;
                    global_wnds.note_sw = NULL;
                }
                else
                {
                    cfg_wnd.entries[0].entry.lbl = malloc(strlen(PREFS_USERNAME_LABEL)+1);
                    cfg_wnd.entries[1].entry.lbl = malloc(strlen(PREFS_LANG_LABEL)+1);
                    cfg_wnd.entries[2].entry.lbl = malloc(strlen(PREFS_IP_LABEL)+1);
                    cfg_wnd.entries[3].entry.lbl = malloc(strlen(PREFS_PORT_LABEL)+1);

                    strcpy(cfg_wnd.entries[0].entry.lbl, PREFS_USERNAME_LABEL);
                    strcpy(cfg_wnd.entries[1].entry.lbl, PREFS_LANG_LABEL);
                    strcpy(cfg_wnd.entries[2].entry.lbl, PREFS_IP_LABEL);
                    strcpy(cfg_wnd.entries[3].entry.lbl, PREFS_PORT_LABEL);

                    cfg_wnd.entries[0].type = CFG_TYPE_STRING;
                    cfg_wnd.entries[1].type = CFG_TYPE_LIST;
                    cfg_wnd.entries[2].type = CFG_TYPE_STRING;
                    cfg_wnd.entries[3].type = CFG_TYPE_INT;

                    cfg_wnd.entries[0].value_ptr = (void *)&config.name;
                    cfg_wnd.entries[1].value_ptr = (void *)&config.language;
                    cfg_wnd.entries[2].value_ptr = (void *)&config.ip;
                    cfg_wnd.entries[3].value_ptr = (void *)&config.port;

                    cfg_wnd.entries[0].ptr_size = sizeof(config.name);
                    cfg_wnd.entries[1].ptr_size = sizeof(config.language);
                    cfg_wnd.entries[2].ptr_size = sizeof(config.ip);
                    cfg_wnd.entries[3].ptr_size = sizeof(config.port);

                    cfg_wnd.entries[1].value_list.size = 2;
                    cfg_wnd.entries[1].value_list.options = malloc(cfg_wnd.entries[1].value_list.size*sizeof(struct option_t));
                    if (cfg_wnd.entries[1].value_list.options != NULL)
                    {
                        strcpy(cfg_wnd.entries[1].value_list.options[0].option, PREFS_LANG_EN_LABEL);
                        strcpy(cfg_wnd.entries[1].value_list.options[1].option, PREFS_LANG_RU_LABEL);

                        cfg_wnd.entries[1].value_list.options[0].value = LANG_EN;
                        cfg_wnd.entries[1].value_list.options[1].value = LANG_RU;

                        for (index = 0; index < cfg_wnd.entries[1].value_list.size; ++index)
                        {
                            cfg_wnd.entries[1].value_list.options[index].str_len = strlen(cfg_wnd.entries[1].value_list.options[index].option);
                            if (config.language == cfg_wnd.entries[1].value_list.options[index].value)
                            {
                                cfg_wnd.entries[1].selection = index;
                            }
                        }
                    }

                    strncpy(cfg_wnd.entries[0].value_str.text, config.name, LABEL_LEN);
                    strncpy(cfg_wnd.entries[2].value_str.text, config.ip, LABEL_LEN);
                    snprintf(cfg_wnd.entries[3].value_str.text, LABEL_LEN, "%d", config.port);

                    cfg_wnd.entries[0].value_str.str_len = strlen(cfg_wnd.entries[0].value_str.text);
                    cfg_wnd.entries[2].value_str.str_len = strlen(cfg_wnd.entries[2].value_str.text);
                    cfg_wnd.entries[3].value_str.str_len = strlen(cfg_wnd.entries[3].value_str.text);

                    cfg_wnd.entries[0].selection = cfg_wnd.entries[0].value_str.str_len;
                    cfg_wnd.entries[2].selection = cfg_wnd.entries[2].value_str.str_len;
                    cfg_wnd.entries[3].selection = cfg_wnd.entries[3].value_str.str_len;
                }

                global_wnds.wnd = newwin(global_dims.wnd_h, global_dims.wnd_w, global_axis.wnd_y, global_axis.wnd_x);

                cfg_wnd.pad_border = derwin(global_wnds.wnd, cfg_dims.pad_border_h, cfg_dims.pad_border_w, cfg_axis.pad_border_y, cfg_axis.pad_border_x);

                cfg_wnd.pad = newpad(cfg_dims.pad_h, cfg_dims.pad_w);

                for (index = 0; index < 4; ++index)
                    cfg_wnd.entries[index].entry.wnd = subpad(cfg_wnd.pad, cfg_dims.entry_h, cfg_dims.entry_w, cfg_axis.entries_y[index], cfg_axis.entries_x[index]);

                global_wnds.note_w = newwin(global_dims.note_h, global_dims.note_w, global_axis.note_y, global_axis.note_x);
                global_wnds.note_sw = derwin(global_wnds.note_w, global_dims.note_h-2, global_dims.note_w-2, 1, 1);

                box(global_wnds.wnd, ACS_VLINE, ACS_HLINE);
                box(cfg_wnd.pad_border, ACS_VLINE, ACS_HLINE);
                box(global_wnds.note_w, ' ', ' ');

                mvwaddch(cfg_wnd.pad_border, 0, cfg_dims.entry_hspacer+cfg_axis.vdelim_x+1, ACS_BSSS);
                for(index = cfg_axis.entries_y[1]-1; index < cfg_dims.pad_h; index+=1+cfg_dims.entry_h)
                {
                    if (index < cfg_dims.pad_border_h-1)
                    {
                        mvwaddch(cfg_wnd.pad_border, index+cfg_dims.entry_h, 0, ACS_LTEE);
                        mvwaddch(cfg_wnd.pad_border, index+cfg_dims.entry_h, cfg_dims.pad_border_w-1, ACS_RTEE);
                    }
                    mvwhline(cfg_wnd.pad, index, 0, ACS_HLINE, create_dims.pad_w);
                    if (index == cfg_dims.pad_h-1)
                        mvwaddch(cfg_wnd.pad, index, cfg_dims.entry_hspacer+cfg_axis.vdelim_x, ACS_BTEE);
                    else
                        mvwaddch(cfg_wnd.pad, index, cfg_dims.entry_hspacer+cfg_axis.vdelim_x, ACS_PLUS);
                    
                }

                // for (index = 0; index < 4; ++index)
                    
                // wbkgd(cfg_wnd.entries[cfg_wnd.line].entry.wnd, COLOR_PAIR(2));
                mvwprintw(global_wnds.wnd, 0, ((global_dims.wnd_w-strlen(PREFS_SCR_LABEL))/2), PREFS_SCR_LABEL);

                wattron(cfg_wnd.entries[cfg_wnd.line].entry.wnd, A_UNDERLINE | A_BOLD);
                for (index = 0; index < 4; ++index)
                {
                    mvwprintw(cfg_wnd.entries[index].entry.wnd, 0, 0, cfg_wnd.entries[index].entry.lbl);
                }
                wattroff(cfg_wnd.entries[cfg_wnd.line].entry.wnd, A_UNDERLINE | A_BOLD);

                for (index = 0; index < 4; ++index)
                {
                    mvwaddch(cfg_wnd.entries[index].entry.wnd, 0, cfg_axis.vdelim_x, ACS_VLINE);
                    switch (cfg_wnd.entries[index].type)
                    {
                    case CFG_TYPE_STRING:
                    case CFG_TYPE_INT:
                    case CFG_TYPE_FLOAT:
                        mvwprintw(cfg_wnd.entries[index].entry.wnd, 0, cfg_axis.fields_x[index], cfg_wnd.entries[index].value_str.text);
                        if (index == cfg_wnd.line)
                        {
                            move(cfg_axis.vis_pad_ys+cfg_axis.entries_y[cfg_wnd.line], cfg_axis.vis_pad_xs+cfg_axis.entries_x[cfg_wnd.line]+cfg_axis.fields_x[cfg_wnd.line]+cfg_wnd.entries[cfg_wnd.line].selection);
                            curs_set(1);
                        }
                        break;
                    case CFG_TYPE_LIST:
                        if (index == cfg_wnd.line)
                        {
                            curs_set(0);
                            wattron(cfg_wnd.entries[index].entry.wnd, A_BOLD);
                            mvwaddch(cfg_wnd.entries[index].entry.wnd, 0, cfg_axis.fields_x[index]-1, '<');
                            wattroff(cfg_wnd.entries[index].entry.wnd, A_BOLD);

                            mvwprintw(cfg_wnd.entries[index].entry.wnd, 0, cfg_axis.fields_x[index]+((LABEL_LEN-cfg_wnd.entries[index].value_list.options[cfg_wnd.entries[index].selection].str_len)/2), "%s", cfg_wnd.entries[index].value_list.options[cfg_wnd.entries[index].selection].option);

                            wattron(cfg_wnd.entries[index].entry.wnd, A_BOLD);
                            mvwaddch(cfg_wnd.entries[index].entry.wnd, 0, cfg_axis.fields_x[index]+LABEL_LEN, '>');
                            wattroff(cfg_wnd.entries[index].entry.wnd, A_BOLD);
                        }
                        else
                            mvwaddch(cfg_wnd.entries[index].entry.wnd, 0, cfg_axis.fields_x[index]-1, '<');
                            mvwprintw(cfg_wnd.entries[index].entry.wnd, 0, cfg_axis.fields_x[index]+((LABEL_LEN-cfg_wnd.entries[index].value_list.options[cfg_wnd.entries[index].selection].str_len)/2), "%s", cfg_wnd.entries[index].value_list.options[cfg_wnd.entries[index].selection].option);
                            mvwaddch(cfg_wnd.entries[index].entry.wnd, 0, cfg_axis.fields_x[index]+LABEL_LEN, '>');
                        break;
                    default:
                        break;
                    }
                }

                mvwprintw(global_wnds.note_sw, 0, (((global_dims.note_w-2)-note_labels.prefs_wnd.size)/2), note_labels.prefs_wnd.text);
            }
            break;
        default:
            ret = EXIT_FAILURE;
            break;
    }

    switch (ret)
    {
        case EXIT_SUCCESS:
            {
                cur_wnd = wnd_type;
                _update_window();
                keypad(global_wnds.wnd, true);
            }
            break;
        case EXIT_FAILURE:
        default:
            break;
    }

    return ret;
}

int _update_window(void)
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
                wnoutrefresh(global_wnds.wnd);
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
                wnoutrefresh(global_wnds.note_w);
                wnoutrefresh(global_wnds.note_sw);
                wnoutrefresh(stdscr);
            }
            break;
        case WND_JOIN_SRV:
            {
                wnoutrefresh(global_wnds.wnd);
                wnoutrefresh(join_srv.top_panel);
                wnoutrefresh(join_srv.sname_w);
                wnoutrefresh(join_srv.saddr_w);
                wnoutrefresh(join_srv.susers_w);
                wnoutrefresh(join_srv.pad_border);
                wnoutrefresh(join_srv.servers_pad);
                wnoutrefresh(join_srv.caddr_w);
                wnoutrefresh(join_srv.label_w);
                wnoutrefresh(join_srv.tb_w);
                wnoutrefresh(join_srv.btns_border);
                wnoutrefresh(join_srv.btns[0].wnd);
                wnoutrefresh(join_srv.btns[1].wnd);
                wnoutrefresh(join_srv.btns[2].wnd);
                wnoutrefresh(global_wnds.note_w);
                wnoutrefresh(global_wnds.note_sw);
                pnoutrefresh(join_srv.servers_pad, join_axis.pad_ys, join_axis.pad_xs, join_axis.vis_pad_ys, join_axis.vis_pad_xs, join_axis.vis_pad_ye, join_axis.vis_pad_xe);
                wnoutrefresh(stdscr);
            }
            break;
        case WND_CREATE_SRV:
            {
                wnoutrefresh(global_wnds.wnd);
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
                wnoutrefresh(global_wnds.note_w);
                wnoutrefresh(global_wnds.note_sw);
                pnoutrefresh(create_srv.pad, create_axis.pad_ys, create_axis.pad_xs, create_axis.vis_pad_ys, create_axis.vis_pad_xs, create_axis.vis_pad_ye, create_axis.vis_pad_xe);
                wnoutrefresh(stdscr);
            }
            break;
        case WND_PREFS:
            {
                wnoutrefresh(global_wnds.wnd);
                wnoutrefresh(cfg_wnd.pad_border);
                for (index = 0; index < 4; ++index)
                    wnoutrefresh(cfg_wnd.entries[index].entry.wnd);
                wnoutrefresh(global_wnds.note_w);
                wnoutrefresh(global_wnds.note_sw);
                pnoutrefresh(cfg_wnd.pad, cfg_axis.pad_ys, cfg_axis.pad_xs, cfg_axis.vis_pad_ys, cfg_axis.vis_pad_xs, cfg_axis.vis_pad_ye, cfg_axis.vis_pad_xe);
                wnoutrefresh(stdscr);
            }
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
                delwin(global_wnds.note_sw);
                delwin(global_wnds.note_w);
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
                delwin(global_wnds.wnd);

                global_wnds.wnd = NULL;
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
                global_wnds.note_w = NULL;
                global_wnds.note_sw = NULL;

                main_menu.selection = 0;

                cur_wnd = WND_NONE;
            }
            break;
        case WND_JOIN_SRV:
            {
                if (join_srv.mode == MODE_TEXTBOX)
                {
                    curs_set(0);
                }

                delwin(global_wnds.note_sw);
                delwin(global_wnds.note_w);
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
                delwin(global_wnds.wnd);

                global_wnds.wnd = NULL;
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
                for (index = 0; index < 3; ++index)
                {
                    join_srv.btns[index].wnd = NULL;
                    join_srv.btns[index].lbl = NULL;
                }
                global_wnds.note_w = NULL;
                global_wnds.note_sw = NULL;

                cur_wnd = WND_NONE;
            }
            break;
        case WND_CREATE_SRV:
            {
                delwin(global_wnds.note_sw);
                delwin(global_wnds.note_w);

                delwin(create_srv.auto_port_w);
                delwin(create_srv.lcl_addr_w);
                delwin(create_srv.port_w);
                delwin(create_srv.addr_w);
                delwin(create_srv.rusers_w);
                delwin(create_srv.musers_w);
                delwin(create_srv.sname_w);
                for (index = 0; index < 7; ++index)
                {
                    if (create_srv.pad_elems[index].lbl != NULL)
                        free(create_srv.pad_elems[index].lbl);
                    delwin(create_srv.pad_elems[index].wnd);
                }
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
                delwin(global_wnds.wnd);

                global_wnds.wnd = NULL;

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

                global_wnds.note_w = NULL;
                global_wnds.note_sw = NULL;

                cur_wnd = WND_NONE;
            }
            break;
        case WND_PREFS:
            {
                curs_set(0);

                delwin(global_wnds.note_sw);
                delwin(global_wnds.note_w);
                for (index = 0; index < sizeof(cfg_wnd.entries)/sizeof(struct cfg_entry_t); ++index)
                {
                    if (cfg_wnd.entries[index].entry.lbl != NULL)
                        free(cfg_wnd.entries[index].entry.lbl);
                    delwin(cfg_wnd.entries[index].entry.wnd);

                    if (cfg_wnd.entries[index].type == CFG_TYPE_LIST && cfg_wnd.entries[index].value_list.options != NULL)
                    {
                        free(cfg_wnd.entries[index].value_list.options);
                        cfg_wnd.entries[index].value_list.options = NULL;
                    }
                }
                delwin(cfg_wnd.pad);
                delwin(cfg_wnd.pad_border);
                delwin(global_wnds.wnd);

                global_wnds.wnd = NULL;
                cfg_wnd.pad_border = NULL;
                cfg_wnd.pad = NULL;
                for (index = 0; index < 4; ++index)
                {
                    cfg_wnd.entries[index].entry.wnd = NULL;
                    cfg_wnd.entries[index].entry.lbl = NULL;
                }
                global_wnds.note_w = NULL;
                global_wnds.note_sw = NULL;

                cur_wnd = WND_NONE;
            }
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

    wclear(popup_w);

    wnoutrefresh(popup_w);
    doupdate();

    delwin(popup_sw);
    delwin(popup_w);

    _draw_window(cur_wnd);

    return ret;
}

int menu_wnd(int *option)
{
    int symbol;
    int index;
    int run_flag = 1;
    int ret = EXIT_SUCCESS;
    main_menu.selection = *option-1;

    _draw_window(WND_MAIN_MENU);

	while(run_flag != 0)
	{
		symbol = wgetch(global_wnds.wnd);

        switch (symbol)
        {
            case '\n':
                {
                    switch (main_menu.selection)
                    {
                    case 0:
                        *option = WND_JOIN_SRV;
                        break;
                    case 1:
                        *option = WND_CREATE_SRV;
                        break;
                    case 2:
                        *option = WND_PREFS;
                        break;
                    case 3:
                    default:
                        *option = WND_NONE;
                        break;
                    }
                    run_flag = 0;
                }
                break;
            case '\t':
                {
                    mvwprintw(main_menu.btns[main_menu.selection].wnd, 1, (mmenu_dims.btns_w-strlen(main_menu.btns[main_menu.selection].lbl))/2, main_menu.btns[main_menu.selection].lbl);

                    if (main_menu.selection == 3)
                        main_menu.selection = 0;
                    else
                        main_menu.selection++;

                    wattron(main_menu.btns[main_menu.selection].wnd, A_BOLD | A_UNDERLINE);
                    mvwprintw(main_menu.btns[main_menu.selection].wnd, 1, (mmenu_dims.btns_w-strlen(main_menu.btns[main_menu.selection].lbl))/2, main_menu.btns[main_menu.selection].lbl);
                    wattroff(main_menu.btns[main_menu.selection].wnd, A_BOLD | A_UNDERLINE);

                    _update_window();
                }
                break;
            case KEY_UP:
                {
                    if (main_menu.selection > 0)
                    {
                        mvwprintw(main_menu.btns[main_menu.selection].wnd, 1, (mmenu_dims.btns_w-strlen(main_menu.btns[main_menu.selection].lbl))/2, main_menu.btns[main_menu.selection].lbl);

                        main_menu.selection--;

                        wattron(main_menu.btns[main_menu.selection].wnd, A_BOLD | A_UNDERLINE);
                        mvwprintw(main_menu.btns[main_menu.selection].wnd, 1, (mmenu_dims.btns_w-strlen(main_menu.btns[main_menu.selection].lbl))/2, main_menu.btns[main_menu.selection].lbl);
                        wattroff(main_menu.btns[main_menu.selection].wnd, A_BOLD | A_UNDERLINE);

                        _update_window();
                    }
                }
                break;
            case KEY_DOWN:
                {
                    if (main_menu.selection < 3)
                    {
                        mvwprintw(main_menu.btns[main_menu.selection].wnd, 1, (mmenu_dims.btns_w-strlen(main_menu.btns[main_menu.selection].lbl))/2, main_menu.btns[main_menu.selection].lbl);

                        main_menu.selection++;

                        wattron(main_menu.btns[main_menu.selection].wnd, A_BOLD | A_UNDERLINE);
                        mvwprintw(main_menu.btns[main_menu.selection].wnd, 1, (mmenu_dims.btns_w-strlen(main_menu.btns[main_menu.selection].lbl))/2, main_menu.btns[main_menu.selection].lbl);
                        wattroff(main_menu.btns[main_menu.selection].wnd, A_BOLD | A_UNDERLINE);

                        _update_window();
                    }
                }
                break;
            case KEY_F(1):
                {
                }
                break;
            case KEY_F(2):
                {
                    if (connection_flag == STATUS_CONNECTED)
                    {
                        disconnect_from_main_server();
                        if (connect_to_main_server() != EXIT_FAILURE)
                            client_send(CONNECT_COMM, WAIT_TRUE, RECV_TIMEOUT);
                    }
                    else
                    {
                        if (connect_to_main_server() != EXIT_FAILURE)
                            client_send(CONNECT_COMM, WAIT_TRUE, RECV_TIMEOUT);
                    }
                }
                break;
            case KEY_F(3):
                {
                    char ch[21];
                    snprintf(ch, 20, "conn flag: %d\0", connection_flag);
                    popup_wnd(ch, POPUP_W_WAIT);
                    if (connection_flag == STATUS_CONNECTED)
                    {
                        disconnect_from_main_server();
                    }
                }
                break;
            case KEY_F(4):
                {
                    *option = WND_NONE;
                    run_flag = 0;
                }
                break;
            default:
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
    int run_flag = 1;
    int ret = EXIT_SUCCESS;
    
    join_srv.mode = MODE_LIST;
    join_srv.line = 0;
    join_srv.selection = 0;

    join_dims.pad_h = sizeof(servers_info)/sizeof(struct server_info_t);
    join_axis.pad_ys = 0;
    join_axis.pad_xs = 0;
    join_axis.pad_ye = join_dims.vis_pad_h - 1;
    join_axis.pad_xe = join_dims.vis_pad_w - 1;

    _draw_window(WND_JOIN_SRV);

    while(run_flag != 0)
    {
        symbol = wgetch(global_wnds.wnd);

        switch (symbol)
        {
            case '\n':
                {
                    char *ch = servers_info[join_srv.line].server_name;
                    popup_wnd(ch, POPUP_W_WAIT);
                }
                break;
            case '\t':
                {
                    switch (join_srv.mode)
                    {
                        case MODE_LIST:
                            mvwhline(join_srv.servers_pad, join_srv.line, 0, ' ', join_dims.pad_w);
                            mvwprintw(join_srv.servers_pad, join_srv.line, 0, "%s", servers_info[join_srv.line].server_name);
                            mvwprintw(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-1, "%s:%d", servers_info[join_srv.line].ip, servers_info[join_srv.line].port);
                            if (servers_info[join_srv.line].max_users != 0)
                                mvwprintw(join_srv.servers_pad, join_srv.line, join_axis.susers_x-1, "%d/%d", servers_info[join_srv.line].cur_users, servers_info[join_srv.line].max_users);
                            else
                                mvwprintw(join_srv.servers_pad, join_srv.line, join_axis.susers_x-1, "%d", servers_info[join_srv.line].cur_users);
                            mvwaddch(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-2, ACS_VLINE);
                            mvwaddch(join_srv.servers_pad, join_srv.line, join_axis.susers_x-2, ACS_VLINE);

                            join_srv.mode = MODE_TEXTBOX;
                            
                            move(global_axis.wnd_y+join_axis.caddr_y+join_axis.tb_y, global_axis.wnd_x+join_axis.caddr_x+join_axis.tb_x);
                            curs_set(1);
                            break;
                        case MODE_TEXTBOX:
                            curs_set(0);

                            join_srv.mode = MODE_LIST;

                            wattron(join_srv.servers_pad, COLOR_PAIR(2));
                            mvwhline(join_srv.servers_pad, join_srv.line, 0, ' ', join_dims.pad_w);
                            mvwprintw(join_srv.servers_pad, join_srv.line, 0, "%s", servers_info[join_srv.line].server_name);
                            mvwprintw(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-1, "%s:%d", servers_info[join_srv.line].ip, servers_info[join_srv.line].port);
                            if (servers_info[join_srv.line].max_users != 0)
                                mvwprintw(join_srv.servers_pad, join_srv.line, join_axis.susers_x-1, "%d/%d", servers_info[join_srv.line].cur_users, servers_info[join_srv.line].max_users);
                            else
                                mvwprintw(join_srv.servers_pad, join_srv.line, join_axis.susers_x-1, "%d", servers_info[join_srv.line].cur_users);
                            mvwaddch(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-2, ACS_VLINE);
                            mvwaddch(join_srv.servers_pad, join_srv.line, join_axis.susers_x-2, ACS_VLINE);
                            wattroff(join_srv.servers_pad, COLOR_PAIR(2));
                            break;
                        default:
                            break;
                    }
                    _update_window();
                }
                break;
            case KEY_UP:
                {
                    if (join_srv.line > 0)
                    {
                        mvwhline(join_srv.servers_pad, join_srv.line, 0, ' ', join_dims.pad_w);
                        mvwprintw(join_srv.servers_pad, join_srv.line, 0, "%s", servers_info[join_srv.line].server_name);
                        mvwprintw(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-1, "%s:%d", servers_info[join_srv.line].ip, servers_info[join_srv.line].port);
                        if (servers_info[join_srv.line].max_users != 0)
                            mvwprintw(join_srv.servers_pad, join_srv.line, join_axis.susers_x-1, "%d/%d", servers_info[join_srv.line].cur_users, servers_info[join_srv.line].max_users);
                        else
                            mvwprintw(join_srv.servers_pad, join_srv.line, join_axis.susers_x-1, "%d", servers_info[join_srv.line].cur_users);
                        mvwaddch(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-2, ACS_VLINE);
                        mvwaddch(join_srv.servers_pad, join_srv.line, join_axis.susers_x-2, ACS_VLINE);

                        join_srv.line--;

                        wattron(join_srv.servers_pad, COLOR_PAIR(2));
                        mvwhline(join_srv.servers_pad, join_srv.line, 0, ' ', join_dims.pad_w);
                        mvwprintw(join_srv.servers_pad, join_srv.line, 0, "%s", servers_info[join_srv.line].server_name);
                        mvwprintw(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-1, "%s:%d", servers_info[join_srv.line].ip, servers_info[join_srv.line].port);
                        if (servers_info[join_srv.line].max_users != 0)
                            mvwprintw(join_srv.servers_pad, join_srv.line, join_axis.susers_x-1, "%d/%d", servers_info[join_srv.line].cur_users, servers_info[join_srv.line].max_users);
                        else
                            mvwprintw(join_srv.servers_pad, join_srv.line, join_axis.susers_x-1, "%d", servers_info[join_srv.line].cur_users);
                        mvwaddch(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-2, ACS_VLINE);
                        mvwaddch(join_srv.servers_pad, join_srv.line, join_axis.susers_x-2, ACS_VLINE);
                        wattroff(join_srv.servers_pad, COLOR_PAIR(2));

                        if (join_srv.line < join_axis.pad_ys)
                        {
                            join_axis.pad_ys += join_srv.line - join_axis.pad_ys;
                            join_axis.pad_ye = join_axis.pad_ys + join_dims.vis_pad_h - 1;
                        }

                        _update_window();
                    }
                }
                break;
            case KEY_DOWN:
                {
                    if (join_srv.line < (sizeof(servers_info)/sizeof(struct server_info_t))-1)
                    {
                        mvwhline(join_srv.servers_pad, join_srv.line, 0, ' ', join_dims.pad_w);
                        mvwprintw(join_srv.servers_pad, join_srv.line, 0, "%s", servers_info[join_srv.line].server_name);
                        mvwprintw(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-1, "%s:%d", servers_info[join_srv.line].ip, servers_info[join_srv.line].port);
                        if (servers_info[join_srv.line].max_users != 0)
                            mvwprintw(join_srv.servers_pad, join_srv.line, join_axis.susers_x-1, "%d/%d", servers_info[join_srv.line].cur_users, servers_info[join_srv.line].max_users);
                        else
                            mvwprintw(join_srv.servers_pad, join_srv.line, join_axis.susers_x-1, "%d", servers_info[join_srv.line].cur_users);
                        mvwaddch(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-2, ACS_VLINE);
                        mvwaddch(join_srv.servers_pad, join_srv.line, join_axis.susers_x-2, ACS_VLINE);

                        join_srv.line++;

                        wattron(join_srv.servers_pad, COLOR_PAIR(2));
                        mvwhline(join_srv.servers_pad, join_srv.line, 0, ' ', join_dims.pad_w);
                        mvwprintw(join_srv.servers_pad, join_srv.line, 0, "%s", servers_info[join_srv.line].server_name);
                        mvwprintw(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-1, "%s:%d", servers_info[join_srv.line].ip, servers_info[join_srv.line].port);
                        if (servers_info[join_srv.line].max_users != 0)
                            mvwprintw(join_srv.servers_pad, join_srv.line, join_axis.susers_x-1, "%d/%d", servers_info[join_srv.line].cur_users, servers_info[join_srv.line].max_users);
                        else
                            mvwprintw(join_srv.servers_pad, join_srv.line, join_axis.susers_x-1, "%d", servers_info[join_srv.line].cur_users);
                        mvwaddch(join_srv.servers_pad, join_srv.line, join_axis.saddr_x-2, ACS_VLINE);
                        mvwaddch(join_srv.servers_pad, join_srv.line, join_axis.susers_x-2, ACS_VLINE);
                        wattroff(join_srv.servers_pad, COLOR_PAIR(2));

                        if (join_srv.line > join_axis.pad_ye)
                        {
                            join_axis.pad_ys += join_srv.line - join_axis.pad_ye;
                            join_axis.pad_ye = join_axis.pad_ys + join_dims.vis_pad_h - 1;
                        }

                        _update_window();
                    }
                }
                break;
            case KEY_LEFT:
                {
                    if (join_srv.selection > 0)
                    {
                        mvwprintw(join_srv.btns[join_srv.selection].wnd, 0, (join_dims.btns_w-strlen(join_srv.btns[join_srv.selection].lbl))/2, join_srv.btns[join_srv.selection].lbl);

                        join_srv.selection--;

                        wattron(join_srv.btns[join_srv.selection].wnd, A_BOLD | A_UNDERLINE);
                        mvwprintw(join_srv.btns[join_srv.selection].wnd, 0, (join_dims.btns_w-strlen(join_srv.btns[join_srv.selection].lbl))/2, join_srv.btns[join_srv.selection].lbl);
                        wattroff(join_srv.btns[join_srv.selection].wnd, A_BOLD | A_UNDERLINE);

                        _update_window();
                    }
                }
                break;
            case KEY_RIGHT:
                {
                    if (join_srv.selection < 2)
                    {
                        mvwprintw(join_srv.btns[join_srv.selection].wnd, 0, (join_dims.btns_w-strlen(join_srv.btns[join_srv.selection].lbl))/2, join_srv.btns[join_srv.selection].lbl);

                        join_srv.selection++;

                        wattron(join_srv.btns[join_srv.selection].wnd, A_BOLD | A_UNDERLINE);
                        mvwprintw(join_srv.btns[join_srv.selection].wnd, 0, (join_dims.btns_w-strlen(join_srv.btns[join_srv.selection].lbl))/2, join_srv.btns[join_srv.selection].lbl);
                        wattroff(join_srv.btns[join_srv.selection].wnd, A_BOLD | A_UNDERLINE);

                        _update_window();
                    }
                }
                break;
            case KEY_F(1):

                break;
            case KEY_F(2):
                {
                }
                break;
            case KEY_F(3):
                {
                    *option = WND_MAIN_MENU;
                    run_flag = 0;
                }
                break;
            case KEY_F(4):
                {
                    *option = WND_NONE;
                    run_flag = 0;
                }
                break;
            default:
                break;
        }
    }

    _delete_window();

    return ret;
}

int create_srv_wnd(int *option)
{
    int symbol;
    int index;
    int run_flag = 1;
    int ret = EXIT_SUCCESS;

    create_srv.mode = MODE_PAD;
    create_srv.line = 0;
    create_srv.selection = 0;

    create_axis.pad_ys = 0;
    create_axis.pad_xs = 0;
    create_axis.pad_ye = create_dims.vis_pad_h - 1;
    create_axis.pad_xe = create_dims.vis_pad_w - 1;

    _draw_window(WND_CREATE_SRV);

    while(run_flag != 0)
    {
        symbol = wgetch(global_wnds.wnd);

        switch (symbol)
        {
            case '\n':
                {
                    _update_window();
                }
                break;
            case '\t':
                {
                    switch (create_srv.mode)
                    {
                        case MODE_PAD:
                            mvwprintw(create_srv.pad_elems[create_srv.line].wnd, 0, 0, create_srv.pad_elems[create_srv.line].lbl);
                            wmove(create_srv.pad_elems[create_srv.line].wnd, 0, create_axis.pad_vdelim);
                            waddch(create_srv.pad_elems[create_srv.line].wnd, ACS_VLINE);

                            create_srv.mode = MODE_BUTTONS;
                            
                            wattron(create_srv.btns[create_srv.selection].wnd, A_BOLD | A_UNDERLINE);
                            wmove(create_srv.btns[create_srv.selection].wnd, 1, (create_dims.btns_w/2)-strlen(create_srv.btns[create_srv.selection].lbl)/2);
                            wprintw(create_srv.btns[create_srv.selection].wnd, "%s", create_srv.btns[create_srv.selection].lbl);
                            wattroff(create_srv.btns[create_srv.selection].wnd, A_BOLD | A_UNDERLINE);
                            break;
                        case MODE_BUTTONS:
                            wmove(create_srv.btns[create_srv.selection].wnd, 1, (create_dims.btns_w/2)-strlen(create_srv.btns[create_srv.selection].lbl)/2);
                            wprintw(create_srv.btns[create_srv.selection].wnd, "%s", create_srv.btns[create_srv.selection].lbl);

                            create_srv.mode = MODE_PAD;

                            wattron(create_srv.pad_elems[create_srv.line].wnd, A_UNDERLINE | A_BOLD);
                            mvwprintw(create_srv.pad_elems[create_srv.line].wnd, 0, 0, create_srv.pad_elems[create_srv.line].lbl);
                            wattroff(create_srv.pad_elems[create_srv.line].wnd, A_UNDERLINE | A_BOLD);
                            wmove(create_srv.pad_elems[create_srv.line].wnd, 0, create_axis.pad_vdelim);
                            waddch(create_srv.pad_elems[create_srv.line].wnd, ACS_VLINE);
                            break;
                        default:
                            break;
                    }
                    _update_window();
                }
                break;
            case KEY_UP:
                {
                    if (create_srv.line > 0)
                    {
                        mvwprintw(create_srv.pad_elems[create_srv.line].wnd, 0, 0, create_srv.pad_elems[create_srv.line].lbl);
                        mvwaddch(create_srv.pad_elems[create_srv.line].wnd, 0, create_axis.pad_vdelim, ACS_VLINE);

                        create_srv.line--;

                        wattron(create_srv.pad_elems[create_srv.line].wnd, A_UNDERLINE | A_BOLD);
                        mvwprintw(create_srv.pad_elems[create_srv.line].wnd, 0, 0, create_srv.pad_elems[create_srv.line].lbl);
                        wattroff(create_srv.pad_elems[create_srv.line].wnd, A_UNDERLINE | A_BOLD);
                        mvwaddch(create_srv.pad_elems[create_srv.line].wnd, 0, create_axis.pad_vdelim, ACS_VLINE);

                        if (create_srv.line < create_axis.pad_ys)
                        {
                            create_axis.pad_ys += create_srv.line - create_axis.pad_ys;
                            create_axis.pad_ye = create_axis.pad_ys + create_dims.vis_pad_h - 1;
                        }

                        _update_window();
                    }
                }
                break;
            case KEY_DOWN:
                {
                    if (create_srv.line < 6)
                    {
                        mvwprintw(create_srv.pad_elems[create_srv.line].wnd, 0, 0, create_srv.pad_elems[create_srv.line].lbl);
                        mvwaddch(create_srv.pad_elems[create_srv.line].wnd, 0, create_axis.pad_vdelim, ACS_VLINE);

                        create_srv.line++;

                        wattron(create_srv.pad_elems[create_srv.line].wnd, A_UNDERLINE | A_BOLD);
                        mvwprintw(create_srv.pad_elems[create_srv.line].wnd, 0, 0, create_srv.pad_elems[create_srv.line].lbl);
                        wattroff(create_srv.pad_elems[create_srv.line].wnd, A_UNDERLINE | A_BOLD);
                        mvwaddch(create_srv.pad_elems[create_srv.line].wnd, 0, create_axis.pad_vdelim, ACS_VLINE);

                        if (create_srv.line > create_axis.pad_ye)
                        {
                            create_axis.pad_ys += create_srv.line - create_axis.pad_ye;
                            create_axis.pad_ye = create_axis.pad_ys + create_dims.vis_pad_h - 1;
                        }

                        _update_window();
                    }
                }
                break;
            case KEY_LEFT:
                {
                    if (create_srv.selection > 0)
                    {
                        mvwprintw(create_srv.btns[create_srv.selection].wnd, 1, (create_dims.btns_w-strlen(create_srv.btns[create_srv.selection].lbl))/2, create_srv.btns[create_srv.selection].lbl);

                        create_srv.selection--;

                        wattron(create_srv.btns[create_srv.selection].wnd, A_BOLD | A_UNDERLINE);
                        mvwprintw(create_srv.btns[create_srv.selection].wnd, 1, (create_dims.btns_w-strlen(create_srv.btns[create_srv.selection].lbl))/2, create_srv.btns[create_srv.selection].lbl);
                        wattroff(create_srv.btns[create_srv.selection].wnd, A_BOLD | A_UNDERLINE);

                        _update_window();
                    }
                }
                break;
            case KEY_RIGHT:
                {
                    if (create_srv.selection < 2)
                    {
                        mvwprintw(create_srv.btns[create_srv.selection].wnd, 1, (create_dims.btns_w-strlen(create_srv.btns[create_srv.selection].lbl))/2, create_srv.btns[create_srv.selection].lbl);

                        create_srv.selection++;

                        wattron(create_srv.btns[create_srv.selection].wnd, A_BOLD | A_UNDERLINE);
                        mvwprintw(create_srv.btns[create_srv.selection].wnd, 1, (create_dims.btns_w-strlen(create_srv.btns[create_srv.selection].lbl))/2, create_srv.btns[create_srv.selection].lbl);
                        wattroff(create_srv.btns[create_srv.selection].wnd, A_BOLD | A_UNDERLINE);

                        _update_window();
                    }
                }
                break;
            case KEY_F(1):
                {
                }
                break;
            case KEY_F(2):
                {
                }
                break;
            case KEY_F(3):
                {
                    *option = WND_MAIN_MENU;
                    run_flag = 0;
                }
                break;
            case KEY_F(4):
                {
                    *option = WND_NONE;
                    run_flag = 0;
                }
                break;
            default:
                break;
        }
    }

    _delete_window();

    return ret;
}

int prefs_wnd(int *option)
{
    int symbol;
    int index;
    int run_flag = 1;
    int ret = EXIT_SUCCESS;
    
    int elems_count = sizeof(cfg_wnd.entries)/sizeof(struct cfg_entry_t);
    cfg_dims.pad_h = cfg_dims.entry_h*elems_count + elems_count;
    cfg_wnd.line = 0;

    _draw_window(WND_PREFS);

    while(run_flag != 0)
    {
        symbol = wgetch(global_wnds.wnd);

        switch (symbol)
        {
            case '\n':
                {
                }
                break;
            case '\t':
                {
                }
                break;
            case KEY_UP:
                {
                    if (cfg_wnd.line > 0)
                    {
                        mvwprintw(cfg_wnd.entries[cfg_wnd.line].entry.wnd, 0, 0, cfg_wnd.entries[cfg_wnd.line].entry.lbl);

                        switch (cfg_wnd.entries[cfg_wnd.line].type)
                        {
                        case CFG_TYPE_STRING:
                        case CFG_TYPE_INT:
                        case CFG_TYPE_FLOAT:
                            curs_set(0);
                            break;
                        case CFG_TYPE_LIST:
                            mvwaddch(cfg_wnd.entries[cfg_wnd.line].entry.wnd, 0, cfg_axis.fields_x[cfg_wnd.line]-1, '<');
                            mvwaddch(cfg_wnd.entries[cfg_wnd.line].entry.wnd, 0, cfg_axis.fields_x[cfg_wnd.line]+LABEL_LEN, '>');
                            break;
                        default:
                            break;
                        }

                        cfg_wnd.line--;

                        wattron(cfg_wnd.entries[cfg_wnd.line].entry.wnd, A_UNDERLINE | A_BOLD);
                        mvwprintw(cfg_wnd.entries[cfg_wnd.line].entry.wnd, 0, 0, cfg_wnd.entries[cfg_wnd.line].entry.lbl);
                        wattroff(cfg_wnd.entries[cfg_wnd.line].entry.wnd, A_UNDERLINE | A_BOLD);

                        switch (cfg_wnd.entries[cfg_wnd.line].type)
                        {
                        case CFG_TYPE_STRING:
                        case CFG_TYPE_INT:
                        case CFG_TYPE_FLOAT:
                            move(cfg_axis.vis_pad_ys+cfg_axis.entries_y[cfg_wnd.line], cfg_axis.vis_pad_xs+cfg_axis.entries_x[cfg_wnd.line]+cfg_axis.fields_x[cfg_wnd.line]+cfg_wnd.entries[cfg_wnd.line].selection);
                            curs_set(1);
                            break;
                        case CFG_TYPE_LIST:
                            curs_set(0);

                            wattron(cfg_wnd.entries[cfg_wnd.line].entry.wnd, A_BOLD);
                            mvwaddch(cfg_wnd.entries[cfg_wnd.line].entry.wnd, 0, cfg_axis.fields_x[cfg_wnd.line]-1, '<');
                            wattroff(cfg_wnd.entries[cfg_wnd.line].entry.wnd, A_BOLD);
                            wattron(cfg_wnd.entries[cfg_wnd.line].entry.wnd, A_BOLD);
                            mvwaddch(cfg_wnd.entries[cfg_wnd.line].entry.wnd, 0, cfg_axis.fields_x[cfg_wnd.line]+LABEL_LEN, '>');
                            wattroff(cfg_wnd.entries[cfg_wnd.line].entry.wnd, A_BOLD);
                            break;
                        default:
                            break;
                        }
                    }
                    _update_window();
                }
                break;
            case KEY_DOWN:
                {
                    if (cfg_wnd.line < elems_count-1)
                    {
                        mvwprintw(cfg_wnd.entries[cfg_wnd.line].entry.wnd, 0, 0, cfg_wnd.entries[cfg_wnd.line].entry.lbl);

                        switch (cfg_wnd.entries[cfg_wnd.line].type)
                        {
                        case CFG_TYPE_STRING:
                        case CFG_TYPE_INT:
                        case CFG_TYPE_FLOAT:
                            curs_set(0);
                            break;
                        case CFG_TYPE_LIST:
                            mvwaddch(cfg_wnd.entries[cfg_wnd.line].entry.wnd, 0, cfg_axis.fields_x[cfg_wnd.line]-1, '<');
                            mvwaddch(cfg_wnd.entries[cfg_wnd.line].entry.wnd, 0, cfg_axis.fields_x[cfg_wnd.line]+LABEL_LEN, '>');
                            break;
                        default:
                            break;
                        }

                        cfg_wnd.line++;

                        wattron(cfg_wnd.entries[cfg_wnd.line].entry.wnd, A_UNDERLINE | A_BOLD);
                        mvwprintw(cfg_wnd.entries[cfg_wnd.line].entry.wnd, 0, 0, cfg_wnd.entries[cfg_wnd.line].entry.lbl);
                        wattroff(cfg_wnd.entries[cfg_wnd.line].entry.wnd, A_UNDERLINE | A_BOLD);

                        switch (cfg_wnd.entries[cfg_wnd.line].type)
                        {
                        case CFG_TYPE_STRING:
                        case CFG_TYPE_INT:
                        case CFG_TYPE_FLOAT:
                            move(cfg_axis.vis_pad_ys+cfg_axis.entries_y[cfg_wnd.line], cfg_axis.vis_pad_xs+cfg_axis.entries_x[cfg_wnd.line]+cfg_axis.fields_x[cfg_wnd.line]+cfg_wnd.entries[cfg_wnd.line].selection);
                            curs_set(1);
                            break;
                        case CFG_TYPE_LIST:
                            curs_set(0);

                            wattron(cfg_wnd.entries[cfg_wnd.line].entry.wnd, A_BOLD);
                            mvwaddch(cfg_wnd.entries[cfg_wnd.line].entry.wnd, 0, cfg_axis.fields_x[cfg_wnd.line]-1, '<');
                            wattroff(cfg_wnd.entries[cfg_wnd.line].entry.wnd, A_BOLD);

                            wattron(cfg_wnd.entries[cfg_wnd.line].entry.wnd, A_BOLD);
                            mvwaddch(cfg_wnd.entries[cfg_wnd.line].entry.wnd, 0, cfg_axis.fields_x[cfg_wnd.line]+LABEL_LEN, '>');
                            wattroff(cfg_wnd.entries[cfg_wnd.line].entry.wnd, A_BOLD);
                            break;
                        default:
                            break;
                        }
                        _update_window();
                    }
                }
                break;
            case KEY_LEFT:
                {
                    switch (cfg_wnd.entries[cfg_wnd.line].type)
                    {
                    case CFG_TYPE_STRING:
                    case CFG_TYPE_INT:
                    case CFG_TYPE_FLOAT:
                        if (cfg_wnd.entries[cfg_wnd.line].selection > 0)
                        {
                            cfg_wnd.entries[cfg_wnd.line].selection--;
                            move(cfg_axis.vis_pad_ys+cfg_axis.entries_y[cfg_wnd.line], cfg_axis.vis_pad_xs+cfg_axis.entries_x[cfg_wnd.line]+cfg_axis.fields_x[cfg_wnd.line]+cfg_wnd.entries[cfg_wnd.line].selection);
                        }
                        break;
                    case CFG_TYPE_LIST:
                        if (cfg_wnd.entries[cfg_wnd.line].selection > 0)
                        {
                            cfg_wnd.entries[cfg_wnd.line].selection--;
                            
                            wattron(cfg_wnd.entries[cfg_wnd.line].entry.wnd, A_BOLD);
                            mvwaddch(cfg_wnd.entries[cfg_wnd.line].entry.wnd, 0, cfg_axis.fields_x[cfg_wnd.line]-1, '<');
                            wattroff(cfg_wnd.entries[cfg_wnd.line].entry.wnd, A_BOLD);

                            mvwprintw(cfg_wnd.entries[cfg_wnd.line].entry.wnd, 0, cfg_axis.fields_x[cfg_wnd.line]+((LABEL_LEN-cfg_wnd.entries[cfg_wnd.line].value_list.options[cfg_wnd.entries[cfg_wnd.line].selection].str_len)/2), "%s", cfg_wnd.entries[cfg_wnd.line].value_list.options[cfg_wnd.entries[cfg_wnd.line].selection].option);

                            wattron(cfg_wnd.entries[cfg_wnd.line].entry.wnd, A_BOLD);
                            mvwaddch(cfg_wnd.entries[cfg_wnd.line].entry.wnd, 0, cfg_axis.fields_x[cfg_wnd.line]+LABEL_LEN, '>');
                            wattroff(cfg_wnd.entries[cfg_wnd.line].entry.wnd, A_BOLD);
                        }
                        break;
                    default:
                        break;
                    }
                    _update_window();
                }
                break;
            case KEY_RIGHT:
                {
                    switch (cfg_wnd.entries[cfg_wnd.line].type)
                    {
                    case CFG_TYPE_STRING:
                    case CFG_TYPE_INT:
                    case CFG_TYPE_FLOAT:
                        if (cfg_wnd.entries[cfg_wnd.line].selection < cfg_wnd.entries[cfg_wnd.line].value_str.str_len)
                        {
                            cfg_wnd.entries[cfg_wnd.line].selection++;
                            move(cfg_axis.vis_pad_ys+cfg_axis.entries_y[cfg_wnd.line], cfg_axis.vis_pad_xs+cfg_axis.entries_x[cfg_wnd.line]+cfg_axis.fields_x[cfg_wnd.line]+cfg_wnd.entries[cfg_wnd.line].selection);
                        }
                        break;
                    case CFG_TYPE_LIST:
                        if (cfg_wnd.entries[cfg_wnd.line].selection < cfg_wnd.entries[cfg_wnd.line].value_list.size-1)
                        {
                            cfg_wnd.entries[cfg_wnd.line].selection++;
                            
                            wattron(cfg_wnd.entries[cfg_wnd.line].entry.wnd, A_BOLD);
                            mvwaddch(cfg_wnd.entries[cfg_wnd.line].entry.wnd, 0, cfg_axis.fields_x[cfg_wnd.line]-1, '<');
                            wattroff(cfg_wnd.entries[cfg_wnd.line].entry.wnd, A_BOLD);

                            mvwprintw(cfg_wnd.entries[cfg_wnd.line].entry.wnd, 0, cfg_axis.fields_x[cfg_wnd.line]+((LABEL_LEN-cfg_wnd.entries[cfg_wnd.line].value_list.options[cfg_wnd.entries[cfg_wnd.line].selection].str_len)/2), "%s", cfg_wnd.entries[cfg_wnd.line].value_list.options[cfg_wnd.entries[cfg_wnd.line].selection].option);

                            wattron(cfg_wnd.entries[cfg_wnd.line].entry.wnd, A_BOLD);
                            mvwaddch(cfg_wnd.entries[cfg_wnd.line].entry.wnd, 0, cfg_axis.fields_x[cfg_wnd.line]+LABEL_LEN, '>');
                            wattroff(cfg_wnd.entries[cfg_wnd.line].entry.wnd, A_BOLD);
                        }
                        break;
                    default:
                        break;
                    }
                    _update_window();
                }
                break;
            case KEY_F(1):

                break;
            case KEY_F(2):
                {
                }
                break;
            case KEY_F(3):
                {
                    *option = WND_MAIN_MENU;
                    run_flag = 0;
                }
                break;
            case KEY_F(4):
                {
                    *option = WND_NONE;
                    run_flag = 0;
                }
                break;
            default:
            break;
        }
    }

    _delete_window();

    return ret;
}
