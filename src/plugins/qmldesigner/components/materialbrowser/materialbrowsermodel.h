/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include <modelnode.h>

#include <QAbstractListModel>
#include <QObject>
#include <QPointer>

namespace QmlDesigner {

class MaterialBrowserModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool isEmpty MEMBER m_isEmpty NOTIFY isEmptyChanged)
    Q_PROPERTY(int selectedIndex MEMBER m_selectedIndex NOTIFY selectedIndexChanged)
    Q_PROPERTY(bool hasQuick3DImport READ hasQuick3DImport WRITE setHasQuick3DImport NOTIFY hasQuick3DImportChanged)

public:
    MaterialBrowserModel(QObject *parent = nullptr);
    ~MaterialBrowserModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setSearchText(const QString &searchText);

    bool hasQuick3DImport() const;
    void setHasQuick3DImport(bool b);

    void setMaterials(const QList<ModelNode> &materials, bool hasQuick3DImport);
    void removeMaterial(const ModelNode &material);
    void updateMaterialName(const ModelNode &material);
    void updateSelectedMaterial();
    int materialIndex(const ModelNode &material) const;
    ModelNode materialAt(int idx) const;

    void resetModel();

    Q_INVOKABLE void selectMaterial(int idx, bool force = false);
    Q_INVOKABLE void deleteMaterial(int idx);
    Q_INVOKABLE void renameMaterial(int idx, const QString &newName);
    Q_INVOKABLE void addNewMaterial();
    Q_INVOKABLE void applyToSelected(qint64 internalId, bool add = false);
    Q_INVOKABLE void openMaterialEditor();

signals:
    void isEmptyChanged();
    void hasQuick3DImportChanged();
    void selectedIndexChanged(int idx);
    void renameMaterialTriggered(const QmlDesigner::ModelNode &material, const QString &newName);
    void applyToSelectedTriggered(const QmlDesigner::ModelNode &material, bool add = false);
    void addNewMaterialTriggered();

private:
    bool isMaterialVisible(int idx) const;
    bool isValidIndex(int idx) const;

    QString m_searchText;
    QList<ModelNode> m_materialList;
    QHash<qint32, int> m_materialIndexHash; // internalId -> index

    int m_selectedIndex = 0;
    bool m_isEmpty = true;
    bool m_hasQuick3DImport = false;
};

} // namespace QmlDesigner
