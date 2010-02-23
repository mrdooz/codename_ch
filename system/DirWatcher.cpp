#include "stdafx.h"
#include "DirWatcher.hpp"
#include "SystemInterface.hpp"


/*
  Directory watcher thread. Uses completion ports to block until it detects a change in the directory tree
  or until a shutdown event is posted
*/
DWORD WINAPI WatcherThread(void* param)
{
  const int32_t NUM_ENTRIES = 128;
  FILE_NOTIFY_INFORMATION info[NUM_ENTRIES];

  WatcherThreadParams *params = (WatcherThreadParams*)param;


  while (true) {

    params->dir_handle = CreateFile("./", FILE_LIST_DIRECTORY, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

    if (params->dir_handle == INVALID_HANDLE_VALUE) {
      return 1;
    }

    params->watcher_completion_port = CreateIoCompletionPort(params->dir_handle, NULL, COMPLETION_KEY_IO, 0);

    if (params->watcher_completion_port == INVALID_HANDLE_VALUE) {
      return 1;
    }

    OVERLAPPED overlapped;
    ZeroMemory(&overlapped, sizeof(overlapped));
    const BOOL res = ReadDirectoryChangesW(
      params->dir_handle, info, sizeof(info), TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE, NULL, &overlapped, NULL);
    if (!res) {
      return 1;
    }

    DWORD bytes;
    ULONG key = COMPLETION_KEY_NONE;
    OVERLAPPED *overlapped_ptr = NULL;
    bool done = false;
    GetQueuedCompletionStatus(params->watcher_completion_port, &bytes, &key, &overlapped_ptr, INFINITE);
    switch (key) {
    case COMPLETION_KEY_NONE: 
      done = true;
      break;
    case COMPLETION_KEY_SHUTDOWN: 
      return 1;

    case COMPLETION_KEY_IO: 
      break;
    }
    CloseHandle(params->dir_handle);
    CloseHandle(params->watcher_completion_port);

    if (done) {
      break;
    } else {
      FileChangedEventArgs *event = new FileChangedEventArgs();
      info[0].FileName[info[0].FileNameLength/2+0] = 0;
      info[0].FileName[info[0].FileNameLength/2+1] = 0;
      UnicodeToAnsiToBuffer(info[0].FileName, event->filename, MAX_PATH);
      // change '\' to '/'
      char *ptr = event->filename;
      while (*ptr) {
        if (*ptr == '\\') {
          *ptr = '/';
        }
        ++ptr;
      }
      g_system->send_event(EventId::FileChanged, event);
    }

  }

  return 0;
}
