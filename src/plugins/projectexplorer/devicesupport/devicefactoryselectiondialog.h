// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QListWidget;
QT_END_NAMESPACE

namespace ProjectExplorer {
class IDeviceFactory;

namespace Internal {

class DeviceFactorySelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DeviceFactorySelectionDialog(QWidget *parent = nullptr);
    Utils::Id selectedId() const;

private:
    void handleItemSelectionChanged();
    void handleItemDoubleClicked();

    QListWidget *m_listWidget;
    QDialogButtonBox *m_buttonBox;
};

} // namespace Internal
} // namespace ProjectExplorer
