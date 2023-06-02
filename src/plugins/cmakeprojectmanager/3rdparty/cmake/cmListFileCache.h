/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#pragma once

#include <algorithm>
#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>

/** \class cmListFileCache
 * \brief A class to cache list file contents.
 *
 * cmListFileCache is a class used to cache the contents of parsed
 * cmake list files.
 */

struct cmListFileArgument
{
  enum Delimiter
  {
    Unquoted,
    Quoted,
    Bracket
  };
  cmListFileArgument() = default;
  cmListFileArgument(std::string v, Delimiter d, long line, long column)
    : Value(std::move(v))
    , Delim(d)
    , Line(line)
    , Column(column)
  {
  }
  bool operator==(const cmListFileArgument& r) const
  {
    return (this->Value == r.Value) && (this->Delim == r.Delim);
  }
  bool operator!=(const cmListFileArgument& r) const { return !(*this == r); }
  std::string Value;
  Delimiter Delim = Unquoted;
  long Line = 0;
  long Column = 0;
};

class cmListFileFunction
{
public:
  cmListFileFunction(std::string name, long line, long lineEnd,
                     std::vector<cmListFileArgument> args)
    : Impl{ std::make_shared<Implementation>(std::move(name), line, lineEnd,
                                             std::move(args)) }
  {
  }

  std::string const& OriginalName() const noexcept
  {
    return this->Impl->OriginalName;
  }

  std::string const& LowerCaseName() const noexcept
  {
    return this->Impl->LowerCaseName;
  }

  long Line() const noexcept { return this->Impl->Line; }
  long LineEnd() const noexcept { return this->Impl->LineEnd; }

  std::vector<cmListFileArgument> const& Arguments() const noexcept
  {
    return this->Impl->Arguments;
  }

private:
  struct Implementation
  {
    Implementation(std::string name, long line, long lineEnd,
                   std::vector<cmListFileArgument> args)
      : OriginalName{ std::move(name) }
      , LowerCaseName{ tolower(this->OriginalName) }
      , Line{ line }
      , LineEnd{ lineEnd }
      , Arguments{ std::move(args) }
    {
    }

    // taken from yaml-cpp
    static bool IsLower(char ch) { return 'a' <= ch && ch <= 'z'; }
    static bool IsUpper(char ch) { return 'A' <= ch && ch <= 'Z'; }
    static char ToLower(char ch) { return IsUpper(ch) ? ch + 'a' - 'A' : ch; }

    std::string tolower(const std::string& str)
    {
        std::string s(str);
        std::transform(s.begin(), s.end(), s.begin(), ToLower);
        return s;
    }

    std::string OriginalName;
    std::string LowerCaseName;
    long Line = 0;
    long LineEnd = 0;
    std::vector<cmListFileArgument> Arguments;
  };

  std::shared_ptr<Implementation const> Impl;
};

struct cmListFile
{
  bool ParseString(const std::string &str, const std::string &virtual_filename, std::string &error);

  std::vector<cmListFileFunction> Functions;
};
