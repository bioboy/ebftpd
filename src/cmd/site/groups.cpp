#include <sstream>
#include <vector>
#include <boost/optional.hpp>
#include "cmd/site/groups.hpp"
#include "acl/group.hpp"
#include "acl/user.hpp"
#include "acl/group.hpp"
#include "text/error.hpp"
#include "text/factory.hpp"
#include "text/template.hpp"
#include "text/templatesection.hpp"
#include "text/tag.hpp"
#include "logs/logs.hpp"
#include "db/group.hpp"
#include "db/user.hpp"

namespace cmd { namespace site
{

void GROUPSCommand::Execute()
{
  boost::optional<text::Template> templ;
  try
  {
    templ.reset(text::Factory::GetTemplate("groups"));
  }
  catch (const text::TemplateError& e)
  {
    control.Reply(ftp::ActionNotOkay, e.Message());
    return;
  }

  auto groups = db::GetGroups();

  std::ostringstream os;
  text::TemplateSection& head = templ->Head();
  os << head.Compile();

  text::TemplateSection& body = templ->Body();

  for (auto& group: groups)
  {    
    size_t numUsers = acl::User::GetUIDs("=" + group.Name()).size();

    body.RegisterValue("users", numUsers);
    body.RegisterValue("group", group.Name());
    body.RegisterValue("descr", group.Description());
    os << body.Compile();
  }

  text::TemplateSection& foot = templ->Foot();
  foot.RegisterValue("total_groups", groups.size());
  os << foot.Compile();

  control.Reply(ftp::CommandOkay, os.str());
}

// end
}
}
