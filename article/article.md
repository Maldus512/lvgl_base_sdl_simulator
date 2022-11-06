
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

