#ifndef EVENT_HPP
#define EVENT_HPP

#pragma warning(push)
#pragma warning(disable: 4100)
#include "FastDelegate.hpp"
#pragma warning(pop)

namespace EventId
{
  enum Enum
  {
    FileChanged,
  };
}

class EventArgs
{
public:
  EventArgs() {}
  virtual ~EventArgs() {}
private:
  EventArgs(const EventArgs&);
  void operator=(const EventArgs&);
};

struct FileChangedEventArgs : public EventArgs
{
  char filename[MAX_PATH];
};

typedef fastdelegate::FastDelegate<void(const EventArgs*)> EventHandler;

typedef fastdelegate::FastDelegate<void()> FileChangedCallback;


#endif
