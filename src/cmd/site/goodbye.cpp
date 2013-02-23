#include "cmd/site/goodbye.hpp"
#include "text/util.hpp"
#include "logs/logs.hpp"
#include "acl/misc.hpp"
#include "fs/path.hpp"

namespace cmd { namespace site
{

void GOODBYECommand::Execute()
{
  fs::Path goodbyePath(acl::message::Choose<acl::message::Goodbye>(client.User()));
  if (!goodbyePath.IsEmpty())
  {
    std::string goodbye;
    auto e = text::GenericTemplate(client, goodbyePath, goodbye);
    if (!e) logs::Error("Failed to display goodbye message: %1%", e.Message());
    else
    {
      control.Reply(ftp::CommandOkay, goodbye);
      return;
    }
  }
  
  control.Reply(ftp::CommandOkay, "No goodbye message");
}

} /* site namespace */
} /* cmd namespace */
