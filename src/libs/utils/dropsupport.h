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
        FileSpec(const QString &path, int r = -1, int c = -1) : filePath(path), line(r), column(c) {}
        QString filePath;
        int line;
        int column;
    };
    // returns true if the event should be accepted
    typedef std::function<bool(QDropEvent*,DropSupport*)> DropFilterFunction;

    DropSupport(QWidget *parentWidget, const DropFilterFunction &filterFunction = DropFilterFunction());

    static QStringList mimeTypesForFilePaths();

signals:
    void filesDropped(const QList<Utils::DropSupport::FileSpec> &files);
    void valuesDropped(const QList<QVariant> &values, const QPoint &dropPos);

public:
    bool isFileDrop(QDropEvent *event) const;
    bool isValueDrop(QDropEvent *event) const;

protected:
    bool eventFilter(QObject *obj, QEvent *event);

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

    void addFile(const QString &filePath, int line = -1, int column = -1);
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
