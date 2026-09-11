// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "rstlang.h"
#include "rstlexer.h"

#include <parsing/memorypool.h>

#include <QAnyStringView>
#include <QList>
#include <QStringList>

#include <cstddef>
#include <iterator>

namespace RstLang {

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

class RSTLANG_EXPORT AST: public Managed
{
public:
    enum Kind {
        Kind_Undefined,

        Kind_Document,
        Kind_Line,

        Kind_Paragraph,
        Kind_LiteralBlock,
        Kind_LineBlock,
        Kind_BlockQuote,
        Kind_Section,
        Kind_DefinitionItem,
        Kind_BulletItem,
        Kind_EnumeratedItem,
        Kind_Field,
        Kind_Directive,
        Kind_Substitution,
        Kind_Target,
        Kind_Comment,
        Kind_Transition,
        Kind_Footnote,
        Kind_Table
    };

    virtual DocumentAST *asDocument() { return nullptr; }
    virtual LineAST *asLine() { return nullptr; }

    virtual BlockAST *asBlock() { return nullptr; }
    virtual ParagraphAST *asParagraph() { return nullptr; }
    virtual LiteralBlockAST *asLiteralBlock() { return nullptr; }
    virtual LineBlockAST *asLineBlock() { return nullptr; }
    virtual BlockQuoteAST *asBlockQuote() { return nullptr; }
    virtual SectionAST *asSection() { return nullptr; }
    virtual DefinitionItemAST *asDefinitionItem() { return nullptr; }
    virtual BulletItemAST *asBulletItem() { return nullptr; }
    virtual EnumeratedItemAST *asEnumeratedItem() { return nullptr; }
    virtual FieldAST *asField() { return nullptr; }
    virtual DirectiveAST *asDirective() { return nullptr; }
    virtual SubstitutionAST *asSubstitution() { return nullptr; }
    virtual TargetAST *asTarget() { return nullptr; }
    virtual CommentAST *asComment() { return nullptr; }
    virtual TransitionAST *asTransition() { return nullptr; }
    virtual FootnoteAST *asFootnote() { return nullptr; }
    virtual TableAST *asTable() { return nullptr; }

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

// One line of the source, the indentation and the markup that opens it
// stripped.
class RSTLANG_EXPORT LineAST: public AST
{
public:
    explicit LineAST(const Token &token)
        : AST(Kind_Line)
        , token(token)
    {}

    LineAST *asLine() override { return this; }
    void accept0(Visitor *visitor) override;

    QStringView text() const { return token.text; }

    Token token;
};

class RSTLANG_EXPORT BlockAST: public AST
{
protected:
    using AST::AST;

public:
    BlockAST *asBlock() override { return this; }
};

// A block that holds other blocks: the body of a list item, of a field, of a
// directive, of a section or of a block quote.
class RSTLANG_EXPORT BodyAST: public BlockAST
{
protected:
    BodyAST(Kind kind, List<BlockAST *> *blocks)
        : BlockAST(kind)
        , blockList(finish(blocks))
    {}

public:
    ListView<BlockAST *> blocks() const { return ListView<BlockAST *>(blockList); }

    List<BlockAST *> *blockList;
};

// A block that is written as a run of lines: a paragraph, a literal block or
// a line block.
class RSTLANG_EXPORT LinesAST: public BlockAST
{
protected:
    LinesAST(Kind kind, List<LineAST *> *lines)
        : BlockAST(kind)
        , lineList(finish(lines))
    {}

public:
    ListView<LineAST *> lines() const { return ListView<LineAST *>(lineList); }

    // The lines joined with a single space, the way a paragraph reads.
    QString text() const;

    // The lines the way they are written, the common indentation removed.
    QString block() const;

    // The column the lines are written at, which is the one to strip off
    // them to read what they say.
    int indent() const;

    List<LineAST *> *lineList;
};

class RSTLANG_EXPORT ParagraphAST: public LinesAST
{
public:
    explicit ParagraphAST(List<LineAST *> *lines)
        : LinesAST(Kind_Paragraph, lines)
    {}

    ParagraphAST *asParagraph() override { return this; }
    void accept0(Visitor *visitor) override;

