// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmakelang.h"
#include "cmakelexer.h"
#include "cmakememorypool.h"

#include <cstddef>
#include <iterator>

namespace CMakeLang {

template <typename T>
class List: public Managed
{
public:
    explicit List(const T &value)
        : value(value)
        , next(this)
    {}

    List(List *previous, const T &value)
        : value(value)
    {
        next = previous->next;
        previous->next = this;
    }

    List *finish()
    {
        List *head = next;
        next = nullptr;
        return head;
    }

    T value;
    List *next = nullptr;
};

template <typename T>
class ListView
{
public:
    class Iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T *;
        using reference = T;

        explicit Iterator(List<T> *node = nullptr)
            : _node(node)
        {}

        T operator*() const { return _node->value; }
        Iterator &operator++() { _node = _node->next; return *this; }
        bool operator==(const Iterator &other) const { return _node == other._node; }
        bool operator!=(const Iterator &other) const { return _node != other._node; }

    private:
        List<T> *_node;
    };

    using value_type = T;
    using const_iterator = Iterator;

    ListView() = default;

    explicit ListView(List<T> *list)
        : _list(list)
    {}

    Iterator begin() const { return Iterator(_list); }
    Iterator end() const { return Iterator(); }

    bool isEmpty() const { return !_list; }

    int size() const
    {
        int count = 0;
        for (List<T> *it = _list; it; it = it->next)
            ++count;
        return count;
    }

    T at(int index) const
    {
        for (List<T> *it = _list; it; it = it->next, --index) {
            if (index == 0)
                return it->value;
        }
        return T();
    }

    T first() const { return at(0); }

    T last() const
    {
        T result = T();
        for (List<T> *it = _list; it; it = it->next)
            result = it->value;
        return result;
    }

private:
    List<T> *_list = nullptr;
};

class CMAKELANG_EXPORT AST: public Managed
{
public:
    enum Kind {
        Kind_Undefined,

        Kind_SourceFile,

        Kind_Command,
        Kind_If,
        Kind_ElseIfClause,
        Kind_ElseClause,
        Kind_ForEach,
        Kind_While,
        Kind_Function,
        Kind_Macro,
        Kind_Block,

        Kind_UnquotedArgument,
        Kind_QuotedArgument,
        Kind_BracketArgument,
        Kind_ParenGroupArgument
    };

    virtual SourceFileAST *asSourceFile() { return nullptr; }

    virtual ElementAST *asElement() { return nullptr; }
    virtual CommandAST *asCommand() { return nullptr; }
    virtual IfAST *asIf() { return nullptr; }
    virtual ElseIfClauseAST *asElseIfClause() { return nullptr; }
    virtual ElseClauseAST *asElseClause() { return nullptr; }
    virtual NestedCommandAST *asNestedCommand() { return nullptr; }
    virtual ForEachAST *asForEach() { return nullptr; }
    virtual WhileAST *asWhile() { return nullptr; }
    virtual FunctionAST *asFunction() { return nullptr; }
    virtual MacroAST *asMacro() { return nullptr; }
    virtual BlockAST *asBlock() { return nullptr; }

    virtual ArgumentAST *asArgument() { return nullptr; }
    virtual UnquotedArgumentAST *asUnquotedArgument() { return nullptr; }
    virtual QuotedArgumentAST *asQuotedArgument() { return nullptr; }
    virtual BracketArgumentAST *asBracketArgument() { return nullptr; }
    virtual ParenGroupArgumentAST *asParenGroupArgument() { return nullptr; }

    void accept(Visitor *visitor);
    static void accept(AST *ast, Visitor *visitor);

    template <typename T>
    static void accept(List<T> *it, Visitor *visitor)
    {
        for (; it; it = it->next)
            accept(it->value, visitor);
    }

    virtual void accept0(Visitor *visitor) = 0;

protected:
    explicit AST(Kind kind)
        : kind(kind)
    {}

