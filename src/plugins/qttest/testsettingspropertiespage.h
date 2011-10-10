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

#ifndef TESTSETTINGSPROPERTIESPAGE_H
#define TESTSETTINGSPROPERTIESPAGE_H

#include "testsettings.h"
#include "ui_testsettingspropertiespage.h"

#include <projectexplorer/iprojectproperties.h>

class TestConfig;

namespace QtTest {

namespace Internal {

const char * const TESTSETTINGS_PANEL_ID("QtTest.TestSettingsPanel");

class TestSettingsPanelFactory : public ProjectExplorer::IProjectPanelFactory
{
public:
    QString id() const;
    QString displayName() const;
    ProjectExplorer::PropertiesPanel *createPanel(ProjectExplorer::Project *project);
    bool supports(ProjectExplorer::Project *project);
};

class TestSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    TestSettingsWidget(ProjectExplorer::Project *project);
    virtual ~TestSettingsWidget();
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);

public slots:
    void onChanged();
    void onUploadScpChanged();
    void onUploadEMailChanged();
    void onAddDirectoryBtnClicked();
    void onClearDirectoryBtnClicked();
    void onSettingsChanged();

private:
    QStringList extraDirectories() const;
    void setExtraDirectories(const QStringList list);

    TestSettings m_testSettings;
    Ui::TestSettingsPropertiesPage m_ui;
    ProjectExplorer::Project *m_project;
    TestConfig *m_testConfig;
    QButtonGroup m_uploadMode;
    QButtonGroup m_uploadMethod;
};

} // namespace Internal
} // namespace QtTest

#endif // TESTSETTINGSPROPERTIESPAGE_H
