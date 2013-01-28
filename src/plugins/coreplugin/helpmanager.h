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

#ifndef HELPMANAGER_H
#define HELPMANAGER_H

#include "core_global.h"

#include <QObject>
#include <QStringList>
#include <QVariant>
#include <QMap>
#include <QHash>

QT_FORWARD_DECLARE_CLASS(QUrl)

namespace Core {
struct HelpManagerPrivate;

namespace Internal {
class CorePlugin;
}

class CORE_EXPORT HelpManager : public QObject
{
    Q_OBJECT

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

public slots:
    void handleHelpRequest(const QString &url);

signals:
    void setupFinished();
    void documentationChanged();
    void collectionFileChanged();
    void helpRequested(const QUrl &url);

private:
    void setupHelpManager();
    void verifyDocumenation();
    HelpManagerPrivate *d;
    friend class Internal::CorePlugin; // setupHelpManager
};

}   // Core

#endif  // HELPMANAGER_H
