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
#include "qml_global.h"
#include "qmlsymbol.h"

namespace QmlEditor {

class QML_EXPORT QmlDocument
{
public:
    typedef QSharedPointer<QmlDocument> Ptr;
    typedef QList<QmlDocument::Ptr> PtrList;
    typedef QMap<QString, Qml::QmlIdSymbol*> IdTable;

protected:
    QmlDocument(const QString &fileName);

public:
    ~QmlDocument();

    static QmlDocument::Ptr create(const QString &fileName);

    QmlJS::AST::UiProgram *program() const;
    QList<QmlJS::DiagnosticMessage> diagnosticMessages() const;

    QString source() const;
    void setSource(const QString &source);

    bool parse();

    bool isParsedCorrectly() const
    { return _parsedCorrectly; }

    IdTable ids() const { return _ids; }

    QString fileName() const { return _fileName; }
    QString path() const { return _path; }
    QString componentName() const { return _componentName; }

    Qml::QmlSymbolFromFile *findSymbol(QmlJS::AST::Node *node) const;
    Qml::QmlSymbol::List symbols() const
    { return _symbols; }

private:
    QmlJS::Engine *_engine;
    QmlJS::NodePool *_pool;
    QmlJS::AST::UiProgram *_program;
    QList<QmlJS::DiagnosticMessage> _diagnosticMessages;
    QString _fileName;
    QString _path;
    QString _componentName;
    QString _source;
    bool _parsedCorrectly;
    IdTable _ids;
    Qml::QmlSymbol::List _symbols;
};

class QML_EXPORT Snapshot: public QMap<QString, QmlDocument::Ptr>
{
public:
    Snapshot();
    ~Snapshot();

    void insert(const QmlDocument::Ptr &document);

    QmlDocument::Ptr document(const QString &fileName) const
    { return value(fileName); }

    QmlDocument::PtrList importedDocuments(const QmlDocument::Ptr &doc, const QString &importPath) const;
    QMap<QString, QmlDocument::Ptr> componentsDefinedByImportedDocuments(const QmlDocument::Ptr &doc, const QString &importPath) const;
};

} // emd of namespace QmlEditor

#endif // QMLDOCUMENT_H
