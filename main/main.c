#include <stdio.h>
#include <unistd.h>
#include "lvgl.h"
#include "sdl/sdl.h"
#include "utils/system_time.h"
#include "emscripten.h"


static void driver_init(void);
static void create_ui(void);
static void do_loop(void*arg);


int main(void) {
    printf("1\n");
    lv_init();
    printf("2\n");
    sdl_init();
    printf("3\n");

    driver_init();
    printf("4\n");
    create_ui();
    printf("5\n");

    printf("Begin main loop\n");
    emscripten_set_main_loop_arg(do_loop, NULL, 100, 1);
    //do_loop(NULL);
    return 0;
}

static void do_loop(void *arg) {
    static unsigned long last_invoked = 0;

    // Run LVGL engine
    /*if (last_invoked > 0) {
        lv_tick_inc(get_millis() - last_invoked);
    }
    last_invoked = get_millis();*/
    lv_tick_inc(10);
    lv_timer_handler();
    //usleep(1000);
}


static void create_ui(void) {
    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_t *lbl = lv_label_create(btn);

    lv_label_set_text(lbl, "Hello world!");

    lv_obj_center(lbl);
    lv_obj_center(btn);
}


static void driver_init(void) {
#define BUFFER_SIZE (SDL_HOR_RES * SDL_VER_RES)
    /*A static or global variable to store the buffers*/
    static lv_disp_draw_buf_t disp_buf;

    /*Static or global buffer(s). The second buffer is optional*/
    static lv_color_t *buf_1[BUFFER_SIZE] = {0};

    /*Initialize `disp_buf` with the buffer(s). With only one buffer use NULL instead buf_2 */
    lv_disp_draw_buf_init(&disp_buf, buf_1, NULL, BUFFER_SIZE);

    static lv_disp_drv_t disp_drv;         /*A variable to hold the drivers. Must be static or global.*/
    lv_disp_drv_init(&disp_drv);           /*Basic initialization*/
    disp_drv.draw_buf = &disp_buf;         /*Set an initialized buffer*/
    disp_drv.flush_cb = sdl_display_flush; /*Set a flush callback to draw to the display*/
    disp_drv.hor_res  = SDL_HOR_RES;       /*Set the horizontal resolution in pixels*/
    disp_drv.ver_res  = SDL_VER_RES;       /*Set the vertical resolution in pixels*/

    lv_disp_t *disp = lv_disp_drv_register(&disp_drv); /*Register the driver and save the created display objects*/
    lv_theme_default_init(disp, lv_color_make(0x77, 0x44, 0xBB), lv_color_make(0x14, 0x14, 0x3C), 1, lv_font_default());

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv); 
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = sdl_mouse_read;
    lv_indev_drv_register(&indev_drv);
}
