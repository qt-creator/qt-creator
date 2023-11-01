// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "storekey.h"

#include <QCompleter>

namespace Utils {

class QtcSettings;

namespace Internal { class HistoryCompleterPrivate; }

class QTCREATOR_UTILS_EXPORT HistoryCompleter : public QCompleter
{
public:
    static void setSettings(QtcSettings *settings);
    HistoryCompleter(const Key &historyKey, QObject *parent = nullptr);
    bool removeHistoryItem(int index);
    QString historyItem() const;
    bool hasHistory() const { return historySize() > 0; }
    static bool historyExistsFor(const Key &historyKey);
    void clearHistory();
    void addEntry(const QString &str);

private:
    ~HistoryCompleter() override;
    int historySize() const;
    int maximalHistorySize() const;
    void setMaximalHistorySize(int numberOfEntries);

private:
    Internal::HistoryCompleterPrivate *d;
};

} // namespace Utils
