#include <boost/algorithm/string/predicate.hpp>
#include "acl/path.hpp"
#include "fs/owner.hpp"
#include "cfg/get.hpp"
#include "util/string.hpp"
#include "acl/user.hpp"
#include "logs/logs.hpp"
#include "util/path/status.hpp"
#include "acl/group.hpp"

namespace acl { namespace path
{

namespace
{

bool HiddenFile(const fs::VirtualPath& path)
{
  std::string dirname(path.Dirname().ToString());
  if (dirname[dirname.length() - 1] != '/') dirname += '/';
  std::string basename(path.Basename().ToString());

  for (auto& hf : cfg::Get().HiddenFiles())
  {
    if (util::string::WildcardMatch(hf.Path(), dirname))
    {
      for (auto& mask : hf.Masks())
      {
        if (util::string::WildcardMatch(mask, basename)) return true;
      }
    }
  }
  return false;
}

bool Evaluate(const std::vector<cfg::setting::Right>& rights, 
              const User& user, const fs::VirtualPath& path)
{
  std::string group;
  bool firstSpecial = true;
  
  for (const auto& right : rights)
  {
    if (right.SpecialVar())
    {
      std::string specialPath(right.Path());
      boost::replace_all(specialPath, "[:username:]", user.Name());
      if (firstSpecial)
      {
        if (user.PrimaryGID() != -1)
        {
          group = acl::GIDToName(user.PrimaryGID());
        }
        firstSpecial = false;
      }
      
      if (!group.empty())
        boost::replace_all(specialPath, "[:groupname:]", group);
        
      if (util::string::WildcardMatch(specialPath, path.ToString()))
        return right.ACL().Evaluate(user);
    }
    else
      if (util::string::WildcardMatch(right.Path(), path.ToString()))
        return right.ACL().Evaluate(user);
  }
  return false;
}

template <Type type>
struct Traits;

template <>
struct Traits<Upload>
{
  static util::Error Allowed(const User& user, const fs::VirtualPath& path)
  {
    if (Evaluate(cfg::Get().Upload(), user, path))
      return util::Error::Success();
    else
      return util::Error::Failure(EACCES);
  }
};

template <>
struct Traits<Resume>
{
  static util::Error Allowed(const User& user, const fs::VirtualPath& path)
  {
    if (Evaluate(cfg::Get().Resume(), user, path))
      return util::Error::Success();
    else
      return util::Error::Failure(EACCES);
  }
};

template <>
struct Traits<Overwrite>
{
  static util::Error Allowed(const User& user, const fs::VirtualPath& path)
  {
    if (Evaluate(cfg::Get().Overwrite(), user, path))
      return util::Error::Success();
    else
      return util::Error::Failure(EACCES);
  }
};

template <>
struct Traits<Makedir>
{
  static util::Error Allowed(const User& user, const fs::VirtualPath& path)
  {
    if (Evaluate(cfg::Get().Makedir(), user, path))
      return util::Error::Success();
    else
      return util::Error::Failure(EACCES);
  }
};

template <>
struct Traits<Download>
{
private:
  static util::Error CheckNoretrieve(const fs::VirtualPath& path)
  {
    const cfg::Config& config = cfg::Get();
    for (auto& mask : config.Noretrieve())
    {
      if (util::string::WildcardMatch(mask, path.Basename().ToString()))
        return util::Error::Failure(EACCES);
    }
    return util::Error::Success();
  }
  
public:
  static util::Error Allowed(const User& user, const fs::VirtualPath& path)
  {
    if (Evaluate(cfg::Get().Download(), user, path))
      return CheckNoretrieve(path);
    else
      return util::Error::Failure(EACCES);
  }
};

template <>
struct Traits<Rename>
{
  static util::Error AllowedOwner(const User& user, const fs::VirtualPath& path)
  {
    if (Evaluate(cfg::Get().Renameown(), user, path))
      return util::Error::Success();
    else
      return util::Error::Failure(EACCES);
  }
  
  static util::Error AllowedOther(const User& user, const fs::VirtualPath& path)
  {
    if (Evaluate(cfg::Get().Rename(), user, path))
      return util::Error::Success();
    else
      return util::Error::Failure(EACCES);
  }
  
  static util::Error Allowed(const User& user, const fs::VirtualPath& path)
  {
    fs::Owner owner = GetOwner(fs::MakeReal(path));
    if (owner.UID() == user.ID() && AllowedOwner(user, path))
      return util::Error::Success();
    else
      return AllowedOther(user, path);
  }
};

template <>
struct Traits<Filemove>
{
  static util::Error Allowed(const User& user, const fs::VirtualPath& path)
  {
    if (Evaluate(cfg::Get().Filemove(), user, path))
      return util::Error::Success();
    else
      return util::Error::Failure(EACCES);
  }
};

template <>
struct Traits<Nuke>
{
  static util::Error Allowed(const User& user, const fs::VirtualPath& path)
  {
    if (Evaluate(cfg::Get().Nuke(), user, path))
      return util::Error::Success();
    else
      return util::Error::Failure(EACCES);
  }
};

template <>
struct Traits<Delete>
{
  static util::Error AllowedOwner(const User& user, const fs::VirtualPath& path)
  {
    if (Evaluate(cfg::Get().Deleteown(), user, path))
      return util::Error::Success();
    else
      return util::Error::Failure(EACCES);
  }

  static util::Error AllowedOther(const User& user, const fs::VirtualPath& path)
  {
    if (Evaluate(cfg::Get().Delete(), user, path))
      return util::Error::Success();
    else
      return util::Error::Failure(EACCES);
  }

