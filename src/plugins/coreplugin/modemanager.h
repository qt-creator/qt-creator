/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef MODEMANAGER_H
#define MODEMANAGER_H

#include <QtCore/QObject>
#include <coreplugin/core_global.h>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Core {

class IMode;

namespace Internal {
    class MainWindow;
    class FancyTabWidget;
}

struct ModeManagerPrivate;

class CORE_EXPORT ModeManager : public QObject
{
    Q_OBJECT

public:
    explicit ModeManager(Internal::MainWindow *mainWindow, Internal::FancyTabWidget *modeStack);
    virtual ~ModeManager();

    void init();
    static ModeManager *instance();

    IMode *currentMode() const;
    IMode *mode(const QString &id) const;

    void addAction(QAction *action, int priority);
    void addProjectSelector(QAction *action);
    void addWidget(QWidget *widget);

    void activateModeType(const QString &type);
    void setModeBarHidden(bool hidden);

signals:
    void currentModeAboutToChange(Core::IMode *mode);

    // the default argument '=0' is important for connects without the oldMode argument.
    void currentModeChanged(Core::IMode *mode, Core::IMode *oldMode = 0);

public slots:
    void activateMode(const QString &id);
    void setFocusToCurrentMode();

private slots:
    void objectAdded(QObject *obj);
    void aboutToRemoveObject(QObject *obj);
    void currentTabAboutToChange(int index);
    void currentTabChanged(int index);
    void updateModeToolTip();
    void enabledStateChanged();

private:
    int indexOf(const QString &id) const;

    ModeManagerPrivate *d;
};

} // namespace Core

#endif // MODEMANAGER_H
