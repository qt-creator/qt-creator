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

#include <abstractview.h>

#include <QPointer>


namespace QmlDesigner {

class ItemLibraryWidget;
class ImageCacheData;
class AsynchronousImageCache;

class ItemLibraryView : public AbstractView
{
    Q_OBJECT

public:
    ItemLibraryView(QObject* parent = nullptr);
    ~ItemLibraryView() override;

    bool hasWidget() const override;
    WidgetInfo widgetInfo() override;

    // AbstractView
    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;
    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports) override;
    void possibleImportsChanged(const QList<Import> &possibleImports) override;
    void usedImportsChanged(const QList<Import> &usedImports) override;
    void documentMessagesChanged(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &warnings) override;
    void updateImport3DSupport(const QVariantMap &supportMap) override;
    void customNotification(const AbstractView *view, const QString &identifier,
                            const QList<ModelNode> &nodeList, const QList<QVariant> &data) override;

    void setResourcePath(const QString &resourcePath);

    AsynchronousImageCache &imageCache();

protected:
    void updateImports();

private:
    std::unique_ptr<ImageCacheData> m_imageCacheData;
    QPointer<ItemLibraryWidget> m_widget;
    bool m_hasErrors = false;
    QVariantMap m_importableExtensions3DMap;
    QVariantMap m_importOptions3DMap;
};

}
