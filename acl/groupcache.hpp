#ifndef __ACL_GROUPCACHE_HPP
#define __ACL_GROUPCACHE_HPP

#include <utility>
#include <boost/unordered_map.hpp>
#include <boost/thread/mutex.hpp>
#include "acl/group.hpp"
#include "util/error.hpp"

namespace acl
{

class GroupCache
{
  typedef boost::unordered_map<std::string, acl::Group*> ByNameMap;
  typedef boost::unordered_map<gid_t, acl::Group*> ByGIDMap;
  
  mutable boost::mutex mutex;
  ByNameMap byName;
  ByGIDMap byGID;
  
  static GroupCache instance;
  
  ~GroupCache();
  
  static void Save(const acl::Group& group);
  
public:
  static bool Exists(const std::string& name);
  static bool Exists(gid_t gid);
  static util::Error Create(const std::string& name);
  static util::Error Delete(const std::string& name);
  static util::Error Rename(const std::string& oldName, const std::string& newName);
  
  // these return const as the group objects should NEVER
  // be modified except via the above functions'
  static const acl::Group Group(const std::string& name);
  static const acl::Group Group(gid_t gid);
};

} /* acl namespace */

#endif