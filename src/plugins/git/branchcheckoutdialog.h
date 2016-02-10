/****************************************************************************
**
** Copyright (C) 2016 Petar Perisin <petar.perisin@gmail.com>
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

namespace Ui { class BranchCheckoutDialog; }

class BranchCheckoutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BranchCheckoutDialog(QWidget *parent, const QString &currentBranch,
                                  const QString &nextBranch);
    ~BranchCheckoutDialog() override;

    void foundNoLocalChanges();
    void foundStashForNextBranch();

    bool makeStashOfCurrentBranch();
    bool moveLocalChangesToNextBranch();
    bool discardLocalChanges();
    bool popStashOfNextBranch();

    bool hasStashForNextBranch();
    bool hasLocalChanges();

private:
    void updatePopStashCheckBox(bool moveChangesChecked);

    Ui::BranchCheckoutDialog *m_ui;
    bool m_foundStashForNextBranch;
    bool m_hasLocalChanges;
};

} // namespace Internal
} // namespace Git
