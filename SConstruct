#!/usr/bin/env python
import os

env = SConscript("godot-cpp/SConstruct")

env.Append(CPPPATH=["src/"])
sources = Glob("src/*.cpp") + Glob("src/*/*.cpp")

lib_name = "liburbex"
bin_dir = "project/bin"

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "{0}/{1}.{2}.{3}.framework/{1}.{2}.{3}".format(bin_dir, lib_name, env["platform"], env["target"]),
        source=sources,
    )
else:
    library = env.SharedLibrary(
        "{}/{}{}{}".format(bin_dir, lib_name, env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)
