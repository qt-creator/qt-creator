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

#ifndef EXTERNALTOOLMANAGER_H
#define EXTERNALTOOLMANAGER_H

#include "core_global.h"

#include <QObject>
#include <QMap>
#include <QList>
#include <QString>

QT_FORWARD_DECLARE_CLASS(QAction)

namespace Core {
class ActionContainer;

namespace Internal {
class ExternalToolRunner;
class ExternalTool;
}

class CORE_EXPORT ExternalToolManager : public QObject
{
    Q_OBJECT

public:
    static ExternalToolManager *instance() { return m_instance; }

    ExternalToolManager();
    ~ExternalToolManager();

    QMap<QString, QList<Internal::ExternalTool *> > toolsByCategory() const;
    QMap<QString, Internal::ExternalTool *> toolsById() const;

    void setToolsByCategory(const QMap<QString, QList<Internal::ExternalTool *> > &tools);

signals:
    void replaceSelectionRequested(const QString &text);

private slots:
    void menuActivated();
    void openPreferences();

private:
    void initialize();
    void parseDirectory(const QString &directory,
                        QMap<QString, QMultiMap<int, Internal::ExternalTool*> > *categoryMenus,
                        QMap<QString, Internal::ExternalTool *> *tools,
                        bool isPreset = false);
    void readSettings(const QMap<QString, Internal::ExternalTool *> &tools,
                      QMap<QString, QList<Internal::ExternalTool*> > *categoryPriorityMap);
    void writeSettings();

    static ExternalToolManager *m_instance;
    QMap<QString, Internal::ExternalTool *> m_tools;
    QMap<QString, QList<Internal::ExternalTool *> > m_categoryMap;
    QMap<QString, QAction *> m_actions;
    QMap<QString, ActionContainer *> m_containers;
    QAction *m_configureSeparator;
    QAction *m_configureAction;

    // for sending the replaceSelectionRequested signal
    friend class Core::Internal::ExternalToolRunner;
};

} // Core


#endif // EXTERNALTOOLMANAGER_H
