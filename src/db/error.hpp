#ifndef __DB_ERROR_HPP
#define __DB_ERROR_HPP

#include <mongo/client/dbclient.h>
#include <sstream>
#include <typeinfo>
#include "logs/logs.hpp"
#include "util/error.hpp"
#include "util/debug.hpp"

namespace db
{

struct LastError : public mongo::BSONObj
{
  explicit LastError(const mongo::BSONObj& errObj) : mongo::BSONObj(errObj) { }
  bool Okay() const { return (*this)["ok"].Number() == 1.0; }
  int NumChanged() const { return (*this)["n"].Int(); }
};

struct DBError : public util::RuntimeError
{
  DBError() : std::runtime_error("Unknown database error") { }
  DBError(const std::string& message) : std::runtime_error(message) { }
};

struct DBWriteError : public DBError
{
  DBWriteError() : std::runtime_error("Error while writing to database") { }
  DBWriteError(const std::string& message) : std::runtime_error(message) { }
};

struct DBReadError : public DBError
{
  DBReadError() : std::runtime_error("Error while querying the database") { }
  DBReadError(const std::string& message) : std::runtime_error(message) { }
};

struct DBKeyError : public DBError
{
  DBKeyError() : std::runtime_error("Duplicate key error") { }
  DBKeyError(const std::string& message) : std::runtime_error(message) { }
};

inline void LogError(std::ostringstream& os, const std::string& errmsg)
{
  os << " : " << errmsg;
  logs::db << os.str() << logs::endl;
}

template <typename T, typename... Args>
void LogError(std::ostringstream& os, const std::string& errmsg, const T& arg, const Args&... args)
{
  os << " : " << arg;
  LogError(os, errmsg, args...);
}

template <typename... Args>
void LogError(const std::string& prefix, const std::string& errmsg, const Args&... args)
{
  std::ostringstream buf;
  buf << std::boolalpha << prefix << " failed";
  LogError(buf, errmsg, args...);
}

template <typename... Args>
void LogException(const std::string& prefix, const std::exception& e, const Args&... args)
{
  LogError(prefix, e.what(), args...);
}

template <typename... Args>
void LogLastError(const std::string& prefix, const mongo::BSONObj& obj, const Args&... args)
{
  LogError(prefix, obj["err"].String(), args...);
}

inline std::ostream& operator<<(std::ostream& os, const std::type_info& info)
{
  return (os << util::debug::Demangle(info.name()));
}

} /* db namespace */

#endif
