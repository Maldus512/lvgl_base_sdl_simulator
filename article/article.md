
# LVGL GUI Library with SDL2 Backend

Today I wish to talk about and Little Virtual Graphic Library, or LVGL for short.
It's a software project of the highest quality, and one that has been most useful to me in the recent years.

In short its name is describing enough: it's a GUI library written in C and meant to build graphical applications.
The catch is that it is hardware agnostic: it doesn't rely on an operating system or on a specific hardware (i.e. a display) to work.
In fact, it targets mainly embedded devices - or at least highly specialized interfaces - instead of general PC environments.

Its ubiquitous nature allows it to be ported to many different platforms with little to no effor.
However, as I will show you shortly being hardware agnostic doesn't preclude the possiblity to work as a regular UI framework for PC - in fact, it fares better than most other libraries.

The following tutorial is based on the last LVGL version to the date of writing, which is 8.3.

## Simple, yet beautiful

LVGL is born from the desire to work with a single UI library on many different platforms.
To achieve this, one must abstract on every aspect of GUI development that is irrevocably tied to the underlying hardware. 

As it turns out, it's not much: for every monitor that LVGL should render the developer must instantiate a `lv_disp_drv_t` instance.
That is the interface that ties the library to the actual pixels on the screen. 
Besides an obvious buffer in RAM that is required to work, this interface needs a single function pointer for all the heavy lifting:

```
void (*flush_cb)(struct _lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);
```

    Note: the `lv_disp_drv_t` structure contains many other function pointers that can be initialized, but none of them is strictly required.
    There are hooks for performance monitoring and optional shortcuts for limit cases.

This function will be called by the LVGL internals to signal that a rectangular `area` should be drawn on the screen.
How this is implemented is up the developer: it can send the data on a SPI connection to a serial display; write pixels on an HDMI monitor through the linux framebuffer interface; dump the resulting image on a PNG file or even through a network connection.

The rest of the library is completely ignorant of what happens outside of its borders, content with just drawing the UI interface in the provided RAM buffer.

Albeit very simple, the approach is extremely powerful. 
I've been working with LVGL on dozens of different targets (resolution, color format, hardware, processor architecture, you name it), all the while developing with the same, familiar API.

One could describe LVGL as being the frontend part of a UI architecture: it provides widgets, drawing routines and input control.
On its own it doesn't work; it needs a backend to be plugged in to actually display images on a screen.

To show you how to build an LVGL application without getting hold of a specific hardware, I'm going to target an SDL2 backend so that the application can be rendered on any machine with a window manager.

Note: even if the end result runs on a PC, this doubles as tutorial for any embedded target - granted one should also provide the proper display driver.

## Requirements

As a minimum, a C compiler is required. I'm going to use gcc, but I don't see why clang shouldn't work.

I'm going to structure the project using [SCons](https://scons.org/). 
It is an elegant and effective build system and I've already lauded it extensively [here](https://maldus512.medium.com/looking-for-a-makefile-alternative-6e7f795b5cad).
It can be installed as a package or through pip.

Since we are going to use the SDL2 backend, you must install the corresponding library (in its development form). 
It is available as an official package in every distribution, just remember to install the developer version.

I'm using git to manage LVGL and similar source code dependencies, but it would work just as well by simply downloading the source code and copying it into the project.

That is all to get started.

## The Project

I like to separate my C projects into two main folders: `main` and `components` (heavily inspired from ESP-IDF projects).

The `main` folder will contain my code, while in `components` I shall add all external libraries.

To include lvgl, enter the `components` directory and run the following commands:

```
cd components
git clone https://github.com/lvgl/lvgl.git
git checkout --track origin/release/v8.3
```

The last command ensures we are using version 8.3 of the library, which is the latest stable at the time of writing.

Before LVGL can be used it should be configured; it does so with a simple header file.
You can find a template in `components/lvgl/lv_conf_template.h`: you should create the folder `main/config` and copy it there, renaming it as `lv_conf.h`.

```
mkdir main/config
cp components/lvgl/lv_conf_template.h main/config/lv_conf.h
```

Note: LVGL integrates some specific frameworks that also manage its configuration with other tools (e.g. with KConfig).
This is not the case here, so we're just going to edit directly the header file.

Before anything else, the configuration file must be anabled by manually changing the preprocessor guard at line 15:

