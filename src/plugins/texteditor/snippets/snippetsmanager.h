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

#ifndef SNIPPETSMANAGER_H
#define SNIPPETSMANAGER_H

#include "snippet.h"

#include <QtCore/QString>
#include <QtCore/QSharedPointer>
#include <QtCore/QList>

QT_FORWARD_DECLARE_CLASS(QXmlStreamWriter)

namespace TextEditor {
namespace Internal {

class SnippetsCollection;

class SnippetsManager
{
private:
    SnippetsManager();
    Q_DISABLE_COPY(SnippetsManager)

public:
    ~SnippetsManager();

    static SnippetsManager *instance();

    void reloadSnippetsCollection();
    void persistSnippetsCollection();
    QSharedPointer<SnippetsCollection> snippetsCollection();

private:
    void loadSnippetsCollection();

    static QList<Snippet> readXML(const QString &fileName);
    static void writeSnippetXML(const Snippet &snippet, QXmlStreamWriter *writer);

    static const QLatin1String kSnippet;
    static const QLatin1String kSnippets;
    static const QLatin1String kTrigger;
    static const QLatin1String kId;
    static const QLatin1String kComplement;
    static const QLatin1String kGroup;
    static const QLatin1String kRemoved;
    static const QLatin1String kModified;

    bool m_collectionLoaded;
    QSharedPointer<SnippetsCollection> m_collection;
    QString m_builtInSnippetsPath;
    QString m_userSnippetsPath;
    QString m_snippetsFileName;
};

} // Internal
} // TextEditor

#endif // SNIPPETSMANAGER_H
