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
#ifndef DUIDOCUMENT_H
#define DUIDOCUMENT_H

#include <QtCore/QSharedPointer>
#include <QtCore/QMap>
#include <QtCore/QString>

#include "duieditor_global.h"

#include "qmljsengine_p.h"
#include "qmljsastfwd_p.h"

namespace DuiEditor {

class DUIEDITOR_EXPORT DuiDocument
{
public:
	typedef QSharedPointer<DuiDocument> Ptr;

protected:
	DuiDocument(const QString &fileName);

public:
	~DuiDocument();

	static DuiDocument::Ptr create(const QString &fileName);

	QmlJS::AST::UiProgram *program() const;
	QList<QmlJS::DiagnosticMessage> diagnosticMessages() const;

	void setSource(const QString &source);
	bool parse();

    bool isParsedCorrectly() const
    { return _parsedCorrectly; }

    QString fileName() const { return _fileName; }

private:
	QmlJS::Engine *_engine;
	QmlJS::NodePool *_pool;
	QmlJS::AST::UiProgram *_program;
	QList<QmlJS::DiagnosticMessage> _diagnosticMessages;
	QString _fileName;
	QString _source;
    bool _parsedCorrectly;
};

class DUIEDITOR_EXPORT Snapshot: protected QMap<QString, DuiDocument::Ptr>
{
public:
	Snapshot();
	~Snapshot();

    void insert(const DuiDocument::Ptr &document)
    { QMap<QString, DuiDocument::Ptr>::insert(document->fileName(), document); }

    typedef QMapIterator<QString, DuiDocument::Ptr> Iterator;
    Iterator iterator() const
    { return Iterator(*this); }

    DuiDocument::Ptr document(const QString &fileName) const
    { return value(fileName); }
};

} // emd of namespace DuiEditor

#endif // DUIDOCUMENT_H
