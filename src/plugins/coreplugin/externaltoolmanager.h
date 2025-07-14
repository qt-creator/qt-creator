// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <QObject>
#include <QMap>
#include <QList>
#include <QString>

namespace Core {

class ExternalTool;

class CORE_EXPORT ExternalToolManager : public QObject
{
    Q_OBJECT

public:
    ExternalToolManager();
    ~ExternalToolManager() override;

    static ExternalToolManager *instance();
    static QMap<QString, QList<ExternalTool *> > toolsByCategory();
    static QMap<QString, ExternalTool *> toolsById();
    static void setToolsByCategory(const QMap<QString, QList<ExternalTool *> > &tools);
    static void emitReplaceSelectionRequested(const QString &output);

signals:
    void replaceSelectionRequested(const QString &text);
};

} // namespace Core
