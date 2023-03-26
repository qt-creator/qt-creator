// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "perfsettings.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QProcess>

QT_BEGIN_NAMESPACE
class QPushButton;
class QTableView;
QT_END_NAMESPACE

namespace Utils { class QtcProcess; }

namespace PerfProfiler {
namespace Internal {

class PerfConfigWidget : public Core::IOptionsPageWidget
{
    Q_OBJECT
public:
    explicit PerfConfigWidget(PerfSettings *settings, QWidget *parent = nullptr);
    ~PerfConfigWidget();

    void updateUi();
    void setTarget(ProjectExplorer::Target *target);
    void setTracePointsButtonVisible(bool visible);

private:
    void apply() final;

    void readTracePoints();
    void handleProcessDone();

    PerfSettings *m_settings;
    std::unique_ptr<Utils::QtcProcess> m_process;

    QTableView *eventsView;
    QPushButton *useTracePointsButton;
    QPushButton *addEventButton;
    QPushButton *removeEventButton;
    QPushButton *resetButton;
};

} // namespace Internal
} // namespace PerfProfiler
