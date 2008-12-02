/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
#ifndef PROITEMS_H
#define PROITEMS_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE

struct AbstractProItemVisitor;

class ProItem
{
public:
    ProItem()
        : m_lineNumber(0)
    {}
    enum ProItemKind {
        ValueKind,
        FunctionKind,
        ConditionKind,
        OperatorKind,
        BlockKind
    };
    virtual ~ProItem() {}

    virtual ProItemKind kind() const = 0;

    void setComment(const QString &comment);
    QString comment() const;

    virtual bool Accept(AbstractProItemVisitor *visitor) = 0;
    int lineNumber() const { return m_lineNumber; }
    void setLineNumber(int lineNumber) { m_lineNumber = lineNumber; }

private:
    QString m_comment;
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
        SingleLine          = 0x10
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

    ProItem::ProItemKind kind() const;

    virtual bool Accept(AbstractProItemVisitor *visitor);
protected:
    QList<ProItem *> m_proitems;
private:
    ProBlock *m_parent;
    int m_blockKind;
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

    virtual bool Accept(AbstractProItemVisitor *visitor);
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

    virtual bool Accept(AbstractProItemVisitor *visitor);
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

    virtual bool Accept(AbstractProItemVisitor *visitor);
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

    virtual bool Accept(AbstractProItemVisitor *visitor);
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

    virtual bool Accept(AbstractProItemVisitor *visitor);
private:
    OperatorKind m_operatorKind;
};

class ProFile : public QObject, public ProBlock
{
    Q_OBJECT

public:
    explicit ProFile(const QString &fileName);
    ~ProFile();

    QString displayFileName() const;
    QString fileName() const;

    void setModified(bool modified);
    bool isModified() const;

    virtual bool Accept(AbstractProItemVisitor *visitor);

private:
    QString m_fileName;
    QString m_displayFileName;
    bool m_modified;
};

QT_END_NAMESPACE

#endif // PROITEMS_H
