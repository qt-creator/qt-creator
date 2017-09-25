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

#include <texteditor/texteditor.h>
#include <texteditor/helpitem.h>

#include <cplusplus/CppDocument.h>

#include <QString>
#include <QStringList>
#include <QSharedPointer>
#include <QTextCursor>
#include <QIcon>

namespace CPlusPlus {
class LookupItem;
class LookupContext;
}

namespace CppTools { class CppModelManager; }

namespace CppEditor {
namespace Internal {

class CppElement;

class CppElementEvaluator
{
public:
    explicit CppElementEvaluator(TextEditor::TextEditorWidget *editor);

    void setTextCursor(const QTextCursor &tc);
    void setLookupBaseClasses(const bool lookup);
    void setLookupDerivedClasses(const bool lookup);

    void execute();
    bool identifiedCppElement() const;
    const QSharedPointer<CppElement> &cppElement() const;
    bool hasDiagnosis() const;
    const QString &diagnosis() const;

private:
    void clear();
    void checkDiagnosticMessage(int pos);
    bool matchIncludeFile(const CPlusPlus::Document::Ptr &document, unsigned line);
    bool matchMacroInUse(const CPlusPlus::Document::Ptr &document, unsigned pos);
    void handleLookupItemMatch(const CPlusPlus::Snapshot &snapshot,
                               const CPlusPlus::LookupItem &lookupItem,
                               const CPlusPlus::LookupContext &lookupContext,
                               const CPlusPlus::Scope *scope);

    TextEditor::TextEditorWidget *m_editor;
    CppTools::CppModelManager *m_modelManager;
    QTextCursor m_tc;
    bool m_lookupBaseClasses;
    bool m_lookupDerivedClasses;
    QSharedPointer<CppElement> m_element;
    QString m_diagnosis;
};

class CppElement
{
protected:
    CppElement();

public:
    virtual ~CppElement();

    TextEditor::HelpItem::Category helpCategory;
    QStringList helpIdCandidates;
    QString helpMark;
    TextEditor::TextEditorWidget::Link link;
    QString tooltip;
};

class Unknown : public CppElement
{
public:
    explicit Unknown(const QString &type);

public:
    QString type;
};

class CppInclude : public CppElement
{
public:
    explicit CppInclude(const CPlusPlus::Document::Include &includeFile);

public:
    QString path;
    QString fileName;
};

class CppMacro : public CppElement
{
public:
    explicit CppMacro(const CPlusPlus::Macro &macro);
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

class CppNamespace : public CppDeclarableElement
{
public:
    explicit CppNamespace(CPlusPlus::Symbol *declaration);
};

class CppClass : public CppDeclarableElement
{
public:
    CppClass();
    explicit CppClass(CPlusPlus::Symbol *declaration);

    bool operator==(const CppClass &other);

    void lookupBases(CPlusPlus::Symbol *declaration, const CPlusPlus::LookupContext &context);
    void lookupDerived(CPlusPlus::Symbol *declaration, const CPlusPlus::Snapshot &snapshot);

public:
    QList<CppClass> bases;
    QList<CppClass> derived;
};

class CppFunction : public CppDeclarableElement
{
public:
    explicit CppFunction(CPlusPlus::Symbol *declaration);
};

class CppEnum : public CppDeclarableElement
{
public:
    explicit CppEnum(CPlusPlus::Enum *declaration);
};

class CppTypedef : public CppDeclarableElement
{
public:
    explicit CppTypedef(CPlusPlus::Symbol *declaration);
};

class CppVariable : public CppDeclarableElement
{
public:
    CppVariable(CPlusPlus::Symbol *declaration,
                const CPlusPlus::LookupContext &context,
                CPlusPlus::Scope *scope);
};

class CppEnumerator : public CppDeclarableElement
{
public:
    explicit CppEnumerator(CPlusPlus::EnumeratorDeclaration *declaration);
};

} // namespace Internal
} // namespace CppEditor
