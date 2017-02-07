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

#include "dropsupport.h"

#include "qtcassert.h"

#include <QUrl>
#include <QWidget>
#include <QDropEvent>
#include <QTimer>

#ifdef Q_OS_OSX
// for file drops from Finder, working around QTBUG-40449
#include "fileutils_mac.h"
#endif

namespace Utils {

static bool isFileDrop(const QMimeData *d, QList<DropSupport::FileSpec> *files = 0)
{
    // internal drop
    if (const DropMimeData *internalData = qobject_cast<const DropMimeData *>(d)) {
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
#ifdef Q_OS_OSX
        // for file drops from Finder, working around QTBUG-40449
        url = Internal::filePathUrl(url);
#endif
        const QString fileName = url.toLocalFile();
        if (!fileName.isEmpty()) {
            hasFiles = true;
            if (files)
                files->append(DropSupport::FileSpec(fileName));
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
    return QStringList("text/uri-list");
}

bool DropSupport::isFileDrop(QDropEvent *event) const
{
    return Utils::isFileDrop(event->mimeData());
}

bool DropSupport::isValueDrop(QDropEvent *event) const
{
    if (const DropMimeData *internalData = qobject_cast<const DropMimeData *>(event->mimeData())) {
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
            const DropMimeData *fileDropMimeData = qobject_cast<const DropMimeData *>(de->mimeData());
            QList<FileSpec> tempFiles;
            if (Utils::isFileDrop(de->mimeData(), &tempFiles)) {
                event->accept();
                accepted = true;
                if (fileDropMimeData && fileDropMimeData->isOverridingFileDropAction())
                    de->setDropAction(fileDropMimeData->overrideFileDropAction());
                else
                    de->acceptProposedAction();
                bool needToScheduleEmit = m_files.isEmpty();
                m_files.append(tempFiles);
                if (needToScheduleEmit) { // otherwise we already have a timer pending
                    // Delay the actual drop, to avoid conflict between
                    // actions that happen when opening files, and actions that the item views do
                    // after the drag operation.
                    // If we do not do this, e.g. dragging from Outline view crashes if the editor and
                    // the selected item changes
                    QTimer::singleShot(100, this, &DropSupport::emitFilesDropped);
                }
            }
            if (fileDropMimeData && !fileDropMimeData->values().isEmpty()) {
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
    emit filesDropped(m_files);
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

void DropMimeData::addFile(const QString &filePath, int line, int column)
{
    // standard mime data
    QList<QUrl> currentUrls = urls();
    currentUrls.append(QUrl::fromLocalFile(filePath));
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
