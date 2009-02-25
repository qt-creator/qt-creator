/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef QT4BUILDENVIRONMENTWIDGET_H
#define QT4BUILDENVIRONMENTWIDGET_H

#include <projectexplorer/buildstep.h>

namespace ProjectExplorer {
class EnvironmentModel;
}

namespace Qt4ProjectManager {

class Qt4Project;

namespace Internal {

namespace Ui {
class Qt4BuildEnvironmentWidget;
}

class Qt4BuildEnvironmentWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    Qt4BuildEnvironmentWidget(Qt4Project *project);
    ~Qt4BuildEnvironmentWidget();

    QString displayName() const;
    void init(const QString &buildConfiguration);

private slots:
    void environmentModelUserChangesUpdated();
    void editEnvironmentButtonClicked();
    void addEnvironmentButtonClicked();
    void removeEnvironmentButtonClicked();
    void unsetEnvironmentButtonClicked();
    void switchEnvironmentTab(int newTab);
    void environmentCurrentIndexChanged(const QModelIndex &current, const QModelIndex &previous);
    void clearSystemEnvironmentCheckBoxClicked(bool);
    void updateButtonsEnabled();

private:
    Ui::Qt4BuildEnvironmentWidget *m_ui;
    Qt4Project *m_pro;
    ProjectExplorer::EnvironmentModel *m_environmentModel;
    QString m_buildConfiguration;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QT4BUILDENVIRONMENTWIDGET_H
