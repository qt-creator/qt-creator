// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofiler_global.h"

#include <QFuture>
#include <qwindowdefs.h>

namespace Utils {
class FilePath;
}

namespace QmlProfiler {

class QmlProfilerPlainViewManagerPrivate;

class QMLPROFILER_EXPORT QmlProfilerPlainViewManager : public QObject
{
    Q_OBJECT
public:
    explicit QmlProfilerPlainViewManager(QObject *parent = nullptr);

    QWidgetList views(QWidget *parent);
    static QString fileDialogTraceFilesFilter();
    QFuture<void> loadTraceFile(const Utils::FilePath &file);
    void clear();
    std::chrono::milliseconds traceDuration() const;

signals:
    void error(const QString &error);
    void loadFinished();
    void gotoSourceLocation(const QString &fileUrl, int lineNumber, int columnNumber);
    void typeSelected(int typeId);

private:
    QmlProfilerPlainViewManagerPrivate *d;
};

} // namespace QmlProfiler
