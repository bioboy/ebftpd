#include <sstream>
#include <cstdlib>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/regex.hpp>
#include "cfg/setting.hpp"
#include "cfg/error.hpp"
#include "util/string.hpp"

namespace cfg { namespace setting
{

Database::Database() :
  name("ebftpd"), 
  address("localhost"), 
  port(27017)
{
  std::ostringstream os;
  os << address << ":" << port;
  host = os.str();
}

Database::Database(const std::vector<std::string>& toks) : port(-1)
{
  name = toks[0];
  address = toks[1];
  try
  {
    port = boost::lexical_cast<int>(toks[2]);
    if (port < 0 || port >= 65535) throw boost::bad_lexical_cast();
  }
  catch (const boost::bad_lexical_cast&)
  {
    throw ConfigError("Invalid port number");
  }

  std::ostringstream os;
  os << address << ":" << port;
  host = os.str();
  
  if (toks.size() == 3) return;
  if (toks.size() != 5) throw ConfigError("Wrong numer of Parameters for database");
  
  login = toks[3];
  password = toks[4];
}

bool Database::NeedAuth() const
{
  if (login.empty()) return false;
  assert(!password.empty());
  return true;
}


AsciiDownloads::AsciiDownloads(const std::vector<std::string>& toks) : size(-1)
{
  try
  {
    size = boost::lexical_cast<int>(toks[0]);
  }
  catch (const boost::bad_lexical_cast&) { }
  if (size == 0) size = -1;
  masks.assign(toks.begin() + 1, toks.end());
}

bool AsciiDownloads::Allowed(off_t size, const std::string& path) const
{
  if (this->size > 0 && size > this->size) return false;
  if (masks.empty()) return true;
  for (auto& mask : masks)
  {
    if (util::WildcardMatch(mask, path)) return true;
  }
  return false;
}


AsciiUploads::AsciiUploads(const std::vector<std::string>& toks)
{
  masks.assign(toks.begin(), toks.end());
}

bool AsciiUploads::Allowed(const std::string& path) const
{
  if (masks.empty()) return true;
  for (auto& mask : masks)
  {
    if (util::WildcardMatch(mask, path)) return true;
  }
  return false;
}

SecureIp::SecureIp(std::vector<std::string> toks)
{
  int numOctets = boost::lexical_cast<int>(toks[0]);
  if (numOctets < 0) throw boost::bad_lexical_cast();
  bool isHostname = util::BoolLexicalCast(toks[1]);
  bool hasIdent = util::BoolLexicalCast(toks[2]);
  strength = acl::IPStrength(numOctets, isHostname, hasIdent);
  toks.erase(toks.begin(), toks.begin()+3);
  acl = acl::ACL(util::Join(toks, " "));
}

SecurePass::SecurePass(std::vector<std::string> toks) :
  strength(toks[0])
{
  toks.erase(toks.begin());
  acl = acl::ACL(util::Join(toks, " "));
}

SpeedLimit::SpeedLimit(std::vector<std::string> toks)
{
  path = toks[0];
  dlLimit = boost::lexical_cast<long>(toks[1]);
  ulLimit = boost::lexical_cast<long>(toks[2]);
  if (toks.size() > 3)
  {
    toks.erase(toks.begin(), toks.begin() + 3);
    acl = acl::ACL(util::Join(toks, " "));
  }
  else
  {
    acl = acl::ACL("*");
  }
}

SimXfers::SimXfers(std::vector<std::string> toks)
{
  maxDownloads = boost::lexical_cast<unsigned>(toks[0]);
  maxUploads = boost::lexical_cast<unsigned>(toks[1]);
}

PasvAddr::PasvAddr(const std::vector<std::string>& toks) :
  addr(toks[0])
{
}

Ports::Ports(const std::vector<std::string>& toks)   
{
  std::vector<std::string> temp;
  for (const auto& token : toks)
  {
    temp.clear();
    util::Split(temp, token, "-");
    if (temp.size() > 2) throw cfg::ConfigError("Invalid port range.");
    int from = boost::lexical_cast<int>(temp[0]);
    int to = from;
    if (temp.size() > 1) to = boost::lexical_cast<int>(temp[1]);
    if (to < from)
      throw cfg::ConfigError("To port lower than from port in port range.");
    if (to < 1024 || from < 1024 || to > 65535 || from > 65535)
      throw cfg::ConfigError("Invalid to port number in port range.");
    ranges.emplace_back(from, to);
  }
}

AllowFxp::AllowFxp(std::vector<std::string> toks)   
{
  downloads = util::BoolLexicalCast(toks[0]);
  uploads   = util::BoolLexicalCast(toks[1]);
  logging   = util::BoolLexicalCast(toks[2]);
  if (toks.size() > 3)
  {
    toks.erase(toks.begin(), toks.begin() + 3);
    acl = acl::ACL(util::Join(toks, " "));
  }
  else
  {
    acl = acl::ACL("*");
  }
}

Alias::Alias(const std::vector<std::string>& toks)   
{
  name = util::ToLowerCopy(toks[0]);
  path = toks[1];
}

Right::Right(std::vector<std::string> toks)
{
  path = toks[0];
  toks.erase(toks.begin());
  acl = acl::ACL(util::Join(toks, " "));
  specialVar = path.find("[:username:]") != std::string::npos ||
               path.find("[:groupname:]") != std::string::npos;
}

PathFilter::~PathFilter()
{
}

PathFilter& PathFilter::operator=(const PathFilter& rhs)
{
  messagePath = rhs.messagePath;
  regex.reset(new boost::regex(*rhs.regex));
  acl = rhs.acl;
  return *this;
}

PathFilter& PathFilter::operator=(PathFilter&& rhs)
{
  messagePath = std::move(rhs.messagePath);
  regex = std::move(rhs.regex);
  acl = std::move(rhs.acl);
  return *this;
}

PathFilter::PathFilter(const PathFilter& other) :
  messagePath(other.messagePath),
  regex(new boost::regex(*other.regex)),
  acl(other.acl)
{
}

PathFilter::PathFilter(PathFilter&& other) :
  messagePath(other.messagePath),
  regex(std::move(other.regex)),
  acl(std::move(other.acl))
{
}

PathFilter::PathFilter(std::vector<std::string> toks)   
{
  messagePath = toks[0];
  try
  {
    regex.reset(new boost::regex(toks[1]));
  }
  catch (const boost::regex_error&)
  {
    throw ConfigError("Invalid regular expression.");
  }
  toks.erase(toks.begin(), toks.begin() + 2);
  acl = acl::ACL(util::Join(toks, " "));
}

const boost::regex& PathFilter::Regex() const { return *regex; }

MaxUsers::MaxUsers(const std::vector<std::string>& toks)   
{
  users = boost::lexical_cast<int>(toks[0]);
  exemptUsers = boost::lexical_cast<int>(toks[1]);
}

ACLInt::ACLInt(std::vector<std::string> toks)   
{
  arg = boost::lexical_cast<int>(toks[0]);
  if (toks.size() > 1)
  {
    toks.erase(toks.begin());
    acl = acl::ACL(util::Join(toks, " ")); 
  }
  else
  {
    acl = acl::ACL("*");
  }
}

Lslong::Lslong(std::vector<std::string> toks)   
{
  options = toks[0];
  if (options[0] == '-') options.erase(0, 1);
  if (toks.size() == 1) return;
  
  maxRecursion = boost::lexical_cast<int>(toks[1]);
  if (maxRecursion < 0) throw boost::bad_lexical_cast();
}

HiddenFiles::HiddenFiles(std::vector<std::string> toks)   
{
  path = toks[0];
  toks.erase(toks.begin());
  masks = toks;
}

Requests::Requests(const std::vector<std::string>& toks)   
{
  path = toks[0];
  max = boost::lexical_cast<int>(toks[1]);
}

Lastonline::Lastonline(const std::vector<std::string>& toks) :
  max(10)
{
  int i = boost::lexical_cast<int>(toks[0]);
  switch (i) 
  {
    case 0:
      type = ALL;
      break;
    case 1:
      type = TIMEOUT;
      break;
    case 2:
      type = ALL_WITH_ACTIVITY;
      break;
    default:
      throw cfg::ConfigError("Invalid lastonline parameter");
      break;
  }
  
  if (toks.size() > 1)
    max = boost::lexical_cast<int>(toks[1]);
}

Creditcheck::Creditcheck(std::vector<std::string> toks)   
{
  path = toks[0];
  ratio = boost::lexical_cast<int>(toks[1]);
  if (ratio < 0) throw ConfigError("creditloss ratio must be 0 or larger");
  toks.erase(toks.begin(), toks.begin()+2);
  acl = acl::ACL(util::Join(toks, " "));
}

Creditloss::Creditloss(std::vector<std::string> toks)   
{
  ratio = boost::lexical_cast<int>(toks[0]);
  if (ratio < 0) throw ConfigError("creditloss ratio must be 0 or larger");
  allowLeechers = util::BoolLexicalCast(toks[1]);
  path = toks[2];
  toks.erase(toks.begin(), toks.begin()+3);
  acl = acl::ACL(util::Join(toks, " "));
}

NukedirStyle::NukedirStyle(const std::vector<std::string>& toks)   
{
  format = toks[0];
  int i = boost::lexical_cast<int>(toks[1]);
  switch (i)
  {
    case 0:
      type = DELETE_ALL;
      break;
    case 1:
      type = DELETE_FILES;
      break;
    case 2:
      type = KEEP;
      break;
    default:
      throw cfg::ConfigError("Invalid nukedir_style parameter");
      break;
  }
  minBytes = boost::lexical_cast<int>(toks[2]);
}

Msgpath::Msgpath(const std::vector<std::string>& toks)   
{
  path = toks[0];
  file = toks[1];
  acl = acl::ACL(util::Join(toks, " "));
}

Privpath::Privpath(std::vector<std::string> toks)   
{
  path = toks[0];
  toks.erase(toks.begin());
  acl = acl::ACL(util::Join(toks, " "));
}

SiteCmd::SiteCmd(const std::vector<std::string>& toks)   
{
  command = util::ToUpperCopy(toks[0]);
  description = toks[1];
  std::string typeStr(util::ToUpperCopy(toks[2]));
  if (typeStr == "EXEC") type = Type::EXEC;
  else if (typeStr == "TEXT") type = Type::TEXT;
  else if (typeStr == "ALIAS")
  {
    type = Type::ALIAS;
    util::ToUpper(target);
  }
  else
    throw cfg::ConfigError("Invalid site_cmd parameter");
  target = toks[3];
  if (toks.size() == 5) arguments = toks[4];
}

Cscript::Cscript(const std::vector<std::string>& toks)   
{
  command = util::ToUpperCopy(toks[0]);
  std::string when = util::ToLowerCopy(toks[1]);
  if (when == "pre") type = Type::PRE;
  else if (when == "post") type = Type::POST;
  else throw cfg::ConfigError("Invalid cscript parameter");
  path = toks[2];
}

struct IdleTimeoutImpl
{
  boost::posix_time::seconds maximum;
  boost::posix_time::seconds minimum;
  boost::posix_time::seconds timeout;

