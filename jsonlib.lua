workspace "jsonlib_workspace"
  configurations { "Debug", "Release" }
  platforms { "x86", "x86_64" }
  location "build"

project "jsonlib"
  kind "StaticLib"
  language "C"
  includedirs { "." }
  filter "system:Windows"
    files { "include/jsonlib/*.h", "src/*.c" }
  filter "system:Linux"
    files { "include/jsonlib/*.h", "src/*.c" }