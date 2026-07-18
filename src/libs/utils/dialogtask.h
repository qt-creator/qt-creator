// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once


#include <QtTaskTree/qtasktree.h>

#include <QDialog>
#include <QMessageBox>

#include <functional>

namespace Utils {

// A ready-made result mapper for QMessageBox-based dialogs: maps an accept-like
// clicked button (AcceptRole or YesRole) to DoneResult::Success, and everything
// else - including closing the dialog without a choice - to DoneResult::Error.
// It is the default mapper for DialogWrapper<QMessageBox> (see below), and can be
// reused when installing a custom mapper.
inline QtTaskTree::DoneResult messageBoxResult(const QMessageBox *box)
{
    QAbstractButton *clicked = box->clickedButton();
    const auto role = clicked ? box->buttonRole(clicked) : QMessageBox::InvalidRole;
    return QtTaskTree::toDoneResult(role == QMessageBox::AcceptRole
                                 || role == QMessageBox::YesRole);
}

template <typename Dialog>
class DialogWrapper
{
    Q_DISABLE_COPY_MOVE(DialogWrapper)

    static_assert(std::is_base_of_v<QDialog, Dialog>,
                  "DialogWrapper<Dialog>: Dialog must be QDialog or a subclass of it.");
    static_assert(std::is_default_constructible_v<Dialog>,
                  "DialogWrapper<Dialog>: Dialog must be default-constructible.");

public:
    using ResultMapper = std::function<QtTaskTree::DoneResult(const Dialog *)>;

    DialogWrapper() = default;

    Dialog *dialog() { return &m_dialog; }
    const Dialog *dialog() const { return &m_dialog; }

    // Reparents the dialog while preserving its window flags. A plain setParent(parent)
    // would reset the flags and turn the top-level dialog into a child widget.
    void setParent(QWidget *parent) { m_dialog.setParent(parent, m_dialog.windowFlags()); }

    int result() const { return m_dialog.result(); }

    // Overrides how the dialog's outcome is mapped to the task's DoneResult.
    // By default a plain QDialog maps Accepted to Success (and everything else to
    // Error), while a QMessageBox uses messageBoxResult().
    void setResultMapper(ResultMapper mapper) { m_resultMapper = std::move(mapper); }

private:
    static ResultMapper defaultResultMapper()
    {
        if constexpr (std::is_base_of_v<QMessageBox, Dialog>)
            return [](const Dialog *dialog) { return messageBoxResult(dialog); };
        else
            return [](const Dialog *dialog) {
                return QtTaskTree::toDoneResult(dialog->result() == QDialog::Accepted);
            };
    }

    Dialog m_dialog;
    ResultMapper m_resultMapper = defaultResultMapper();

    template <typename T> friend class DialogTaskAdapter;
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
        QObject::connect(dialog, &QDialog::finished, iface, [task, iface](int) {
            iface->reportDone(task->m_resultMapper(task->dialog()));
        }, Qt::SingleShotConnection);
        dialog->open();
    }
};

template <typename Dialog>
using DialogTask = QtTaskTree::QCustomTask<DialogWrapper<Dialog>, DialogTaskAdapter<Dialog>>;

} // namespace Utils
