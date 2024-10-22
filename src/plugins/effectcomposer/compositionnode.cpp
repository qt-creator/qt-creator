// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compositionnode.h"

#include "effectcomposeruniformsmodel.h"
#include "effectshaderscodeeditor.h"
#include "effectutils.h"
#include "propertyhandler.h"
#include "uniform.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

namespace EffectComposer {

CompositionNode::CompositionNode(const QString &effectName, const QString &qenPath,
                                 const QJsonObject &jsonObject)
{
    QJsonObject json;
    if (jsonObject.isEmpty()) {
        QFile qenFile(qenPath);
        if (!qenFile.open(QIODevice::ReadOnly)) {
            qWarning("Couldn't open effect file.");
            return;
        }

        QByteArray loadData = qenFile.readAll();
        QJsonParseError parseError;
        QJsonDocument jsonDoc(QJsonDocument::fromJson(loadData, &parseError));

        if (parseError.error != QJsonParseError::NoError) {
            QString error = QString("Error parsing effect node");
            QString errorDetails = QString("%1: %2").arg(parseError.offset).arg(parseError.errorString());
            qWarning() << error;
            qWarning() << errorDetails;
            return;
        }
        json = jsonDoc.object().value("QEN").toObject();
        parse(effectName, qenPath, json);
    }
    else {
        parse(effectName, "", jsonObject);
    }
}

CompositionNode::~CompositionNode() = default;

QString CompositionNode::fragmentCode() const
{
    return m_fragmentCode;
}

QString CompositionNode::vertexCode() const
{
    return m_vertexCode;
}

QString CompositionNode::description() const
{
    return m_description;
}

QString CompositionNode::id() const
{
    return m_id;
}

QObject *CompositionNode::uniformsModel()
{
    return &m_unifomrsModel;
}

QStringList CompositionNode::requiredNodes() const
{
    return m_requiredNodes;
}

bool CompositionNode::isEnabled() const
{
    return m_isEnabled;
}

void CompositionNode::setIsEnabled(bool newIsEnabled)
{
    if (newIsEnabled != m_isEnabled) {
        m_isEnabled = newIsEnabled;
        emit isEnabledChanged();
    }
}

bool CompositionNode::isDependency() const
{
    return m_refCount > 0;
}

CompositionNode::NodeType CompositionNode::type() const
{
    return m_type;
}

void CompositionNode::parse(const QString &effectName, const QString &qenPath, const QJsonObject &json)
{
    int version = -1;
    if (json.contains("version"))
        version = json["version"].toInt(-1);
    if (version != 1) {
        QString error = QString("Error: Unknown effect version (%1)").arg(version);
        qWarning() << qPrintable(error);
        return;
    }

    m_name = json.value("name").toString();
    m_description = json.value("description").toString();
    setFragmentCode(EffectUtils::codeFromJsonArray(json.value("fragmentCode").toArray()));
    setVertexCode(EffectUtils::codeFromJsonArray(json.value("vertexCode").toArray()));

    if (json.contains("extraMargin"))
        m_extraMargin = json.value("extraMargin").toInt();

    if (json.contains("enabled"))
        m_isEnabled = json["enabled"].toBool();

    m_id = json.value("id").toString();
    if (m_id.isEmpty() && !qenPath.isEmpty()) {
        QString fileName = qenPath.split('/').last();
        fileName.chop(4); // remove ".qen"
        m_id = fileName;
    }

    // parse properties
    QJsonArray jsonProps = json.value("properties").toArray();
    for (const QJsonValueConstRef &prop : jsonProps) {
        const auto uniform = new Uniform(effectName, prop.toObject(), qenPath);
        m_unifomrsModel.addUniform(uniform);
        m_uniforms.append(uniform);
        g_propertyData.insert(uniform->name(), uniform->value());
        if (uniform->type() == Uniform::Type::Define) {
            // Changing defines requires rebaking the shaders
            connect(uniform, &Uniform::uniformValueChanged, this, &CompositionNode::rebakeRequested);
        }
    }

    // Seek through code to get tags
    QStringList shaderCodeLines;
    shaderCodeLines += m_vertexCode.split('\n');
    shaderCodeLines += m_fragmentCode.split('\n');
    for (const QString &codeLine : std::as_const(shaderCodeLines)) {
        QString trimmedLine = codeLine.trimmed();
        if (trimmedLine.startsWith("@requires")) {
            // Get the required node, remove "@requires "
            QString nodeId = trimmedLine.sliced(10).toLower();
            if (!nodeId.isEmpty() && !m_requiredNodes.contains(nodeId))
                m_requiredNodes << nodeId;
        }
    }
}

void CompositionNode::ensureShadersCodeEditor()
{
    if (m_shadersCodeEditor)
        return;

    m_shadersCodeEditor = Utils::makeUniqueObjectLatePtr<EffectShadersCodeEditor>(name());
    m_shadersCodeEditor->setFragmentValue(fragmentCode());
    m_shadersCodeEditor->setVertexValue(vertexCode());

    connect(m_shadersCodeEditor.get(), &EffectShadersCodeEditor::vertexValueChanged, this, [this] {
        setVertexCode(m_shadersCodeEditor->vertexValue());
    });

    connect(m_shadersCodeEditor.get(), &EffectShadersCodeEditor::fragmentValueChanged, this, [this] {
        setFragmentCode(m_shadersCodeEditor->fragmentValue());
    });

    connect(
        m_shadersCodeEditor.get(),
        &EffectShadersCodeEditor::rebakeRequested,
        this,
        &CompositionNode::rebakeRequested);
}

void CompositionNode::requestRebakeIfLiveUpdateMode()
{
    if (m_shadersCodeEditor && m_shadersCodeEditor->liveUpdate())
        emit rebakeRequested();
}

QList<Uniform *> CompositionNode::uniforms() const
{
    return m_uniforms;
}

int CompositionNode::incRefCount()
{
    ++m_refCount;

    if (m_refCount == 1)
        emit isDepencyChanged();

    return m_refCount;
}

int CompositionNode::decRefCount()
{
    --m_refCount;

    if (m_refCount == 0)
        emit isDepencyChanged();

    return m_refCount;
}

void CompositionNode::setRefCount(int count)
{
    bool notifyChange = (m_refCount > 0 && count == 0) || (m_refCount <= 0 && count > 0);

    m_refCount = count;

    if (notifyChange)
        emit isDepencyChanged();
}

void CompositionNode::setFragmentCode(const QString &fragmentCode)
{
    if (m_fragmentCode == fragmentCode)
        return;

    m_fragmentCode = fragmentCode;
    emit fragmentCodeChanged();

    requestRebakeIfLiveUpdateMode();
}

void CompositionNode::setVertexCode(const QString &vertexCode)
{
    if (m_vertexCode == vertexCode)
        return;

    m_vertexCode = vertexCode;
    emit vertexCodeChanged();

    requestRebakeIfLiveUpdateMode();
}

void CompositionNode::openShadersCodeEditor()
{
    ensureShadersCodeEditor();
    m_shadersCodeEditor->showWidget();
}

QString CompositionNode::name() const
{
    return m_name;
}

} // namespace EffectComposer

