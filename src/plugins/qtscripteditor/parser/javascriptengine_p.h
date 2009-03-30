#ifndef JAVASCRIPTENGINE_P_H
#define JAVASCRIPTENGINE_P_H

#include "javascriptvalue.h"
#include <QString>
#include <QSet>

namespace JavaScript {

class Node;
class Lexer;
class NodePool;

namespace AST {

class Node;

} // end of namespace AST

namespace Ecma {

class RegExp
{
public:
    enum RegExpFlag {
        Global     = 0x01,
        IgnoreCase = 0x02,
        Multiline  = 0x04
    };

public:
  static int flagFromChar(const QChar &);
};

} // end of namespace Ecma

} // end of namespace JavaScript



class JavaScriptNameIdImpl
{
  QString _text;

public:
  JavaScriptNameIdImpl(const QChar *u, int s)
    : _text(u, s)
  { }

  const QString asString() const
  { return _text; }

  bool operator == (const JavaScriptNameIdImpl &other) const
  { return _text == other._text; }

  bool operator != (const JavaScriptNameIdImpl &other) const
  { return _text != other._text; }

  bool operator < (const JavaScriptNameIdImpl &other) const
  { return _text < other._text; }
};

inline uint qHash(const JavaScriptNameIdImpl &id)
{ return qHash(id.asString()); }

class JavaScriptEnginePrivate
{
  JavaScript::Lexer *_lexer;
  JavaScript::NodePool *_nodePool;
  JavaScript::AST::Node *_ast;
  QSet<JavaScriptNameIdImpl> _literals;

public:
  JavaScriptEnginePrivate()
    : _lexer(0), _nodePool(0), _ast(0)
  { }

  JavaScriptNameIdImpl *JavaScriptEnginePrivate::intern(const QChar *u, int s)
  { return const_cast<JavaScriptNameIdImpl *>(&*_literals.insert(JavaScriptNameIdImpl(u, s))); }

  JavaScript::Lexer *lexer() const
  { return _lexer; }

  void setLexer(JavaScript::Lexer *lexer)
  { _lexer = lexer; }

  JavaScript::NodePool *nodePool() const
  { return _nodePool; }

  void setNodePool(JavaScript::NodePool *nodePool)
  { _nodePool = nodePool; }

  JavaScript::AST::Node *ast() const
  { return _ast; }

  JavaScript::AST::Node *changeAbstractSyntaxTree(JavaScript::AST::Node *node)
  {
    JavaScript::AST::Node *previousAST = _ast;
    _ast = node;
    return previousAST;
  }
};


#endif // JAVASCRIPTENGINE_P_H
