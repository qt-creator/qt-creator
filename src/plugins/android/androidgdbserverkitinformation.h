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

#ifndef ANDROIDGDBSERVERKITINFORMATION_H
#define ANDROIDGDBSERVERKITINFORMATION_H

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitconfigwidget.h>

QT_FORWARD_DECLARE_CLASS(QLabel);
QT_FORWARD_DECLARE_CLASS(QPushButton);

namespace ProjectExplorer {
}

namespace Android {
namespace Internal {

class AndroidGdbServerKitInformationWidget : public ProjectExplorer::KitConfigWidget
{
    Q_OBJECT
public:
    AndroidGdbServerKitInformationWidget(ProjectExplorer::Kit *kit, bool sticky);

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
    explicit AndroidGdbServerKitInformation();
    Core::Id dataId() const;

    unsigned int priority() const; // the higher the closer to the top.

    QVariant defaultValue(ProjectExplorer::Kit *) const;

    QList<ProjectExplorer::Task> validate(const ProjectExplorer::Kit *) const;

    ItemList toUserOutput(const ProjectExplorer::Kit *) const;

    ProjectExplorer::KitConfigWidget *createConfigWidget(ProjectExplorer::Kit *) const;

    static Utils::FileName gdbServer(const ProjectExplorer::Kit *kit);
    static void setGdbSever(ProjectExplorer::Kit *kit, const Utils::FileName &gdbServerCommand);
    static Utils::FileName autoDetect(ProjectExplorer::Kit *kit);
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDGDBSERVERKITINFORMATION_H
