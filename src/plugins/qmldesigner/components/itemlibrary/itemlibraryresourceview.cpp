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

#include "itemlibraryresourceview.h"

#include "customfilesystemmodel.h"

#include <theme.h>
#include <imagecache.h>

#include <QAction>
#include <QActionGroup>
#include <QDebug>
#include <QDrag>
#include <QFileSystemModel>
#include <QMimeData>
#include <QPainter>
#include <QPixmap>
#include <QtGui/qevent.h>

#include <QProxyStyle>

#include <functional>

enum { debug = 0 };

namespace QmlDesigner {

void ItemLibraryResourceView::addSizeAction(QActionGroup *group, const QString &text, int gridSize, int iconSize)
{
    auto action = new QAction(text, group);
    group->addAction(action);
    action->setCheckable(true);
    QAction::connect(action, &QAction::triggered, this, [this, gridSize, iconSize]() {
        setViewMode(QListView::IconMode);
        setGridSize(QSize(gridSize, gridSize));
        setIconSize(QSize(iconSize, iconSize));
        setDragEnabled(true);
        setWrapping(true);
    });
}

ItemLibraryResourceView::ItemLibraryResourceView(ImageCache &fontImageCache, QWidget *parent) :
        QListView(parent)
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAttribute(Qt::WA_MacShowFocusRect, false);

    setGridSize(QSize(128, 128));
    setIconSize(QSize(96, 96));
    setSpacing(4);

    setViewMode(QListView::IconMode);
    setMovement(QListView::Static);
    setResizeMode(QListView::Adjust);
    setSelectionRectVisible(false);
    setWrapping(true);
    setWordWrap(true);

    setDragDropMode(QAbstractItemView::DragOnly);

    setContextMenuPolicy(Qt::ActionsContextMenu);

    auto actionGroup = new QActionGroup(this);
    actionGroup->setExclusive(true);

    addSizeAction(actionGroup, tr("Large Icons"), 256, 192);
    addSizeAction(actionGroup, tr("Medium Icons"), 128, 96);
    addSizeAction(actionGroup, tr("Small Icons"), 96, 48);

    QAction *action = new QAction(tr("List"), actionGroup);
    actionGroup->addAction(action);
    action->setCheckable(true);
    QAction::connect(action, &QAction::triggered, this, [this](){
        setViewMode(QListView::ListMode);
        setGridSize(QSize());
        setIconSize(QSize(32, 32));
        setDragEnabled(true);
        setWrapping(false);
    });

    QAction *defaultAction = actionGroup->actions().at(1);
    defaultAction->toggle();

    addActions(actionGroup->actions());

    viewport()->setAttribute(Qt::WA_Hover);
    m_fontPreviewTooltipBackend = std::make_unique<PreviewTooltipBackend>(fontImageCache);
    // Note: Though the text specified here appears in UI, it shouldn't be translated, as it's
    // a commonly used sentence to preview the font glyphs in latin fonts.
    // For fonts that do not have latin glyphs, the font family name will have to
    // suffice for preview. Font family name is inserted into %1 at render time.
    m_fontPreviewTooltipBackend->setState(QStringLiteral("%1@%2@%3")
                                          .arg(QString::number(300),
                                               Theme::getColor(Theme::DStextColor).name(),
                                               QStringLiteral("%1\n\n"
                                                              "The quick brown fox jumps\n"
                                                              "over the lazy dog\n"
                                                              "1234567890")));
}

void ItemLibraryResourceView::startDrag(Qt::DropActions /* supportedActions */)
{
    if (debug)
        qDebug() << Q_FUNC_INFO;

    const auto indexes = selectedIndexes();
    if (indexes.isEmpty())
        return;

    const QModelIndex &index = indexes.constFirst();
    if (!index.isValid())
        return;

    auto fileSystemModel = qobject_cast<CustomFileSystemModel*>(model());
    Q_ASSERT(fileSystemModel);
    QPair<QString, QByteArray> typeAndData = fileSystemModel->resourceTypeAndData(index);

    if (typeAndData.first.isEmpty())
        return;

    QFileInfo fileInfo = fileSystemModel->fileInfo(index);

    auto drag = new QDrag(this);
    drag->setPixmap(fileSystemModel->fileIcon(index).pixmap(128, 128));
    QMimeData *mimeData = new QMimeData;
    mimeData->setData(QLatin1String("application/vnd.bauhaus.libraryresource"),
                      fileInfo.absoluteFilePath().toUtf8());
    mimeData->setData(typeAndData.first, typeAndData.second);
    drag->setMimeData(mimeData);
    drag->exec();
}

bool ItemLibraryResourceView::viewportEvent(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        auto fileSystemModel = qobject_cast<CustomFileSystemModel *>(model());
        Q_ASSERT(fileSystemModel);
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
        QModelIndex index = indexAt(helpEvent->pos());
        if (index.isValid()) {
            QFileInfo fi = fileSystemModel->fileInfo(index);
            if (fileSystemModel->previewableSuffixes().contains(fi.suffix())) {
                QString filePath = fi.absoluteFilePath();
                if (!filePath.isEmpty()) {
                    if (!m_fontPreviewTooltipBackend->isVisible()
                            || m_fontPreviewTooltipBackend->path() != filePath) {
                        m_fontPreviewTooltipBackend->setPath(filePath);
                        m_fontPreviewTooltipBackend->setName(fi.fileName());
                        m_fontPreviewTooltipBackend->showTooltip();
                    } else {
                        m_fontPreviewTooltipBackend->reposition();
                    }
                    return true;
                }
            }
        }
        m_fontPreviewTooltipBackend->hideTooltip();
    } else if (event->type() == QEvent::Leave) {
        m_fontPreviewTooltipBackend->hideTooltip();
    } else if (event->type() == QEvent::HoverMove) {
        if (m_fontPreviewTooltipBackend->isVisible()) {
            auto fileSystemModel = qobject_cast<CustomFileSystemModel *>(model());
            Q_ASSERT(fileSystemModel);
            auto *he = static_cast<QHoverEvent *>(event);
            QModelIndex index = indexAt(he->pos());
            if (index.isValid()) {
                QFileInfo fi = fileSystemModel->fileInfo(index);
                if (fi.absoluteFilePath() != m_fontPreviewTooltipBackend->path())
                    m_fontPreviewTooltipBackend->hideTooltip();
                else
                    m_fontPreviewTooltipBackend->reposition();
            }
        }
    }

    return QListView::viewportEvent(event);
}

} // namespace QmlDesigner

