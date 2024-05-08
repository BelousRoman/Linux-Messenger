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

#define GFX_ELEM_VOFF                   0
#define GFX_ELEM_HOFF                   1

#define SUBWND_SET_H                    size.ws_row/1.5
#define SUBWND_DEF_H                    10

#define SUBWND_SET_W                    size.ws_col/1.5
#define SUBWND_DEF_W                    120

#define WLCM_WND_H_RU                   6
#define WLCM_WND_W_RU                   95

#define WLCM_WND_H_EN                   7
#define WLCM_WND_W_EN                   46

#define PANEL_SET_H                     12
#define PANEL_MIN_H                     12
#define PANEL_DEF_H                     12

#define PANEL_SET_W                     60
#define PANEL_MIN_W                     40
#define PANEL_DEF_W                     40

#define ELEM_SET_H                      3
#define ELEM_MIN_H                      3
#define ELEM_DEF_H                      3

#define ELEM_SET_W                      30
#define ELEM_MIN_W                      20
#define ELEM_DEF_W                      20

#define MENU_SCR_LABEL                  "Welcome to Linux Messenger"
#define MENU_SCR_NOTE_LABEL             "Shortcuts:"
#define MENU_SCR_NOTE                   "F1: Help\tF4: Quit"
#define MENU_SCR_CLT_BTN_LABEL          "Join server"
#define MENU_SCR_SRV_BTN_LABEL          "Create a server"
#define MENU_SCR_CFG_BTN_LABEL          "Configuration"
#define MENU_SCR_QUIT_BTN_LABEL         "Quit"

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
#define CREATE_SCR_NOTE                 "F1: Help\tF3: Back\tF4: Quit"

#define PREFS_SCR_LABEL                 "Preferences"
// #define PREFS_SCR_NOTE                  "1\t2\t3\t4\t5\t6\t7"
// #define sz "F3"
#define PREFS_SCR_NOTE                  "F1: Save\tF2: Menu\tF3: Back\tF4: Quit\tF5: Save\tF6: Cancel\tF7: Reset"

enum wait_wnd_cond
{
    await
};

/* Global variable, used to store terminal's size in columns and rows */
extern struct winsize size;

void init_graphics();
void deinit_graphics();

int wait_wnd(char *, int);

int menu_wnd();

int join_srv_wnd();
int create_srv_wnd();

int prefs_wnd();

#endif // _GRAPHICS_H