// Copyright (C) 2016 AudioCodes Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLabel;
class QPlainTextEdit;
class QVBoxLayout;
QT_END_NAMESPACE

namespace ClearCase::Internal {

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

    ActivitySelector *m_actSelector = nullptr;

    QVBoxLayout *m_verticalLayout;
    QLabel *m_lblComment;
    QPlainTextEdit *m_txtComment;
    QCheckBox *m_chkReserved;
    QCheckBox *m_chkUnreserved;
    QCheckBox *m_chkPTime;
    QCheckBox *m_hijackedCheckBox;
};

} // ClearCase::Internal
