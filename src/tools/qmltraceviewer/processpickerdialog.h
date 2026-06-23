// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/processinfo.h>

#include <QDialog>

#include <optional>

QT_BEGIN_NAMESPACE
class QPushButton;
class QSortFilterProxyModel;
class QStandardItemModel;
class QTreeView;
QT_END_NAMESPACE

namespace Utils { class FancyLineEdit; }

namespace QmlTraceViewer {

// A minimal "attach to process" picker: lists the running processes with a
// type-to-filter field and returns the one the user chooses. Self-contained so
// the standalone viewer needs no ProjectExplorer dependency.
class ProcessPickerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProcessPickerDialog(QWidget *parent = nullptr);

    std::optional<Utils::ProcessInfo> selectedProcess() const;

    // Shows the dialog modally; returns the chosen process, or nullopt if the
    // user cancelled.
    static std::optional<Utils::ProcessInfo> pickProcess(QWidget *parent = nullptr);

private:
    void updateOkButton();

    QList<Utils::ProcessInfo> m_processes;
    QStandardItemModel *m_model = nullptr;
    QSortFilterProxyModel *m_proxy = nullptr;
    QTreeView *m_view = nullptr;
    Utils::FancyLineEdit *m_filter = nullptr;
    QPushButton *m_okButton = nullptr;
};

} // namespace QmlTraceViewer
