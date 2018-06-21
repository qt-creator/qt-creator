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

#include <projectexplorer/namedwidget.h>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLabel;
class QPushButton;
class QTreeView;
class QSortFilterProxyModel;
class QMenu;
QT_END_NAMESPACE

namespace Utils {
class CategorySortFilterModel;
class FancyLineEdit;
class ProgressIndicator;
} // namespace Utils

namespace CMakeProjectManager {

class ConfigModel;

namespace Internal {

class CMakeBuildConfiguration;

class CMakeBuildSettingsWidget : public ProjectExplorer::NamedWidget
{
    Q_OBJECT
public:
    CMakeBuildSettingsWidget(CMakeBuildConfiguration *bc);

    void setError(const QString &message);
    void setWarning(const QString &message);

private:
    void updateButtonState();
    void updateAdvancedCheckBox();
    void updateFromKit();

    void setConfigurationForCMake();
    void updateSelection(const QModelIndex &current, const QModelIndex &previous);
    QAction *createForceAction(int type, const QModelIndex &idx);

    bool eventFilter(QObject *target, QEvent *event);

    CMakeBuildConfiguration *m_buildConfiguration;
    QTreeView *m_configView;
    ConfigModel *m_configModel;
    Utils::CategorySortFilterModel *m_configFilterModel;
    Utils::CategorySortFilterModel *m_configTextFilterModel;
    Utils::ProgressIndicator *m_progressIndicator;
    QPushButton *m_addButton;
    QMenu *m_addButtonMenu;
    QPushButton *m_editButton;
    QPushButton *m_unsetButton;
    QPushButton *m_resetButton;
    QCheckBox *m_showAdvancedCheckBox;
    QPushButton *m_reconfigureButton;
    QTimer m_showProgressTimer;
    Utils::FancyLineEdit *m_filterEdit;
    QLabel *m_errorLabel;
    QLabel *m_warningLabel;
    QLabel *m_errorMessageLabel;
    QLabel *m_warningMessageLabel;
};

} // namespace Internal
} // namespace CMakeProjectManager