    ~AST() = default; // Managed types cannot be deleted.

    template <typename T>
    static List<T> *finish(List<T> *list)
    {
        return list ? list->finish() : nullptr;
    }

public:
    int kind;
    int position = 0;
    int length = 0;
    int line = 1;
    int column = 1;
};

class CMAKELANG_EXPORT ElementAST: public AST
{
protected:
    using AST::AST;

public:
    ElementAST *asElement() override { return this; }
};

class CMAKELANG_EXPORT ArgumentAST: public AST
{
protected:
    ArgumentAST(Kind kind, const Token &token)
        : AST(kind)
        , token(token)
    {}

public:
    ArgumentAST *asArgument() override { return this; }

    QString value() const { return token.text(); }

    Token token;
};

class CMAKELANG_EXPORT UnquotedArgumentAST: public ArgumentAST
{
public:
    explicit UnquotedArgumentAST(const Token &token)
        : ArgumentAST(Kind_UnquotedArgument, token)
    {}

    UnquotedArgumentAST *asUnquotedArgument() override { return this; }
    void accept0(Visitor *visitor) override;
};

class CMAKELANG_EXPORT QuotedArgumentAST: public ArgumentAST
{
public:
    explicit QuotedArgumentAST(const Token &token)
        : ArgumentAST(Kind_QuotedArgument, token)
    {}

    QuotedArgumentAST *asQuotedArgument() override { return this; }
    void accept0(Visitor *visitor) override;
};

class CMAKELANG_EXPORT BracketArgumentAST: public ArgumentAST
{
public:
    explicit BracketArgumentAST(const Token &token)
        : ArgumentAST(Kind_BracketArgument, token)
    {}

    BracketArgumentAST *asBracketArgument() override { return this; }
    void accept0(Visitor *visitor) override;
};

class CMAKELANG_EXPORT ParenGroupArgumentAST: public ArgumentAST
{
public:
    ParenGroupArgumentAST(const Token &leftParen, List<ArgumentAST *> *arguments,
                          const Token &rightParen)
        : ArgumentAST(Kind_ParenGroupArgument, leftParen)
        , leftParen(leftParen)
        , rightParen(rightParen)
        , argumentList(finish(arguments))
    {}

    ParenGroupArgumentAST *asParenGroupArgument() override { return this; }
    void accept0(Visitor *visitor) override;

    ListView<ArgumentAST *> arguments() const { return ListView<ArgumentAST *>(argumentList); }

    Token leftParen;
    Token rightParen;
    List<ArgumentAST *> *argumentList;
};

class CMAKELANG_EXPORT CommandAST: public ElementAST
{
public:
    CommandAST(const Token &name, const Token &leftParen, List<ArgumentAST *> *arguments,
               const Token &rightParen)
        : ElementAST(Kind_Command)
        , name(name)
        , leftParen(leftParen)
        , rightParen(rightParen)
        , argumentList(finish(arguments))
    {}

    CommandAST *asCommand() override { return this; }
    void accept0(Visitor *visitor) override;

    QString commandName() const { return name.text(); }

    bool isNamed(QAnyStringView spelling) const
    {
        return QAnyStringView::compare(spelling, name.spelling, Qt::CaseInsensitive) == 0;
    }

    ListView<ArgumentAST *> arguments() const { return ListView<ArgumentAST *>(argumentList); }

    int lineEnd() const { return rightParen.line; }

    Token name;
    Token leftParen;
    Token rightParen;
    List<ArgumentAST *> *argumentList;
};

class CMAKELANG_EXPORT ElseIfClauseAST: public AST
{
public:
    ElseIfClauseAST(CommandAST *command, List<ElementAST *> *elements)
        : AST(Kind_ElseIfClause)
        , command(command)
        , elementList(finish(elements))
    {}

    ElseIfClauseAST *asElseIfClause() override { return this; }
    void accept0(Visitor *visitor) override;

