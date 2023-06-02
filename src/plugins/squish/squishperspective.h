// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "squishconstants.h"

#include <debugger/debuggermainwindow.h>

#include <utils/treemodel.h>

namespace Utils { class TreeView; }

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

class InspectedObjectItem : public Utils::TreeItem
{
public:
    InspectedObjectItem() = default;
    InspectedObjectItem(const QString &v, const QString &t) : value(v), type(t) {}
    QVariant data(int column, int role) const override;
    QString value;
    QString type;
    QString fullName; // FIXME this might be non-unique
    bool expanded = false;
};

class InspectedPropertyItem : public Utils::TreeItem
{
public:
    InspectedPropertyItem() = default;
    InspectedPropertyItem(const QString &n, const QString &v)
        : name(n), value(v) { parseAndUpdateChildren(); }
    QVariant data(int column, int role) const override;
    QString name;
    QString value;
    bool expanded = false;
private:
    void parseAndUpdateChildren();
};

class SquishPerspective : public Utils::Perspective
{
    Q_OBJECT
public:
    enum PerspectiveMode { NoMode, Interrupted, Running, Recording, Querying, Configuring };

    SquishPerspective();
    void initPerspective();
    void setPerspectiveMode(PerspectiveMode mode);
    PerspectiveMode perspectiveMode() const { return m_mode; }

    void updateStatus(const QString &status);

    void showControlBar(SquishXmlOutputHandler *xmlOutputHandler);
    void destroyControlBar();
    void resetAutId();

signals:
    void stopRequested();
    void stopRecordRequested();
    void interruptRequested();
    void runRequested(StepMode mode);
    void inspectTriggered();

private:
    void onStopTriggered();
    void onStopRecordTriggered();
    void onPausePlayTriggered();
    void onLocalsUpdated(const QString &output);
    void onObjectPicked(const QString &output);
    void onUpdateChildren(const QString &name, const QStringList &children);
    void onPropertiesFetched(const QStringList &properties);

    QAction *m_stopRecordAction = nullptr;
    QAction *m_pausePlayAction = nullptr;
    QAction *m_stepInAction = nullptr;
    QAction *m_stepOverAction = nullptr;
    QAction *m_stepOutAction = nullptr;
    QAction *m_stopAction = nullptr;
    QAction *m_inspectAction = nullptr;
    QLabel *m_status = nullptr;
    class SquishControlBar *m_controlBar = nullptr;
    Utils::TreeModel<LocalsItem> m_localsModel;
    Utils::TreeModel<InspectedObjectItem> m_objectsModel;
    Utils::TreeModel<InspectedPropertyItem> m_propertiesModel;
    Utils::TreeView *m_objectsView = nullptr;
    PerspectiveMode m_mode = NoMode;
    bool m_autIdKnown = false;

    friend class SquishControlBar;
};

} // namespace Internal
} // namespace Squish

