/*
 * original clock - Simple analog clock
 *
 * This file is part of original clock
 *
 * Copyright (C) 2019 - munyamunya.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * g++ `pkg-config --cflags gtk+-3.0` -o originalclock main.cpp `pkg-config --libs gtk+-3.0`
 */

//#include <locale.h>
#include <stdint.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <gtk/gtk.h>


#define     LOCALE_JA_JP
//#define   LOCALE_EN_US


typedef                 int8_t      int8;
typedef                 int16_t     int16;
typedef                 int32_t     int32;
typedef     unsigned    int         uint;
typedef                 uint8_t     uint8;
typedef                 uint16_t    uint16;
typedef                 uint32_t    uint32;
typedef     unsigned    short       ushort;
typedef     unsigned    long        ulong;
#ifndef     byte
typedef     unsigned    char        byte;
#endif

typedef     struct stat             Stat;


//--------------------------------------------------------------------------------------------------
// Global variables for file path
//--------------------------------------------------------------------------------------------------

// $HOME/.orgclock <= 200
// ex  /home/anonymous/.orgclock
//
// /home/anonymous/.orgclock/themes/themeName      /fileName
// +-----------------------+ +----+ +------ - - --+ +------+
// 200 byte + Null (2 byte)  123456 30 byte         14 byte|
// |                                                       |
// +-------------------------------------------------------+
// 255 byte
//
// analogBkgr.png
// 123456789a1234

#define         MAX_BYTES_PATH_INL      255     // including the null
#define         MAX_BYTES_CONF_DIR      200     // not including the null

char            g_path[ MAX_BYTES_PATH_INL ];
char            *g_pConfigDir;
char            *g_pThemeRootDir;
char            *g_pThemeDir;


// reference
// ttps://askubuntu.com/questions/859945/what-is-the-maximum-length-of-a-file-path-in-ubuntu
//
// BTRFS        255 bytes
// exFAT        255 UTF-16 characters
// ext2         255 bytes
// ext3         255 bytes
// ext3cow      255 bytes
// ext4         255 bytes
// FAT32        8.3 (255 UCS-2 code units with VFAT LFNs)
// NTFS         255 characters
// XFS          255 bytes


//--------------------------------------------------------------------------------------------------
// Common functions
//--------------------------------------------------------------------------------------------------

void    init_bktransprnt(GtkWidget *pWidget);

#define         MB_MSG_INFO     GTK_MESSAGE_INFO
//#define       MB_MSG_WARN     GTK_MESSAGE_WARNING
//#define       MB_MSG_ASK      GTK_MESSAGE_QUESTION
#define         MB_MSG_ERR      GTK_MESSAGE_ERROR
//#define       MB_MSG_OTHR     GTK_MESSAGE_OTHER

//#define       MB_BTN_NOBTN    GTK_BUTTONS_NONE
#define         MB_BTN_OK       GTK_BUTTONS_OK
//#define       MB_BTN_X        GTK_BUTTONS_CLOSE
//#define       MB_BTN_CNCL     GTK_BUTTONS_CANCEL
#define         MB_BTN_YN       GTK_BUTTONS_YES_NO
#define         MB_BTN_OKCN     GTK_BUTTONS_OK_CANCEL

gint    message_box(char *pMssg, GtkMessageType msgType, GtkButtonsType btnType);

int32   get_dir_list(char *pDir, char pplist[][ MAX_BYTES_PATH_INL ], int32 maxDirs);
int32   get_font_size(char *pFontDsc);

long    SimpleFileRead (const char *p_path, byte *p_buffer, long size);
bool    SimpleFileWrite(const char *p_path, byte *p_buffer, long size);


//
//  CLASS   CarioSurface
//
//--------------------------------------------------------------------------------------------------
class CarioSurface {
public:
                    CarioSurface();
                    ~CarioSurface();
    bool            create(char* pPath);
    void            free();
    operator        cairo_surface_t*();
    int32           getWidth();
    int32           getHeight();

private:
    cairo_surface_t     *m_pSf;
    int32               m_width,
                        m_height;
};

CarioSurface::CarioSurface() {
    m_pSf = NULL;
    m_width = 0;
    m_height = 0;
}

CarioSurface::~CarioSurface() {
    if (m_pSf != NULL) {
        cairo_surface_destroy(m_pSf);
        m_pSf = NULL;
    }
}

bool    CarioSurface::create(char* pPath) {
    m_pSf = cairo_image_surface_create_from_png(pPath);

    if (m_pSf == NULL) {
        return false;
    }

    m_width  = cairo_image_surface_get_width(m_pSf);
    m_height = cairo_image_surface_get_height(m_pSf);

    if (m_width == 0) {
        return false;
    }

    return true;
}

void    CarioSurface::free() {
    if (m_pSf != NULL) {
        cairo_surface_destroy(m_pSf);
        m_pSf = NULL;
    }

    m_width  = 0;
    m_height = 0;
}

CarioSurface::operator cairo_surface_t*() {
    return m_pSf;
}

inline  int32   CarioSurface::getWidth() {
    return m_width;
}

inline  int32   CarioSurface::getHeight(){
    return m_height;
}


//--------------------------------------------------------------------------------------------------
// Application status flags
//--------------------------------------------------------------------------------------------------

// analog
#define         MODE_ANALOG                 0x10000000

// degital
#define         MODE_DIGITAL                0x00000000

// analog  clock with bk image
#define         MODE_AN_BKGR_IMG            0x01000000

// digital clock with bk image
#define         MODE_DG_BKGR_IMG            0x00100000

// image of solid color
#define         MODE_MASK_IMG_SCL           0x01100000

// analog, digital common flags

// displays year month day (day of week), hour min
#define         MODE_YMD_HM                 0x00010000

// displays month day (day of week), hour min sec
#define         MODE_MD_HMS                 0x00000000

// The lower 16 bits are reserved for the additional functionality.


//--------------------------------------------------------------------------------------------------
// Application parameter structure
//--------------------------------------------------------------------------------------------------

#define         _MAX_THEMES                 30
//#define       _NUM_CHRS_THMNM             30
//#define       _NUM_CHRS_THMNM_INL         32
#define         _NUM_BYTES_THMNM            60
#define         _NUM_BYTES_THMNM_INL        64
#define         _NUM_BYTES_FNDSC            62
#define         _NUM_BYTES_FNDSC_INL        64

struct AppParam {
    uint32      m_mode;
    char        m_themeName[ _NUM_BYTES_THMNM_INL ];
    //char      m_prevThemeName[ _NUM_BYTES_THMNM_INL ];
    char        m_fontName[ _NUM_BYTES_FNDSC_INL ];
    int32       m_fontSize;
    uint32      m_fontColor,
                //m_msgFontColor,
                m_bkgrColor;
                //m_msgBkgrColor,
                //m_colorKey;
    byte        m_useClrKey,
                m_alpha;
    int32       m_wndX,
                m_wndY;
};

