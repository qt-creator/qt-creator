// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profiler_global.h"

#include <qwindowdefs.h>

#include <QObject>

namespace Timeline { class RangeDetailsWidget; }
namespace Utils { class FilePath; }

namespace Profiler {

class PROFILER_EXPORT QmlProfilerPlainViewManager : public QObject
{
    Q_OBJECT

public:
    // `details` is the range details panel this manager's views fill. It is owned
    // by the caller, which lets every profiler backend share a single one.
    explicit QmlProfilerPlainViewManager(Timeline::RangeDetailsWidget *details,
                                         QObject *parent = nullptr);
    ~QmlProfilerPlainViewManager();

    QWidgetList views(QWidget *parent);
    static QString fileDialogTraceFilesFilter();
    void loadTraceFile(const Utils::FilePath &file);
    void clear();
    std::chrono::milliseconds traceDuration() const;

signals:
    void error(const QString &error);
    void loadFinished();
    void gotoSourceLocation(const QString &fileUrl, int lineNumber, int columnNumber);
    void typeSelected(int typeId);

private:
    class QmlProfilerPlainViewManagerPrivate *d;
};

} // namespace Profiler
