#!/usr/bin/env python
import os

env = SConscript("godot-cpp/SConstruct")

env.Append(CPPPATH=["src/"])
if env["platform"] == "linux":
    env.Append(CCFLAGS=["-pthread"], LINKFLAGS=["-pthread"])
if ARGUMENTS.get("sanitize", "no") == "yes":
    env = env.Clone()
    sanitize_flags = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-sanitize-recover=undefined"]
    env.Append(CCFLAGS=sanitize_flags, LINKFLAGS=sanitize_flags)
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
