/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef CPPHIGHLEVELMODEL_H
#define CPPHIGHLEVELMODEL_H

#include "cppeditor.h"

#include <texteditor/helpitem.h>

#include <cplusplus/CppDocument.h>
#include <cplusplus/Overview.h>

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QSharedPointer>
#include <QtGui/QTextCursor>
#include <QtGui/QIcon>

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
    CppElementEvaluator(CPPEditorWidget *editor);

    void setTextCursor(const QTextCursor &tc);
    void setLookupBaseClasses(const bool lookup);

    void execute();
    bool identifiedCppElement() const;
    const QSharedPointer<CppElement> &cppElement() const;
    bool hasDiagnosis() const;
    const QString &diagnosis() const;

private:
    void clear();
    void checkDiagnosticMessage(const CPlusPlus::Document::Ptr &document, unsigned line);
    bool matchIncludeFile(const CPlusPlus::Document::Ptr &document, unsigned line);
    bool matchMacroInUse(const CPlusPlus::Document::Ptr &document, unsigned pos);
    void handleLookupItemMatch(const CPlusPlus::Snapshot &snapshot,
                               const CPlusPlus::LookupItem &lookupItem,
                               const CPlusPlus::LookupContext &lookupContext);

    CPPEditorWidget *m_editor;
    CPlusPlus::CppModelManagerInterface *m_modelManager;
    QTextCursor m_tc;
    bool m_lookupBaseClasses;
    QSharedPointer<CppElement> m_element;
    QString m_diagnosis;
};

class CppElement
{
public:
    virtual ~CppElement();

    const TextEditor::HelpItem::Category &helpCategory() const;
    const QStringList &helpIdCandidates() const;
    const QString &helpMark() const;
    const CPPEditorWidget::Link &link() const;
    const QString &tooltip() const;

protected:
    CppElement();

    void setHelpCategory(const TextEditor::HelpItem::Category &category);
    void setLink(const CPPEditorWidget::Link &link);
    void setTooltip(const QString &tooltip);
    void setHelpIdCandidates(const QStringList &candidates);
    void addHelpIdCandidate(const QString &candidate);
    void setHelpMark(const QString &mark);

private:
    TextEditor::HelpItem::Category m_helpCategory;
    QStringList m_helpIdCandidates;
    QString m_helpMark;
    CPPEditorWidget::Link m_link;
    QString m_tooltip;
};

class Unknown : public CppElement
{
public:
    Unknown(const QString &type);
    virtual ~Unknown();

    const QString &type() const;

private:
    QString m_type;
};

class CppInclude : public CppElement
{
public:
    CppInclude(const CPlusPlus::Document::Include &includeFile);
    virtual ~CppInclude();

    const QString &path() const;
    const QString &fileName() const;

private:
    QString m_path;
    QString m_fileName;
};

class CppMacro : public CppElement
{
public:
    CppMacro(const CPlusPlus::Macro &macro);
    virtual ~CppMacro();
};

class CppDeclarableElement : public CppElement
{
public:
    CppDeclarableElement(CPlusPlus::Symbol *declaration);
    virtual ~CppDeclarableElement();

    const QString &name() const;
    const QString &qualifiedName() const;
    const QString &type() const;
    const QIcon &icon() const;

protected:
    void setName(const QString &name);
    void setQualifiedName(const QString &name);
    void setType(const QString &type);
    void setIcon(const QIcon &icon);

private:
    QString m_name;
    QString m_qualifiedName;
    QString m_type;
    QIcon m_icon;
};

class CppNamespace : public CppDeclarableElement
{
public:
    CppNamespace(CPlusPlus::Symbol *declaration);
    virtual ~CppNamespace();
};

class CppClass : public CppDeclarableElement
{
public:
    CppClass(CPlusPlus::Symbol *declaration);
    virtual ~CppClass();

    void lookupBases(CPlusPlus::Symbol *declaration, const CPlusPlus::LookupContext &context);

    const QList<CppClass> &bases() const;

private:
    QList<CppClass> m_bases;
};

class CppFunction : public CppDeclarableElement
{
public:
    CppFunction(CPlusPlus::Symbol *declaration);
    virtual ~CppFunction();
};

class CppEnum : public CppDeclarableElement
{
public:
    CppEnum(CPlusPlus::Symbol *declaration);
    virtual ~CppEnum();
};

class CppTypedef : public CppDeclarableElement
{
public:
    CppTypedef(CPlusPlus::Symbol *declaration);
    virtual ~CppTypedef();
};

class CppVariable : public CppDeclarableElement
{
public:
    CppVariable(CPlusPlus::Symbol *declaration,
                const CPlusPlus::LookupContext &context,
                CPlusPlus::Scope *scope);
    virtual ~CppVariable();
};

class CppTemplate : public CppDeclarableElement
{
public:
    CppTemplate(CPlusPlus::Symbol *declaration);
    virtual ~CppTemplate();

    bool isClassTemplate() const;
    bool isFunctionTemplate() const;

private:
    bool m_isClassTemplate;
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPHIGHLEVELMODEL_H
