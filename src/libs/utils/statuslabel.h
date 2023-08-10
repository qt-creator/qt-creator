// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

#include <QLabel>

namespace Utils {

class QTCREATOR_UTILS_EXPORT StatusLabel : public QLabel
{
public:
    explicit StatusLabel(QWidget *parent = nullptr);

    void showStatusMessage(const QString &message, int timeoutMS = 5000);
    void clearStatusMessage();

private:
    void slotTimeout();
    void stopTimer();

    QTimer *m_timer = nullptr;
    QString m_lastPermanentStatusMessage;
};

} // namespace Utils
