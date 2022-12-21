// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QVariantMap>
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTableWidget;
QT_END_NAMESPACE

namespace QbsProjectManager {
namespace Internal {

class CustomQbsPropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomQbsPropertiesDialog(const QVariantMap &properties, QWidget *parent = nullptr);

    QVariantMap properties() const;

private:
    void addProperty();
    void removeSelectedProperty();
    void handleCurrentItemChanged();

private:
    QTableWidget *m_propertiesTable;
    QPushButton *m_removeButton;
};

} // namespace Internal
} // namespace QbsProjectManager
