/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef PROITEMS_H
#define PROITEMS_H

#include <QtCore/QString>
#include <QtCore/QVector>

QT_BEGIN_NAMESPACE

#ifdef PROPARSER_THREAD_SAFE
typedef QAtomicInt ProItemRefCount;
#else
class ProItemRefCount {
public:
    ProItemRefCount(int cnt = 0) : m_cnt(cnt) {}
    bool ref() { return ++m_cnt != 0; }
    bool deref() { return --m_cnt != 0; }
    ProItemRefCount &operator=(int value) { m_cnt = value; return *this; }
private:
    int m_cnt;
};
#endif

namespace ProStringConstants {
enum OmitPreHashing { NoHash };
}

class ProString {
public:
    ProString();
    ProString(const ProString &other);
    ProString(const ProString &other, ProStringConstants::OmitPreHashing);
    explicit ProString(const QString &str);
    ProString(const QString &str, ProStringConstants::OmitPreHashing);
    explicit ProString(const char *str);
    ProString(const char *str, ProStringConstants::OmitPreHashing);
    QString toQString() const;
    QString &toQString(QString &tmp) const;
    bool operator==(const ProString &other) const;
    bool operator==(const QString &other) const;
    bool operator==(const QLatin1String &other) const;
    bool operator!=(const ProString &other) const { return !(*this == other); }
    bool operator!=(const QString &other) const { return !(*this == other); }
    bool operator!=(const QLatin1String &other) const { return !(*this == other); }
    bool isEmpty() const { return !m_length; }
    int size() const { return m_length; }
    const QChar *constData() const { return m_string.constData() + m_offset; }
    ProString mid(int off, int len = -1) const;
    ProString left(int len) const { return mid(0, len); }
    ProString right(int len) const { return mid(qMax(0, size() - len)); }
    ProString trimmed() const;
    void clear() { m_string.clear(); m_length = 0; }

private:
    QString m_string;
    int m_offset, m_length;
    mutable uint m_hash;
    uint updatedHash() const;
    friend uint qHash(const ProString &str);
    friend QString operator+(const ProString &one, const ProString &two);
};
Q_DECLARE_TYPEINFO(ProString, Q_MOVABLE_TYPE);

uint qHash(const ProString &str);
QString operator+(const ProString &one, const ProString &two);
inline QString operator+(const ProString &one, const QString &two)
    { return one + ProString(two, ProStringConstants::NoHash); }
inline QString operator+(const QString &one, const ProString &two)
    { return ProString(one, ProStringConstants::NoHash) + two; }

class ProStringList : public QVector<ProString> {
public:
    ProStringList() {}
    ProStringList(const ProString &str) { *this << str; }
    QString join(const QString &sep) const;
    void removeDuplicates();
};

class ProItem
{
public:
    enum ProItemKind {
        ConditionKind,
        OpNotKind,
        OpAndKind,
        OpOrKind,
        VariableKind,
        BranchKind,
        LoopKind,
        FunctionDefKind
    };

    ProItem(ProItemKind kind) : m_kind(kind), m_lineNumber(0) {}

    ProItemKind kind() const { return m_kind; }

    int lineNumber() const { return m_lineNumber; }
    void setLineNumber(int lineNumber) { m_lineNumber = lineNumber; }

    ProItem *next() const { return m_next; }
    ProItem **nextRef() { return &m_next; }

    static void disposeItems(ProItem *nitm);

private:
    ProItem *m_next;
    ProItemKind m_kind;
    int m_lineNumber;
};

class ProVariable : public ProItem
{
public:
    enum VariableOperator {
        AddOperator         = 0,
        RemoveOperator      = 1,
        ReplaceOperator     = 2,
        SetOperator         = 3,
        UniqueAddOperator   = 4
    };

    ProVariable(const ProString &name) : ProItem(VariableKind),
            m_variableKind(SetOperator), m_variable(name) {}
    void setVariableOperator(VariableOperator variableKind) { m_variableKind = variableKind; }
    VariableOperator variableOperator() const { return m_variableKind; }
    ProString variable() const { return m_variable; }
    void setValue(const ProString &value) { m_value = value; }
    ProString value() const { return m_value; }

private:
    VariableOperator m_variableKind;
    ProString m_variable;
    ProString m_value;
};

class ProCondition : public ProItem
{
public:
    explicit ProCondition(const QString &text) : ProItem(ConditionKind), m_text(text) {}
    QString text() const { return m_text; }

private:
    QString m_text;
};

class ProBranch : public ProItem
{
public:
    ProBranch() : ProItem(BranchKind) {}
    ~ProBranch();

    ProItem *thenItems() const { return m_thenItems; }
    ProItem **thenItemsRef() { return &m_thenItems; }
    ProItem *elseItems() const { return m_elseItems; }
    ProItem **elseItemsRef() { return &m_elseItems; }

private:
    ProItem *m_thenItems;
    ProItem *m_elseItems;
};

class ProLoop : public ProItem
{
public:
    ProLoop(const QString &var, const QString &expr)
        : ProItem(LoopKind), m_variable(ProString(var)),
          m_expression(ProString(expr, ProStringConstants::NoHash)) {}
    ~ProLoop();

    ProString variable() const { return m_variable; }
    ProString expression() const { return m_expression; }
    ProItem *items() const { return m_proitems; }
    ProItem **itemsRef() { return &m_proitems; }

private:
    ProString m_variable;
    ProString m_expression;
    ProItem *m_proitems;
};

class ProFunctionDef : public ProItem
{
public:
    enum FunctionType { TestFunction, ReplaceFunction };

    ProFunctionDef(const QString &name, FunctionType type)
        : ProItem(FunctionDefKind), m_name(ProString(name)), m_type(type), m_refCount(1) {}
    ~ProFunctionDef();

    ProString name() const { return m_name; }
    FunctionType type() const { return m_type; }
    ProItem *items() const { return m_proitems; }
    ProItem **itemsRef() { return &m_proitems; }

    void ref() { m_refCount.ref(); }
    void deref() { if (!m_refCount.deref()) delete this; }

private:
    ProString m_name;
    FunctionType m_type;
    ProItemRefCount m_refCount;
    ProItem *m_proitems;
};

class ProFile
{
public:
    explicit ProFile(const QString &fileName);
    ~ProFile();

    QString displayFileName() const { return m_displayFileName; }
    QString fileName() const { return m_fileName; }
    QString directoryName() const { return m_directoryName; }
    ProItem *items() const { return m_proitems; }
    ProItem **itemsRef() { return &m_proitems; }

    void ref() { m_refCount.ref(); }
    void deref() { if (!m_refCount.deref()) delete this; }

private:
    ProItem *m_proitems;
    ProItemRefCount m_refCount;
    QString m_fileName;
    QString m_displayFileName;
    QString m_directoryName;
};

QT_END_NAMESPACE

#endif // PROITEMS_H
