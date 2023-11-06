// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QFrame>

#include <coreplugin/icontext.h>

class StudioQuickWidget;

namespace QmlDesigner {

class CollectionDetailsModel;
class CollectionDetailsSortFilterModel;
class CollectionSourceModel;
class CollectionView;
class ModelNode;

class CollectionWidget : public QFrame
{
    Q_OBJECT

public:
    CollectionWidget(CollectionView *view);
    void contextHelp(const Core::IContext::HelpCallback &callback) const;

    QPointer<CollectionSourceModel> sourceModel() const;
    QPointer<CollectionDetailsModel> collectionDetailsModel() const;

    void reloadQmlSource();

    virtual QSize minimumSizeHint() const;

    Q_INVOKABLE bool loadJsonFile(const QString &jsonFileAddress, const QString &collectionName = {});
    Q_INVOKABLE bool loadCsvFile(const QString &csvFileAddress, const QString &collectionName = {});
    Q_INVOKABLE bool isJsonFile(const QString &jsonFileAddress) const;
    Q_INVOKABLE bool isCsvFile(const QString &csvFileAddress) const;
    Q_INVOKABLE bool addCollection(const QString &collectionName,
                                   const QString &collectionType,
                                   const QString &sourceAddress,
                                   const QVariant &sourceNode);

    void warn(const QString &title, const QString &body);

private:
    QPointer<CollectionView> m_view;
    QPointer<CollectionSourceModel> m_sourceModel;
    QPointer<CollectionDetailsModel> m_collectionDetailsModel;
    std::unique_ptr<CollectionDetailsSortFilterModel> m_collectionDetailsSortFilterModel;
    QScopedPointer<StudioQuickWidget> m_quickWidget;
};

} // namespace QmlDesigner