  static util::Error Allowed(const User& user, const fs::VirtualPath& path)
  {
    fs::Owner owner = GetOwner(fs::MakeReal(path));
    if (owner.UID() == user.ID() && AllowedOwner(user, path))
      return util::Error::Success();
    else
      return AllowedOther(user, path);
  }
};

template <>
struct Traits<View>
{
  static util::Error Allowed(const User&, const fs::VirtualPath& path)
  {
    if (HiddenFile(path)) return util::Error::Failure(ENOENT);
    else return util::Error::Success();
  }
};

template <>
struct Traits<Hideinwho>
{
  static util::Error Allowed(const User& user, const fs::VirtualPath& path)
  {
    if (Evaluate(cfg::Get().Hideinwho(), user, path))
      return util::Error::Success();
    else
      return util::Error::Failure(EACCES);
  }
};

template <>
struct Traits<Freefile>
{
  static util::Error Allowed(const User& user, const fs::VirtualPath& path)
  {
    if (Evaluate(cfg::Get().Freefile(), user, path))
      return util::Error::Success();
    else
      return util::Error::Failure(EACCES);
  }
};

template <>
struct Traits<Nostats>
{
  static util::Error Allowed(const User& user, const fs::VirtualPath& path)
  {
    if (Evaluate(cfg::Get().Nostats(), user, path))
      return util::Error::Success();
    else
      return util::Error::Failure(EACCES);
  }
};

template <>
struct Traits<Hideowner>
{
  static util::Error Allowed(const User& user, const fs::VirtualPath& path)
  {
    if (Evaluate(cfg::Get().Hideowner(), user, path))
      return util::Error::Success();
   else
      return util::Error::Failure(EACCES);
  }
};

}

bool PrivatePath(const fs::VirtualPath& path, const User& user)
{
  for (const auto& pp : cfg::Get().Privpath())
  {
    if (!path.ToString().compare(0, pp.Path().length(), pp.Path()))
      return !pp.ACL().Evaluate(user);
  }
  return false;
}

template <Type type>
util::Error InnerAllowed(const User& user, const fs::VirtualPath& path)
{ 
  if (PrivatePath(path, user)) return util::Error::Failure(ENOENT);
  return Traits<type>::Allowed(user, path);
}

template <Type type>
util::Error FileAllowed(const User& user, const fs::VirtualPath& path)
{  
  return InnerAllowed<type>(user, path);
}

template util::Error FileAllowed<Upload>(const User& user, const fs::VirtualPath& path);
template util::Error FileAllowed<Resume>(const User& user, const fs::VirtualPath& path);
template util::Error FileAllowed<Overwrite>(const User& user, const fs::VirtualPath& path);
template util::Error FileAllowed<Download>(const User& user, const fs::VirtualPath& path);
template util::Error FileAllowed<Rename>(const User& user, const fs::VirtualPath& path);
template util::Error FileAllowed<Filemove>(const User& user, const fs::VirtualPath& path);
template util::Error FileAllowed<Delete>(const User& user, const fs::VirtualPath& path);
template util::Error FileAllowed<View>(const User& user, const fs::VirtualPath& path);
template util::Error FileAllowed<Hideinwho>(const User& user, const fs::VirtualPath& path);
template util::Error FileAllowed<Freefile>(const User& user, const fs::VirtualPath& path);
template util::Error FileAllowed<Nostats>(const User& user, const fs::VirtualPath& path);
template util::Error FileAllowed<Hideowner>(const User& user, const fs::VirtualPath& path);

template <Type type>
util::Error DirAllowed(const User& user, const fs::VirtualPath& path)
{
  if (path.IsEmpty()) return util::Error::Failure(EINVAL);
  if (path.ToString()[path.Length() - 1] != '/') 
    return InnerAllowed<type>(user, fs::VirtualPath(path.ToString() + '/'));
  else
    return InnerAllowed<type>(user, path);
}

template util::Error DirAllowed<Makedir>(const User& user, const fs::VirtualPath& path);
template util::Error DirAllowed<Rename>(const User& user, const fs::VirtualPath& path);
template util::Error DirAllowed<Nuke>(const User& user, const fs::VirtualPath& path);
template util::Error DirAllowed<Delete>(const User& user, const fs::VirtualPath& path);
template util::Error DirAllowed<View>(const User& user, const fs::VirtualPath& path);
template util::Error DirAllowed<Hideinwho>(const User& user, const fs::VirtualPath& path);
template util::Error DirAllowed<Hideowner>(const User& user, const fs::VirtualPath& path);

template <Type type>
util::Error Allowed(const User& user, const fs::VirtualPath& path)
{
  if (util::path::IsDirectory(fs::MakeReal(path).ToString()))
    return DirAllowed<type>(user, path);
  else
    return FileAllowed<type>(user, path);
}

template util::Error Allowed<View>(const User& user, const fs::VirtualPath& path);

util::Error Filter(const User& user, const fs::Path& basename, 
    fs::Path& messagePath)
{
  for (auto& filter : cfg::Get().PathFilter())
  {
    if (filter.ACL().Evaluate(user))
    {
      if (!boost::regex_match(basename.ToString(), filter.Regex()))
      {
        messagePath = fs::Path(filter.MessagePath());
        return util::Error::Failure(EACCES);
      }
      break;
    }
  }
  return util::Error::Success();
}

} /* path namespace */
} /* acl namespace */
