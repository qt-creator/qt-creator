/****************************************************************************
**
** Copyright (C) 2016 AudioCodes Ltd.
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
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

namespace ClearCase {
namespace Internal {

namespace Ui { class CheckOutDialog; }

class ActivitySelector;

class CheckOutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CheckOutDialog(const QString &fileName, bool isUcm, bool showComment,
                            QWidget *parent = nullptr);
    ~CheckOutDialog() override;

    QString activity() const;
    QString comment() const;
    bool isReserved() const;
    bool isUnreserved() const;
    bool isPreserveTime() const;
    bool isUseHijacked() const;
    void hideHijack();

    void hideComment();

private:
    void toggleUnreserved(bool checked);

    Ui::CheckOutDialog *ui;
    ActivitySelector *m_actSelector = nullptr;
};

} // namespace Internal
} // namespace ClearCase
