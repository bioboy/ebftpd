#ifndef __DB_INTERFACE_HPP
#define __DB_INTERFACE_HPP

#include <mongo/client/dbclient.h>
#include <vector>
#include "acl/types.hpp"
#include "acl/user.hpp"
#include "acl/group.hpp"
#include "stats/stat.hpp"

namespace db 
{
  
  // initalize
  void Initalize(); // should only be called once by initial thread
  
  // stats functions
  void IncrementStats(const acl::User& user,
    long  long kbytes, double xfertime, stats::Direction direction);
  void DecrementStats(const acl::User& user,
    long long kbytes, double xfertime, stats::Direction direction);


// end
}
#endif
