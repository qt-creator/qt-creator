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

#include "utils_global.h"

#include <QCompleter>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Utils {

namespace Internal { class HistoryCompleterPrivate; }

class QTCREATOR_UTILS_EXPORT HistoryCompleter : public QCompleter
{
    Q_OBJECT

public:
    static void setSettings(QSettings *settings);
    HistoryCompleter(const QString &historyKey, QObject *parent = 0);
    bool removeHistoryItem(int index);
    QString historyItem() const;
    static bool historyExistsFor(const QString &historyKey);

private:
    ~HistoryCompleter();
    int historySize() const;
    int maximalHistorySize() const;
    void setMaximalHistorySize(int numberOfEntries);

public Q_SLOTS:
    void clearHistory();
    void addEntry(const QString &str);

private:
    Internal::HistoryCompleterPrivate *d;
};

} // namespace Utils
