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

namespace QmlJS {

class Bind;
class Snapshot;

class QMLJS_EXPORT Document
{
public:
    typedef QSharedPointer<Document> Ptr;

protected:
    Document(const QString &fileName);

public:
    ~Document();

    static Document::Ptr create(const QString &fileName);

    AST::UiProgram *qmlProgram() const;
    AST::Program *jsProgram() const;
    AST::ExpressionNode *expression() const;
    AST::Node *ast() const;

    QList<DiagnosticMessage> diagnosticMessages() const;

    QString source() const;
    void setSource(const QString &source);

    bool parseQml();
    bool parseJavaScript();
    bool parseExpression();

    bool isParsedCorrectly() const
    { return _parsedCorrectly; }

    Bind *bind() const;

    int documentRevision() const;
    void setDocumentRevision(int documentRevision);

    QString fileName() const;
    QString path() const;
    QString componentName() const;

private:
    bool parse_helper(int kind);

private:
    QmlJS::Engine *_engine;
    NodePool *_pool;
    AST::Node *_ast;
    Bind *_bind;
    int _documentRevision;
    bool _parsedCorrectly;
    QList<QmlJS::DiagnosticMessage> _diagnosticMessages;
    QString _fileName;
    QString _path;
    QString _componentName;
    QString _source;

    // for documentFromSource
    friend class Snapshot;
};

class QMLJS_EXPORT Snapshot
{
    typedef QMap<QString, Document::Ptr> _Base;
    QMap<QString, Document::Ptr> _documents;

public:
    Snapshot();
    ~Snapshot();

    typedef _Base::iterator iterator;
    typedef _Base::const_iterator const_iterator;

    const_iterator begin() const { return _documents.begin(); }
    const_iterator end() const { return _documents.end(); }

    void insert(const Document::Ptr &document);

    Document::Ptr document(const QString &fileName) const
    { return _documents.value(fileName); }

    Document::Ptr documentFromSource(const QString &code,
                                     const QString &fileName) const;

    QList<Document::Ptr> importedDocuments(const Document::Ptr &doc, const QString &importPath) const;
    QMap<QString, Document::Ptr> componentsDefinedByImportedDocuments(const Document::Ptr &doc, const QString &importPath) const;
};

} // end of namespace Qml

#endif // QMLDOCUMENT_H
