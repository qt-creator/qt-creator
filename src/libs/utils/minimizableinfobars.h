// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "id.h"
#include "infobar.h"
#include "store.h"

#include <QHash>
#include <QObject>

#include <functional>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT MinimizableInfoBars : public QObject
{
public:
    using ActionCreator = std::function<QAction *(QWidget *widget)>;

public:
    explicit MinimizableInfoBars(InfoBar &infoBar);

    void setSettingsGroup(const Key &settingsGroup);
    void setPossibleInfoBarEntries(const QList<InfoBarEntry> &entries);

    void createShowInfoBarActions(const ActionCreator &actionCreator) const;

    void setInfoVisible(const Id &id, bool visible);

private:
    void createActions();

    Key settingsKey(const Id &id) const;
    bool showInInfoBar(const Id &id) const;
    void setShowInInfoBar(const Id &id, bool show);

    void updateInfo(const Id &id);

    void showInfoBar(const Id &id);

    InfoBar &m_infoBar;
    Key m_settingsGroup;
    QHash<Id, QAction *> m_actions;
    QHash<Id, bool> m_isInfoVisible;
    QHash<Id, InfoBarEntry> m_infoEntries;
};

} // namespace Utils
