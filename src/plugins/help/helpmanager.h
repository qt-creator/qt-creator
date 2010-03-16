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

#include <QtCore/QObject>

QT_FORWARD_DECLARE_CLASS(QHelpEngineCore)
QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QStringList)

namespace Help {
namespace Internal {
class HelpPlugin;
}   // Internal

class HELP_EXPORT HelpManager : public QObject
{
    Q_OBJECT
public:
    HelpManager(Internal::HelpPlugin *plugin);
    ~HelpManager();

    void handleHelpRequest(const QString &url);
    void registerDocumentation(const QStringList &fileNames);

    static QString collectionFilePath();
    static QHelpEngineCore& helpEngineCore();

signals:
    void registerDocumentation();

private:
    Internal::HelpPlugin *m_plugin;

    static QHelpEngineCore* m_coreEngine;
};

}   // Help

#endif  // HELPMANAGER_H
