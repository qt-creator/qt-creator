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

#include <utils/id.h>

#include <QAction>
#include <QHash>
#include <QObject>

#include <functional>

namespace Utils {
class InfoBar;
}

namespace CppEditor {
namespace Internal {

class MinimizableInfoBars : public QObject
{
    Q_OBJECT

public:
    using ActionCreator = std::function<QAction *(QWidget *widget)>;

public:
    explicit MinimizableInfoBars(Utils::InfoBar &infoBar);

    void setSettingsGroup(const QString &settingsGroup);
    void createShowInfoBarActions(const ActionCreator &actionCreator) const;

    void processHasProjectPart(bool hasProjectPart);

private:
    void createActions();

    QString settingsKey(const QString &id) const;
    bool showHeaderErrorInfoBar() const;
    void setShowHeaderErrorInfoBar(bool show);
    bool showNoProjectInfoBar() const;
    void setShowNoProjectInfoBar(bool show);

    void updateNoProjectConfiguration();

    void addNoProjectConfigurationEntry(const Utils::Id &id);

private:
    Utils::InfoBar &m_infoBar;
    QString m_settingsGroup;
    QHash<Utils::Id, QAction *> m_actions;

    bool m_hasProjectPart = true;
};

} // namespace Internal
} // namespace CppEditor