// テーマ、フォントの変更の適用。それ以外は変更の有無にかかわらず適用
#define         APPLY_THNM                  0x00000010
#define         APPLY_FONT                  0x00000020

// 設定と予定の変更の検出
#define         CONFIG_PARAM                0x00000001
#define         CONFIG_MSSG                 0x00000002


//--------------------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------------------

// Application's main window
GtkWidget       *g_pAppWnd = NULL;

// Popup Menu
GtkWidget       *g_pPopupMenu = NULL,
                *g_pMenuItem0 = NULL,
                *g_pMenuItem1 = NULL,
                *g_pMenuItem2 = NULL,
                *g_pMenuItem3 = NULL;

// Appearence dialog's widgets

GtkWidget       *g_pApprcDlg = NULL;

GtkComboBox     *g_pCombo;
GtkFontChooser  *g_pFontChooser;

GSList          *g_pRadioGrp1,
                *g_pRadioGrp2;
GtkRadioButton  *g_pRadio1,
                *g_pRadio2,
                *g_pRadio3,
                *g_pRadio4;
GtkEntry        *g_pFontColor,
                *g_pMonoColor,
                *g_pOpacity;

// Cario surfaces
CarioSurface    g_analogBkgr;
CarioSurface    g_digitalBkgr;
CarioSurface    g_hands[ 3 ];

// Application's parameters
AppParam        g_appParam;
uint32          g_fConfig = 0;

pthread_t       g_thr_id;
pthread_mutex_t g_mutex;
bool            g_thr_run = true;


static void appwindow_activate(GtkApplication *pApp, gpointer pData);

static gboolean appwindow_draw(GtkWidget *pWidget, cairo_t *cr, gpointer pData);
static gboolean appwindow_clicked(GtkWidget *pWidget, GdkEvent *pEvent);
static gboolean appwindow_delete(GtkWidget *pWidget, GdkEvent *pEvent, gpointer pData);
//static void appwindow_destroy(void);

void*   timer_thread(void *threadid);

static void menu_item0_selected(gchar *string);
static void menu_item1_selected(gchar *string);
static void menu_item2_selected(gchar *string);
static void menu_item3_selected(gchar *string);

static void config_window_applybtn_clicked(GtkWidget *widget, gpointer data);
static void config_window_closebtn_clicked(GtkWidget *widget, gpointer data);


#ifdef      LOCALE_JA_JP
char    g_builder_ui[] = { "config_ja.ui" };

char    g_menuItemStr[ 4 ][ 64 ] = {
    { "タイトルバー" },
    { "デジタルアナログ切り替え" },
    { "外観の設定" },
    { "終了" }
};
#else
char    g_builder_ui[] = { "config_en.ui" };

char    g_menuItemStr[ 4 ][ 32 ] = {
    { "Title bar" },
    { "Analog <-> Degital" },
    { "Appearance Setting" },
    { "Exit" }
};
#endif


//--------------------------------------------------------------------------------------------------
// theme loading
//--------------------------------------------------------------------------------------------------

#define         LT_OK                   0
#define         LT_E_TOO_LONG_NAME      1
#define         LT_E_BKGR_REQUIRED      2
#define         LT_E_BKGR_INVLD         3
#define         LT_E_HAND_REQUIRED      4
#define         LT_E_HAND_INVLD         5


#ifdef      LOCALE_JA_JP
char    g_LoadThemeErrMssgs[ 6 ][ 128 ] = {
    { "成功" },
    { "テーマ名は30文字以内でつけてください" },
    { "時計の背景 (analogBkgr、digitalBkgr) は必須です" },
    { "時計の背景 (analogBkgr、digitalBkgr) のサイズが不正です。\nアナログは縦横比 1: 1です" },
    { "時計の針 (hourHand、minHand、secHand) は必須です" },
    { "時計の針 (hourHand、minHand、secHand) のサイズが不正です" }
};
#else
char    g_LoadThemeErrMssgs[ 6 ][ 128 ] = {
    { "success" },
    { "theme name must less than 30 characters" },
    { "clock backgrounds (analogBkgr、digitalBkgr) are mandatory" },
    { "the size of clock backgrounds (analogBkgr、digitalBkgr) is incorrect.\nanalog' virt and horz size shoud be the same" },
    { "clock hands (hourHand、minHand、secHand) are mondatory" },
    { "the size of clock hands (hourHand、minHand、secHand) is incorrect." }
};
#endif

int32   LoadThemeImages(char*);
void    FreeThemeImages();


//--------------------------------------------------------------------------------------------------
// string literals
//--------------------------------------------------------------------------------------------------

#ifdef      LOCALE_JA_JP
char    g_dayWek[ 7 ][ 6 ] = {
    { "日" },   { "月" },   { "火" },   { "水" },   { "木" },   { "金" },   { "土" }
};
#else
char    g_dayWek[ 7 ][ 6 ] = {
    { "Sun" },  { "Mon" },  { "Thu" },  { "Wed" },  { "Thu" },  { "Fri" },  { "Sat" }
};
#endif

#define ERR_CONFDIR_TOO_LONG        0
#define ERR_DEFAULT_THEME_REQUIRED  1
#define ERR_FONTDSC_TOO_LONG        2

#ifdef      LOCALE_JA_JP
char    g_errorMssg[ 3 ][ 96 ] = {
    { "設定ディレクトリ ($HOME/.orgclock) は %d byteより短くなければなりません\n" },
    { "少なくとも既定のテーマとpng ファイルが必要です\n" },
    { "フォントの定義が長すぎます\n" }
};
#else
char    g_errorMssg[ 3 ][ 96 ] = {
    { "Config directory ($HOME/.orgclock) must be less than %d bytes.\n" },
    { "You at least need the default theme and some png files.\n" },
    { "Font description is too long to apply.\n" }
};
#endif


