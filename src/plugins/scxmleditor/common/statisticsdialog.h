// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "ui_statisticsdialog.h"

#include <QDialog>

namespace ScxmlEditor {

namespace PluginInterface { class ScxmlDocument; }

namespace Common {

class StatisticsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StatisticsDialog(QWidget *parent = nullptr);

    void setDocument(PluginInterface::ScxmlDocument *doc);

private:
    Ui::StatisticsDialog m_ui;
};

} // namespace Common
} // namespace ScxmlEditor
