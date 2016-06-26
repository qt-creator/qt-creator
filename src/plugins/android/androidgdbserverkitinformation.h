/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitconfigwidget.h>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
QT_END_NAMESPACE

namespace Android {
namespace Internal {

class AndroidGdbServerKitInformationWidget : public ProjectExplorer::KitConfigWidget
{
    Q_OBJECT
public:
    AndroidGdbServerKitInformationWidget(ProjectExplorer::Kit *kit,
                                         const ProjectExplorer::KitInformation *ki);
    ~AndroidGdbServerKitInformationWidget() override;

    QString displayName() const override;
    QString toolTip() const override;
    void makeReadOnly() override;
    void refresh() override;
    bool visibleInKit() override;

    QWidget *mainWidget() const override;
    QWidget *buttonWidget() const override;

private:
    void autoDetectDebugger();
    void showDialog();

    QLabel *m_label;
    QPushButton *m_button;
};

class AndroidGdbServerKitInformation : public ProjectExplorer::KitInformation
{
    Q_OBJECT
public:
    AndroidGdbServerKitInformation();

    QVariant defaultValue(const ProjectExplorer::Kit *) const override;

    QList<ProjectExplorer::Task> validate(const ProjectExplorer::Kit *) const override;

    ItemList toUserOutput(const ProjectExplorer::Kit *) const override;

    ProjectExplorer::KitConfigWidget *createConfigWidget(ProjectExplorer::Kit *) const override;

    static Core::Id id();
    static bool isAndroidKit(const ProjectExplorer::Kit *kit);
    static Utils::FileName gdbServer(const ProjectExplorer::Kit *kit);
    static void setGdbSever(ProjectExplorer::Kit *kit, const Utils::FileName &gdbServerCommand);
    static Utils::FileName autoDetect(const ProjectExplorer::Kit *kit);
};

} // namespace Internal
} // namespace Android
