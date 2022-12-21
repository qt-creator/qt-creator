// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QComboBox>

namespace Utils {

// Documentation inside.
class QTCREATOR_UTILS_EXPORT TextFieldComboBox : public QComboBox
{
    Q_PROPERTY(QString indexText READ text WRITE setText)
    Q_OBJECT

public:
    explicit TextFieldComboBox(QWidget *parent = nullptr);

    QString text() const;
    void setText(const QString &s);

    void setItems(const QStringList &displayTexts, const QStringList &values);

signals:
    void text4Changed(const QString &); // Do not conflict with Qt 3 compat signal.

private:
    void slotCurrentIndexChanged(int);

    inline QString valueAt(int) const;
};

} // namespace Utils
