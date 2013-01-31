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

#include "qmljslocatordata.h"

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsutils.h>
//#include <qmljs/qmljsinterpreter.h>
#include <qmljs/parser/qmljsast_p.h>

#include <QFileInfo>

using namespace QmlJSTools::Internal;
using namespace QmlJS;
using namespace QmlJS::AST;

LocatorData::LocatorData(QObject *parent)
    : QObject(parent)
{
    QmlJS::ModelManagerInterface *manager = QmlJS::ModelManagerInterface::instance();

    connect(manager, SIGNAL(documentUpdated(QmlJS::Document::Ptr)),
            this, SLOT(onDocumentUpdated(QmlJS::Document::Ptr)));
    connect(manager, SIGNAL(aboutToRemoveFiles(QStringList)),
            this, SLOT(onAboutToRemoveFiles(QStringList)));
}

LocatorData::~LocatorData()
{}

namespace {

class FunctionFinder : protected AST::Visitor
{
    QList<LocatorData::Entry> m_entries;
    Document::Ptr m_doc;
    QString m_context;
    QString m_documentContext;

public:
    FunctionFinder()
    {}

    QList<LocatorData::Entry> run(const Document::Ptr &doc)
    {
        m_doc = doc;
        if (!doc->componentName().isEmpty())
            m_documentContext = doc->componentName();
        else
            m_documentContext = QFileInfo(doc->fileName()).fileName();
        accept(doc->ast(), m_documentContext);
        return m_entries;
    }

protected:
    QString contextString(const QString &extra)
    {
        return QString::fromLatin1("%1, %2").arg(extra, m_documentContext);
    }

    LocatorData::Entry basicEntry(SourceLocation loc)
    {
        LocatorData::Entry entry;
        entry.type = LocatorData::Function;
        entry.extraInfo = m_context;
        entry.fileName = m_doc->fileName();
        entry.line = loc.startLine;
        entry.column = loc.startColumn - 1;
        return entry;
    }

    void accept(Node *ast, const QString &context)
    {
        const QString old = m_context;
        m_context = context;
        Node::accept(ast, this);
        m_context = old;
    }

    bool visit(FunctionDeclaration *ast)
    {
        return visit(static_cast<FunctionExpression *>(ast));
    }

    bool visit(FunctionExpression *ast)
    {
        if (ast->name.isEmpty())
            return true;

        LocatorData::Entry entry = basicEntry(ast->identifierToken);

        entry.type = LocatorData::Function;
        entry.displayName = ast->name.toString();
        entry.displayName += QLatin1Char('(');
        for (FormalParameterList *it = ast->formals; it; it = it->next) {
            if (it != ast->formals)
                entry.displayName += QLatin1String(", ");
            if (!it->name.isEmpty())
                entry.displayName += it->name.toString();
        }
        entry.displayName += QLatin1Char(')');
        entry.symbolName = entry.displayName;

        m_entries += entry;

        accept(ast->body, contextString(QString::fromLatin1("function %1").arg(entry.displayName)));
        return false;
    }

    bool visit(UiScriptBinding *ast)
    {
        if (!ast->qualifiedId)
            return true;
        const QString qualifiedIdString = toString(ast->qualifiedId);

        if (cast<Block *>(ast->statement)) {
            LocatorData::Entry entry = basicEntry(ast->qualifiedId->identifierToken);
            entry.displayName = qualifiedIdString;
            entry.symbolName = qualifiedIdString;
            m_entries += entry;
        }

        accept(ast->statement, contextString(toString(ast->qualifiedId)));
        return false;
    }

    bool visit(UiObjectBinding *ast)
    {
        if (!ast->qualifiedTypeNameId)
            return true;

        QString context = toString(ast->qualifiedTypeNameId);
        const QString id = idOfObject(ast);
        if (!id.isEmpty())
            context = QString::fromLatin1("%1 (%2)").arg(id, context);
        accept(ast->initializer, contextString(context));
        return false;
    }

    bool visit(UiObjectDefinition *ast)
    {
        if (!ast->qualifiedTypeNameId)
            return true;

        QString context = toString(ast->qualifiedTypeNameId);
        const QString id = idOfObject(ast);
        if (!id.isEmpty())
            context = QString::fromLatin1("%1 (%2)").arg(id, context);
        accept(ast->initializer, contextString(context));
        return false;
    }
};
} // anonymous namespace

QHash<QString, QList<LocatorData::Entry> > LocatorData::entries() const
{
    return m_entries;
}

void LocatorData::onDocumentUpdated(const QmlJS::Document::Ptr &doc)
{
    QList<Entry> entries = FunctionFinder().run(doc);
    m_entries.insert(doc->fileName(), entries);
}

void LocatorData::onAboutToRemoveFiles(const QStringList &files)
{
    foreach (const QString &file, files) {
        m_entries.remove(file);
    }
}

