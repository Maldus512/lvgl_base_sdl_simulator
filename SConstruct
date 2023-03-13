import os
import multiprocessing
from pathlib import Path


# Name of the application
PROGRAM = "app.html"

# Project paths
MAIN = "main"
COMPONENTS = "components"
CONFIG = f"{MAIN}/config"
LVGL = f"{COMPONENTS}/lvgl"
DRIVERS = f"{COMPONENTS}/lv_drivers"

# Compilation flags
CFLAGS = ["-Wall", "-Wextra", "-g", "-O0", ]
CPPPATH = [COMPONENTS, MAIN, LVGL, CONFIG, DRIVERS]
CPPDEFINES = ["LV_CONF_INCLUDE_SIMPLE"]


def main():
    # If not specified, guess how many threads the task can be split into
    num_cpu = multiprocessing.cpu_count()
    SetOption("num_jobs", num_cpu)
    print("Running with -j {}".format(GetOption("num_jobs")))

    env_options = {
        "ENV": os.environ,
        "CC": ARGUMENTS.get('cc', 'gcc'),
        # Include the external environment to access DISPLAY and run the app as a target
        "CPPPATH": CPPPATH,
        "CPPDEFINES": CPPDEFINES,
        "CCFLAGS": CFLAGS,
        "LIBS" : ["-lSDL2"],
    }
    env = Environment(**env_options)

    # Project sources
    sources = [File(filename) for filename in Path(
        f"{MAIN}").rglob("*.c")]  # application files
    sources += [File(filename)
                for filename in Path(f"{LVGL}/src").rglob("*.c")]  # LVGL
    sources += [File(filename)
                for filename in Path(DRIVERS).rglob("*.c")]  # Drivers

    env.Program(PROGRAM, sources)


main()
