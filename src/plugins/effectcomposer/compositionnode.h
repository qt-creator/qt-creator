// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "effectcomposeruniformsmodel.h"

#include <utils/uniqueobjectptr.h>

#include <memory>

#include <QJsonObject>
#include <QObject>

namespace EffectComposer {

class EffectShadersCodeEditor;
struct ShaderEditorData;

class CompositionNode : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString nodeName READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(bool nodeEnabled READ isEnabled WRITE setIsEnabled NOTIFY isEnabledChanged)
    Q_PROPERTY(bool isDependency READ isDependency NOTIFY isDepencyChanged)
    Q_PROPERTY(bool isCustom READ isCustom NOTIFY isCustomChanged)
    Q_PROPERTY(QObject *nodeUniformsModel READ uniformsModel NOTIFY uniformsModelChanged)
    Q_PROPERTY(
        QString fragmentCode
        READ fragmentCode
        WRITE setFragmentCode
        NOTIFY fragmentCodeChanged)
    Q_PROPERTY(QString vertexCode READ vertexCode WRITE setVertexCode NOTIFY vertexCodeChanged)

public:
    enum NodeType {
        SourceNode = 0,
        DestinationNode,
        CustomNode
    };

    CompositionNode(const QString &effectName, const QString &qenPath, const QJsonObject &json = {});
    virtual ~CompositionNode();

    QString fragmentCode() const;
    QString vertexCode() const;
    QString description() const;
    QString id() const;

    QObject *uniformsModel();

    QStringList requiredNodes() const;

    NodeType type() const;

    bool isEnabled() const;
    void setIsEnabled(bool newIsEnabled);

    bool isDependency() const;
    bool isCustom() const;
    void setCustom(bool enable);

    QString name() const;
    void setName(const QString &name);

    QList<Uniform *> uniforms() const;

    int incRefCount();
    int decRefCount();
    void setRefCount(int count);

    int extraMargin() const { return m_extraMargin; }

    void setFragmentCode(const QString &fragmentCode);
    void setVertexCode(const QString &vertexCode);

    void markAsSaved();

    void openCodeEditor();
    void addUniform(const QVariantMap &data);
    void updateUniform(int index, const QVariantMap &data);
    void updateAreUniformsInUse(bool force = false);

signals:
    void uniformsModelChanged();
    void isEnabledChanged();
    void isDepencyChanged();
    void rebakeRequested();
    void fragmentCodeChanged();
    void vertexCodeChanged();
    void nameChanged();
    void isCustomChanged();

private slots:
    void onUniformRenamed(const QString &oldName, const QString &newName);

private:
    void parse(const QString &effectName, const QString &qenPath, const QJsonObject &json);
    void ensureCodeEditorData();
    void requestRebakeIfLiveUpdateMode();

    QString m_name;
    NodeType m_type = CustomNode;
    QString m_fragmentCode;
    QString m_vertexCode;
    QString m_description;
    QStringList m_requiredNodes;
    QString m_id;
    bool m_isEnabled = true;
    bool m_isCustom = false;
    int m_refCount = 0;
    int m_extraMargin = 0;
    bool m_InUseCheckNeeded = false;

    EffectComposerUniformsModel m_uniformsModel;
    std::unique_ptr<ShaderEditorData> m_shaderEditorData;
};

} // namespace EffectComposer
