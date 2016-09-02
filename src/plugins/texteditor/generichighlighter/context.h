/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "includerulesinstruction.h"

#include <QString>
#include <QList>
#include <QSharedPointer>

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

    void setLineEmptyContext(const QString &context);
    const QString &lineEmptyContext() const;

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
    QString m_lineEmptyContext;
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
