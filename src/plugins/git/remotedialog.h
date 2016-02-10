/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QDialog>

namespace Git {
namespace Internal {

namespace Ui {
class RemoteDialog;
class RemoteAdditionDialog;
} // namespace Ui

class GitClient;
class RemoteModel;

// --------------------------------------------------------------------------
// RemoteAdditionDialog:
// --------------------------------------------------------------------------

class RemoteAdditionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RemoteAdditionDialog(QWidget *parent = 0);
    ~RemoteAdditionDialog() override;

    QString remoteName() const;
    QString remoteUrl() const;

    void clear();

private:
    Ui::RemoteAdditionDialog *m_ui;
};

// --------------------------------------------------------------------------
// RemoteDialog:
// --------------------------------------------------------------------------

class RemoteDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RemoteDialog(QWidget *parent = 0);
    ~RemoteDialog() override;

    void refresh(const QString &repository, bool force);

private:
    void refreshRemotes();
    void addRemote();
    void removeRemote();
    void pushToRemote();
    void fetchFromRemote();

    void updateButtonState();

    Ui::RemoteDialog *m_ui;

    RemoteModel *m_remoteModel;
    RemoteAdditionDialog *m_addDialog = nullptr;
};

} // namespace Internal
} // namespace Git
