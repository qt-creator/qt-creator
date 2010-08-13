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

#ifndef CPPHIGHLEVELMODEL_H
#define CPPHIGHLEVELMODEL_H

#include "cppeditor.h"
#include "cpphoverhandler.h"

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

class CPPEditor;
class CppElement;

class CppElementEvaluator
{
public:
    CppElementEvaluator(CPPEditor *editor);

    void setTextCursor(const QTextCursor &tc);
    void setLookupBaseClasses(const bool lookup);

    QSharedPointer<CppElement> identifyCppElement();

private:
    void evaluate();
    bool matchDiagnosticMessage(const CPlusPlus::Document::Ptr &document, unsigned line);
    bool matchIncludeFile(const CPlusPlus::Document::Ptr &document, unsigned line);
    bool matchMacroInUse(const CPlusPlus::Document::Ptr &document, unsigned pos);
    void handleLookupItemMatch(const CPlusPlus::Snapshot &snapshot,
                               const CPlusPlus::LookupItem &lookupItem,
                               const CPlusPlus::LookupContext &lookupContext);

    CPPEditor *m_editor;
    CppTools::CppModelManagerInterface *m_modelManager;
    QTextCursor m_tc;
    bool m_lookupBaseClasses;
    QSharedPointer<CppElement> m_element;
};

class CppElement
{
public:
    virtual ~CppElement();

    const CppHoverHandler::HelpCandidate::Category &helpCategory() const;
    const QStringList &helpIdCandidates() const;
    const QString &helpMark() const;
    const CPPEditor::Link &link() const;
    const QString &tooltip() const;

protected:
    CppElement();

    void setHelpCategory(const CppHoverHandler::HelpCandidate::Category &category);
    void setLink(const CPPEditor::Link &link);
    void setTooltip(const QString &tooltip);
    void setHelpIdCandidates(const QStringList &candidates);
    void addHelpIdCandidate(const QString &candidate);
    void setHelpMark(const QString &mark);

private:
    CppHoverHandler::HelpCandidate::Category m_helpCategory;
    QStringList m_helpIdCandidates;
    QString m_helpMark;
    CPPEditor::Link m_link;
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

class CppDiagnosis : public CppElement
{
public:
    CppDiagnosis(const CPlusPlus::Document::DiagnosticMessage &message);
    virtual ~CppDiagnosis();

    const QString &text() const;

private:
    QString m_text;
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

} // namespace Internal
} // namespace CppEditor

#endif // CPPHIGHLEVELMODEL_H
