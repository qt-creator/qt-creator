// Copyright (C) 2016 Petar Perisin <petar.perisin@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QGroupBox;
class QRadioButton;
QT_END_NAMESPACE

namespace Git {
namespace Internal {

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

    bool m_foundStashForNextBranch = false;
    bool m_hasLocalChanges = true;

    QGroupBox *m_localChangesGroupBox;
    QRadioButton *m_makeStashRadioButton;
    QRadioButton *m_moveChangesRadioButton;
    QRadioButton *m_discardChangesRadioButton;
    QCheckBox *m_popStashCheckBox;
};

} // namespace Internal
} // namespace Git
