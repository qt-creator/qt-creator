// Copyright (C) 2016 AudioCodes Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

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
