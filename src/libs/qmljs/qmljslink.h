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

#ifndef QMLJSLINK_H
#define QMLJSLINK_H

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/parser/qmljsastfwd_p.h>

#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QCoreApplication>

namespace QmlJS {

class NameId;

/*
    Helper for building a context.
*/
class QMLJS_EXPORT Link
{
    Q_DECLARE_TR_FUNCTIONS(QmlJS::Link)

public:
    // Link all documents in snapshot
    Link(Interpreter::Context *context, const Document::Ptr &doc, const Snapshot &snapshot,
         const QStringList &importPaths);
    ~Link();

    QList<DiagnosticMessage> diagnosticMessages() const;

private:
    Interpreter::Engine *engine();

    void makeComponentChain(
            Document::Ptr doc,
            Interpreter::ScopeChain::QmlComponentChain *target,
            QHash<Document *, Interpreter::ScopeChain::QmlComponentChain *> *components);

    static QList<Document::Ptr> reachableDocuments(Document::Ptr startDoc, const Snapshot &snapshot,
                                                   const QStringList &importPaths);
    static AST::UiQualifiedId *qualifiedTypeNameId(AST::Node *node);

    void linkImports();
    void initializeScopeChain();

    void populateImportedTypes(Interpreter::ObjectValue *typeEnv, Document::Ptr doc);
    void importFile(Interpreter::ObjectValue *typeEnv, Document::Ptr doc,
                    AST::UiImport *import);
    void importNonFile(Interpreter::ObjectValue *typeEnv, Document::Ptr doc,
                       AST::UiImport *import);
    void importObject(Bind *bind, const QString &name, Interpreter::ObjectValue *object, NameId *targetNamespace);

    void error(const Document::Ptr &doc, const AST::SourceLocation &loc, const QString &message);

private:
    Document::Ptr _doc;
    Snapshot _snapshot;
    Interpreter::Context *_context;
    const QStringList _importPaths;

    QList<DiagnosticMessage> _diagnosticMessages;
};

} // namespace QmlJS

#endif // QMLJSLINK_H
