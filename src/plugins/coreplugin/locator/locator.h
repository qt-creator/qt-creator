// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ilocatorfilter.h"
#include "../actionmanager/command.h"

#include <extensionsystem/iplugin.h>
#include <utils/aspects.h>

#include <QtTaskTree/QSingleTaskTreeRunner>

#include <QObject>
#include <QTimer>

namespace Core {
namespace Internal {

class LocatorData;
class LocatorWidget;

struct LocatorSettings : public Utils::AspectContainer
{
    LocatorSettings();

    Utils::BoolAspect useCenteredPopup{this};
    Utils::BoolAspect useTabCompletion{this};
    Utils::BoolAspect relativePaths{this};
    Utils::IntegerAspect refreshInterval{this}; // minutes
};

LocatorSettings &locatorSettings();

class Locator : public QObject
{
    Q_OBJECT

public:
    Locator();
    ~Locator() override;

    static Locator *instance();
    void aboutToShutdown();

    void initialize();
    void extensionsInitialized();
    bool delayedInitialize();

    static QList<ILocatorFilter *> filters();
    QList<ILocatorFilter *> customFilters();
    void setFilters(QList<ILocatorFilter *> f);
    void setCustomFilters(QList<ILocatorFilter *> f);

    static void showFilter(ILocatorFilter *filter, LocatorWidget *widget);

signals:
    void filtersChanged();

public slots:
    void refresh(const QList<ILocatorFilter *> &filters);
    void saveSettings() const;

private:
    void loadSettings();
    void updateFilterActions();
    void updateEditorManagerPlaceholderText();

    LocatorData *m_locatorData = nullptr;

    bool m_settingsInitialized = false;
    LocatorSettings m_settings;
    QList<ILocatorFilter *> m_filters;
    QList<ILocatorFilter *> m_customFilters;
    QMap<Utils::Id, QAction *> m_filterActionMap;
    QTimer m_refreshTimer;
    QSingleTaskTreeRunner m_taskTreeRunner;
    QList<ILocatorFilter *> m_refreshingFilters;
};

} // namespace Internal
} // namespace Core
