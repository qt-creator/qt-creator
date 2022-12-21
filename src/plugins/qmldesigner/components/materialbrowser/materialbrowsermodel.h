// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qjsonobject.h"
#include <modelnode.h>
#include <qmlobjectnode.h>

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
    Q_PROPERTY(bool hasModelSelection READ hasModelSelection WRITE setHasModelSelection NOTIFY hasModelSelectionChanged)
    Q_PROPERTY(bool hasMaterialLibrary READ hasMaterialLibrary WRITE setHasMaterialLibrary NOTIFY hasMaterialLibraryChanged)
    Q_PROPERTY(QString copiedMaterialType READ copiedMaterialType WRITE setCopiedMaterialType NOTIFY copiedMaterialTypeChanged)
    Q_PROPERTY(QStringList defaultMaterialSections MEMBER m_defaultMaterialSections NOTIFY materialSectionsChanged)
    Q_PROPERTY(QStringList principledMaterialSections MEMBER m_principledMaterialSections NOTIFY materialSectionsChanged)
    Q_PROPERTY(QStringList specularGlossyMaterialSections MEMBER m_specularGlossyMaterialSections NOTIFY materialSectionsChanged)
    Q_PROPERTY(QStringList customMaterialSections MEMBER m_customMaterialSections NOTIFY materialSectionsChanged)

public:
    MaterialBrowserModel(QObject *parent = nullptr);
    ~MaterialBrowserModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setSearchText(const QString &searchText);
    void refreshSearch();

    bool hasQuick3DImport() const;
    void setHasQuick3DImport(bool b);

    bool hasModelSelection() const;
    void setHasModelSelection(bool b);

    bool hasMaterialLibrary() const;
    void setHasMaterialLibrary(bool b);

    bool isEmpty() const { return m_isEmpty; }

    QString copiedMaterialType() const;
    void setCopiedMaterialType(const QString &matType);

    QList<ModelNode> materials() const;
    void setMaterials(const QList<ModelNode> &materials, bool hasQuick3DImport);
    void removeMaterial(const ModelNode &material);
    void deleteSelectedMaterial();
    void updateMaterialName(const ModelNode &material);
    void updateSelectedMaterial();
    int materialIndex(const ModelNode &material) const;
    ModelNode materialAt(int idx) const;
    ModelNode selectedMaterial() const;
    bool loadPropertyGroups(const QString &path);
    void unloadPropertyGroups();

    void resetModel();

    Q_INVOKABLE void selectMaterial(int idx, bool force = false);
    Q_INVOKABLE void duplicateMaterial(int idx);
    Q_INVOKABLE void copyMaterialProperties(int idx, const QString &section);
    Q_INVOKABLE void pasteMaterialProperties(int idx);
    Q_INVOKABLE void deleteMaterial(int idx);
    Q_INVOKABLE void renameMaterial(int idx, const QString &newName);
    Q_INVOKABLE void addNewMaterial();
    Q_INVOKABLE void applyToSelected(qint64 internalId, bool add = false);
    Q_INVOKABLE void openMaterialEditor();
    Q_INVOKABLE bool isCopiedMaterialValid() const;

    struct PropertyCopyData
    {
        PropertyName name;
        TypeName dynamicTypeName;
        QVariant value;
        bool isBinding = false;
        bool isValid = false;
    };

signals:
    void isEmptyChanged();
    void hasQuick3DImportChanged();
    void hasModelSelectionChanged();
    void hasMaterialLibraryChanged();
    void copiedMaterialTypeChanged();
    void materialSectionsChanged();
    void selectedIndexChanged(int idx);
    void renameMaterialTriggered(const QmlDesigner::ModelNode &material, const QString &newName);
    void applyToSelectedTriggered(const QmlDesigner::ModelNode &material, bool add = false);
    void addNewMaterialTriggered();
    void duplicateMaterialTriggered(const QmlDesigner::ModelNode &material);
    void pasteMaterialPropertiesTriggered(
            const QmlDesigner::ModelNode &material,
            const QList<QmlDesigner::MaterialBrowserModel::PropertyCopyData> &props,
            bool all);

private:
    bool isMaterialVisible(int idx) const;
    bool isValidIndex(int idx) const;

    QString m_searchText;
    QList<ModelNode> m_materialList;
    QStringList m_defaultMaterialSections;
    QStringList m_specularGlossyMaterialSections;
    QStringList m_principledMaterialSections;
    QStringList m_customMaterialSections;
    ModelNode m_copiedMaterial;
    QList<PropertyCopyData> m_copiedMaterialProps;
    QHash<qint32, int> m_materialIndexHash; // internalId -> index
    QJsonObject m_propertyGroupsObj;

    int m_selectedIndex = -1;
    bool m_isEmpty = true;
    bool m_hasQuick3DImport = false;
    bool m_hasModelSelection = false;
    bool m_hasMaterialLibrary = false;
    bool m_allPropsCopied = true;
    QString m_copiedMaterialType;
};

} // namespace QmlDesigner
