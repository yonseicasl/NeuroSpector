#include <iostream> 
#include "version.h"

void print_banner() {
    std::clog << "=======================================================" << std::endl;
    std::clog << "   _  __                  ____             __          "        << std::endl;
    std::clog << "  / |/ /__ __ _________  / __/__  ___ ____/ /____  ____"        << std::endl;
    std::clog << " /    / -_) // / __/ _ \\_\\ \\/ _ \\/ -_) __/ __/ _ \\/ __/"   << std::endl;
    std::clog << "/_/|_/\\__/\\_,_/_/  \\___/___/ ___/\\__/\\__/\\__/\\___/_/   " << std::endl;
    std::clog << "                          /_/                          "        << std::endl;
    std::clog << "                                        Version: " << FULL_VERSION << std::endl;
    std::clog << "=======================================================" << std::endl;
    return;
}