#include <sstream>
#include "cmd/site/grpdel.hpp"
#include "acl/groupcache.hpp"
#include "db/user/user.hpp"
#include "main.hpp"

namespace cmd { namespace site
{

cmd::Result GRPDELCommand::Execute()
{
  std::ostringstream acl;
  acl << "=" << args[1];
  boost::ptr_vector<acl::User> users;
  
  util::Error e = db::user::UsersByACL(users, acl.str());
  if (!e) 
  {
    control.Reply(ftp::ActionNotOkay, "GRPDEL failed: " + e.Message());
    return cmd::Result::Okay;
  }

  if (!users.empty())
  {
    control.Reply(ftp::ActionNotOkay, "GRPDEL failed: Group must be empty before deletion.");
    return cmd::Result::Okay;
  }

  e = acl::GroupCache::Delete(args[1]);
  if (!e)
    control.Reply(ftp::ActionNotOkay, "GRPDEL failed: " + e.Message());
  else
    control.Reply(ftp::CommandOkay, "Group " + args[1] + " deleted.");

  return cmd::Result::Okay;
}

} /* site namespace */
} /* cmd namespace */