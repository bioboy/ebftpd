#ifndef __UTIL_MISC_HPP
#define __UTIL_MISC_HPP

#include <string>
#include <cstdlib>
#include <sstream>
#include <vector>
#include <fstream>
#include <boost/regex.hpp>

namespace util
{

template<class Collection>
void SplitToType(const std::string& s, char delim, Collection& result)
{
  std::istringstream is(s);
  typename Collection::value_type value;
  while (is >> value)
  {
    result.emplace_back(value);
    if (is.peek() == delim) is.ignore();
  }
}

inline bool ReadLinesToVector(std::ifstream& fin, std::vector<std::string>& lines)
{
  std::string line;
  while (std::getline(fin, line)) lines.emplace_back(line);
  return !fin.fail();
}

inline bool ReadLinesToVector(const std::string& path, std::vector<std::string>& lines)
{
  std::ifstream fin(path.c_str());
  if (!fin) return false;
  return ReadLinesToVector(fin, lines);
}

inline bool ReadFileToString(std::ifstream& fin, std::string& buffer)
{
  buffer.assign(std::istreambuf_iterator<char>(fin),
                std::istreambuf_iterator<char>());
  return !fin.fail();
}

inline bool ReadFileToString(const std::string& path, std::string& buffer)
{
  std::ifstream fin(path.c_str());
  if (!fin) return false;
  return ReadFileToString(fin, buffer);
}

inline std::string EscapeRegex(const std::string& term)
{
  boost::regex esc("[\\^\\.\\$\\|\\(\\)\\[\\]\\*\\+\\?\\/\\\\]");
  std::string rep("\\\\\\1");
  return boost::regex_replace(term, esc, rep, boost::match_default | boost::format_sed);
}
                         

inline std::string WildcardToRegex(const std::string& wc)
{
  const std::string escape = "+()^$.{}[]|\\";
  std::string re;
  for (char ch : wc)
  {
    if (ch == '*') re += ".*";
    else if (ch == '?') re += '.';
    else if (escape.find(ch) != std::string::npos)
    {
      re += '\\';
      re += ch;
    }
    else
      re += ch;
  }
  return re;
}

void SetProcessTitle(const std::string& title);

} /* util namespace */

#endif