//
//  Function    appwindow_activate
//
//  gtk application's activate event
//
//--------------------------------------------------------------------------------------------------
static void appwindow_activate(GtkApplication *pApp, gpointer pData) {

    if (pthread_mutex_init(&g_mutex, NULL) != 0) {
        return;
    }

    g_pAppWnd = gtk_application_window_new(pApp);
    gtk_window_set_title(GTK_WINDOW(g_pAppWnd), "Original Clock");
    gtk_window_move(GTK_WINDOW(g_pAppWnd), g_appParam.m_wndX, g_appParam.m_wndY);

    if (g_appParam.m_mode & MODE_ANALOG) {
        gtk_window_set_default_size(GTK_WINDOW(g_pAppWnd), g_analogBkgr.getWidth(),
                g_analogBkgr.getHeight());
    } else {
        gtk_window_set_default_size(GTK_WINDOW(g_pAppWnd), g_digitalBkgr.getWidth(),
                g_digitalBkgr.getHeight());
    }

    g_pPopupMenu = gtk_menu_new();

    g_pMenuItem0 = gtk_menu_item_new_with_label(g_menuItemStr[0]);
    gtk_menu_shell_append(GTK_MENU_SHELL(g_pPopupMenu), g_pMenuItem0);
    g_signal_connect_swapped(g_pMenuItem0, "activate",
            G_CALLBACK(menu_item0_selected), (gpointer) g_strdup("item0"));
    gtk_widget_show(g_pMenuItem0);

    g_pMenuItem1 = gtk_menu_item_new_with_label(g_menuItemStr[1]);
    gtk_menu_shell_append(GTK_MENU_SHELL(g_pPopupMenu), g_pMenuItem1);
    g_signal_connect_swapped(g_pMenuItem1, "activate",
            G_CALLBACK(menu_item1_selected), (gpointer) g_strdup("item1"));
    gtk_widget_show(g_pMenuItem1);

    g_pMenuItem2 = gtk_menu_item_new_with_label(g_menuItemStr[2]);
    gtk_menu_shell_append(GTK_MENU_SHELL(g_pPopupMenu), g_pMenuItem2);
    g_signal_connect_swapped(g_pMenuItem2, "activate",
            G_CALLBACK(menu_item2_selected), (gpointer) g_strdup("item2"));
    gtk_widget_show(g_pMenuItem2);

    g_pMenuItem3 = gtk_menu_item_new_with_label(g_menuItemStr[3]);
    gtk_menu_shell_append(GTK_MENU_SHELL(g_pPopupMenu), g_pMenuItem3);
    g_signal_connect_swapped(g_pMenuItem3, "activate",
            G_CALLBACK(menu_item3_selected), (gpointer) g_strdup("item3"));
    gtk_widget_show(g_pMenuItem3);

    //GtkWidget *pImage = NULL;
    //pImage = gtk_image_new_from_file("./test.png");
    //gtk_widget_set_opacity(pImage, 0.0);
    //gtk_container_add(GTK_CONTAINER(g_pAppWnd), pImage);

    init_bktransprnt(g_pAppWnd);

    gtk_window_set_keep_above(GTK_WINDOW(g_pAppWnd), true);
    gtk_window_set_decorated(GTK_WINDOW(g_pAppWnd), false);

    g_signal_connect(g_pAppWnd, "draw",    G_CALLBACK(appwindow_draw), g_pAppWnd);
    g_signal_connect(g_pAppWnd, "button_press_event", G_CALLBACK(appwindow_clicked), g_pAppWnd);
    g_signal_connect(g_pAppWnd, "delete-event", G_CALLBACK(appwindow_delete), g_pAppWnd);
    //g_signal_connect(g_pAppWnd, "destroy", G_CALLBACK(appwindow_destroy), g_pAppWnd);
    //g_signal_connect(g_pAppWnd, "destroy", G_CALLBACK(g_application_quit), g_pAppWnd);
    
    // when using G_APPLICATION, you don't need to connect g_application_quit
    // to window's destroy signal explicitly. it is done automatically.
    // otherwise "GLib-GIO-CRITICAL **: g_application_quit: assertion
    // 'G_IS_APPLICATION (application)' failed" occurs
    // ttps://stackoverflow.com/questions/43029025/c-gtk-g-application-quit

    gtk_widget_show_all(g_pAppWnd);

    double  aa = (double)g_appParam.m_alpha;
    aa = aa / 255;

    gtk_widget_set_opacity(g_pAppWnd, aa);

    pthread_create(&g_thr_id, NULL, timer_thread, (void*)NULL);
    pthread_detach(g_thr_id);
}


