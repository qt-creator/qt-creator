/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#define cmListFileCache_cxx
#include "cmListFileCache.h"

#include <cmakelang/cmakeast.h>
#include <cmakelang/cmakeastvisitor.h>
#include <cmakelang/cmakeengine.h>
#include <cmakelang/cmakeparser.h>

#include <QByteArray>
#include <QString>

#include <sstream>
#include <utility>

namespace {

std::string toStdString(QStringView text)
{
    // Almost every token of a CMake file is plain ASCII, where the UTF-8 form
    // is the same length and needs neither a QByteArray nor a conversion.
    // Narrowing and collecting the high bits in one branch-free loop lets it
    // vectorize; only the rare token that is not ASCII is converted twice.
    std::string result(size_t(text.size()), '\0');
    char *out = result.data();
    char16_t bits = 0;
    for (const QChar c : text) {
        bits |= c.unicode();
        *out++ = char(c.unicode());
    }
    if (bits & 0xff80) {
        const QByteArray utf8 = text.toUtf8();
        return std::string(utf8.constData(), size_t(utf8.size()));
    }
    return result;
}

std::string tokenText(const CMakeLang::Token &token)
{
    return token.value ? toStdString(*token.value) : toStdString(token.spelling);
}

class FunctionCollector : public CMakeLang::Visitor
{
public:
    FunctionCollector(cmListFile *listFile, std::string &error)
        : m_listFile(listFile)
        , m_error(error)
    {}

    bool ok() const { return m_ok; }

    bool visit(CMakeLang::CommandAST *ast) override
    {
        if (!m_ok)
            return false;

        std::vector<cmListFileArgument> arguments;
        if (!addArguments(ast->arguments, arguments)) {
            m_ok = false;
            return false;
        }

        m_listFile->Functions.emplace_back(tokenText(ast->name),
                                           ast->name.line,
                                           ast->rightParen.line,
                                           std::move(arguments));
        return false;
    }

private:
    // A paren group nests as deeply as the input says, so what is left of the
    // enclosing lists is kept on a worklist rather than on the call stack.
    bool addArguments(CMakeLang::List<CMakeLang::ArgumentAST *> *list,
                      std::vector<cmListFileArgument> &out)
    {
        struct Pending
        {
            CMakeLang::List<CMakeLang::ArgumentAST *> *rest;
            const CMakeLang::Token *rightParen;
        };
        std::vector<Pending> pending;

        for (;;) {
            while (list) {
                CMakeLang::ArgumentAST *argument = list->value;
                list = list->next;

                if (CMakeLang::ParenGroupArgumentAST *group = argument->asParenGroupArgument()) {
                    out.emplace_back("(",
                                     cmListFileArgument::Unquoted,
                                     group->leftParen.line,
                                     group->leftParen.column);
                    pending.push_back({list, &group->rightParen});
                    list = group->arguments;
                    continue;
                }

                cmListFileArgument::Delimiter delimiter = cmListFileArgument::Unquoted;
                if (argument->asQuotedArgument())
                    delimiter = cmListFileArgument::Quoted;
                else if (argument->asBracketArgument())
                    delimiter = cmListFileArgument::Bracket;

                if (!checkSeparation(argument->token, delimiter))
                    return false;

                out.emplace_back(tokenText(argument->token),
                                 delimiter,
                                 argument->token.line,
                                 argument->token.column);
            }

            if (pending.empty())
                return true;

            const Pending &group = pending.back();
            out.emplace_back(")",
                             cmListFileArgument::Unquoted,
                             group.rightParen->line,
                             group.rightParen->column);
            list = group.rest;
            pending.pop_back();
        }
    }

    bool checkSeparation(const CMakeLang::Token &token, cmListFileArgument::Delimiter delimiter)
    {
        if (token.separation == CMakeLang::Token::SeparationOkay)
            return true;

        const bool isError = token.separation == CMakeLang::Token::SeparationError
                             || delimiter == cmListFileArgument::Bracket;
        if (!isError)
            return true;

        std::ostringstream m;
        m << "Syntax Error in cmake code at "
          << "column " << token.column << "\n"
          << "Argument not separated from preceding token by whitespace.";
        m_error += m.str();
        m_error += "\n";
        return false;
    }

    cmListFile *m_listFile;
    std::string &m_error;
    bool m_ok = true;
};

} // namespace

bool cmListFile::ParseString(const std::string &str,
                             const std::string & /*virtual_filename*/,
                             std::string &error)
{
    const QString source = QString::fromStdString(str);

    CMakeLang::Engine engine;
    CMakeLang::Parser parser(&engine, source);

    // The parser recovers from a syntax error at the next line, so the
    // commands around a broken one are collected either way and only the
    // return value tells that the file did not parse cleanly.
    FunctionCollector collector(this, error);
    collector.accept(parser.parse());

    bool ok = collector.ok();
    for (const CMakeLang::Diagnostic &diagnostic : engine.diagnostics()) {
        if (!diagnostic.isError())
            continue;
        error += diagnostic.message.toStdString();
        error += "\n";
        ok = false;
    }
    return ok;
}
