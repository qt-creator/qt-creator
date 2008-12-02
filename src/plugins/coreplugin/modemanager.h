/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/

#ifndef MODEMANAGER_H
#define MODEMANAGER_H

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QVector>

#include <coreplugin/core_global.h>

QT_BEGIN_NAMESPACE
class QSignalMapper;
class QMenu;
QT_END_NAMESPACE

namespace Core {

class ICommand;
class IMode;

namespace Internal {
class FancyTabWidget;
class FancyActionBar;
class MainWindow;
} // namespace Internal

class CORE_EXPORT ModeManager : public QObject
{
    Q_OBJECT

public:
    ModeManager(Internal::MainWindow *mainWindow, Internal::FancyTabWidget *modeStack);

    void init();
    static ModeManager *instance() { return m_instance; }

    IMode* currentMode() const;
    IMode* mode(const QString &id) const;

    void addAction(ICommand *command, int priority, QMenu *menu = 0);
    void addWidget(QWidget *widget);

signals:
    void currentModeAboutToChange(Core::IMode *mode);
    void currentModeChanged(Core::IMode *mode);

public slots:
    void activateMode(const QString &id);
    void setFocusToCurrentMode();

private slots:
    void objectAdded(QObject *obj);
    void aboutToRemoveObject(QObject *obj);
    void currentTabAboutToChange(int index);
    void currentTabChanged(int index);
    void updateModeToolTip();

private:
    int indexOf(const QString &id) const;

    static ModeManager *m_instance;
    Internal::MainWindow *m_mainWindow;
    Internal::FancyTabWidget *m_modeStack;
    Internal::FancyActionBar *m_actionBar;
    QMap<ICommand*, int> m_actions;
    QVector<IMode*> m_modes;
    QVector<ICommand*> m_modeShortcuts;
    QSignalMapper *m_signalMapper;
    QList<int> m_addedContexts;
};

} // namespace Core

#endif // MODEMANAGER_H