//
//  Function    appwindow_draw
//
//--------------------------------------------------------------------------------------------------
static gboolean appwindow_draw(GtkWidget *pWidget, cairo_t *cr, gpointer pData) {

    static char fontName[ _NUM_BYTES_FNDSC_INL ];
    static char timeStr1[ 128 ];
    static char timeStr2[ 64 ];

    time_t      utcNow;
    tm          *pLcltm;

    int32       anw, anh,
                dgw, dgh,
                hdw, hdh,
                cy,
                hourHandDegree,
                minHandDegree,
                secHandDegree,
                lnh,
                textx,
                texty;

    cairo_matrix_t          mat;
    cairo_text_extents_t    extents;

    uint32      mode;
    int32       fontSize;
    uint32      fontColor,
                bkgrColor;
    byte        alpha;

    double      rr, gg, bb, aa;

    pthread_mutex_lock(&g_mutex);

    ::time(&utcNow);
    pLcltm = ::localtime(&utcNow);
    mode = g_appParam.m_mode;
    ::memcpy(fontName, g_appParam.m_fontName, _NUM_BYTES_FNDSC_INL);
    fontSize  = g_appParam.m_fontSize;
    fontColor = g_appParam.m_fontColor;
    bkgrColor = g_appParam.m_bkgrColor;
    alpha = g_appParam.m_alpha;

    pthread_mutex_unlock(&g_mutex);

    // Calculate the clock hand degrees- - - - - - - -

    if (pLcltm ->tm_hour < 12) {
        hourHandDegree = 30 * (pLcltm ->tm_hour) + 30 * (pLcltm ->tm_min) / 60;
    } else {
        hourHandDegree = 30 * (pLcltm ->tm_hour - 12) + 30 * (pLcltm ->tm_min) / 60;
    }

    minHandDegree = (360 * pLcltm ->tm_min / 60);
    secHandDegree = (360 * pLcltm ->tm_sec / 60);

    // Set up matrix to reset translation and rotation

    // typedef struct {
    //   double xx; double yx;
    //   double xy; double yy;
    //   double x0; double y0;
    // } cairo_matrix_t;
    // x' = xx * x + xy * y + x0;
    // y' = yx * x + yy * y + y0;
    // x'   xx  xy  x0   x
    // y' = yx  yy  y0 * y
    // 1    0   0   1    1

    mat.xx = 1; mat.xy = 0;
    mat.x0 = 0;
    mat.yx = 0; mat.yy = 1;
    mat.y0 = 0;

    if (mode & MODE_ANALOG) {

        // Draw background - - - - - - - - - - - - - - - -

        if (mode & MODE_AN_BKGR_IMG) {

            // Make the background transparent - - - - - - - -

            cairo_save(cr);
            cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.0);
            cairo_paint(cr);
            cairo_restore(cr);

            cairo_set_source_surface(cr, g_analogBkgr, 0, 0);
            cairo_paint(cr);
        } else {
            rr = (bkgrColor & 0xff0000) >> 16;  rr /= 255;
            gg = (bkgrColor & 0x00ff00) >>  8;  gg /= 255;
            bb = bkgrColor & 0x0000ff;  bb /= 255;

            // Fill the background with a solid color- - - - -
            cairo_save(cr);
            cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
            cairo_set_source_rgba(cr, rr, gg, bb, 1.0);
            cairo_paint(cr);
            cairo_restore(cr);

            cairo_set_source_surface(cr, g_analogBkgr, 0, 0);
            cairo_paint(cr);
        }

        // Draw the hour hand- - - - - - - - - - - - - - -

        anw = g_analogBkgr.getWidth();
        anh = g_analogBkgr.getHeight();

        cairo_translate(cr, anw/2, anh/2);

        hdw = g_hands[ 0 ].getWidth();
        hdh = g_hands[ 0 ].getHeight();

        cairo_rotate(cr, 3.14 * hourHandDegree/180);

        if (hdh > anh/2) {
            cy = - anh/2 + 1;
        } else {
            cy = - hdw + 1;
        }

        cairo_set_source_surface(cr, g_hands[ 0 ], -hdw/2, cy);
        cairo_paint(cr);

        // Draw the minute hand- - - - - - - - - - - - - -

        hdw = g_hands[ 1 ].getWidth();
        hdh = g_hands[ 1 ].getHeight();

        cairo_rotate(cr, 3.14 * (minHandDegree - hourHandDegree)/180);

        if (hdh > anh/2) {
            cy = - anh/2 + 1;
        } else {
            cy = - hdw + 1;
        }

        cairo_set_source_surface(cr, g_hands[ 1 ], -hdw/2, cy);
        cairo_paint(cr);

        // Draw the second hand- - - - - - - - - - - - - -

        if (! (mode & MODE_YMD_HM)) {
            cairo_rotate(cr, 3.14 * (secHandDegree - minHandDegree)/180);

            hdw = g_hands[ 2 ].getWidth();
            hdh = g_hands[ 2 ].getHeight();

            if (hdh > anh/2) {
                cy = - anh/2 + 1;
            } else {
                cy = - hdw + 1;
            }

            cairo_set_source_surface(cr, g_hands[ 2 ], -hdw/2, cy);
            cairo_paint(cr);
        }

        // Create the date and time string - - - - - - - -

        if (mode & MODE_YMD_HM) {
            pLcltm->tm_year = (1900 + pLcltm->tm_year) % 100;
            ::sprintf(timeStr1, "%02d %02d/%02d (%s)",
                      pLcltm->tm_year, (pLcltm->tm_mon + 1), pLcltm->tm_mday, g_dayWek[pLcltm->tm_wday]);
            ::sprintf(timeStr2, "%02d:%02d",
                      pLcltm->tm_hour, pLcltm->tm_min);
        } else {
            ::sprintf(timeStr1, "%02d/%02d (%s)",
                      (pLcltm->tm_mon + 1), pLcltm->tm_mday, g_dayWek[pLcltm->tm_wday]);
            ::sprintf(timeStr2, "%02d:%02d:%02d",
                      pLcltm->tm_hour, pLcltm->tm_min, pLcltm->tm_sec);
        }

        // Reset translation and rotation- - - - - - - - -

        cairo_set_matrix(cr, &mat);

        // Draw the date and time string - - - - - - - - -

        // Set the font face and size
        cairo_select_font_face(cr, fontName, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, fontSize);

        rr = (fontColor & 0xff0000) >> 16;  rr /= 255;
        gg = (fontColor & 0x00ff00) >>  8;  gg /= 255;
        bb = fontColor & 0x0000ff;  bb /= 255;

        // Set the font color
        cairo_set_source_rgb(cr, rr, gg, bb);

        cairo_text_extents(cr, timeStr1, &extents);

        lnh = anw / 8;

        textx = anw / 2 - (extents.width/2 + extents.x_bearing);
        texty = lnh * 5 + lnh/2 - (extents.height/2 + extents.y_bearing);

        cairo_move_to(cr, textx, texty);
        cairo_show_text(cr, timeStr1);

        cairo_text_extents(cr, timeStr2, &extents);

        textx = anw / 2 - (extents.width/2 + extents.x_bearing);
        texty += lnh;

        cairo_move_to(cr, textx, texty);
        cairo_show_text(cr, timeStr2);

    } else {
        // Draw background - - - - - - - - - - - - - - - -

        if (mode & MODE_DG_BKGR_IMG) {
            cairo_set_source_surface(cr, g_digitalBkgr, 0, 0);
            cairo_paint(cr);
        } else {
            rr = (bkgrColor & 0xff0000) >> 16;  rr /= 255;
            gg = (bkgrColor & 0x00ff00) >>  8;  gg /= 255;
            bb = bkgrColor & 0x0000ff;  bb /= 255;

            // Fill the background with a solid color- - - - -

            cairo_save(cr);
            cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
            cairo_set_source_rgba(cr, rr, gg, bb, 1.0);
            cairo_paint(cr);
            cairo_restore(cr);

            cairo_set_source_surface(cr, g_digitalBkgr, 0, 0);
            cairo_paint(cr);
        }


        // Create the date and time string - - - - - - - -

        if (mode & MODE_YMD_HM) {
            pLcltm->tm_year = 1900 + pLcltm->tm_year;
            ::sprintf(timeStr1, "%04d %02d/%02d (%s) %02d:%02d",
                        pLcltm->tm_year, (pLcltm->tm_mon + 1), pLcltm->tm_mday, g_dayWek[pLcltm->tm_wday],
                        pLcltm->tm_hour, pLcltm->tm_min);
        } else {
            ::sprintf(timeStr1, "%02d/%02d (%s) %02d:%02d:%02d",
                        (pLcltm->tm_mon + 1), pLcltm->tm_mday, g_dayWek[pLcltm->tm_wday],
                        pLcltm->tm_hour, pLcltm->tm_min, pLcltm->tm_sec);
        }

        // Draw the date and time string - - - - - - - - -

        dgw = g_digitalBkgr.getWidth();
        dgh = g_digitalBkgr.getHeight();

        // Set the font face and size
        cairo_select_font_face(cr, fontName, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, fontSize);

        cairo_text_extents(cr, timeStr1, &extents);

        rr = (fontColor & 0xff0000) >> 16;  rr /= 255;
        gg = (fontColor & 0x00ff00) >>  8;  gg /= 255;
        bb = fontColor & 0x0000ff;  bb /= 255;

        // Set the font color
        cairo_set_source_rgb(cr, rr, gg, bb);

        textx = dgw / 2 - (extents.width/2 + extents.x_bearing);
        texty = dgh / 2 - (extents.height/2 + extents.y_bearing);

        cairo_move_to(cr, textx, texty);
        cairo_show_text(cr, timeStr1);
    }

    return TRUE;
}