    // Whether the paragraph introduces the literal block that follows it.
    bool opensLiteralBlock() const;
};

class RSTLANG_EXPORT LiteralBlockAST: public LinesAST
{
public:
    explicit LiteralBlockAST(List<LineAST *> *lines)
        : LinesAST(Kind_LiteralBlock, lines)
    {}

    LiteralBlockAST *asLiteralBlock() override { return this; }
    void accept0(Visitor *visitor) override;
};

class RSTLANG_EXPORT LineBlockAST: public LinesAST
{
public:
    explicit LineBlockAST(List<LineAST *> *lines)
        : LinesAST(Kind_LineBlock, lines)
    {}

    LineBlockAST *asLineBlock() override { return this; }
    void accept0(Visitor *visitor) override;
};

class RSTLANG_EXPORT BlockQuoteAST: public BodyAST
{
public:
    explicit BlockQuoteAST(List<BlockAST *> *blocks)
        : BodyAST(Kind_BlockQuote, blocks)
    {}

    BlockQuoteAST *asBlockQuote() override { return this; }
    void accept0(Visitor *visitor) override;
};

class RSTLANG_EXPORT SectionAST: public BodyAST
{
public:
    SectionAST(const Token &title, List<BlockAST *> *blocks)
        : BodyAST(Kind_Section, blocks)
        , token(title)
    {}

    SectionAST *asSection() override { return this; }
    void accept0(Visitor *visitor) override;

    QStringView title() const { return token.text; }

    // The outermost section of a document is at level one.
    int level() const { return token.level; }

    Token token;
};

// A term and what it means.  This is how the options of a command are
// written: the option, and the paragraph indented under it.
class RSTLANG_EXPORT DefinitionItemAST: public BodyAST
{
public:
    DefinitionItemAST(List<LineAST *> *term, List<BlockAST *> *blocks)
        : BodyAST(Kind_DefinitionItem, blocks)
        , termList(finish(term))
    {}

    DefinitionItemAST *asDefinitionItem() override { return this; }
    void accept0(Visitor *visitor) override;

    ListView<LineAST *> termLines() const { return ListView<LineAST *>(termList); }
    QString term() const;

    List<LineAST *> *termList;
};

class RSTLANG_EXPORT ItemAST: public BodyAST
{
protected:
    ItemAST(Kind kind, const Token &marker, List<BlockAST *> *blocks)
        : BodyAST(kind, blocks)
        , token(marker)
    {}

public:
    QStringView marker() const { return token.name; }

    Token token;
};

class RSTLANG_EXPORT BulletItemAST: public ItemAST
{
public:
    BulletItemAST(const Token &marker, List<BlockAST *> *blocks)
        : ItemAST(Kind_BulletItem, marker, blocks)
    {}

    BulletItemAST *asBulletItem() override { return this; }
    void accept0(Visitor *visitor) override;
};

class RSTLANG_EXPORT EnumeratedItemAST: public ItemAST
{
public:
    EnumeratedItemAST(const Token &marker, List<BlockAST *> *blocks)
        : ItemAST(Kind_EnumeratedItem, marker, blocks)
    {}

    EnumeratedItemAST *asEnumeratedItem() override { return this; }
    void accept0(Visitor *visitor) override;
};

class RSTLANG_EXPORT FieldAST: public BodyAST
{
public:
    FieldAST(const Token &name, List<BlockAST *> *blocks)
        : BodyAST(Kind_Field, blocks)
        , token(name)
    {}

    FieldAST *asField() override { return this; }
    void accept0(Visitor *visitor) override;

    QStringView fieldName() const { return token.name; }

    // What stands on the line of the field itself.  The rest of the value is
    // in the blocks.
    QStringView value() const { return token.text; }

    Token token;
};

class RSTLANG_EXPORT DirectiveAST: public BodyAST
{
public:
    DirectiveAST(const Token &directive, List<BlockAST *> *blocks)
        : BodyAST(Kind_Directive, blocks)
        , token(directive)
    {}

    DirectiveAST *asDirective() override { return this; }
    void accept0(Visitor *visitor) override;

