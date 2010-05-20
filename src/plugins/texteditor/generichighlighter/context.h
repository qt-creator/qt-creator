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

#ifndef CONTEXT_H
#define CONTEXT_H

#include "includerulesinstruction.h"

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QSharedPointer>

namespace TextEditor {
namespace Internal {

class Rule;
class HighlightDefinition;

class Context
{
public:
    Context();
    Context(const Context &context);
    const Context &operator=(Context copy);
    ~Context();

    void configureId(const int unique);
    const QString &id() const;

    void setName(const QString &name);
    const QString &name() const;

    void setLineBeginContext(const QString &context);
    const QString &lineBeginContext() const;

    void setLineEndContext(const QString &context);
    const QString &lineEndContext() const;

    void setFallthroughContext(const QString &context);
    const QString &fallthroughContext() const;

    void setItemData(const QString &itemData);
    const QString &itemData() const;

    void setFallthrough(const QString &fallthrough);
    bool isFallthrough() const;

    void setDynamic(const QString &dynamic);
    bool isDynamic() const;
    void updateDynamicRules(const QStringList &captures) const;

    void addRule(const QSharedPointer<Rule> &rule);
    void addRule(const QSharedPointer<Rule> &rule, int index);
    const QList<QSharedPointer<Rule> > &rules() const;

    void addIncludeRulesInstruction(const IncludeRulesInstruction &instruction);
    const QList<IncludeRulesInstruction> &includeRulesInstructions() const;
    void clearIncludeRulesInstructions();

    void setDefinition(const QSharedPointer<HighlightDefinition> &definition);
    const QSharedPointer<HighlightDefinition> &definition() const;

    void swap(Context &context);

private:
    QString m_id;
    QString m_name;
    QString m_lineBeginContext;
    QString m_lineEndContext;
    QString m_fallthroughContext;
    QString m_itemData;
    bool m_fallthrough;
    bool m_dynamic;

    QList<QSharedPointer<Rule> > m_rules;
    QList<IncludeRulesInstruction> m_instructions;

    QSharedPointer<HighlightDefinition> m_definition;
};

} // namespace Internal
} // namespace TextEditor

#endif // CONTEXT_H
