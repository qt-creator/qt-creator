// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QtTaskTree/qtasktree.h>

#include <QDialog>

namespace Utils {

template <typename Dialog>
class DialogWrapper
{
    Q_DISABLE_COPY_MOVE(DialogWrapper)

    static_assert(std::is_base_of_v<QDialog, Dialog>,
                  "DialogWrapper<Dialog>: Dialog must be QDialog or a subclass of it.");
    static_assert(std::is_default_constructible_v<Dialog>,
                  "DialogWrapper<Dialog>: Dialog must be default-constructible.");

public:
    DialogWrapper() = default;

    Dialog *dialog() { return &m_dialog; }
    const Dialog *dialog() const { return &m_dialog; }

    // Reparents the dialog while preserving its window flags. A plain setParent(parent)
    // would reset the flags and turn the top-level dialog into a child widget.
    void setParent(QWidget *parent) { m_dialog.setParent(parent, m_dialog.windowFlags()); }

    int result() const { return m_dialog.result(); }

private:
    Dialog m_dialog;
};

template <typename Dialog>
class DialogTaskAdapter
{
public:
    void operator()(DialogWrapper<Dialog> *task, QtTaskTree::QTaskInterface *iface) const
    {
        QDialog *dialog = task->dialog();
        // The wrapper owns the dialog (it's a value member), so the dialog must not
        // delete itself. A QDialog subclass may have set the attribute in its
        // constructor; warn and force it off.
        if (dialog->testAttribute(Qt::WA_DeleteOnClose)) {
            qWarning("DialogTask: Qt::WA_DeleteOnClose is managed by the task; forcing it off.");
            dialog->setAttribute(Qt::WA_DeleteOnClose, false);
        }
        QObject::connect(dialog, &QDialog::finished, iface, [iface](int result) {
            iface->reportDone(result == QDialog::Accepted ? QtTaskTree::DoneResult::Success
                                                          : QtTaskTree::DoneResult::Error);
        }, Qt::SingleShotConnection);
        dialog->open();
    }
};

template <typename Dialog>
using DialogTask = QtTaskTree::QCustomTask<DialogWrapper<Dialog>, DialogTaskAdapter<Dialog>>;

} // namespace Utils
