// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

namespace ScxmlEditor {

namespace PluginInterface { class ScxmlDocument; }

namespace Common {

class Statistics;

class StatisticsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StatisticsDialog(QWidget *parent = nullptr);

    void setDocument(PluginInterface::ScxmlDocument *doc);

private:
    Statistics *m_statistics;
};

} // namespace Common
} // namespace ScxmlEditor
