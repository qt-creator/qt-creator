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

#include <coreplugin/dialogs/ioptionspage.h>
#include <projectexplorer/kitconfigwidget.h>
#include <projectexplorer/abi.h>
#include <utils/detailswidget.h>
#include <utils/fileutils.h>
#include <utils/pathchooser.h>

#include <QDialog>
#include <QStandardItemModel>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QPushButton;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

// -----------------------------------------------------------------------
// DebuggerKitConfigWidget
// -----------------------------------------------------------------------

class DebuggerKitConfigWidget : public ProjectExplorer::KitConfigWidget
{
    Q_OBJECT

public:
    DebuggerKitConfigWidget(ProjectExplorer::Kit *workingCopy,
                            const ProjectExplorer::KitInformation *ki);
    ~DebuggerKitConfigWidget() override;

    QString displayName() const override;
    QString toolTip() const override;
    void makeReadOnly() override;
    void refresh() override;
    QWidget *buttonWidget() const override;
    QWidget *mainWidget() const override;

private:
    void manageDebuggers();
    void currentDebuggerChanged(int idx);

    QVariant currentId() const;
    void updateComboBox(const QVariant &id);

    bool m_isReadOnly;
    bool m_ignoreChanges;
    QComboBox *m_comboBox;
    QPushButton *m_manageButton;
};

} // namespace Internal
} // namespace Debugger
