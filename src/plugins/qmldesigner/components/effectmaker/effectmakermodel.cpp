// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectmakermodel.h"

#include "compositionnode.h"
#include "uniform.h"

#include <QRegularExpression>

#include <utils/qtcassert.h>

namespace QmlDesigner {

EffectMakerModel::EffectMakerModel(QObject *parent)
    : QAbstractListModel{parent}
{
}

QHash<int, QByteArray> EffectMakerModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "nodeName";
    roles[UniformsRole] = "nodeUniformsModel";
    return roles;
}

int EffectMakerModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return m_nodes.count();
}

QVariant EffectMakerModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.isValid() && index.row() < m_nodes.size(), return {});
    QTC_ASSERT(roleNames().contains(role), return {});

    return m_nodes.at(index.row())->property(roleNames().value(role));
}

void EffectMakerModel::addNode(const QString &nodeQenPath)
{
    beginInsertRows({}, m_nodes.size(), m_nodes.size());
    auto *node = new CompositionNode(nodeQenPath);
    m_nodes.append(node);
    endInsertRows();
}

void EffectMakerModel::removeNode(int idx)
{
    beginRemoveRows({}, idx, idx);
    CompositionNode *node = m_nodes.at(idx);
    m_nodes.removeAt(idx);
    delete node;
    endRemoveRows();
}

const QList<Uniform *> EffectMakerModel::allUniforms()
{
    QList<Uniform *> uniforms = {};
    for (const auto &node : std::as_const(m_nodes))
        uniforms.append(static_cast<EffectMakerUniformsModel *>(node->uniformsModel())->uniforms());
    return uniforms;
}

const QString EffectMakerModel::getBufUniform()
{
    QList<Uniform *> uniforms = allUniforms();
    QString s;
    s += "layout(std140, binding = 0) uniform buf {\n";
    s += "    mat4 qt_Matrix;\n";
    s += "    float qt_Opacity;\n";
    if (m_shaderFeatures.enabled(ShaderFeatures::Time))
        s += "    float iTime;\n";
    if (m_shaderFeatures.enabled(ShaderFeatures::Frame))
        s += "    int iFrame;\n";
    if (m_shaderFeatures.enabled(ShaderFeatures::Resolution))
        s += "    vec3 iResolution;\n";
    if (m_shaderFeatures.enabled(ShaderFeatures::Mouse))
        s += "    vec4 iMouse;\n";
    for (const auto uniform : uniforms) {
        // TODO: Check if uniform is already added.
        if (uniform->type() != Uniform::Type::Sampler && uniform->type() != Uniform::Type::Define) {
            QString type = Uniform::stringFromType(uniform->type());
            QString props = "    " + type + " " + uniform->name() + ";\n";
            s += props;
        }
    }
    s += "};\n";
    return s;
}

const QString EffectMakerModel::getVSUniforms()
{
    QString s;
    s += "#version 440\n";
    s += '\n';
    s += "layout(location = 0) in vec4 qt_Vertex;\n";
    s += "layout(location = 1) in vec2 qt_MultiTexCoord0;\n";
    s += "layout(location = 0) out vec2 texCoord;\n";
    if (m_shaderFeatures.enabled(ShaderFeatures::FragCoord))
        s += "layout(location = 1) out vec2 fragCoord;\n";
    s += '\n';
    s += getBufUniform();
    s += '\n';
    s += "out gl_PerVertex { vec4 gl_Position; };\n";
    s += '\n';
    return s;
}

const QString EffectMakerModel::getFSUniforms()
{
    QList<Uniform *> uniforms = allUniforms();
    QString s;
    s += "#version 440\n";
    s += '\n';
    s += "layout(location = 0) in vec2 texCoord;\n";
    if (m_shaderFeatures.enabled(ShaderFeatures::FragCoord))
        s += "layout(location = 1) in vec2 fragCoord;\n";
    s += "layout(location = 0) out vec4 fragColor;\n";
    s += '\n';
    s += getBufUniform();
    s += '\n';

    bool usesSource = m_shaderFeatures.enabled(ShaderFeatures::Source);
    if (usesSource)
        s += "layout(binding = 1) uniform sampler2D iSource;\n";

    // Add sampler uniforms
    int bindingIndex = usesSource ? 2 : 1;
    for (const auto uniform : uniforms) {
        // TODO: Check if uniform is already added.
        if (uniform->type() == Uniform::Type::Sampler) {
            // Start index from 2, 1 is source item
            QString props = QString("layout(binding = %1) uniform sampler2D %2")
                                .arg(bindingIndex).arg(uniform->name());
            s += props + ";\n";
            bindingIndex++;
        }
    }
    s += '\n';
    if (m_shaderFeatures.enabled(ShaderFeatures::BlurSources)) {
        const int blurItems = 5;
        for (int i = 1; i <= blurItems; i++) {
            QString props = QString("layout(binding = %1) uniform sampler2D iSourceBlur%2")
                                .arg(bindingIndex).arg(QString::number(i));
            s += props + ";\n";
            bindingIndex++;
        }
        s += '\n';
    }
    return s;
}


// Detects common GLSL error messages and returns potential
// additional error information related to them.
QString EffectMakerModel::detectErrorMessage(const QString &errorMessage)
{
    static QHash<QString, QString> nodeErrors {
        { "'BLUR_HELPER_MAX_LEVEL' : undeclared identifier", "BlurHelper"},
        { "'iSourceBlur1' : undeclared identifier", "BlurHelper"},
        { "'hash23' : no matching overloaded function found", "NoiseHelper" },
        { "'HASH_BOX_SIZE' : undeclared identifier", "NoiseHelper" },
        { "'pseudo3dNoise' : no matching overloaded function found", "NoiseHelper" }
    };

    QString missingNodeError = QStringLiteral("Are you missing a %1 node?\n");
    QHash<QString, QString>::const_iterator i = nodeErrors.constBegin();
    while (i != nodeErrors.constEnd()) {
        if (errorMessage.contains(i.key()))
            return missingNodeError.arg(i.value());
        ++i;
    }
    return QString();
}

// Return first error message (if any)
EffectError EffectMakerModel::effectError() const
{
    for (const EffectError &e : std::as_const(m_effectErrors)) {
        if (!e.m_message.isEmpty())
            return e;
    }
    return {};
}

// Set the effect error message with optional type and lineNumber.
// Type comes from ErrorTypes, defaulting to common errors (-1).
// Note that type must match with UI editor tab index.
void EffectMakerModel::setEffectError(const QString &errorMessage, int type, int lineNumber)
{
    EffectError error;
    error.m_type = type;
    if (type == 1 || type == 2) {
        // For shaders, get the line number from baker output.
        // Which is something like "ERROR: :15: message"
        int glslErrorLineNumber = -1;
        static QRegularExpression spaceReg("\\s+");
        QStringList errorStringList = errorMessage.split(spaceReg, Qt::SkipEmptyParts);
        if (errorStringList.size() >= 2) {
            QString lineString  = errorStringList.at(1).trimmed();
            if (lineString.size() >= 3) {
                // String is ":[linenumber]:", get only the number.
                glslErrorLineNumber = lineString.sliced(1, lineString.size() - 2).toInt();
            }
        }
        error.m_line = glslErrorLineNumber;
    } else {
        // For QML (and others) use given linenumber
        error.m_line = lineNumber;
    }

    QString additionalErrorInfo = detectErrorMessage(errorMessage);
    error.m_message = additionalErrorInfo + errorMessage;
    m_effectErrors.insert(type, error);
    Q_EMIT effectErrorChanged();
}

void EffectMakerModel::resetEffectError(int type)
{
    if (m_effectErrors.contains(type)) {
        m_effectErrors.remove(type);
        Q_EMIT effectErrorChanged();
    }
}

} // namespace QmlDesigner
