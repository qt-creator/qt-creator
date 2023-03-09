// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "typehierarchybuilder.h"

#include <coreplugin/helpitem.h>
#include <cplusplus/CppDocument.h>
#include <texteditor/texteditor.h>
#include <utils/utilsicons.h>

#include <QFuture>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QTextCursor>

#include <functional>

namespace CPlusPlus {
class ClassOrNamespace;
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
                                                            const Utils::FilePath &filePath);
    bool identifiedCppElement() const;
    const QSharedPointer<CppElement> &cppElement() const;
    bool hasDiagnosis() const;
    const QString &diagnosis() const;

    static Utils::Link linkFromExpression(const QString &expression, const Utils::FilePath &filePath);

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
    Utils::CodeModelIcon::Type iconType;
    QString name;
    QString qualifiedName;
    QString type;
};

class CppClass : public CppDeclarableElement
{
public:
    CppClass();
    explicit CppClass(CPlusPlus::Symbol *declaration);

    CppClass *toCppClass() final;

    void lookupBases(const QFuture<void> &future, CPlusPlus::Symbol *declaration,
                     const CPlusPlus::LookupContext &context);
    void lookupDerived(const QFuture<void> &future, CPlusPlus::Symbol *declaration,
                       const CPlusPlus::Snapshot &snapshot);

    QList<CppClass> bases;
    QList<CppClass> derived;

private:
    void addBaseHierarchy(const QFuture<void> &future,
                          const CPlusPlus::LookupContext &context,
                          CPlusPlus::ClassOrNamespace *hierarchy,
                          QSet<CPlusPlus::ClassOrNamespace *> *visited);
    void addDerivedHierarchy(const TypeHierarchy &hierarchy);
};

} // namespace Internal
} // namespace CppEditor
