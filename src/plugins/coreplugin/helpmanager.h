/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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
class MainWindow;
}

class CORE_EXPORT HelpManager : public QObject
{
    Q_OBJECT

public:
    typedef QHash<QString, QStringList> Filters;

    static QObject *instance();
    static QString collectionFilePath();

    static void registerDocumentation(const QStringList &fileNames);
    static void unregisterDocumentation(const QStringList &nameSpaces);

    static QMap<QString, QUrl> linksForKeyword(const QString &key);
    static QMap<QString, QUrl> linksForIdentifier(const QString &id);
    static QStringList findKeywords(const QString &key,
                             Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive,
                             int maxHits = INT_MAX);

    static QUrl findFile(const QUrl &url);
    static QByteArray fileData(const QUrl &url);

    static QStringList registeredNamespaces();
    static QString namespaceFromFile(const QString &file);
    static QString fileFromNamespace(const QString &nameSpace);

    static void setCustomValue(const QString &key, const QVariant &value);
    static QVariant customValue(const QString &key, const QVariant &value = QVariant());

    static Filters filters();
    static Filters fixedFilters();

    static Filters userDefinedFilters();
    static void removeUserDefinedFilter(const QString &filter);
    static void addUserDefinedFilter(const QString &filter, const QStringList &attr);

public slots:
    static void handleHelpRequest(const QString &url);

signals:
    void setupFinished();
    void documentationChanged();
    void collectionFileChanged();
    void helpRequested(const QUrl &url);

private:
    explicit HelpManager(QObject *parent = 0);
    ~HelpManager();

    static void setupHelpManager();
    static void verifyDocumenation();
    friend class Core::Internal::CorePlugin; // setupHelpManager
    friend class Core::Internal::MainWindow; // constructor/destructor
};

}   // Core

#endif  // HELPMANAGER_H
