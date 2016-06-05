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

#include <projectexplorer/kitconfigwidget.h>

QT_BEGIN_NAMESPACE
class QComboBox;
class QPushButton;
QT_END_NAMESPACE

namespace QtSupport {
namespace Internal {

class QtKitConfigWidget : public ProjectExplorer::KitConfigWidget
{
    Q_OBJECT

public:
    QtKitConfigWidget(ProjectExplorer::Kit *k, const ProjectExplorer::KitInformation *ki);
    ~QtKitConfigWidget() override;

    QString displayName() const override;

    void makeReadOnly() override;

    void refresh() override;
    QWidget *mainWidget() const override;
    QWidget *buttonWidget() const override;
    QString toolTip() const override;

private:
    void versionsChanged(const QList<int> &added, const QList<int> &removed, const QList<int> &changed);
    void manageQtVersions();
    void currentWasChanged(int idx);
    int findQtVersion(const int id) const;

    QComboBox *m_combo;
    QPushButton *m_manageButton;
};

} // namespace Internal
} // namespace Debugger
