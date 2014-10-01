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
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef LOCALHELPMANAGER_H
#define LOCALHELPMANAGER_H

#include <QMutex>
#include <QObject>
#include <QUrl>
#include <QMetaType>

QT_FORWARD_DECLARE_CLASS(QHelpEngine)

class BookmarkManager;

namespace Help {
namespace Internal {

class LocalHelpManager : public QObject
{
    Q_OBJECT

public:
    struct HelpData {
        QUrl resolvedUrl;
        QByteArray data;
        QString mimeType;
    };

    LocalHelpManager(QObject *parent = 0);
    ~LocalHelpManager();

    static LocalHelpManager *instance();

    void setupGuiHelpEngine();
    void setEngineNeedsUpdate();

    static QHelpEngine& helpEngine();
    static BookmarkManager& bookmarkManager();

    static QVariant engineFontSettings();
    static bool isValidUrl(const QString &link);

    static QByteArray loadErrorMessage(const QUrl &url, const QString &errorString);
    Q_INVOKABLE static Help::Internal::LocalHelpManager::HelpData helpData(const QUrl &url);

private:
    bool m_guiNeedsSetup;
    bool m_needsCollectionFile;

    static QMutex m_guiMutex;
    static QHelpEngine *m_guiEngine;

    static QMutex m_bkmarkMutex;
    static BookmarkManager *m_bookmarkManager;
};

}   // Internal
}   // Help

Q_DECLARE_METATYPE(Help::Internal::LocalHelpManager::HelpData)

#endif  // LOCALHELPMANAGER_H
