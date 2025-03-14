// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmlobjectnode.h>

#include <QObject>
#include <QTimer>

namespace QmlDesigner {

class ModelNode;

class QmlMaterialNodeProxy : public QObject
{
    Q_OBJECT

    Q_PROPERTY(ModelNode materialNode READ materialNode NOTIFY materialNodeChanged)
    Q_PROPERTY(QStringList possibleTypes READ possibleTypes NOTIFY possibleTypesChanged)
    Q_PROPERTY(int possibleTypeIndex READ possibleTypeIndex NOTIFY possibleTypeIndexChanged)
    Q_PROPERTY(QString previewEnv MEMBER m_previewEnv WRITE setPreviewEnv NOTIFY previewEnvChanged)
    Q_PROPERTY(QString previewModel MEMBER m_previewModel WRITE setPreviewModel NOTIFY previewModelChanged)

public:
    enum class ToolBarAction {
        ApplyToSelected = 0,
        ApplyToSelectedAdd,
        AddNewMaterial,
        DeleteCurrentMaterial,
        OpenMaterialBrowser
    };
    Q_ENUM(ToolBarAction)

    explicit QmlMaterialNodeProxy();
    ~QmlMaterialNodeProxy() override;

    void setup(const ModelNodes &editorNodes);

    QStringList possibleTypes() const { return m_possibleTypes; }

    ModelNode materialNode() const;
    ModelNodes editorNodes() const;

    int possibleTypeIndex() const { return m_possibleTypeIndex; }

    void setCurrentType(const QString &type);

    Q_INVOKABLE void toolBarAction(int action);

    void setPreviewEnv(const QString &envAndValue);
    void setPreviewModel(const QString &modelStr);

    void handleAuxiliaryPropertyChanges();

    static void registerDeclarativeType();

signals:
    void possibleTypesChanged();
    void possibleTypeIndexChanged();
    void materialNodeChanged();
    void previewEnvChanged();
    void previewModelChanged();

private: // Methods
    void setPossibleTypes(const QStringList &types);
    void updatePossibleTypes();
    void updatePossibleTypeIndex();
    void updatePreviewModel();
    void setMaterialNode(const QmlObjectNode &material);
    void setEditorNodes(const ModelNodes &editorNodes);

    bool hasQuick3DImport() const;

    AbstractView *materialView() const;

private:
    QmlObjectNode m_materialNode;
    ModelNodes m_editorNodes;

    QStringList m_possibleTypes;
    int m_possibleTypeIndex = -1;
    QString m_currentType;

    QString m_previewEnv;
    QString m_previewModel;
    QTimer m_previewUpdateTimer;
};

} // namespace QmlDesigner
