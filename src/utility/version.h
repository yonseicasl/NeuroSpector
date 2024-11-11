#define VERSION_MAJOR 1
#define VERSION_MINOR 6
#define VERSION_PATCH 0
#define TOSTR(str) #str
#define TOSTR_VER(major, minor, patch) TOSTR(major.minor.patch)

#define FULL_VERSION TOSTR_VER(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH)