```
/**
 * @file lv_conf.h
 * Configuration file for v8.3.3
 */

/*
 * Copy this file as `lv_conf.h`
 * 1. simply next to the `lvgl` folder
 * 2. or any other places and
 *    - define `LV_CONF_INCLUDE_SIMPLE`
 *    - add the path as include path
 */

/* clang-format off */
#if 0 /*Set it to "1" to enable content*/   // <-- Change this 0 to 1

#ifndef LV_CONF_H
#define LV_CONF_H

// ...
```

There are a lot of configuration options, but at this stage the defaults are good enough. 
The importance of most of them depends heavily on the specifics of the target: for example, `LV_COLOR_DEPTH` should be set in accordance with the capabilities of the display LVGL is working with.
A wrong value here will inevitably result in garbled output on the screen. However, SDL is able to handle pretty much any of the available formats, so it doesn't concern us.

The next step is to write a bare minimum of initialization code and setup the build system. Create the file `main/main.c` with the following content:

```
#include <stdio.h>
#include <unistd.h>
#include "lvgl.h"


int main(void) {
    // What oh, LVGL!
    lv_init();

    printf("Begin main loop\n");
    for (;;) {
        // Run LVGL engine
        lv_tick_inc(1);
        lv_timer_handler();

        usleep(1000);
    }

    return 0;
}
```

This will be the starting point and the beating heart of our application.
The inclusions are straightforward: `stdio.h` to print stuff on the terminal, `unistd.h` to access the `usleep` function and - of course - `lvgl.h` for the graphics library.

There are only two points of interest here:

    1. `lv_init()` initializes the library. It's mandatory to invoke it before doing anything related to LVGL.
    2. `lv_tick_inc(1)` and `lv_timer_handler()` are where LVGL actually works. 
        The latter call is needed to run the drawing engine, manage input devices and periodic tasks. Somewhere deep inside, at some point, the `flush_cb` callback will interact with the actual screen.
        The former informs the engine that some units of time have passed - in this case, 1. Implicitly this counts milliseconds.

A little barebones, but this could already be called an LVGL application. We will eventually add an SDL2 driver, but for now let's get to compiling.

### Building

As previously stated, I'm using SCons for this project. Here is the SConstruct file:

```
import os
import multiprocessing
from pathlib import Path


# Name of the application
PROGRAM = "app"

# Project paths
MAIN = "main"
COMPONENTS = "components"
CONFIG = f"{MAIN}/config"
LVGL = f"{COMPONENTS}/lvgl"

# Compilation flags
CFLAGS = ["-Wall", "-Wextra", "-g", "-O0", ]
CPPPATH = [COMPONENTS, MAIN, LVGL, CONFIG]
CPPDEFINES = ["LV_CONF_INCLUDE_SIMPLE"]


def main():
    # If not specified, guess how many threads the task can be split into
    num_cpu = multiprocessing.cpu_count()
    SetOption("num_jobs", num_cpu)
    print("Running with -j {}".format(GetOption("num_jobs")))

    env_options = {
        "CPPPATH": CPPPATH,
        "CPPDEFINES": CPPDEFINES,
        "CCFLAGS": CFLAGS,
    }
    env = Environment(**env_options)

    # Project sources
    sources = [File(filename) for filename in Path(
        f"{MAIN}").rglob("*.c")]  # application files
    sources += [File(filename)
                for filename in Path(f"{LVGL}/src").rglob("*.c")]  # LVGL

    env.Program(PROGRAM, sources)


main()
```

At the beginning of the file I declare a few helpful constants, like the name of the program, the path of every major component and some compilation flags.
Among those, two are of notable importance:

    1. The folder where LVGL's source code resides should be included in the `CPPPATH` to make sure its headers are found.
    2. Throughout the project the macro `LV_CONF_INCLUDE_SIMPLE` is defined. This is an additional configuration bit to tell LVGL that it can find the file `lv_conf.h` as is in the include path; otherwise it will look for `../../lv_conf.h`, which is not where we stored it.

The `main` python function is where the building hierarchy is defined. 
First we tell SCons how many jobs should be run, depending on the number of processor cores: LVGL is a medium-sized library and we should take advantage of parallel compilation if possible.

Then we instantiate a build environment using all the previously defined compilation flags.

Finally all the .c source files from LVGL and our application should be collected in a list and specified as dependency for the final executable.
To do this I simply leverage on the `Path` object from the `pathlib` python library, which allows me to recursively search for all files matching a regex in a folder.
For now, there are only two folders with sources in this project: `main` and `components/lvgl/src`.

`env.Program(PROGRAM, sources)` finalizes the act. Running `scons` should now compile everything, and the resulting `app` can be executed.

Of course, this does nothing but print "Begin main loop" on the terminal, but up until now we've laid the foundations. Let's add a window.

### Hardware Abstraction

The following steps are going to specialize this project to be displayed on an SDL2 window.
Normally, this would require a small tutorial on its own to understand how that works; luckily this time someone already fixed the dirty work for us.

[lv_drivers](https://github.com/lvgl/lv_drivers) is a collection of interfaces made for LVGL for many common display drivers.
Among them there is also an SDL2 driver.

Add the driver library with the following commands:

```
cd components
git clone https://github.com/lvgl/lv_drivers.git
git checkout --track origin/release/v8.3
```

Just like LVGL the drivers have a 8.3 branch to align with the main library, and just like LVGL there is an header to configure it.
Copy `components/lv_drivers/lv_drv_conf_template.h` to `main/config/lv_drv_conf.h` and enable it by setting the preprocessor directive at line 11 to 1; also enable the SDL driver by doing the same at line 89:

```
/**
 * @file lv_drv_conf.h
 * Configuration file for v8.3.0
 */

/*
 * COPY THIS FILE AS lv_drv_conf.h
 */

/* clang-format off */
#if 0 /*Set it to "1" to enable the content*/   // <-- Change this 0 to 1

#ifndef LV_DRV_CONF_H
#define LV_DRV_CONF_H

// ...

/* SDL based drivers for display, mouse, mousewheel and keyboard*/
#ifndef USE_SDL
# define USE_SDL 0  // <-- Change this 0 to 1
#endif
```

Now we need to include the driver library in the compilation process, which requires three simple additions to `SConstruct`.

First, declare a new variable for the drivers' path and add said variable to the list of include paths:

```
DRIVERS = f"{COMPONENTS}/lv_drivers"

# ...

CPPPATH = [COMPONENTS, MAIN, LVGL, CONFIG, DRIVERS]
```

Then add a new option to the environment dictionary to link SDL2 to the application:

```
env_options = {
    # Include the external environment to access DISPLAY and run the app as a target
    "CPPPATH": CPPPATH,
    "CPPDEFINES": CPPDEFINES,
    "CCFLAGS": CFLAGS,
    "LIBS" : ["-lSDL2"],
}
```

Without this step the compilation will fail with several errors mentioning undefined references to `SDL_` functions.

Finally add another round of source files to the `sources` list:

```
sources += [File(filename)
            for filename in Path(DRIVERS).rglob("*.c")]  # Drivers
```

That's it for SCons. Now open up again main.c and add an inclusion for `"sdl/sdl.h"` and call `sdl_init()` right after `lv_init()`.
The order is important here: since `sdl_init()` is part of the LVGL driver for SDL2, LVGL must already be initialized before calling it.

After this we need to connect the SDL2 driver to the LVGL drawing engine. This is done be initializing a display and input devices.

```
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
```

The display needs a memory buffer to work with. 
Right now I'm using a buffer as big as the whole screen, but LVGL can work with smaller fractions.

The display driver itself is the `lv_disp_drv_t` structure, and it is given 3 vital pieces of information:

    1. The display buffer it should work on.
    2. The hardware abstraction, i.e. the function that draws on the screen.
    3. The size of the screen (horizontal and vertical resolution)

This is registered as display to the library and a theme - the default one - is assigned to it.

Note: LVGL could potentially work with multiple screens at once! The only constraint is that all of them must have the same color depth.

LVGL can also manage user input, usually in the form of a touch screen; here we're talking about your mouse or trackpad.

```
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv); /*Basic initialization*/
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = sdl_mouse_read;
    lv_indev_drv_register(&indev_drv);
```

This piece of code registers an input device which is polled by the internal engine with `indev_drv.read_cb`. 
`sdl_mouse_read`, just like `sdl_display_flush`, is courtesy of the SDL2 driver.

Note that `disp_buf`, `buf_1`, `disp_drv` and `indev_drv` are declared as `static`; they should persist in memory even if we return from whathever function we are working on because LVGL expects their memory to be persistent.
Allocating them dynamically with `malloc` or similar would work just as well.

Compile again and run: now you should be greeted by a working - albeit empty - window. This is normal, as we have yet to fill it with anything meaningful.

For now, a simple button will suffice. Add the following code after all the initializations and before the main loop:

```
    lv_obj_t *btn = lv_btn_create(lv_scr_act());    // Create a new button on the current screen
    lv_obj_t *lbl = lv_label_create(btn);           // Add a label to this button

    lv_label_set_text(lbl, "Hello world!");

    lv_obj_center(lbl);
    lv_obj_center(btn);
```

I'm not going to delve on the LVGL API itself; [there's an excellent documentation for that](https://docs.lvgl.io/8.3/).
Suffice to say that I am creating a button containing a label and putting it in the center of the screen.

Compile one last time, run and voila'!


## Timing Considerations

Before closing the topic I want to hover a little more on how LVGL handles time.

This example increases the library's tick count by one every millisecond. 
This is correct to an extent: besides the fact that `usleep` is not precise in its delay, the application may be held for more than 1000 microseconds by other tasks, making the reported elapsed time even more inaccurate.

To be fair this is not particularly important, as the kind of graphical work LVGL does under the hood is not impaired by the occasional few milliseconds of error; but we are here to learn, so let's make a good job.

### Custom Time

One option is to give LVGL a direct binding to the system time, so it can autonomously read the current tick count instead of relying on `lv_tick_inc` to be called.

This is achieved via the `LV_TICK_CUSTOM*` configuration options. 
First, we need a monotonic (i.e. always growing) source for the number of elapsed milliseconds.
Create the file `main/utils/system_time.c` with the following content:

```
#include <time.h>
#include <sys/time.h>


unsigned long get_millis(void) {
    unsigned long   now_ms;
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    now_ms = ts.tv_sec * 1000UL + ts.tv_nsec / 1000000UL;

    return now_ms;
}
```

There may be other ways to get a similar result; I'm most happy with this one. 
`get_millis` will return the number of milliseconds that have passed since the machine was turned on.

Add a corresponding header file with the forward declaration, then go to line 88 of `lv_conf.h` and edit it like so:

```
/*Use a custom tick source that tells the elapsed time in milliseconds.
 *It removes the need to manually update the tick with `lv_tick_inc()`)*/
#define LV_TICK_CUSTOM 1
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE "utils/system_time.h"   /*Header for the system time function*/
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (get_millis())    /*Expression evaluating to current system time in ms*/
#endif   /*LV_TICK_CUSTOM*/
```

Now you can remove the call to `lv_tick_inc` because LVGL is equipped to keep track of time on its own.

While convenient I have grown to dislike this approach ever since it caused my application to hang.
This happened because I was rendering a page with lots of animations - i.e. time dependant drawing tasks.

Imagine there are `n` animated objects on your screen with a frame rate of 30 fps, which means about one frame every 33 ms.
The `lv_timer_handler` call loops on registered tasks, among which there are the ones who manage those animations. 

It starts with animation 1, handles the drawing and sends it to the screen - both potentially time consuming tasks.
Then it moves to the second animation, the third and so on. 
When it finishes the last one `lv_timer_handler` should return... Except by now 33 ms have already passed since the first animation's update and LVGL knows it should render the next frame.
The cycle restarts and your application never exits from `lv_timer_handler`.

This can also happen if you put too much load on LVGL's timers for reasons other than animating objects. 
It is a problematic situation that should be resolved by reducing said load; nonetheless, it would still be desirable for the application to be more resilient.


### Proper Time Tracking

Another option I'm more fond of is to still feed ticks directly but calculate the elapsed time since the last invocation exactly.

```
int main(int argc, char *argv[]) {
    static unsigned long last_invoked = 0;
    /* ... */
    for (;;) {
        // Run LVGL engine
        if (last_invoked > 0) {
            lv_tick_inc(get_millis() - last_invoked);
        }
        last_invoked = get_millis();
        lv_timer_handler();

        usleep(1000);
    }

    return 0;
}
```

By taking the clock away we make sure that `lv_timer_handler` will return even in the event of tasks spilling over; The rendering process will slow down, but won't hang.


## Conclusion

This is how to setup a project using LVGL. As stated in the introduction interfacing with a specific display may be a little more complex, but chances are `lv_drivers` contains the code required by the model you are using - or something pretty similar.

Working on your machine instead of the target brings other advantages besides learning the approach. 
It is possible to run at least the UI part of an embedded application on the compiling PC to shave time from the code-compile-debug cycle.

In conclusion: LVGL is great, consider it for your next C project - embedded or otherwise.