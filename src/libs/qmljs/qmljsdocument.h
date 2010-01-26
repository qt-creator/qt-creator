/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
#ifndef QMLDOCUMENT_H
#define QMLDOCUMENT_H

#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QPair>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>

#include "parser/qmljsengine_p.h"
#include "qmljs_global.h"
#include "qmljssymbol.h"

namespace QmlJS {

class QMLJS_EXPORT Document
{
public:
    typedef QSharedPointer<Document> Ptr;
    typedef QList<Document::Ptr> PtrList;
    typedef QMap<QString, IdSymbol*> IdTable;

protected:
    Document(const QString &fileName);

public:
    ~Document();

    static Document::Ptr create(const QString &fileName);

    QmlJS::AST::UiProgram *qmlProgram() const;
    QmlJS::AST::Program *jsProgram() const;
    QmlJS::AST::ExpressionNode *expression() const;
    QmlJS::AST::Node *ast() const;

    QList<QmlJS::DiagnosticMessage> diagnosticMessages() const;

    QString source() const;
    void setSource(const QString &source);

    bool parseQml();
    bool parseJavaScript();
    bool parseExpression();

    bool isParsedCorrectly() const
    { return _parsedCorrectly; }

    int documentRevision() const;
    void setDocumentRevision(int documentRevision);

    IdTable ids() const { return _ids; }

    QString fileName() const { return _fileName; }
    QString path() const { return _path; }
    QString componentName() const { return _componentName; }

    QmlJS::SymbolFromFile *findSymbol(QmlJS::AST::Node *node) const;
    QmlJS::Symbol::List symbols() const
    { return _symbols; }

private:
    QmlJS::Engine *_engine;
    QmlJS::NodePool *_pool;
    QmlJS::AST::Node *_ast;
    int _documentRevision;
    bool _parsedCorrectly;
    QList<QmlJS::DiagnosticMessage> _diagnosticMessages;
    QString _fileName;
    QString _path;
    QString _componentName;
    QString _source;
    IdTable _ids;
    QmlJS::Symbol::List _symbols;
};

class QMLJS_EXPORT Snapshot: public QMap<QString, Document::Ptr>
{
public:
    Snapshot();
    ~Snapshot();

    void insert(const Document::Ptr &document);

    Document::Ptr document(const QString &fileName) const
    { return value(fileName); }

    Document::PtrList importedDocuments(const Document::Ptr &doc, const QString &importPath) const;
    QMap<QString, Document::Ptr> componentsDefinedByImportedDocuments(const Document::Ptr &doc, const QString &importPath) const;
};

} // end of namespace Qml

#endif // QMLDOCUMENT_H
