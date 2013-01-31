/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPHIGHLEVELMODEL_H
#define CPPHIGHLEVELMODEL_H

#include "cppeditor.h"

#include <texteditor/helpitem.h>
#include <cpptools/symbolfinder.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/Overview.h>

#include <QString>
#include <QStringList>
#include <QSharedPointer>
#include <QTextCursor>
#include <QIcon>

namespace CPlusPlus {
class LookupItem;
class LookupContext;
}

namespace CppTools {
class CppModelManagerInterface;
}

namespace CppEditor {
namespace Internal {

class CPPEditorWidget;
class CppElement;

class CppElementEvaluator
{
public:
    explicit CppElementEvaluator(CPPEditorWidget *editor);

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
                               const CPlusPlus::LookupContext &lookupContext);

    CPPEditorWidget *m_editor;
    CPlusPlus::CppModelManagerInterface *m_modelManager;
    QTextCursor m_tc;
    bool m_lookupBaseClasses;
    bool m_lookupDerivedClasses;
    QSharedPointer<CppElement> m_element;
    QString m_diagnosis;
    CppTools::SymbolFinder m_symbolFinder;
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
    CPPEditorWidget::Link link;
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

#endif // CPPHIGHLEVELMODEL_H
