#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <curses.h>
// #include <cursesw.h>

#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <signal.h>
#include <mqueue.h>
#include "network.h"

#define GFX_ELEM_VOFF                   0
#define GFX_ELEM_HOFF                   1

#define SUBWND_SET_H                    size.ws_row
#define SUBWND_DEF_H                    10

#define SUBWND_SET_W                    size.ws_col
#define SUBWND_DEF_W                    120

#define WLCM_WND_H_RU                   5
#define WLCM_WND_W_RU                   95

#define WLCM_WND_H_EN                   7
#define WLCM_WND_W_EN                   45

#define PANEL_SET_H                     12
#define PANEL_MIN_H                     12
#define PANEL_DEF_H                     12

#define PANEL_SET_W                     60
#define PANEL_MIN_W                     40
#define PANEL_DEF_W                     40

#define ELEM_SET_H                      ELEM_MIN_H
#define ELEM_MIN_H                      ELEM_DEF_H
#define ELEM_DEF_H                      3

#define ELEM_SET_W                      ELEM_MIN_W
#define ELEM_MIN_W                      ELEM_DEF_W
#define ELEM_DEF_W                      20

#define MENU_SCR_LABEL                  "Welcome to Linux Messenger"
#define MENU_SCR_NOTE_LABEL             "Shortcuts:"
#define MENU_SCR_CLT_BTN_LABEL          "Join server"
#define MENU_SCR_SRV_BTN_LABEL          "Create a server"
#define MENU_SCR_CFG_BTN_LABEL          "Configuration"
#define MENU_SCR_QUIT_BTN_LABEL         "Quit"
#define MENU_SCR_NOTE_CONNECTED         "F1: Help\tF2: Reconnect\tF3: Disconnect\tF4: Quit"
#define MENU_SCR_NOTE_DISCONNECTED      "F1: Help\tF2: Connect\tF3: Change address\tF4: Quit"
#define MENU_SCR_NOTE_CONNECTING        "F1: Help\tF4: Quit"

#define JOIN_SCR_LABEL                  "Join server"
#define JOIN_SCR_SRV_NAME_LABEL         "Server name"
#define JOIN_SCR_SRV_ADDR_LABEL         "Address"
#define JOIN_SCR_CONN_USERS_LABEL       "Users"
#define JOIN_SCR_MAN_ADDR_LABEL         "Input address:"
#define JOIN_SCR_JOIN_BTN_LABEL         "Join"
#define JOIN_SCR_REFRESH_BTN_LABEL      "Refresh"
#define JOIN_SCR_CLEAR_BTN_LABEL        "Clear"
#define JOIN_SCR_NOTE                   "TAB: Change mode\tF1: Help\tF3: Back\tF4: Quit"

#define CREATE_SCR_LABEL                "Create server"
#define CREATE_SCR_SRV_INFO_TEMPLATE    "Server properties:\n* Server name:\t\t%s\n* Maximum users:\t%s\n* Address:\t\t%s"
#define CREATE_SCR_SRV_NAME_LABEL       "Server name"
#define CREATE_SCR_CONN_USERS_LABEL     "Maximum users"
#define CREATE_SCR_RESTR_USERS_LABEL    "Users restriction"
#define CREATE_SCR_SRV_ADDR_LABEL       "Address"
#define CREATE_SCR_SRV_PORT_LABEL       "Port"
#define CREATE_SCR_LCL_ADDR_LABEL       "Local address"
#define CREATE_SCR_AUTO_PORT_LABEL      "Auto port"
#define CREATE_SCR_CREATE_BTN_LABEL     "Create"
#define CREATE_SCR_DEFAULT_BTN_LABEL    "Default"
#define CREATE_SCR_CLEAR_BTN_LABEL      "Clear"
#define CREATE_SCR_NOTE                 "TAB: Change mode\tF1: Help\tF3: Back\tF4: Quit"

#define PREFS_SCR_LABEL                 "Preferences"
#define PREFS_SCR_NOTE                  "F1: Save\tF2: Menu\tF3: Back\tF4: Quit\tF5: Save\tF6: Cancel\tF7: Reset"

enum cur_wnd_enum
{
    WND_NONE = 0,
    WND_MAIN_MENU,
    WND_JOIN_SRV,
    WND_CREATE_SRV,
    WND_PREFS
};

enum popup_wnd_type
{
    POPUP_W_BLOCK = 1,
    POPUP_W_WAIT,
    POPUP_W_CONFIRM,
    POPUP_W_INFO
};

enum join_wnd_tab_mode
{
    MODE_LIST = 1,
    MODE_TEXTBOX
};

enum create_wnd_tab_mode
{
    MODE_PAD = 1,
    MODE_BUTTONS
};

struct label_t
{
    char *text;
    int size;
};

struct note_labels_t
{
    struct label_t mmenu_connected;
    struct label_t mmenu_disconnected;
    struct label_t mmenu_connecting;
    struct label_t join_srv;
    struct label_t create_srv;
    struct label_t prefs_wnd;
};

struct elem_wnd_t
{
    WINDOW *wnd;
    char *lbl;
};

struct global_dims_t
{
    int wnd_h;
    int wnd_w;
    int subwnd_h;
    int subwnd_w;
    int elem_h;
    int elem_w;
    int note_h;
    int note_w;
};

struct global_axis_t
{
    int wnd_y;
    int wnd_x;
    int subwnd_y;
    int subwnd_x;
    int note_y;
    int note_x;
};

struct global_wnds_t
{
    WINDOW *wnd;
    WINDOW *note_w;
    WINDOW *note_sw;
};

