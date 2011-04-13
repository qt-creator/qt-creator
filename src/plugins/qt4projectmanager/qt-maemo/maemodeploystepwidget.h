/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#ifndef MAEMODEPLOYSTEPWIDGET_H
#define MAEMODEPLOYSTEPWIDGET_H

#include <projectexplorer/buildstep.h>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MaemoDeployStepWidget;
}
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {
class MaemoDeployStep;

class MaemoDeployStepWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    MaemoDeployStepWidget(MaemoDeployStep *step);
    ~MaemoDeployStepWidget();

private:
    Q_SLOT void handleDeviceUpdate();
    Q_SLOT void setCurrentDeviceConfig(int index);
    Q_SLOT void setDeployToSysroot(bool doDeloy);
    Q_SLOT void setModel(int row);
    Q_SLOT void handleModelListToBeReset();
    Q_SLOT void handleModelListReset();
    Q_SLOT void addDesktopFile();
    Q_SLOT void addIcon();
    Q_SLOT void showDeviceConfigurations();

    virtual void init();
    virtual QString summaryText() const;
    virtual QString displayName() const;

    Ui::MaemoDeployStepWidget *ui;
    MaemoDeployStep * const m_step;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMODEPLOYSTEPWIDGET_H
