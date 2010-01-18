/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include <QtCore/QList>

QT_BEGIN_NAMESPACE

struct AbstractProItemVisitor;

class ProItem
{
public:
    enum ProItemKind {
        ValueKind,
        FunctionKind,
        ConditionKind,
        OperatorKind,
        BlockKind
    };

    enum ProItemReturn {
        ReturnFalse,
        ReturnTrue,
        ReturnBreak,
        ReturnNext,
        ReturnLoop,
        ReturnSkip,
        ReturnReturn
   };

    ProItem() : m_lineNumber(0) {}
    virtual ~ProItem() {}

    virtual ProItemKind kind() const = 0;

    virtual ProItemReturn Accept(AbstractProItemVisitor *visitor) = 0;
    int lineNumber() const { return m_lineNumber; }
    void setLineNumber(int lineNumber) { m_lineNumber = lineNumber; }

private:
    int m_lineNumber;
};

class ProBlock : public ProItem
{
public:
    enum ProBlockKind {
        NormalKind          = 0x00,
        ScopeKind           = 0x01,
        ScopeContentsKind   = 0x02,
        VariableKind        = 0x04,
        ProFileKind         = 0x08,
        FunctionBodyKind    = 0x10,
        SingleLine          = 0x80
    };

    ProBlock(ProBlock *parent);
    ~ProBlock();

    void appendItem(ProItem *proitem);
    void setItems(const QList<ProItem *> &proitems);
    QList<ProItem *> items() const;

    void setBlockKind(int blockKind);
    int blockKind() const;

    void setParent(ProBlock *parent);
    ProBlock *parent() const;

    void ref() { ++m_refCount; }
    void deref() { if (!--m_refCount) delete this; }

    ProItem::ProItemKind kind() const;

    virtual ProItemReturn Accept(AbstractProItemVisitor *visitor);
protected:
    QList<ProItem *> m_proitems;
private:
    ProBlock *m_parent;
    int m_blockKind;
    int m_refCount;
};

class ProVariable : public ProBlock
{
public:
    enum VariableOperator {
        AddOperator         = 0,
        RemoveOperator      = 1,
        ReplaceOperator     = 2,
        SetOperator         = 3,
        UniqueAddOperator   = 4
    };

    ProVariable(const QString &name, ProBlock *parent);

    void setVariableOperator(VariableOperator variableKind);
    VariableOperator variableOperator() const;

    void setVariable(const QString &name);
    QString variable() const;

    virtual ProItemReturn Accept(AbstractProItemVisitor *visitor);
private:
    VariableOperator m_variableKind;
    QString m_variable;
};

class ProValue : public ProItem
{
public:
    ProValue(const QString &value, ProVariable *variable);

    void setValue(const QString &value);
    QString value() const;

    void setVariable(ProVariable *variable);
    ProVariable *variable() const;

    ProItem::ProItemKind kind() const;

    virtual ProItemReturn Accept(AbstractProItemVisitor *visitor);
private:
    QString m_value;
    ProVariable *m_variable;
};

class ProFunction : public ProItem
{
public:
    explicit ProFunction(const QString &text);

    void setText(const QString &text);
    QString text() const;

    ProItem::ProItemKind kind() const;

    virtual ProItemReturn Accept(AbstractProItemVisitor *visitor);
private:
    QString m_text;
};

class ProCondition : public ProItem
{
public:
    explicit ProCondition(const QString &text);

    void setText(const QString &text);
    QString text() const;

    ProItem::ProItemKind kind() const;

    virtual ProItemReturn Accept(AbstractProItemVisitor *visitor);
private:
    QString m_text;
};

class ProOperator : public ProItem
{
public:
    enum OperatorKind {
        OrOperator      = 1,
        NotOperator     = 2
    };

    explicit ProOperator(OperatorKind operatorKind);

    void setOperatorKind(OperatorKind operatorKind);
    OperatorKind operatorKind() const;

    ProItem::ProItemKind kind() const;

    virtual ProItemReturn Accept(AbstractProItemVisitor *visitor);
private:
    OperatorKind m_operatorKind;
};

class ProFile : public ProBlock
{
public:
    explicit ProFile(const QString &fileName);
    ~ProFile();

    QString displayFileName() const;
    QString fileName() const;
    QString directoryName() const;

    virtual ProItemReturn Accept(AbstractProItemVisitor *visitor);

private:
    QString m_fileName;
    QString m_displayFileName;
    QString m_directoryName;
};

QT_END_NAMESPACE

#endif // PROITEMS_H
