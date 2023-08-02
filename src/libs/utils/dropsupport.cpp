// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dropsupport.h"

#include "qtcassert.h"

#include <QUrl>
#include <QWidget>
#include <QDropEvent>
#include <QTimer>

#ifdef Q_OS_MACOS
// for file drops from Finder, working around QTBUG-40449
#include "fileutils_mac.h"
#endif

namespace Utils {

static bool isFileDropMime(const QMimeData *d, QList<DropSupport::FileSpec> *files = nullptr)
{
    // internal drop
    if (const auto internalData = qobject_cast<const DropMimeData *>(d)) {
        if (files)
            *files = internalData->files();
        return !internalData->files().isEmpty();
    }

    // external drop
    if (files)
        files->clear();
    // Extract dropped files from Mime data.
    if (!d->hasUrls())
        return false;
    const QList<QUrl> urls = d->urls();
    if (urls.empty())
        return false;
    // Try to find local files
    bool hasFiles = false;
    const QList<QUrl>::const_iterator cend = urls.constEnd();
    for (QList<QUrl>::const_iterator it = urls.constBegin(); it != cend; ++it) {
        QUrl url = *it;
        const QString fileName = url.toLocalFile();
        if (!fileName.isEmpty()) {
            hasFiles = true;
            if (files)
                files->append(DropSupport::FileSpec(FilePath::fromString(fileName)));
            else
                break; // No result list, sufficient for checking
        }
    }
    return hasFiles;
}

DropSupport::DropSupport(QWidget *parentWidget, const DropFilterFunction &filterFunction)
    : QObject(parentWidget),
      m_filterFunction(filterFunction)
{
    QTC_ASSERT(parentWidget, return);
    parentWidget->setAcceptDrops(true);
    parentWidget->installEventFilter(this);
}

QStringList DropSupport::mimeTypesForFilePaths()
{
    return {"text/uri-list"};
}

bool DropSupport::isFileDrop(QDropEvent *event)
{
    return isFileDropMime(event->mimeData());
}

bool DropSupport::isValueDrop(QDropEvent *event)
{
    if (const auto internalData = qobject_cast<const DropMimeData *>(event->mimeData())) {
        return !internalData->values().isEmpty();
    }
    return false;
}

bool DropSupport::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj)
    if (event->type() == QEvent::DragEnter) {
        auto dee = static_cast<QDragEnterEvent *>(event);
        if ((isFileDrop(dee) || isValueDrop(dee)) && (!m_filterFunction || m_filterFunction(dee, this))) {
            event->accept();
        } else {
            event->ignore();
        }
        return true;
    } else if (event->type() == QEvent::DragMove) {
        event->accept();
        return true;
    } else if (event->type() == QEvent::Drop) {
        bool accepted = false;
        auto de = static_cast<QDropEvent *>(event);
        if (!m_filterFunction || m_filterFunction(de, this)) {
            const auto fileDropMimeData = qobject_cast<const DropMimeData *>(de->mimeData());
            QList<FileSpec> tempFiles;
            if (isFileDropMime(de->mimeData(), &tempFiles)) {
                event->accept();
                accepted = true;
                if (fileDropMimeData && fileDropMimeData->isOverridingFileDropAction())
                    de->setDropAction(fileDropMimeData->overrideFileDropAction());
                else
                    de->acceptProposedAction();
                bool needToScheduleEmit = m_files.isEmpty();
                m_files.append(tempFiles);
                m_dropPos = de->pos();
                if (needToScheduleEmit) { // otherwise we already have a timer pending
                    // Delay the actual drop, to avoid conflict between
                    // actions that happen when opening files, and actions that the item views do
                    // after the drag operation.
                    // If we do not do this, e.g. dragging from Outline view crashes if the editor and
                    // the selected item changes
                    QTimer::singleShot(100, this, &DropSupport::emitFilesDropped);
                }
            } else if (fileDropMimeData && !fileDropMimeData->values().isEmpty()) {
                event->accept();
                accepted = true;
                bool needToScheduleEmit = m_values.isEmpty();
                m_values.append(fileDropMimeData->values());
                m_dropPos = de->pos();
                if (needToScheduleEmit)
                    QTimer::singleShot(100, this, &DropSupport::emitValuesDropped);
            }
        }
        if (!accepted) {
            event->ignore();
        }
        return true;
    }
    return false;
}

void DropSupport::emitFilesDropped()
{
    QTC_ASSERT(!m_files.isEmpty(), return);
    emit filesDropped(m_files, m_dropPos);
    m_files.clear();
}

void DropSupport::emitValuesDropped()
{
    QTC_ASSERT(!m_values.isEmpty(), return);
    emit valuesDropped(m_values, m_dropPos);
    m_values.clear();
}

/*!
    Sets the drop action to effectively use, instead of the "proposed" drop action from the
    drop event. This can be useful when supporting move drags within an item view, but not
    "moving" an item from the item view into a split.
 */
DropMimeData::DropMimeData()
    : m_overrideDropAction(Qt::IgnoreAction),
      m_isOverridingDropAction(false)
{

}

void DropMimeData::setOverrideFileDropAction(Qt::DropAction action)
{
    m_isOverridingDropAction = true;
    m_overrideDropAction = action;
}

Qt::DropAction DropMimeData::overrideFileDropAction() const
{
    return m_overrideDropAction;
}

bool DropMimeData::isOverridingFileDropAction() const
{
    return m_isOverridingDropAction;
}

void DropMimeData::addFile(const FilePath &filePath, int line, int column)
{
    // standard mime data
    QList<QUrl> currentUrls = urls();
    currentUrls.append(filePath.toUrl());
    setUrls(currentUrls);
    // special mime data
    m_files.append(DropSupport::FileSpec(filePath, line, column));
}

QList<DropSupport::FileSpec> DropMimeData::files() const
{
    return m_files;
}

void DropMimeData::addValue(const QVariant &value)
{
    m_values.append(value);
}

QList<QVariant> DropMimeData::values() const
{
    return m_values;
}

} // namespace Utils
