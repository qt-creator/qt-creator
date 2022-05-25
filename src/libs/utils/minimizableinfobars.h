/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "utils_global.h"

#include "id.h"
#include "infobar.h"

#include <QHash>
#include <QObject>

#include <functional>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT MinimizableInfoBars : public QObject
{
    Q_OBJECT

public:
    using ActionCreator = std::function<QAction *(QWidget *widget)>;

public:
    explicit MinimizableInfoBars(InfoBar &infoBar);

    void setSettingsGroup(const QString &settingsGroup);
    void setPossibleInfoBarEntries(const QList<InfoBarEntry> &entries);

    void createShowInfoBarActions(const ActionCreator &actionCreator) const;

    void setInfoVisible(const Id &id, bool visible);

private:
    void createActions();

    QString settingsKey(const Id &id) const;
    bool showInInfoBar(const Id &id) const;
    void setShowInInfoBar(const Id &id, bool show);

    void updateInfo(const Id &id);

    void showInfoBar(const Id &id);

private:
    InfoBar &m_infoBar;
    QString m_settingsGroup;
    QHash<Id, QAction *> m_actions;
    QHash<Id, bool> m_isInfoVisible;
    QHash<Id, InfoBarEntry> m_infoEntries;
};

} // namespace Utils
