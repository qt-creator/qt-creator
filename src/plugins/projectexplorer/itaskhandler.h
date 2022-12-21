// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"
#include "task.h"

#include <utils/id.h>

#include <QObject>
#include <QString>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT ITaskHandler : public QObject
{
    Q_OBJECT

public:
    explicit ITaskHandler(bool isMultiHandler = false);
    ~ITaskHandler() override;

    virtual bool isDefaultHandler() const { return false; }
    virtual bool canHandle(const Task &) const { return m_isMultiHandler; }
    virtual void handle(const Task &);       // Non-multi-handlers should implement this.
    virtual void handle(const Tasks &tasks); // Multi-handlers should implement this.
    virtual Utils::Id actionManagerId() const { return Utils::Id(); }
    virtual QAction *createAction(QObject *parent) const = 0;

    bool canHandle(const Tasks &tasks) const;

private:
    const bool m_isMultiHandler;
};

} // namespace ProjectExplorer
