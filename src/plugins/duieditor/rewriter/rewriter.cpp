#include "rewriter.h"
#include "parser/javascriptast_p.h"

using namespace JavaScript;

void Rewriter::replace(const AST::SourceLocation &loc, const QString &text)
{ replace(loc.offset, loc.length, text); }

void Rewriter::remove(const AST::SourceLocation &loc)
{ return replace(loc.offset, loc.length, QString()); }

void Rewriter::remove(const AST::SourceLocation &firstLoc, const AST::SourceLocation &lastLoc)
{ return replace(firstLoc.offset, lastLoc.offset + lastLoc.length - firstLoc.offset, QString()); }

void Rewriter::insertTextBefore(const AST::SourceLocation &loc, const QString &text)
{ replace(loc.offset, 0, text); }

void Rewriter::insertTextAfter(const AST::SourceLocation &loc, const QString &text)
{ replace(loc.offset + loc.length, 0, text); }

void Rewriter::replace(int offset, int length, const QString &text)
{ textWriter.replace(offset, length, text); }

void Rewriter::insertText(int offset, const QString &text)
{ replace(offset, 0, text); }

void Rewriter::removeText(int offset, int length)
{ replace(offset, length, QString()); }

QString Rewriter::textAt(const AST::SourceLocation &loc) const
{ return _code.mid(loc.offset, loc.length); }

QString Rewriter::textAt(const AST::SourceLocation &firstLoc, const AST::SourceLocation &lastLoc) const
{ return _code.mid(firstLoc.offset, lastLoc.offset + lastLoc.length - firstLoc.offset); }

void Rewriter::accept(JavaScript::AST::Node *node)
{ JavaScript::AST::Node::acceptChild(node, this); }

void Rewriter::moveTextBefore(const AST::SourceLocation &firstLoc,
                              const AST::SourceLocation &lastLoc,
                              const AST::SourceLocation &loc)
{
    textWriter.move(firstLoc.offset, lastLoc.offset + lastLoc.length - firstLoc.offset, loc.offset);
}

void Rewriter::moveTextAfter(const AST::SourceLocation &firstLoc,
                             const AST::SourceLocation &lastLoc,
                             const AST::SourceLocation &loc)
{
    textWriter.move(firstLoc.offset, lastLoc.offset + lastLoc.length - firstLoc.offset, loc.offset + loc.length);
}

