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

#include <coreplugin/helpitem.h>
#include <texteditor/texteditor.h>

#include <cplusplus/CppDocument.h>

#include <QFuture>
#include <QIcon>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QTextCursor>

#include <functional>

namespace CPlusPlus {
class LookupItem;
class LookupContext;
}

namespace CppEditor {
class CppModelManager;

namespace Internal {
class CppElement;

class CppElementEvaluator final
{
public:
    explicit CppElementEvaluator(TextEditor::TextEditorWidget *editor);
    ~CppElementEvaluator();

    void setTextCursor(const QTextCursor &tc);

    void execute();
    static QFuture<QSharedPointer<CppElement>> asyncExecute(TextEditor::TextEditorWidget *editor);
    static QFuture<QSharedPointer<CppElement>> asyncExecute(const QString &expression,
                                                            const QString &fileName);
    bool identifiedCppElement() const;
    const QSharedPointer<CppElement> &cppElement() const;
    bool hasDiagnosis() const;
    const QString &diagnosis() const;

    static Utils::Link linkFromExpression(const QString &expression, const QString &fileName);

private:
    class CppElementEvaluatorPrivate *d;
};

class CppClass;

class CppElement
{
protected:
    CppElement();

public:
    virtual ~CppElement();

    virtual CppClass *toCppClass();

    Core::HelpItem::Category helpCategory = Core::HelpItem::Unknown;
    QStringList helpIdCandidates;
    QString helpMark;
    Utils::Link link;
    QString tooltip;
};

class CppDeclarableElement : public CppElement
{
public:
    explicit CppDeclarableElement(CPlusPlus::Symbol *declaration);

public:
    CPlusPlus::Symbol *declaration;
    QString name;
    QString qualifiedName;
    QString type;
    QIcon icon;
};

class CppClass : public CppDeclarableElement
{
public:
    CppClass();
    explicit CppClass(CPlusPlus::Symbol *declaration);

    bool operator==(const CppClass &other);

    CppClass *toCppClass() final;

    void lookupBases(QFutureInterfaceBase &futureInterface,
                     CPlusPlus::Symbol *declaration, const CPlusPlus::LookupContext &context);
    void lookupDerived(QFutureInterfaceBase &futureInterface,
                       CPlusPlus::Symbol *declaration, const CPlusPlus::Snapshot &snapshot);

public:
    QList<CppClass> bases;
    QList<CppClass> derived;
};

} // namespace Internal
} // namespace CppEditor