    ListView<ElementAST *> elements() const { return ListView<ElementAST *>(elementList); }

    CommandAST *command;
    List<ElementAST *> *elementList;
};

class CMAKELANG_EXPORT ElseClauseAST: public AST
{
public:
    ElseClauseAST(CommandAST *command, List<ElementAST *> *elements)
        : AST(Kind_ElseClause)
        , command(command)
        , elementList(finish(elements))
    {}

    ElseClauseAST *asElseClause() override { return this; }
    void accept0(Visitor *visitor) override;

    ListView<ElementAST *> elements() const { return ListView<ElementAST *>(elementList); }

    CommandAST *command;
    List<ElementAST *> *elementList;
};

class CMAKELANG_EXPORT IfAST: public ElementAST
{
public:
    IfAST(CommandAST *ifCommand, List<ElementAST *> *elements,
          List<ElseIfClauseAST *> *elseIfClauses, ElseClauseAST *elseClause,
          CommandAST *endIfCommand)
        : ElementAST(Kind_If)
        , ifCommand(ifCommand)
        , elementList(finish(elements))
        , elseIfClauseList(finish(elseIfClauses))
        , elseClause(elseClause)
        , endIfCommand(endIfCommand)
    {}

    IfAST *asIf() override { return this; }
    void accept0(Visitor *visitor) override;

    ListView<ElementAST *> elements() const { return ListView<ElementAST *>(elementList); }

    ListView<ElseIfClauseAST *> elseIfClauses() const
    {
        return ListView<ElseIfClauseAST *>(elseIfClauseList);
    }

    CommandAST *ifCommand;
    List<ElementAST *> *elementList;
    List<ElseIfClauseAST *> *elseIfClauseList;
    ElseClauseAST *elseClause;
    CommandAST *endIfCommand;
};

class CMAKELANG_EXPORT NestedCommandAST: public ElementAST
{
protected:
    NestedCommandAST(Kind kind, CommandAST *openCommand, List<ElementAST *> *elements,
                     CommandAST *closeCommand)
        : ElementAST(kind)
        , openCommand(openCommand)
        , elementList(finish(elements))
        , closeCommand(closeCommand)
    {}

public:
    NestedCommandAST *asNestedCommand() override { return this; }

    ListView<ElementAST *> elements() const { return ListView<ElementAST *>(elementList); }

    CommandAST *openCommand;
    List<ElementAST *> *elementList;
    CommandAST *closeCommand;
};

#define CMAKELANG_NESTED_COMMAND(Name, KindName) \
    class CMAKELANG_EXPORT Name##AST: public NestedCommandAST \
    { \
    public: \
        Name##AST(CommandAST *openCommand, List<ElementAST *> *elements, \
                  CommandAST *closeCommand) \
            : NestedCommandAST(KindName, openCommand, elements, closeCommand) \
        {} \
        Name##AST *as##Name() override { return this; } \
        void accept0(Visitor *visitor) override; \
    }

CMAKELANG_NESTED_COMMAND(ForEach, Kind_ForEach);
CMAKELANG_NESTED_COMMAND(While, Kind_While);
CMAKELANG_NESTED_COMMAND(Function, Kind_Function);
CMAKELANG_NESTED_COMMAND(Macro, Kind_Macro);
CMAKELANG_NESTED_COMMAND(Block, Kind_Block);

#undef CMAKELANG_NESTED_COMMAND

class CMAKELANG_EXPORT SourceFileAST: public AST
{
public:
    explicit SourceFileAST(List<ElementAST *> *elements)
        : AST(Kind_SourceFile)
        , elementList(finish(elements))
    {}

    SourceFileAST *asSourceFile() override { return this; }
    void accept0(Visitor *visitor) override;

    ListView<ElementAST *> elements() const { return ListView<ElementAST *>(elementList); }

    List<ElementAST *> *elementList;
};

} // namespace CMakeLang
