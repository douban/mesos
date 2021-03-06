#ifndef __PROCESS_MUTEX_HPP__
#define __PROCESS_MUTEX_HPP__

#include <queue>

#include <process/future.hpp>
#include <process/internal.hpp>
#include <process/owned.hpp>

#include <stout/memory.hpp>
#include <stout/nothing.hpp>

namespace process {

class Mutex
{
public:
  Mutex() : data(new Data()) {}

  Future<Nothing> lock()
  {
    Future<Nothing> future = Nothing();

    internal::acquire(&data->lock);
    {
      if (!data->locked) {
        data->locked = true;
      } else {
        Owned<Promise<Nothing> > promise(new Promise<Nothing>());
        data->promises.push(promise);
        future = promise->future();
      }
    }
    internal::release(&data->lock);

    return future;
  }

  void unlock()
  {
    internal::acquire(&data->lock);
    {
      if (!data->promises.empty()) {
        // TODO(benh): Skip a future that has been discarded?
        data->promises.front()->set(Nothing());
        data->promises.pop();
      } else {
        data->locked = false;
      }
    }
    internal::release(&data->lock);
  }

private:
  struct Data
  {
    Data() : lock(0), locked(false) {}

    ~Data()
    {
      // TODO(benh): Fail promises?
    }

    // Rather than use a process to serialize access to the mutex's
    // internal data we use a low-level "lock" which we acquire and
    // release using atomic builtins.
    int lock;

    // Describes the state of this mutex.
    bool locked;

    // Represents "waiters" for this lock.
    std::queue<Owned<Promise<Nothing> > > promises;
  };

  memory::shared_ptr<Data> data;
};

} // namespace process {

#endif // __PROCESS_MUTEX_HPP__
