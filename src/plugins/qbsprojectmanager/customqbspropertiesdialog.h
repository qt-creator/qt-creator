// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QVariantMap>
#include <QDialog>

namespace QbsProjectManager {
namespace Internal {
namespace Ui { class CustomQbsPropertiesDialog; }

class CustomQbsPropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomQbsPropertiesDialog(const QVariantMap &properties, QWidget *parent = nullptr);

    QVariantMap properties() const;
    ~CustomQbsPropertiesDialog() override;

private:
    void addProperty();
    void removeSelectedProperty();
    void handleCurrentItemChanged();

private:
    Ui::CustomQbsPropertiesDialog * const m_ui;
};

} // namespace Internal
} // namespace QbsProjectManager
