/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmljslocatordata.h"

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/parser/qmljsast_p.h>

using namespace QmlJSTools::Internal;
using namespace QmlJS;
using namespace QmlJS::Interpreter;
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
static QString findId(UiObjectInitializer *initializer)
{
    if (!initializer)
        return QString();
    for (UiObjectMemberList *member = initializer->members; member; member = member->next) {
        if (UiScriptBinding *script = cast<UiScriptBinding *>(member->member)) {
            if (!script->qualifiedId || !script->qualifiedId->name || script->qualifiedId->next)
                continue;
            if (script->qualifiedId->name->asString() != QLatin1String("id"))
                continue;
            if (ExpressionStatement *expStmt = cast<ExpressionStatement *>(script->statement)) {
                if (IdentifierExpression *identExp = cast<IdentifierExpression *>(expStmt->expression)) {
                    if (identExp->name)
                        return identExp->name->asString();
                }
            }
        }
    }
    return QString();
}

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
        if (!doc->componentName().isEmpty()) {
            m_documentContext = doc->componentName();
        } else {
            m_documentContext = QFileInfo(doc->fileName()).fileName();
        }
        accept(doc->ast(), m_documentContext);
        return m_entries;
    }

protected:
    QString contextString(const QString &extra)
    {
        return QString("%1, %2").arg(extra, m_documentContext);
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
        if (!ast->name)
            return true;

        LocatorData::Entry entry = basicEntry(ast->identifierToken);

        entry.type = LocatorData::Function;
        entry.displayName = ast->name->asString();
        entry.displayName += QLatin1Char('(');
        for (FormalParameterList *it = ast->formals; it; it = it->next) {
            if (it != ast->formals)
                entry.displayName += QLatin1String(", ");
            if (it->name)
                entry.displayName += it->name->asString();
        }
        entry.displayName += QLatin1Char(')');
        entry.symbolName = entry.displayName;

        m_entries += entry;

        accept(ast->body, contextString(QString("function %1").arg(entry.displayName)));
        return false;
    }

    bool visit(UiScriptBinding *ast)
    {
        if (!ast->qualifiedId)
            return true;
        const QString qualifiedIdString = Bind::toString(ast->qualifiedId);

        if (cast<Block *>(ast->statement)) {
            LocatorData::Entry entry = basicEntry(ast->qualifiedId->identifierToken);
            entry.displayName = qualifiedIdString;
            entry.symbolName = qualifiedIdString;
            m_entries += entry;
        }

        accept(ast->statement, contextString(Bind::toString(ast->qualifiedId)));
        return false;
    }

    bool visit(UiObjectBinding *ast)
    {
        if (!ast->qualifiedTypeNameId)
            return true;

        QString context = Bind::toString(ast->qualifiedTypeNameId);
        const QString id = findId(ast->initializer);
        if (!id.isEmpty())
            context = QString("%1 (%2)").arg(id, context);
        accept(ast->initializer, contextString(context));
        return false;
    }

    bool visit(UiObjectDefinition *ast)
    {
        if (!ast->qualifiedTypeNameId)
            return true;

        QString context = Bind::toString(ast->qualifiedTypeNameId);
        const QString id = findId(ast->initializer);
        if (!id.isEmpty())
            context = QString("%1 (%2)").arg(id, context);
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

