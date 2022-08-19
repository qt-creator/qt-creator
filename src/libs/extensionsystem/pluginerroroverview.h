// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "extensionsystem_global.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
class QListWidgetItem;
QT_END_NAMESPACE

namespace ExtensionSystem {

namespace Internal {
namespace Ui { class PluginErrorOverview; }
} // namespace Internal

class EXTENSIONSYSTEM_EXPORT PluginErrorOverview : public QDialog
{
    Q_OBJECT

public:
    explicit PluginErrorOverview(QWidget *parent = nullptr);
    ~PluginErrorOverview() override;

private:
    void showDetails(QListWidgetItem *item);

    Internal::Ui::PluginErrorOverview *m_ui;
};

} // ExtensionSystem
