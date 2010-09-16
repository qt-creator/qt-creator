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

#ifndef HELPMANAGER_H
#define HELPMANAGER_H

#include "core_global.h"

#include <QtCore/QScopedPointer>

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/QMap>
#include <QtCore/QHash>

QT_FORWARD_DECLARE_CLASS(QUrl)

namespace Core {
struct HelpManagerPrivate;

class CORE_EXPORT HelpManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(HelpManager)

public:
    typedef QHash<QString, QStringList> Filters;
    explicit HelpManager(QObject *parent = 0);
    virtual ~HelpManager();

    static HelpManager* instance();
    static QString collectionFilePath();

    void registerDocumentation(const QStringList &fileNames);
    void unregisterDocumentation(const QStringList &nameSpaces);

    QMap<QString, QUrl> linksForKeyword(const QString &key) const;
    QMap<QString, QUrl> linksForIdentifier(const QString &id) const;
    QStringList findKeywords(const QString &key, int maxHits = INT_MAX) const;

    QUrl findFile(const QUrl &url) const;
    QByteArray fileData(const QUrl &url) const;
    void handleHelpRequest(const QString &url);

    QStringList registeredNamespaces() const;
    QString namespaceFromFile(const QString &file) const;
    QString fileFromNamespace(const QString &nameSpace) const;

    void setCustomValue(const QString &key, const QVariant &value);
    QVariant customValue(const QString &key, const QVariant &value = QVariant()) const;

    Filters filters() const;
    Filters fixedFilters() const;

    Filters userDefinedFilters() const;
    void removeUserDefinedFilter(const QString &filter);
    void addUserDefinedFilter(const QString &filter, const QStringList &attr);

signals:
    void setupFinished();
    void documentationChanged();
    void collectionFileChanged();
    void helpRequested(const QUrl &url);

private slots:
    void setupHelpManager();
    void collectionFileModified();

private:
    void verifyDocumenation();

    QScopedPointer<HelpManagerPrivate> d;
};

}   // Core

#endif  // HELPMANAGER_H
