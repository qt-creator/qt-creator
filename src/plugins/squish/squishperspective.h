// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

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
    enum class State {
        None,
        Starting,
        Running,
        RunRequested,
        StepInRequested,
        StepOverRequested,
        StepReturnRequested,
        Interrupted,
        InterruptRequested,
        Canceling,
        Canceled,
        CancelRequested,
        CancelRequestedWhileInterrupted,
        Finished
    };

    SquishPerspective();
    void initPerspective();

    State state() const { return m_state; }
    void setState(State state);
    void updateStatus(const QString &status);

    void showControlBar(SquishXmlOutputHandler *xmlOutputHandler);
    void destroyControlBar();

signals:
    void stateChanged(State state);

private:
    void onStopTriggered();
    void onPausePlayTriggered();
    void onLocalsUpdated(const QString &output);
    bool isStateTransitionValid(State newState) const;

    QAction *m_pausePlayAction = nullptr;
    QAction *m_stepInAction = nullptr;
    QAction *m_stepOverAction = nullptr;
    QAction *m_stepOutAction = nullptr;
    QAction *m_stopAction = nullptr;
    QLabel *m_status = nullptr;
    class SquishControlBar *m_controlBar = nullptr;
    Utils::TreeModel<LocalsItem> m_localsModel;
    State m_state = State::None;

    friend class SquishControlBar;
};

} // namespace Internal
} // namespace Squish

