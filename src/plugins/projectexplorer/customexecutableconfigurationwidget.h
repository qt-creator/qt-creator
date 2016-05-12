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

#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLineEdit;
class QComboBox;
class QLabel;
class QAbstractButton;
QT_END_NAMESPACE

namespace Utils {
class DetailsWidget;
class PathChooser;
}

namespace ProjectExplorer {

class ArgumentsAspect;
class TerminalAspect;
class CustomExecutableRunConfiguration;

namespace Internal {

class CustomExecutableConfigurationWidget : public QWidget
{
    Q_OBJECT

public:
    enum ApplyMode { InstantApply, DelayedApply};
    CustomExecutableConfigurationWidget(CustomExecutableRunConfiguration *rc, ApplyMode mode);
    ~CustomExecutableConfigurationWidget();

    void apply(); // only used for DelayedApply
    bool isValid() const;

signals:
    void validChanged();

private:
    void changed();
    void executableEdited();
    void workingDirectoryEdited();
    void environmentWasChanged();

    bool m_ignoreChange = false;
    CustomExecutableRunConfiguration *m_runConfiguration = 0;
    ProjectExplorer::ArgumentsAspect *m_temporaryArgumentsAspect = 0;
    ProjectExplorer::TerminalAspect *m_temporaryTerminalAspect = 0;
    Utils::PathChooser *m_executableChooser = 0;
    Utils::PathChooser *m_workingDirectory = 0;
    Utils::DetailsWidget *m_detailsContainer = 0;
};

} // namespace Internal
} // namespace ProjectExplorer
