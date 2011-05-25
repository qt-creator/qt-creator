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
** Nokia at info@qt.nokia.com.
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

namespace RemoteLinux {
namespace Internal {
class AbstractLinuxDeviceDeployStep;
class AbstractMaemoDeployStep;

class MaemoDeployStepBaseWidget : public QWidget
{
    Q_OBJECT

public:
    MaemoDeployStepBaseWidget(AbstractLinuxDeviceDeployStep *step);
    ~MaemoDeployStepBaseWidget();

    void init();
    QString summaryText() const;

signals:
    void updateSummary();

private:
    Q_SLOT void handleDeviceUpdate();
    Q_SLOT void setCurrentDeviceConfig(int index);
    Q_SLOT void showDeviceConfigurations();
    Q_SLOT void handleStepToBeRemoved(int step);

    Ui::MaemoDeployStepWidget *ui;
    AbstractLinuxDeviceDeployStep *const m_step;
};

class MaemoDeployStepWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    MaemoDeployStepWidget(AbstractMaemoDeployStep *step);

private:
    virtual void init() { return m_baseWidget.init(); }
    virtual QString summaryText() const { return m_baseWidget.summaryText(); }
    virtual QString displayName() const { return QString(); }

    MaemoDeployStepBaseWidget m_baseWidget;
};

} // namespace Internal
} // namespace RemoteLinux

#endif // MAEMODEPLOYSTEPWIDGET_H