//
//  Function    appwindow_clicked
//
//--------------------------------------------------------------------------------------------------
static gboolean appwindow_clicked(GtkWidget *pWidget, GdkEvent *pEvent) {

    if (pEvent ->type == GDK_BUTTON_PRESS) {
        // single click with the left mouse button ?
        if (((GdkEventButton*)pEvent)->button == 1) {

            pthread_mutex_lock(&g_mutex);

            if (g_appParam.m_mode & MODE_YMD_HM) {
                g_appParam.m_mode &= (~MODE_YMD_HM);
            } else {
                g_appParam.m_mode |= MODE_YMD_HM;
            }
            g_fConfig |= CONFIG_PARAM;

            pthread_mutex_unlock(&g_mutex);

            // redraw clock
            gtk_widget_queue_draw(pWidget);

            // TRUE to stop other handlers from being invoked for the event.
            return TRUE;

        // single click with the right mouse button ?
        } else if (((GdkEventButton*)pEvent)->button == 3) {
            gtk_menu_popup(GTK_MENU(g_pPopupMenu), NULL, NULL, NULL, NULL, 0,
                           ((GdkEventButton*)pEvent)->time);
            return TRUE;
        } else {
            ;
        }
    }

    // FALSE to propagate the event further.
    return FALSE;
}


//
//  Function    appwindow_delete
//
//--------------------------------------------------------------------------------------------------
static gboolean appwindow_delete(GtkWidget *pWidget, GdkEvent *pEvent, gpointer pData) {

    gint    x, y;

    pthread_mutex_lock(&g_mutex);

    gtk_window_get_position(GTK_WINDOW(g_pAppWnd), &x, &y);

    if (g_appParam.m_wndX != x || g_appParam.m_wndY != y) {
        g_appParam.m_wndX = x;
        g_appParam.m_wndY = y;
        g_fConfig |= CONFIG_PARAM;
    }

    g_thr_run = false;

    pthread_mutex_unlock(&g_mutex);

    // FALSE to propagate the event further
    return FALSE;
}


//
//  Function    appwindow_destroy
//
//--------------------------------------------------------------------------------------------------
//static void   appwindow_destroy(void) {}


//
//  Function    timer_thread
//
//--------------------------------------------------------------------------------------------------
void* timer_thread(void *threadid) {

    time_t      utcNow;
    tm          *pLcltm;
    int32       prvMin = 60;
    uint32      mode;

    pthread_mutex_lock(&g_mutex);
    mode = g_appParam.m_mode;
    pthread_mutex_unlock(&g_mutex);

    for (;;) {
        // Synchronous processing start
        pthread_mutex_lock(&g_mutex);

        if (! g_thr_run) {
            pthread_mutex_unlock(&g_mutex);
            // Synchronous processing end
            break;
        }

        mode = g_appParam.m_mode;

        // When displaying hours and minutes
        if (mode & MODE_YMD_HM) {
            ::time(&utcNow);
            pLcltm = ::localtime(&utcNow);
        }

        pthread_mutex_unlock(&g_mutex);
        // Synchronous processing end

        // When displaying years
        if (mode & MODE_YMD_HM) {
            if (prvMin != pLcltm ->tm_min) {
                prvMin = pLcltm ->tm_min;
                gtk_widget_queue_draw(g_pAppWnd);
            }

            ::sleep(3);

        // When displaying seconds
        } else {
            gtk_widget_queue_draw(g_pAppWnd);
            ::sleep(1);
        }
    }

    pthread_exit(NULL);
}


//
//  Function    menu_item1_selected
//
//--------------------------------------------------------------------------------------------------
static void menu_item0_selected(gchar *string) {
    static  bool deco = false;
    deco = deco ? false : true;
    gtk_window_set_decorated(GTK_WINDOW(g_pAppWnd), deco);
}


//
//  Function    menu_item2_selected
//
//--------------------------------------------------------------------------------------------------
static void menu_item1_selected(gchar *string) {

    int32   w, h;

    pthread_mutex_lock(&g_mutex);

    if (g_appParam.m_mode & MODE_ANALOG) {
        g_appParam.m_mode &= (~MODE_ANALOG);
        w = g_digitalBkgr.getWidth();
        h = g_digitalBkgr.getHeight();
    } else {
        g_appParam.m_mode |= MODE_ANALOG;
        w = g_analogBkgr.getWidth();
        h = g_analogBkgr.getHeight();
    }
    g_fConfig |= CONFIG_PARAM;

    pthread_mutex_unlock(&g_mutex);

    //gtk_widget_set_size_request(g_pAppWnd, w, h);
    gtk_window_resize(GTK_WINDOW(g_pAppWnd), w, h);
}


//
//  Function    menu_item3_selected
//
//--------------------------------------------------------------------------------------------------
static void menu_item2_selected(gchar *string) {

    GtkBuilder  *pBuilder;
    GObject     *pFontBtn;
    GObject     *pApplyBtn,
                *pCloseBtn;
    //GtkCssProvider    *pCssProvider;
    GError      *error = NULL;

    char        dirs[ 96 ][ MAX_BYTES_PATH_INL ];
    char        buffer[ 16 ];
    int32       i, numDirs;
    uint32      mode,
                fontColor,
                bkgrColor;
    byte        alpha;

    if (g_pApprcDlg != NULL) {
        return;
    }

    // Construct a GtkBuilder instance - - - - - - - -

    pBuilder = gtk_builder_new();

    // Load UI description
    if (gtk_builder_add_from_file(pBuilder, g_builder_ui, &error) == 0) {
        g_printerr("Error loading file: %s\n", error->message);
        g_clear_error(&error);
        return;
    }

    g_pApprcDlg = GTK_WIDGET(gtk_builder_get_object(pBuilder, "window1"));

    gtk_window_set_title(GTK_WINDOW(g_pApprcDlg), "Clock Config");
    //gtk_widget_set_size_request(GTK_WIDGET(g_pApprcDlg), 350, 300);

    // Initialize theme name combobox- - - - - - - - -

    g_pCombo = GTK_COMBO_BOX(gtk_builder_get_object(pBuilder, "combobox1"));

    *g_pThemeRootDir = '\0';

    numDirs = get_dir_list(g_path, dirs, 96);

    for (i = 0; i < numDirs; i ++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_pCombo), (const char*)dirs[i]);
        //gtk_combo_box_text_append();

        if (::strcmp(g_appParam.m_themeName, dirs[i]) == 0) {
            gtk_combo_box_set_active(g_pCombo, i);
        }
    }

    // Initialize font chooser - - - - - - - - - - - -

    pFontBtn = gtk_builder_get_object(pBuilder, "fontbutton1");
    g_pFontChooser = GTK_FONT_CHOOSER(pFontBtn);

    pthread_mutex_lock(&g_mutex);

    mode = g_appParam.m_mode;
    gtk_font_chooser_set_font(g_pFontChooser, g_appParam.m_fontName);

    fontColor = g_appParam.m_fontColor;
    bkgrColor = g_appParam.m_bkgrColor;
    alpha = g_appParam.m_alpha;

    pthread_mutex_unlock(&g_mutex);

    // Initialize radio buttons- - - - - - - - - - - -

    g_pRadio1 = GTK_RADIO_BUTTON(gtk_builder_get_object(pBuilder, "radiobutton1"));
    g_pRadio2 = GTK_RADIO_BUTTON(gtk_builder_get_object(pBuilder, "radiobutton2"));
    g_pRadioGrp1 = gtk_radio_button_get_group(g_pRadio1);
    gtk_radio_button_set_group(g_pRadio2, g_pRadioGrp1);

    g_pRadio3 = GTK_RADIO_BUTTON(gtk_builder_get_object(pBuilder, "radiobutton3"));
    g_pRadio4 = GTK_RADIO_BUTTON(gtk_builder_get_object(pBuilder, "radiobutton4"));
    g_pRadioGrp2 = gtk_radio_button_get_group(g_pRadio3);
    gtk_radio_button_set_group(g_pRadio4, g_pRadioGrp2);

    if (mode & MODE_AN_BKGR_IMG) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_pRadio1), true);
    } else {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_pRadio2), true);
    }
    if (mode & MODE_DG_BKGR_IMG) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_pRadio3), true);
    } else {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_pRadio4), true);
    }

    // Get gtk entries (text box) and buttons- - - - -

    g_pFontColor = GTK_ENTRY(gtk_builder_get_object(pBuilder, "entry1"));
    g_pMonoColor = GTK_ENTRY(gtk_builder_get_object(pBuilder, "entry3"));
    g_pOpacity   = GTK_ENTRY(gtk_builder_get_object(pBuilder, "entry5"));

    pApplyBtn  = gtk_builder_get_object(pBuilder, "button1");
    pCloseBtn  = gtk_builder_get_object(pBuilder, "button2");

    ::sprintf(buffer, "%06x", fontColor);
    gtk_entry_set_text(g_pFontColor, buffer);
    ::sprintf(buffer, "%06x", bkgrColor);
    gtk_entry_set_text(g_pMonoColor, buffer);
    ::sprintf(buffer, "%02d", alpha);
    gtk_entry_set_text(g_pOpacity, buffer);

    // Connect signal handlers to the constructed widgets- - - - - - - - -

    g_signal_connect(pApplyBtn, "clicked", G_CALLBACK(config_window_applybtn_clicked), g_pApprcDlg);
    g_signal_connect(pCloseBtn, "clicked", G_CALLBACK(config_window_closebtn_clicked), g_pApprcDlg);

    // Css - - - - - - - - - - - - - - - - - - - - - -

    /*
    pCssProvider = gtk_css_provider_new();

    gtk_css_provider_load_from_path(pCssProvider, "cssdata.css", NULL);

    //gchar   cssdata[] = { "* { background-color:#808080; } #wgtname { background-color:#ff00ff; }" };
    //gtk_css_provider_load_from_data(pCssProvider, cssdata, strlen(cssdata), &error);
    //gtk_css_provider_load_from_data(pCssProvider, cssdata, -1, NULL);

    if (error != NULL) {
        g_printerr("Error loading file: %s\n", error ->message);
        g_clear_error(&error);
        return;
    }

    //gtk_widget_set_name(GTK_WIDGET(pFontColor), "entry1");
    //gtk_widget_set_name(GTK_WIDGET(pMssgColor), "entry2");
    gtk_widget_set_name(GTK_WIDGET(g_pMonoColor), "entry3");
    //gtk_widget_set_name(GTK_WIDGET(pTrnsColor), "entry4");
    gtk_widget_set_name(GTK_WIDGET(pApplyBtn), "button1");

    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(pCssProvider), GTK_STYLE_PROVIDER_PRIORITY_USER);

    g_object_unref(pCssProvider);
     */

    g_object_unref(G_OBJECT(pBuilder));
}


//
//  Function    menu_item4_selected
//
//--------------------------------------------------------------------------------------------------
static void menu_item3_selected(gchar *string) {
    gtk_window_close(GTK_WINDOW(g_pAppWnd));
}


//
//  Function    config_window_applybtn_clicked
//
//--------------------------------------------------------------------------------------------------
static void config_window_applybtn_clicked(GtkWidget *widget, gpointer data) {

    GtkWidget   *config_window = (GtkWidget*)data;

    gchar       *pFontDsc;
    gchar       *pCmbTxt;
    gchar       *pTxt;
    int32       w, h;
    uint32      color;
    double      aa;
    int32       rtrnd;
    bool        rtrn;

    pCmbTxt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(g_pCombo));
    pFontDsc = gtk_font_chooser_get_font(g_pFontChooser);

    pthread_mutex_lock(&g_mutex);

    if (pCmbTxt != NULL && *pCmbTxt != '\0') {
        rtrn = false;

        rtrnd = LoadThemeImages(pCmbTxt);

        if (rtrnd == LT_OK) {
            ::strcpy(g_appParam.m_themeName, pCmbTxt);
            g_fConfig |= CONFIG_PARAM;
            rtrn = true;
        } else {
            LoadThemeImages(g_appParam.m_themeName);
        }

        g_free(pCmbTxt);

        if (! rtrn) {
            pthread_mutex_unlock(&g_mutex);

            message_box(g_LoadThemeErrMssgs[ rtrnd ], MB_MSG_ERR, MB_BTN_OK);
            return;
        }
    }

    if (g_appParam.m_mode & MODE_ANALOG) {
        w = g_analogBkgr.getWidth();
        h = g_analogBkgr.getHeight();
    } else {
        w = g_digitalBkgr.getWidth();
        h = g_digitalBkgr.getHeight();
    }

    rtrn = false;

    if (pFontDsc != NULL && *pFontDsc != '\0' && ::strlen(pFontDsc) < _NUM_BYTES_FNDSC) {
        ::strcpy(g_appParam.m_fontName, pFontDsc);
        g_appParam.m_fontSize = get_font_size(g_appParam.m_fontName);
        g_fConfig |= CONFIG_PARAM;
        rtrn = true;
    }

    g_free(pFontDsc);

    if (! rtrn) {
        pthread_mutex_unlock(&g_mutex);

        message_box(g_errorMssg[ ERR_FONTDSC_TOO_LONG ], MB_MSG_ERR, MB_BTN_OK);
        return;
    }

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_pRadio1))) {
        g_appParam.m_mode |= MODE_AN_BKGR_IMG;
    } else {
        g_appParam.m_mode &= (~MODE_AN_BKGR_IMG);
    }

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_pRadio3))) {
        g_appParam.m_mode |= MODE_DG_BKGR_IMG;
    } else {
        g_appParam.m_mode &= (~MODE_DG_BKGR_IMG);
    }

    pTxt = (gchar*)gtk_entry_get_text(g_pFontColor);
    if (pTxt != NULL && *pTxt != '\0') {
        g_appParam.m_fontColor = (uint32)::strtol(pTxt, NULL, 16);
    }

    pTxt = (gchar*)gtk_entry_get_text(g_pMonoColor);
    if (pTxt != NULL && *pTxt != '\0') {
        g_appParam.m_bkgrColor = (uint32)::strtol(pTxt, NULL, 16);
    }

    pTxt = (gchar*)gtk_entry_get_text(g_pOpacity);
    if (pTxt != NULL && *pTxt != '\0') {
        g_appParam.m_alpha = (uint32)::strtol(pTxt, NULL, 10);
        if (g_appParam.m_alpha < 32) {
            g_appParam.m_alpha = 32;
        }
    }
    aa = (double)g_appParam.m_alpha;

    pthread_mutex_unlock(&g_mutex);

    //gtk_widget_set_size_request(g_pAppWnd, w, h);
    gtk_window_resize(GTK_WINDOW(g_pAppWnd), w, h);

    aa = aa / 255;
    gtk_widget_set_opacity(g_pAppWnd, aa);

    // redraw clock
    gtk_widget_queue_draw(g_pAppWnd);
}


