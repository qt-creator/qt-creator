/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ANDROIDGDBSERVERKITINFORMATION_H
#define ANDROIDGDBSERVERKITINFORMATION_H

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
    ~AndroidGdbServerKitInformationWidget();

    QString displayName() const;
    QString toolTip() const;
    void makeReadOnly();
    void refresh();
    bool visibleInKit();

    QWidget *mainWidget() const;
    QWidget *buttonWidget() const;

private slots:
    void autoDetectDebugger();
    void showDialog();
private:
    QLabel *m_label;
    QPushButton *m_button;
};

class AndroidGdbServerKitInformation : public ProjectExplorer::KitInformation
{
    Q_OBJECT
public:
    AndroidGdbServerKitInformation();

    QVariant defaultValue(ProjectExplorer::Kit *) const;

    QList<ProjectExplorer::Task> validate(const ProjectExplorer::Kit *) const;

    ItemList toUserOutput(const ProjectExplorer::Kit *) const;

    ProjectExplorer::KitConfigWidget *createConfigWidget(ProjectExplorer::Kit *) const;

    static Core::Id id();
    static bool isAndroidKit(const ProjectExplorer::Kit *kit);
    static Utils::FileName gdbServer(const ProjectExplorer::Kit *kit);
    static void setGdbSever(ProjectExplorer::Kit *kit, const Utils::FileName &gdbServerCommand);
    static Utils::FileName autoDetect(ProjectExplorer::Kit *kit);
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDGDBSERVERKITINFORMATION_H
