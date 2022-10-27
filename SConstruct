import os
import multiprocessing
from pathlib import Path


PROGRAM = "./app"
MAIN = "main"
COMPONENTS = "components"
CONFIG = f"{MAIN}/config"
LVGL = f"{COMPONENTS}/lvgl"
DRIVERS = f"{COMPONENTS}/lv_drivers"

CFLAGS = ["-Wall", "-Wextra", "-g", "-O0", ]
LDLIBS = ["-lSDL2"]
CPPPATH = [
    COMPONENTS, f'#{MAIN}', f"#{LVGL}", f"#{DRIVERS}", f"#{CONFIG}"
]
CPPDEFINES = ["LV_CONF_INCLUDE_SIMPLE"]


def PhonyTargets(
    target,
    action,
    depends,
    env=None,
):
    # Creates a Phony target
    if not env:
        env = DefaultEnvironment()
    t = env.Alias(target, depends, action)
    env.AlwaysBuild(t)


def main():
    num_cpu = multiprocessing.cpu_count()
    SetOption("num_jobs", num_cpu)
    print("Running with -j {}".format(GetOption('num_jobs')))

    env_options = {
        "ENV": os.environ,
        "CC": "gcc",
        "CPPPATH": CPPPATH,
        "CPPDEFINES": CPPDEFINES,
        "CCFLAGS": CFLAGS,
        "LIBS": LDLIBS,
    }

    env = Environment(**env_options)
    env.Tool("compilation_db")

    sources = [File(filename) for filename in Path(f"{MAIN}").rglob('*.c')]
    sources += [File(filename)
                for filename in Path(f'{LVGL}/src').rglob('*.c')]
    sources += [File(filename) for filename in Path(DRIVERS).rglob('*.c')]

    prog = env.Program(PROGRAM, sources)

    PhonyTargets("run", PROGRAM, prog, env)
    compileDB = env.CompilationDatabase("compile_commands.json")
    env.Depends(prog, compileDB)


main()