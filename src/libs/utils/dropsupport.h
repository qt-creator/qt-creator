// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"

#include <QObject>
#include <QMimeData>
#include <QPoint>

#include <functional>

QT_BEGIN_NAMESPACE
class QDropEvent;
class QWidget;
QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT DropSupport : public QObject
{
    Q_OBJECT
public:
    struct FileSpec {
        FileSpec(const FilePath &path, int r = -1, int c = -1) : filePath(path), line(r), column(c) {}
        FilePath filePath;
        int line;
        int column;
    };
    // returns true if the event should be accepted
    using DropFilterFunction = std::function<bool(QDropEvent*, DropSupport*)>;

    DropSupport(QWidget *parentWidget, const DropFilterFunction &filterFunction = DropFilterFunction());

    static QStringList mimeTypesForFilePaths();

signals:
    void filesDropped(const QList<DropSupport::FileSpec> &files, const QPoint &dropPos);
    void valuesDropped(const QList<QVariant> &values, const QPoint &dropPos);

public:
    static bool isFileDrop(QDropEvent *event);
    static bool isValueDrop(QDropEvent *event);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void emitFilesDropped();
    void emitValuesDropped();

    DropFilterFunction m_filterFunction;
    QList<FileSpec> m_files;
    QList<QVariant> m_values;
    QPoint m_dropPos;

};

class QTCREATOR_UTILS_EXPORT DropMimeData : public QMimeData
{
    Q_OBJECT
public:
    DropMimeData();

    void setOverrideFileDropAction(Qt::DropAction action);
    Qt::DropAction overrideFileDropAction() const;
    bool isOverridingFileDropAction() const;

    void addFile(const FilePath &filePath, int line = -1, int column = -1);
    QList<DropSupport::FileSpec> files() const;

    void addValue(const QVariant &value);
    QList<QVariant> values() const;

private:
    QList<DropSupport::FileSpec> m_files;
    QList<QVariant> m_values;
    Qt::DropAction m_overrideDropAction;
    bool m_isOverridingDropAction;
};

} // namespace Utils
