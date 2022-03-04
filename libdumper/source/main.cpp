// Copyright (c) 2021 Al Azif
// License: GPLv3

#include "common.hpp"
#include "dump.hpp"
#include "dumper.h"
#include <iostream>

#define DUMPER_LOG "/user/app/NPXS39041/logs/itemzflow.log"


bool Dumper(char* dump_path, char* title_id, Dump_Options opt, char* title)
{
    /*-- INIT LOGGING FUNCS --*/
    log_set_quiet(false);
    log_set_level(LOG_DEBUG);
    FILE* fp = fopen(DUMPER_LOG, "w");
    if(fp != NULL)
      log_add_fp(fp, LOG_DEBUG);

    log_info("LibDumper Started with path: %s tid: %s title: %s opt: %i", dump_path, title_id, title, opt);
    return dump::__dump(std::string(dump_path), std::string(title_id), opt, std::string(title));
}