    // The type without the domain it is written in, so that "cmake:command"
    // and "command" both read as "command".
    QStringView type() const;
    QStringView domain() const;

    bool isNamed(QAnyStringView spelling) const;

    // What stands behind the "::".
    QStringView argument() const { return token.text; }

    // The fields the body opens with configure the directive; what follows
    // them is what it holds.
    QList<FieldAST *> options() const;
    ListView<BlockAST *> content() const;

    // The value of one of the options, unset where the directive has none of
    // that name.
    QStringView option(QAnyStringView name) const;

    Token token;
};

// A definition of the form ".. |name| replace:: text".
class RSTLANG_EXPORT SubstitutionAST: public BodyAST
{
public:
    SubstitutionAST(const Token &substitution, List<BlockAST *> *blocks)
        : BodyAST(Kind_Substitution, blocks)
        , token(substitution)
    {}

    SubstitutionAST *asSubstitution() override { return this; }
    void accept0(Visitor *visitor) override;

    QStringView substitutionName() const { return token.name; }

    // The directive that produces the text, "replace" for all but a few.
    QStringView directiveType() const;

    // What stands behind the "::" of that directive.
    QStringView argument() const;

    Token token;
};

// A definition of the form ".. [the label] the text": a footnote where the
// label reads as a number or as "#", a citation where it reads as a name.
class RSTLANG_EXPORT FootnoteAST: public BodyAST
{
public:
    FootnoteAST(const Token &footnote, List<BlockAST *> *blocks)
        : BodyAST(Kind_Footnote, blocks)
        , token(footnote)
    {}

    FootnoteAST *asFootnote() override { return this; }
    void accept0(Visitor *visitor) override;

    QStringView label() const { return token.name; }

    // What stands on the line of the label itself, which the blocks hold as
    // the first line of the body.
    QStringView text() const { return token.text; }

    Token token;
};

// A table, the way reStructuredText draws one: cells set in columns that the
// rules above and below them mark out.
class RSTLANG_EXPORT TableAST: public BlockAST
{
public:
    explicit TableAST(const Token &table)
        : BlockAST(Kind_Table)
        , token(table)
    {}

    TableAST *asTable() override { return this; }
    void accept0(Visitor *visitor) override;

    // The cells of the table, row by row, every row holding one entry per
    // column.  Where the table draws a heading, "headerRows" is told how many
    // of the first rows it covers.
    QList<QStringList> rows(int *headerRows = nullptr) const;

    // The table the way it is written, the rules included.
    QStringView block() const { return token.spelling; }

    Token token;
};

class RSTLANG_EXPORT TargetAST: public BlockAST
{
public:
    explicit TargetAST(const Token &target)
        : BlockAST(Kind_Target)
        , token(target)
    {}

    TargetAST *asTarget() override { return this; }
    void accept0(Visitor *visitor) override;

    QStringView targetName() const { return token.name; }
    QStringView link() const { return token.text; }

    Token token;
};

class RSTLANG_EXPORT CommentAST: public BlockAST
{
public:
    explicit CommentAST(const Token &comment)
        : BlockAST(Kind_Comment)
        , token(comment)
    {}

    CommentAST *asComment() override { return this; }
    void accept0(Visitor *visitor) override;

    QStringView text() const { return token.text; }

    Token token;
};

class RSTLANG_EXPORT TransitionAST: public BlockAST
{
public:
    explicit TransitionAST(const Token &transition)
        : BlockAST(Kind_Transition)
        , token(transition)
    {}

    TransitionAST *asTransition() override { return this; }
    void accept0(Visitor *visitor) override;

    Token token;
};

class RSTLANG_EXPORT DocumentAST: public AST
{
public:
    explicit DocumentAST(List<BlockAST *> *blocks)
        : AST(Kind_Document)
        , blockList(finish(blocks))
    {}

    DocumentAST *asDocument() override { return this; }
    void accept0(Visitor *visitor) override;

    ListView<BlockAST *> blocks() const { return ListView<BlockAST *>(blockList); }

    List<BlockAST *> *blockList;
};

} // namespace RstLang
