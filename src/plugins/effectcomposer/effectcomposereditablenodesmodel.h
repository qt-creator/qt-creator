// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QPointer>

namespace EffectComposer {
class EffectComposerModel;

class EffectComposerEditableNodesModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int selectedIndex MEMBER m_selectedIndex NOTIFY selectedIndexChanged)

public:
    explicit EffectComposerEditableNodesModel(QObject *parent = nullptr);

    struct Item
    {
        QString nodeName;
        int sourceId;
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    void setSourceModel(EffectComposerModel *sourceModel);
    QModelIndex proxyIndex(int sourceIndex) const;

    Q_INVOKABLE void openCodeEditor(int proxyIndex);

signals:
    void selectedIndexChanged(int);

private slots:
    void onSourceDataChanged(
        const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles);
    void onCodeEditorIndexChanged(int sourceIndex);
    void reload();

private:
    QPointer<EffectComposerModel> m_sourceModel;
    QList<Item> m_data;
    QMap<int, int> m_sourceToItemMap;
    int m_selectedIndex = -1;
};

} // namespace EffectComposer
