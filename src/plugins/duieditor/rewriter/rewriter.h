#ifndef REWRITER_H
#define REWRITER_H

#include <QList>
#include <QString>

#include "textwriter.h"
#include "parser/javascriptastvisitor_p.h"

namespace DuiEditor {
namespace Internal {

////////////////////////////////////////////////////////////////////////////////
// Replacement
////////////////////////////////////////////////////////////////////////////////
class Replacement
{
    int _offset;
    int _length;
    QString _text;

public:
    Replacement(int offset = 0, int length = 0, const QString &text = QString())
        : _offset(offset), _length(length), _text(text)
    { }

    bool isNull() const { return _offset == _length; }
    operator bool() const { return ! isNull(); }

    int offset() const { return _offset; }
    int length() const { return _length; }
    QString text() const { return _text; }
};



////////////////////////////////////////////////////////////////////////////////
// Rewriter
////////////////////////////////////////////////////////////////////////////////
class Rewriter: public JavaScript::AST::Visitor
{
protected:
    TextWriter textWriter;
public:
    //
    // Token based API
    //

    /// Returns the text of the token at the given \a location.
    QString textAt(const JavaScript::AST::SourceLocation &location) const;

    QString textAt(const JavaScript::AST::SourceLocation &firstLoc,
                   const JavaScript::AST::SourceLocation &lastLoc) const;

    /// Replace the token at \a loc with the given \a text.
    void replace(const JavaScript::AST::SourceLocation &loc, const QString &text);

    /// Remove the token at the given \a location.
    void remove(const JavaScript::AST::SourceLocation &location);

    /// Remove all tokens in the range [\a firstLoc, \a lastLoc].
    void remove(const JavaScript::AST::SourceLocation &firstLoc, const JavaScript::AST::SourceLocation &lastLoc);

    /// Insert \a text before the token at the given \a location.
    void insertTextBefore(const JavaScript::AST::SourceLocation &location, const QString &text);

    /// Insert \a text after the token at the given \a location.
    void insertTextAfter(const JavaScript::AST::SourceLocation &loc, const QString &text);

    void moveTextBefore(const JavaScript::AST::SourceLocation &firstLoc,
                        const JavaScript::AST::SourceLocation &lastLoc,
                        const JavaScript::AST::SourceLocation &loc);

    void moveTextAfter(const JavaScript::AST::SourceLocation &firstLoc,
                       const JavaScript::AST::SourceLocation &lastLoc,
                       const JavaScript::AST::SourceLocation &loc);

    //
    // low-level offset based API
    //
    void replace(int offset, int length, const QString &text);
    void insertText(int offset, const QString &text);
    void removeText(int offset, int length);

    /// Visit the given \a node.
    void accept(JavaScript::AST::Node *node);

    /// Returns the original unchanged source code.
    QString code() const { return _code; }

    /// Returns the list of replacements.
    QList<Replacement> replacementList() const { return _replacementList; }

protected:
    /// \internal
    void setCode(const QString &code) { _code = code; }

private:
    QString _code;
    QList<Replacement> _replacementList;
};

} // end of namespace Internal
} // end of namespace DuiEditor

#endif // REWRITER_H
