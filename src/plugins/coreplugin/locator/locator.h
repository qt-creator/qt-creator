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

#include "ilocatorfilter.h"
#include "locatorconstants.h"

#include <coreplugin/actionmanager/command.h>

#include <QFuture>
#include <QObject>
#include <QTimer>

namespace Core {
namespace Internal {

class LocatorData;

class Locator : public QObject
{
    Q_OBJECT

public:
    Locator();
    ~Locator() override;

    static Locator *instance();

    void initialize();
    void extensionsInitialized();
    bool delayedInitialize();

    static QList<ILocatorFilter *> filters();
    QList<ILocatorFilter *> customFilters();
    void setFilters(QList<ILocatorFilter *> f);
    void setCustomFilters(QList<ILocatorFilter *> f);
    int refreshInterval() const;
    void setRefreshInterval(int interval);

signals:
    void filtersChanged();

public slots:
    void refresh(QList<ILocatorFilter *> filters);
    void saveSettings() const;

private:
    void loadSettings();
    void updateFilterActions();
    void updateEditorManagerPlaceholderText();

    LocatorData *m_locatorData = nullptr;

    bool m_settingsInitialized = false;
    QList<ILocatorFilter *> m_filters;
    QList<ILocatorFilter *> m_customFilters;
    QMap<Utils::Id, QAction *> m_filterActionMap;
    QTimer m_refreshTimer;
    QFuture<void> m_refreshTask;
    QList<ILocatorFilter *> m_refreshingFilters;
};

} // namespace Internal
} // namespace Core
