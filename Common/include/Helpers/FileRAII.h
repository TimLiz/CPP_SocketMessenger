#ifndef MYMESSENGER_FILERAII_H
#define MYMESSENGER_FILERAII_H
#include "RAII.h"

#include "cstdio"

namespace RAII {
RAII_GEN_RESOURCE_WRAPPER_EXT(wFile, FILE, std::fclose, true);
}
#endif