//
//  Function    config_window_closebtn_clicked
//
//--------------------------------------------------------------------------------------------------
static void config_window_closebtn_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget   *config_window = (GtkWidget*)data;
    gtk_widget_destroy(config_window);
    g_pApprcDlg = NULL;
}


//
//  Function    main
//
//--------------------------------------------------------------------------------------------------
int     main(int argc, char **argv) {

    GtkApplication      *pApp;
    int         status;
    int32       numBytes;
    int32       rtrnd;

    // Character classification and case conversion.
    // setlocale(LC_CTYPE, "ja_JP.UTF-8");

    // Initialize the config directory $HOME/.orgclock.

    ::sprintf(g_path, "%s%s", (char*)getenv("HOME"), "/.orgclock");
    numBytes = (int32)::strlen(g_path);

    if (numBytes > MAX_BYTES_CONF_DIR) {
        ::printf(g_errorMssg[ ERR_CONFDIR_TOO_LONG ], MAX_BYTES_CONF_DIR);
        return 0;
    }

    g_pConfigDir = &(g_path[ numBytes ]);

    ::strcpy(g_pConfigDir, "/themes");
    g_pThemeRootDir = g_pConfigDir + 7;

    // Check if default theme directory exists and it is a directory.

    Stat    st;

    ::strcpy(g_pThemeRootDir, "/default");

    if (stat(g_path, &st) != 0 || !S_ISDIR(st.st_mode)){
        ::printf("%s", g_errorMssg[ ERR_DEFAULT_THEME_REQUIRED ]);
        return 0;
    }

    // Read the application setting (param.dat) and theme images

    ::strcpy(g_pConfigDir, "/param.dat");

    if (! SimpleFileRead(g_path, (byte*)&g_appParam, sizeof(g_appParam))) {

        g_appParam.m_mode = MODE_ANALOG | MODE_AN_BKGR_IMG | MODE_DG_BKGR_IMG | MODE_MD_HMS;//MODE_YMD_HM;
        //::wcscpy(g_appParam.m_themeName, L"default");
        //::wcscpy(g_appParam.m_prevThemeName, L"default");
        //::wcscpy(g_appParam.m_fontName, L"Serif 18");
        ::strcpy(g_appParam.m_themeName, "default");
        ::strcpy(g_appParam.m_fontName, "Serif 18");
        g_appParam.m_fontSize  = 18;
        g_appParam.m_fontColor = 0x000000;
        //g_appParam.m_msgFontColor = 0xff0000;
        g_appParam.m_bkgrColor = 0x808080;
        //g_appParam.m_msgBkgrColor = 0x000000;
        //g_appParam.m_colorKey  = 0xffffff;
        g_appParam.m_useClrKey = 1;
        g_appParam.m_alpha = 192;
        g_appParam.m_wndX  = 50;
        g_appParam.m_wndY  = 50;
    }

    gtk_init(&argc, &argv);

    rtrnd = LoadThemeImages(g_appParam.m_themeName);

    if (rtrnd != LT_OK) {
        message_box(g_LoadThemeErrMssgs[ rtrnd ], MB_MSG_ERR, MB_BTN_OK);
        return status;
    }

    pApp = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);

    g_signal_connect(pApp, "activate", G_CALLBACK(appwindow_activate), NULL);

    // If you're calling g_application_run(), you don't need to call gtk_main() as well:
    // the run() method will spin the main loop for you.
    // You also don't use gtk_main_quit() to stop the application's main loop:
    // you should use g_application_quit() instead.
    // ttps://stackoverflow.com/questions/36905118/code-cannot-exit-from-gtk-application-apparently-no-message-loops
    
    status = g_application_run(G_APPLICATION(pApp), argc, argv);

    g_object_unref(pApp);

    pthread_mutex_destroy(&g_mutex);

    if (g_fConfig) {
        ::strcpy(g_pConfigDir, "/param.dat");
        //if (message_box(g_path, MB_MSG_ERR, MB_BTN_OKCN) == GTK_RESPONSE_OK) {
        SimpleFileWrite(g_path, (byte*)&g_appParam, sizeof(g_appParam));
        //}
    }

    return status;
}


//
//  Function    init_bktransprnt
//
// reference
// zetcode.com/gfx/cairo/root/
//
//--------------------------------------------------------------------------------------------------
void    init_bktransprnt(GtkWidget *pWidget) {

    GdkScreen   *pScreen;
    GdkVisual   *pVisual;

    gtk_widget_set_app_paintable(pWidget, TRUE);
    pScreen = gdk_screen_get_default();
    pVisual = gdk_screen_get_rgba_visual(pScreen);

    if (pVisual != NULL && gdk_screen_is_composited(pScreen)) {
        gtk_widget_set_visual(pWidget, pVisual);
    }
}


//
//  Function    message_box
//
//  Return Values
//      GTK_RESPONSE_OK(-5), GTK_RESPONSE_CANCEL(-6)
//      gtkdialog.h
//
//--------------------------------------------------------------------------------------------------
gint    message_box(char *pMssg, GtkMessageType msgType, GtkButtonsType btnType) {

    GtkWidget   *pMsgdlg;
    gint        rtrn;

    pMsgdlg = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, msgType, btnType, "%s", pMssg);

    rtrn = gtk_dialog_run(GTK_DIALOG(pMsgdlg));

    gtk_widget_destroy(pMsgdlg);

    return rtrn;
}


//
//  Function    get_dir_list
//
//--------------------------------------------------------------------------------------------------
int32   get_dir_list(char *pDir, char pplist[][  MAX_BYTES_PATH_INL ], int32 maxDirs) {

    struct dirent *dir;
    DIR     *d;
    int32   numDirs = 0;

    if (! (d = opendir(pDir))) {
        return 0;
    }

    while ((dir = readdir(d)) != NULL && numDirs < maxDirs) {
        if (! (::strcmp(dir->d_name, ".") == 0 || ::strcmp(dir->d_name, "..") == 0)) {
            ::strcpy(pplist[ numDirs ], dir->d_name);
            numDirs ++;
        }
    }

    closedir(d);

    return numDirs;
}


