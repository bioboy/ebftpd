#ifndef __CMD_RFC_FACTORY_HPP
#define __CMD_RFC_FACTORY_HPP

#include <memory>
#include <unordered_map>
#include "cmd/command.hpp"
#include "ftp/client.hpp"
#include "ftp/replycodes.hpp"

namespace cmd { namespace rfc
{

template <class BaseT>
class CreatorBase
{
public:  
  virtual ~CreatorBase() { }
  
  virtual BaseT* Create(ftp::Client& client, const std::string& argStr, const Args& args) = 0;
};

template <class CommandT>
class Creator : public CreatorBase<cmd::Command>
{
public:
  CommandT* Create(ftp::Client& client, const std::string& argStr, const Args& args)
  {
    return new CommandT(client, argStr, args);
  }
};

class CommandDef
{
  int minimumArgs;
  int maximumArgs;
  ftp::ClientState requiredState;
  ftp::ReplyCode failCode;
  std::shared_ptr<CreatorBase<cmd::Command>> creator;
  std::string syntax;
  
public:
  CommandDef(int minimumArgs, int maximumArgs, 
             ftp::ClientState requiredState,
             ftp::ReplyCode failCode,
             const std::shared_ptr<CreatorBase<cmd::Command>>& creator,
             const std::string& syntax) :
    minimumArgs(minimumArgs),
    maximumArgs(maximumArgs),
    requiredState(requiredState),
    failCode(failCode),
    creator(creator),
    syntax(syntax)
  {
  }
  
  bool CheckArgs(const std::vector<std::string>& args) const
  {
    int argsSize = static_cast<int>(args.size()) - 1;
    return (argsSize >= minimumArgs &&
            (maximumArgs == -1 || argsSize <= maximumArgs));
  }

  ftp::ReplyCode FailCode() const { return failCode; }
  ftp::ClientState RequiredState() const { return requiredState; }
  const std::string& Syntax() const { return syntax; }
  
  cmd::CommandPtr Create(ftp::Client& client, const std::string& argStr,
                         const Args& args) const
  {
    if (!creator) return nullptr;
    return cmd::CommandPtr(creator->Create(client, argStr, args));
  }
};

typedef boost::optional<const CommandDef&> CommandDefOptRef;

class Factory
{
public:
  typedef std::unordered_map<std::string, CommandDef> CommandDefMap;

private:                            
  CommandDefMap defs;
   
  Factory();
  
  static std::unique_ptr<Factory> factory;
  
public:
  static CommandDefOptRef Lookup(const std::string& command);
  static const CommandDefMap& Commands() { return factory->defs; }
  static void Initialise() { factory.reset(new Factory()); }
};

} /* rfc namespace */
} /* cmd namespace */

#endif