  static const std::unique_ptr<IdleTimeoutImpl> defaults;

  IdleTimeoutImpl() :
    maximum(defaults->maximum), minimum(defaults->minimum),
    timeout(defaults->timeout)
  { }

  IdleTimeoutImpl(const boost::posix_time::seconds& maximum,
                  const boost::posix_time::seconds& minimum,
                  const boost::posix_time::seconds& timeout) :
    maximum(maximum), minimum(minimum),
    timeout(timeout)
  { }

  IdleTimeoutImpl(const std::vector<std::string>& toks) :
    maximum(defaults->maximum),
    minimum(defaults->minimum),
    timeout(defaults->timeout)
  {
    namespace pt = boost::posix_time;
    timeout = pt::seconds(boost::lexical_cast<long>(toks[0]));
    minimum = pt::seconds(boost::lexical_cast<long>(toks[1]));
    maximum = pt::seconds(boost::lexical_cast<long>(toks[2]));
    
    if (timeout.total_seconds() < 1 || minimum.total_seconds() < 1 || maximum.total_seconds() < 1)
      throw ConfigError("Times in idle_timeout must be larger than zero");
    if (minimum >= maximum)
      throw ConfigError("Mnimum must be smaller than maximum in idle_timeout");
    if (timeout < minimum || timeout > maximum)
      throw ConfigError("Default timeout must be larger than or equal to minimum "
                        "and smaller than or equal to maximum in idle_timeout");
  }
};

const std::unique_ptr<IdleTimeoutImpl> IdleTimeoutImpl::defaults(new IdleTimeoutImpl(
  boost::posix_time::seconds(72000),
  boost::posix_time::seconds(1),
  boost::posix_time::seconds(900)
));

IdleTimeout::IdleTimeout() :
  pimpl(new IdleTimeoutImpl())
{
}

IdleTimeout::IdleTimeout(const std::vector<std::string>& toks) :
  pimpl(new IdleTimeoutImpl(toks))
{
}

IdleTimeout::~IdleTimeout()
{
}

IdleTimeout& IdleTimeout::operator=(const IdleTimeout& rhs)
{
  pimpl.reset(new IdleTimeoutImpl(*rhs.pimpl));
  return *this;
}

IdleTimeout& IdleTimeout::operator=(IdleTimeout&& rhs)
{
  pimpl = std::move(rhs.pimpl);
  return *this;
}

IdleTimeout::IdleTimeout(const IdleTimeout& other) :
  pimpl(new IdleTimeoutImpl(*other.pimpl))
{
}

IdleTimeout::IdleTimeout(IdleTimeout&& other) :
  pimpl(std::move(other.pimpl))
{
}

boost::posix_time::seconds IdleTimeout::Maximum() const { return pimpl->maximum; }
boost::posix_time::seconds IdleTimeout::Minimum() const { return pimpl->minimum; }
boost::posix_time::seconds IdleTimeout::Timeout() const { return pimpl->timeout; }

CheckScript::CheckScript(const std::vector<std::string>& toks) :
  path(toks[0]), mask(toks.size() == 2 ? toks[1] : "*"), disabled(toks[0] == "none")
{
}

Log::Log(const std::string& name, const std::vector<std::string>& toks) :
  name(name),
  console(util::BoolLexicalCast(toks[0])),
  file(util::BoolLexicalCast(toks[1])),
  database(toks.size() == 3 && boost::lexical_cast<long>(toks[2]))
{
  if (database < 0) throw boost::bad_lexical_cast();
}

// end namespace
}
}