//
//  Function    get_font_size
//
//--------------------------------------------------------------------------------------------------
int32   get_font_size(char *pFontDsc) {

    int32   i, numBytes, val;
    uint8   upper, lower;

    numBytes = ::strlen(pFontDsc);
    val = 0;

    for (i = 0; i < numBytes; i ++) {
        upper = *(pFontDsc + i) & 0xf0;
        lower = *(pFontDsc + i) & 0x0f;

        if (upper == 0x30 && lower < 9) {
            val *= 10;
            val += lower;
        }
    }

    return val;
}


//
//  Function    SimpleFileRead
//
//  Reads binary data from the file.
//  files larger than 0x7fffffff are not supported
//
//  Parameters
//      p_path      [in]    path
//        ex "/folder/chara1.txt"
//      p_buffer    [out]   buffer to read from file
//      size        [in]    buffer's size in bytes
//
//  Return Values
//      success     number of bytes read
//      failure     0. sets 0 to buffer's first 2 bytes
//
//--------------------------------------------------------------------------------------------------
long    SimpleFileRead(const char *p_path, byte *p_buffer, long size) {

    FILE    *fp;
    long    fileSizeLow;
    long    numBytesRead;
    bool    result;

    if (p_path == 0 || p_buffer == 0) {
        return 0;
    }

    if ((fp = fopen(p_path, "rb")) == NULL) {
        return 0;
    }

    fseek(fp, 0, SEEK_END);     // seek to end of file
    fileSizeLow = ftell(fp);    // get current file pointer
    fseek(fp, 0, SEEK_SET);     // seek back to beginning of file

    if (fileSizeLow > 0x7fffffff || fileSizeLow > size || fileSizeLow < 0) {
        ::fclose(fp);
        return 0;
    }

    numBytesRead = (long)::fread(p_buffer, sizeof(byte), ((size_t)fileSizeLow), fp);
    ::fclose(fp);

    if (numBytesRead == fileSizeLow) {
        return numBytesRead;
    }

    return 0;
}


//
//  Function    SimpleFileWrite
//
//  Writes binary data to the file. if the file exists, overwrite it.
//  files larger than 0x7fffffff are not supported
//
//  Parameters
//      pPath       [in]    path
//        ex "/folder/chara1.txt"
//      pBuffer     [in]    buffer to write to file
//      size        [in]    size to write in bytes
//
//  Return Values
//      success     true
//      failuter    false
//
//--------------------------------------------------------------------------------------------------
bool    SimpleFileWrite(const char *p_path, byte *p_buffer, long size) {

    FILE*   fp;
    long    numBytesWritten;
    bool    result;

    if (p_path == 0 || p_buffer == 0) {
        return false;
    }

    if (size > 0x7fffffff || size < 0) {
        return false;
    }

    if ((fp = fopen(p_path, "wb")) == NULL) {
        return false;
    }

    numBytesWritten = (long)::fwrite(p_buffer, sizeof(byte), (size_t)size, fp);
    ::fclose(fp);

    if (numBytesWritten == size) {
        return true;
    }

    return false;
}


//
//  Function    LoadThemeImages
//
//--------------------------------------------------------------------------------------------------
int32   LoadThemeImages(char *pThemeName) {

    //char  themeNm[ _NUM_BYTES_THMNM_INL ];
    //mbstate_t mbstt;

    int32   numBytes;

    int32   anw, anh, dgw, dgh,
            hdw, hdh;

    numBytes = ::strlen(pThemeName);
    if (numBytes > _NUM_BYTES_THMNM) {
        return LT_E_TOO_LONG_NAME;
    }

    // Convert wide charactor string to multibyte string
    //::memset(themeNm, 0, _NUM_BYTES_THMNM_INL);
    //numChrs = ::wcslen(pThemeName);
    //::wcsrtombs(themeNm, (const wchar_t**)&pThemeName, numChrs, &mbstt);
    //numBytes = ::strlen(themeNm);

    // Write theme root path
    ::strcpy(g_pConfigDir, "/themes");

    // Write theme path
    *g_pThemeRootDir = '/';
    ::strcpy((char*)(g_pThemeRootDir + 1), pThemeName);

    g_pThemeDir = g_pThemeRootDir + (1 + numBytes);

    FreeThemeImages();

    // Read theme images

    ::strcpy((char*)g_pThemeDir, "/analogBkgr.png");
    g_analogBkgr.create((char*)g_path);
    anw = g_analogBkgr.getWidth();
    anh = g_analogBkgr.getHeight();

    if (! (100 <= anw && anw <= 500 && anw == anh)) {
        return LT_E_BKGR_INVLD;
    }

    ::strcpy(g_pThemeDir, "/digitalBkgr.png");
    g_digitalBkgr.create((char*)g_path);
    dgw = g_digitalBkgr.getWidth();
    dgh = g_digitalBkgr.getHeight();

    if (! (100 <= dgw && dgw <= 500 && 100 <= dgh && dgh <= 500)) {
        return LT_E_BKGR_INVLD;
    }

    ::strcpy(g_pThemeDir, "/hourHand.png");
    g_hands[ 0 ].create((char*)g_path);
    hdw = g_hands[ 0 ].getWidth();
    hdh = g_hands[ 0 ].getHeight();

    if (! (0 < hdw && hdw < (anw / 2) && 0 < hdh && hdw < anh)) {
        return LT_E_HAND_INVLD;
    }

    ::strcpy(g_pThemeDir, "/minHand.png");
    g_hands[ 1 ].create((char*)g_path);
    hdw = g_hands[ 1 ].getWidth();
    hdh = g_hands[ 1 ].getHeight();

    if (! (0 < hdw && hdw < (anw / 2) && 0 < hdh && hdw < anh)) {
        return LT_E_HAND_INVLD;
    }

    ::strcpy(g_pThemeDir, "/secHand.png");
    g_hands[ 2 ].create((char*)g_path);
    hdw = g_hands[ 2 ].getWidth();
    hdh = g_hands[ 2 ].getHeight();

    if (! (0 < hdw && hdw < (anw / 2) && 0 < hdh && hdw < anh)) {
        return LT_E_HAND_INVLD;
    }

    return LT_OK;
}


//
//  Function    FreeThemeImages
//
//--------------------------------------------------------------------------------------------------
void    FreeThemeImages() {
    g_analogBkgr.free();
    g_digitalBkgr.free();
    g_hands[ 0 ].free();
    g_hands[ 1 ].free();
    g_hands[ 2 ].free();
}
