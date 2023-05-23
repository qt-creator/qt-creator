/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#define cmListFileCache_cxx
#include "cmListFileCache.h"

#include <memory>
#include <sstream>
#include <utility>

#include "cmListFileLexer.h"

struct cmListFileParser
{
  cmListFileParser(cmListFile* lf, std::string &error);
  ~cmListFileParser();
  cmListFileParser(const cmListFileParser&) = delete;
  cmListFileParser& operator=(const cmListFileParser&) = delete;
  void IssueError(std::string const& text) const;
  bool ParseFile(const char* filename);
  bool ParseString(const std::string &str, const std::string &virtual_filename);
  bool Parse();
  bool ParseFunction(const char* name, long line);
  bool AddArgument(cmListFileLexer_Token* token,
                   cmListFileArgument::Delimiter delim);
  cmListFile* ListFile;
  cmListFileLexer* Lexer;
  std::string FunctionName;
  long FunctionLine;
  long FunctionLineEnd;
  std::vector<cmListFileArgument> FunctionArguments;
  std::string &Error;
  enum
  {
    SeparationOkay,
    SeparationWarning,
    SeparationError
  } Separation;
};

cmListFileParser::cmListFileParser(cmListFile *lf, std::string &error)
    : ListFile(lf)
    , Lexer(cmListFileLexer_New())
    , Error(error)
{
}

cmListFileParser::~cmListFileParser()
{
  cmListFileLexer_Delete(this->Lexer);
}

void cmListFileParser::IssueError(const std::string& text) const
{
  Error += text;
  Error += "\n";
}

bool cmListFileParser::ParseString(const std::string &str,
                                   const std::string &/*virtual_filename*/)
{
  if (!cmListFileLexer_SetString(this->Lexer, str.c_str(), (int)str.size())) {
    this->IssueError("cmListFileCache: cannot allocate buffer.");
    return false;
  }

  return this->Parse();
}

bool cmListFileParser::Parse()
{
  // Use a simple recursive-descent parser to process the token
  // stream.
  bool haveNewline = true;
  while (cmListFileLexer_Token* token = cmListFileLexer_Scan(this->Lexer)) {
    if (token->type == cmListFileLexer_Token_Space) {
    } else if (token->type == cmListFileLexer_Token_Newline) {
      haveNewline = true;
    } else if (token->type == cmListFileLexer_Token_CommentBracket) {
      haveNewline = false;
    } else if (token->type == cmListFileLexer_Token_Identifier) {
      if (haveNewline) {
        haveNewline = false;
        if (this->ParseFunction(token->text, token->line)) {
          this->ListFile->Functions.emplace_back(
            std::move(this->FunctionName), this->FunctionLine,
            this->FunctionLineEnd, std::move(this->FunctionArguments));
        } else {
          return false;
        }
      } else {
        std::ostringstream error;
        error << "Parse error.  Expected a newline, got "
              << cmListFileLexer_GetTypeAsString(this->Lexer, token->type)
              << " with text \"" << token->text << "\".";
        this->IssueError(error.str());
        return false;
      }
    } else {
      std::ostringstream error;
      error << "Parse error.  Expected a command name, got "
            << cmListFileLexer_GetTypeAsString(this->Lexer, token->type)
            << " with text \"" << token->text << "\".";
      this->IssueError(error.str());
      return false;
    }
  }

  return true;
}

bool cmListFile::ParseString(const std::string &str, const std::string& virtual_filename, std::string &error)
{
  bool parseError = false;

  {
    cmListFileParser parser(this, error);
    parseError = !parser.ParseString(str, virtual_filename);
  }

  return !parseError;
}

bool cmListFileParser::ParseFunction(const char* name, long line)
{
  // Ininitialize a new function call.
  this->FunctionName = name;
  this->FunctionLine = line;

  // Command name has already been parsed.  Read the left paren.
  cmListFileLexer_Token* token;
  while ((token = cmListFileLexer_Scan(this->Lexer)) &&
         token->type == cmListFileLexer_Token_Space) {
  }
  if (!token) {
    std::ostringstream error;
    /* clang-format off */
    error << "Unexpected end of file.\n"
          << "Parse error.  Function missing opening \"(\".";
    /* clang-format on */
    this->IssueError(error.str());
    return false;
  }
  if (token->type != cmListFileLexer_Token_ParenLeft) {
    std::ostringstream error;
    error << "Parse error.  Expected \"(\", got "
          << cmListFileLexer_GetTypeAsString(this->Lexer, token->type)
          << " with text \"" << token->text << "\".";
    this->IssueError(error.str());
    return false;
  }

  // Arguments.
  unsigned long parenDepth = 0;
  this->Separation = SeparationOkay;
  while ((token = cmListFileLexer_Scan(this->Lexer))) {
    if (token->type == cmListFileLexer_Token_Space ||
        token->type == cmListFileLexer_Token_Newline) {
      this->Separation = SeparationOkay;
      continue;
    }
    if (token->type == cmListFileLexer_Token_ParenLeft) {
      parenDepth++;
      this->Separation = SeparationOkay;
      if (!this->AddArgument(token, cmListFileArgument::Unquoted)) {
        return false;
      }
    } else if (token->type == cmListFileLexer_Token_ParenRight) {
      if (parenDepth == 0) {
        this->FunctionLineEnd = token->line;
        return true;
      }
      parenDepth--;
      this->Separation = SeparationOkay;
      if (!this->AddArgument(token, cmListFileArgument::Unquoted)) {
        return false;
      }
      this->Separation = SeparationWarning;
    } else if (token->type == cmListFileLexer_Token_Identifier ||
               token->type == cmListFileLexer_Token_ArgumentUnquoted) {
      if (!this->AddArgument(token, cmListFileArgument::Unquoted)) {
        return false;
      }
      this->Separation = SeparationWarning;
    } else if (token->type == cmListFileLexer_Token_ArgumentQuoted) {
      if (!this->AddArgument(token, cmListFileArgument::Quoted)) {
        return false;
      }
      this->Separation = SeparationWarning;
    } else if (token->type == cmListFileLexer_Token_ArgumentBracket) {
      if (!this->AddArgument(token, cmListFileArgument::Bracket)) {
        return false;
      }
      this->Separation = SeparationError;
    } else if (token->type == cmListFileLexer_Token_CommentBracket) {
      this->Separation = SeparationError;
    } else {
      // Error.
      std::ostringstream error;
      error << "Parse error.  Function missing ending \")\".  "
            << "Instead found "
            << cmListFileLexer_GetTypeAsString(this->Lexer, token->type)
            << " with text \"" << token->text << "\".";
      this->IssueError(error.str());
      return false;
    }
  }

  std::ostringstream error;
  error << "Parse error.  Function missing ending \")\".  "
        << "End of file reached.";
  IssueError(error.str());
  return false;
}

bool cmListFileParser::AddArgument(cmListFileLexer_Token* token,
                                   cmListFileArgument::Delimiter delim)
{
  this->FunctionArguments.emplace_back(token->text, delim, token->line, token->column);
  if (this->Separation == SeparationOkay) {
    return true;
  }
  bool isError = (this->Separation == SeparationError ||
                  delim == cmListFileArgument::Bracket);
  std::ostringstream m;

  m << "Syntax " << (isError ? "Error" : "Warning") << " in cmake code at "
    << "column " << token->column << "\n"
    << "Argument not separated from preceding token by whitespace.";
  /* clang-format on */
  if (isError) {
    IssueError(m.str());
    return false;
  }
  return true;
}
