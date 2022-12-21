// Copyright (C) 2016 AudioCodes Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace ClearCase::Internal {

class ActivitySelector : public QWidget
{
    Q_OBJECT

public:
    explicit ActivitySelector(QWidget *parent = nullptr);
    QString activity() const;
    void setActivity(const QString &act);
    void addKeep();
    bool refresh();
    bool changed() { return m_changed; }

    void newActivity();

private:
    void userChanged();

    bool m_changed = false;
    QComboBox *m_cmbActivity = nullptr;
};

} // ClearCase::Internal