struct mmenu_dims_t
{
    int header_h;
    int header_w;
    int status_h;
    int status_w;
    int btns_h;
    int btns_w;
    int btns_border_h;// = (elem_h*4)+2+(GFX_ELEM_VOFF*5);
    int btns_border_w;// = elem_w+2+(GFX_ELEM_HOFF*2);
    int panel_h;
    int panel_w;
};

struct mmenu_axis_t
{
    int header_y;
    int header_x;
    int status_y;
    int status_x;
    int btns_border_y;
    int btns_border_x;
    int btns_y[4];
    int btns_x[4];
    int panel_y;
    int panel_x;
};

struct mmenu_wnd_t
{
    WINDOW *header_w;
    WINDOW *status_w;
    WINDOW *status_sw;
    WINDOW *btns_border;
    struct elem_wnd_t btns[4];
    WINDOW *panel_w;
    WINDOW *panel_sw;
    int selection;
};

struct join_dims_t
{
    int sinfo_h;
    int sinfo_w;
    int susers_h;
    int susers_w;
    int saddr_h;
    int saddr_w;
    int sname_h;
    int sname_w;
    int caddr_h;
    int caddr_w;
    int lbl_h;
    int lbl_w;
    int tb_h;
    int tb_w;
    int btns_border_h;
    int btns_border_w;
    int btns_h;
    int btns_w;
    int pad_border_h;
    int pad_border_w;
    int vis_pad_h;
    int vis_pad_w;
    int pad_h;
    int pad_w;
};

struct join_axis_t
{
    int top_panel_y;
    int top_panel_x;
    int susers_y;
    int susers_x;
    int saddr_y;
    int saddr_x;
    int sname_y;
    int sname_x;
    int pad_border_y;
    int pad_border_x;
    int vis_pad_ys;
    int vis_pad_xs;
    int vis_pad_ye;
    int vis_pad_xe;
    int pad_ys;
    int pad_xs;
    int pad_ye;
    int pad_xe;
    int caddr_y;
    int caddr_x;
    int lbl_y;
    int lbl_x;
    int tb_y;
    int tb_x;
    int btns_border_y;
    int btns_border_x;
    int btns_y[3];
    int btns_x[3];
};

struct join_wnd_t
{
    WINDOW *top_panel;
    WINDOW *sname_w;
    WINDOW *saddr_w;
    WINDOW *susers_w;
    WINDOW *pad_border;
    WINDOW *servers_pad;
    WINDOW *caddr_w;
    WINDOW *label_w;
    WINDOW *tb_w;
    WINDOW *btns_border;
    struct elem_wnd_t btns[3];
    struct server_info_t *servers;
    int servers_count;
    int line;
    // int vis_line;
    int selection;
    int mode;
};

struct create_dims_t
{
    int pad_elem_h;
    int pad_elem_w;
    int sname_h;
    int sname_w;
    int musers_h;
    int musers_w;
    int rusers_h;
    int rusers_w;
    int saddr_h;
    int saddr_w;
    int sport_h;
    int sport_w;
    int lcl_addr_h;
    int lcl_addr_w;
    int auto_port_h;
    int auto_port_w;
    int btns_h;
    int btns_w;
    int vis_pad_h;
    int vis_pad_w;
    int pad_h;
    int pad_w;
    int pad_border_h;
    int pad_border_w;
    int srv_info_h;
    int srv_info_sw_h;
    int srv_info_w;
    int subwnd_h;
    int subwnd_w;
};

struct create_axis_t
{
    int srv_info_y;
    int srv_info_x;
    int srv_info_sw_y;
    int srv_info_sw_x;
    int pad_border_y;
    int pad_border_x;
    int vis_pad_ys;
    int vis_pad_xs;
    int vis_pad_ye;
    int vis_pad_xe;
    int pad_ys;
    int pad_xs;
    int pad_ye;
    int pad_xe;
    int pad_elems_y[7];
    int pad_elems_x[7];
    int sname_y;
    int sname_x;
    int musers_y;
    int musers_x;
    int rusers_y;
    int rusers_x;
    int saddr_y;
    int saddr_x;
    int sport_y;
    int sport_x;
    int lcl_addr_y;
    int lcl_addr_x;
    int pad_vdelim;
    int auto_port_y;
    int auto_port_x;
    int btns_y[3];
    int btns_x[3];
};

struct create_wnd_t
{
    WINDOW *subwnd;
    WINDOW *srv_info_w;
    WINDOW *srv_info_sw;
    WINDOW *pad_border;
    WINDOW *pad;
    struct elem_wnd_t pad_elems[7];
    WINDOW *sname_w;
    WINDOW *musers_w;
    WINDOW *rusers_w;
    WINDOW *addr_w;
    WINDOW *port_w;
    WINDOW *lcl_addr_w;
    WINDOW *auto_port_w;
    struct elem_wnd_t btns[3];
    
    int line;
    // int vis_line;
    int selection;
    int mode;
};

struct cfg_dims_t
{
    int pad_border_h;
    int pad_border_w;
    int vis_pad_h;
    int vis_pad_w;
    int pad_h;
    int pad_w;
};

struct cfg_axis_t
{
    int pad_border_y;
    int pad_border_x;
    int vis_pad_ys;
    int vis_pad_xs;
    int vis_pad_ye;
    int vis_pad_xe;
    int pad_ys;
    int pad_xs;
    int pad_ye;
    int pad_xe;
};

struct cfg_wnd_t
{
    WINDOW *pad;
    WINDOW *pad_border;
    int line;
};

extern int connection_flag;

/* Global variable, used to store terminal's size in columns and rows */
extern struct winsize size;

int init_graphics();
void deinit_graphics();

void handle_msg(union sigval);

int popup_wnd(char *, int, ...);

int menu_wnd(int *);

int join_srv_wnd(int *);
int create_srv_wnd(int *);

int prefs_wnd(int *);

#endif // _GRAPHICS_H