// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/actionmanager/actioncontainer.h>

#include <QObject>
#include <QAction>

#include <memory>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerTool;
class QmlProfilerActions : public QObject
{
    Q_OBJECT
public:
    explicit QmlProfilerActions(QObject *parent = nullptr);

    void attachToTool(QmlProfilerTool *tool);
    void registerActions();

private:
    std::unique_ptr<Core::ActionContainer> m_options;
    std::unique_ptr<QAction> m_loadQmlTrace;
    std::unique_ptr<QAction> m_saveQmlTrace;
    std::unique_ptr<QAction> m_runAction;
    std::unique_ptr<QAction> m_attachAction;
};

} // namespace Internal
} // namespace QmlProfiler
