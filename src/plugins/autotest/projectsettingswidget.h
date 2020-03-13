/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QTimer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QTreeWidget;
class QTreeWidgetItem;
QT_END_NAMESPACE

namespace ProjectExplorer { class Project; }

namespace Autotest {

class ITestFramework;

namespace Internal {

class TestProjectSettings;

class ProjectTestSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ProjectTestSettingsWidget(ProjectExplorer::Project *project,
                                       QWidget *parent = nullptr);
private:
    void populateFrameworks(const QMap<Autotest::ITestFramework *, bool> &frameworks);
    void onActiveFrameworkChanged(QTreeWidgetItem *item, int column);
    TestProjectSettings *m_projectSettings;
    QComboBox *m_useGlobalSettings = nullptr;
    QTreeWidget *m_activeFrameworks = nullptr;
    QComboBox *m_runAfterBuild = nullptr;
    QTimer m_syncFrameworksTimer;
};

} // namespace Internal
} // namespace Autotest
