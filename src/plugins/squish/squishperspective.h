// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "squishconstants.h"

#include <debugger/debuggermainwindow.h>

#include <utils/treemodel.h>

namespace Squish {
namespace Internal {

class SquishXmlOutputHandler;

class LocalsItem : public Utils::TreeItem
{
public:
    LocalsItem() = default;
    LocalsItem(const QString &n, const QString &t, const QString &v) : name(n), type(t), value(v) {}
    QVariant data(int column, int role) const override;
    QString name;
    QString type;
    QString value;
    bool expanded = false;
};

class SquishPerspective : public Utils::Perspective
{
    Q_OBJECT
public:
    enum PerspectiveMode { NoMode, Interrupted, Running, Recording, Querying };

    SquishPerspective();
    void initPerspective();
    void setPerspectiveMode(PerspectiveMode mode);
    PerspectiveMode perspectiveMode() const { return m_mode; }

    void updateStatus(const QString &status);

    void showControlBar(SquishXmlOutputHandler *xmlOutputHandler);
    void destroyControlBar();

signals:
    void stopRequested();
    void stopRecordRequested();
    void interruptRequested();
    void runRequested(StepMode mode);

private:
    void onStopTriggered();
    void onStopRecordTriggered();
    void onPausePlayTriggered();
    void onLocalsUpdated(const QString &output);

    QAction *m_stopRecordAction = nullptr;
    QAction *m_pausePlayAction = nullptr;
    QAction *m_stepInAction = nullptr;
    QAction *m_stepOverAction = nullptr;
    QAction *m_stepOutAction = nullptr;
    QAction *m_stopAction = nullptr;
    QLabel *m_status = nullptr;
    class SquishControlBar *m_controlBar = nullptr;
    Utils::TreeModel<LocalsItem> m_localsModel;
    PerspectiveMode m_mode = NoMode;

    friend class SquishControlBar;
};

} // namespace Internal
} // namespace Squish

