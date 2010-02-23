#ifndef DIR_WATCHER_HPP
#define DIR_WATCHER_HPP

#include "Event.hpp"

enum {
  COMPLETION_KEY_NONE         =   0,
  COMPLETION_KEY_SHUTDOWN     =   1,
  COMPLETION_KEY_IO           =   2
};

struct WatcherThreadParams
{
  WatcherThreadParams() : dir_handle(INVALID_HANDLE_VALUE), watcher_completion_port(INVALID_HANDLE_VALUE) {}
  HANDLE dir_handle;
  HANDLE watcher_completion_port;
};

DWORD WINAPI WatcherThread(void *param);

#endif
