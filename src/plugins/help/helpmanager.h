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

#include "help_global.h"

#include <QtCore/QMutex>
#include <QtCore/QObject>

QT_FORWARD_DECLARE_CLASS(QHelpEngine)
QT_FORWARD_DECLARE_CLASS(QHelpEngineCore)
QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QStringList)

class BookmarkManager;

namespace Help {

class HELP_EXPORT HelpManager : public QObject
{
    Q_OBJECT
public:
    HelpManager(QObject *parent = 0);
    ~HelpManager();

    static HelpManager& instance();

    void setupGuiHelpEngine();
    bool guiEngineNeedsUpdate() const;

    void handleHelpRequest(const QString &url);

    void verifyDocumenation();
    void registerDocumentation(const QStringList &fileNames);
    void unregisterDocumentation(const QStringList &nameSpaces);

    static QHelpEngine& helpEngine();
    static QString collectionFilePath();
    static QHelpEngineCore& helpEngineCore();

    static BookmarkManager& bookmarkManager();

signals:
    void helpRequested(const QString &Url);

private:
    static bool m_guiNeedsSetup;
    static bool m_needsCollectionFile;

    static QMutex m_guiMutex;
    static QHelpEngine *m_guiEngine;

    static QMutex m_coreMutex;
    static QHelpEngineCore *m_coreEngine;

    static HelpManager *m_helpManager;
    static BookmarkManager *m_bookmarkManager;
};

}   // Help

#endif  // HELPMANAGER_H
