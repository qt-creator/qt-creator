// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectmakermodel.h"

#include "compositionnode.h"
#include "syntaxhighlighterdata.h"
#include "uniform.h"

#include <QByteArrayView>
#include <QVector2D>

#include <utils/qtcassert.h>

namespace EffectMaker {

enum class FileType
{
    Binary,
    Text
};

static bool writeToFile(const QByteArray &buf, const QString &filename, FileType fileType)
{
    QDir().mkpath(QFileInfo(filename).path());
    QFile f(filename);
    QIODevice::OpenMode flags = QIODevice::WriteOnly | QIODevice::Truncate;
    if (fileType == FileType::Text)
        flags |= QIODevice::Text;
    if (!f.open(flags)) {
        qWarning() << "Failed to open file for writing:" << filename;
        return false;
    }
    f.write(buf);
    return true;
}

EffectMakerModel::EffectMakerModel(QObject *parent)
    : QAbstractListModel{parent}
{
    m_vertexShaderFile.setFileTemplate(QDir::tempPath() + "/dsem_XXXXXX.vert.qsb");
    m_fragmentShaderFile.setFileTemplate(QDir::tempPath() + "/dsem_XXXXXX.frag.qsb");
    // TODO: Will be revisted later when saving output files
    if (m_vertexShaderFile.open())
        qInfo() << "Using temporary vs file:" << m_vertexShaderFile.fileName();
    if (m_fragmentShaderFile.open())
        qInfo() << "Using temporary fs file:" << m_fragmentShaderFile.fileName();

    // Prepare baker
    m_baker.setGeneratedShaderVariants({ QShader::StandardShader });
    updateBakedShaderVersions();
}

QHash<int, QByteArray> EffectMakerModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "nodeName";
    roles[EnabledRole] = "nodeEnabled";
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

bool EffectMakerModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || !roleNames().contains(role))
        return false;

    if (role == EnabledRole) {
        m_nodes.at(index.row())->setIsEnabled(value.toBool());
        emit dataChanged(index, index, {role});
    }

    return true;
}

void EffectMakerModel::setIsEmpty(bool val)
{
    if (m_isEmpty != val) {
        m_isEmpty = val;
        emit isEmptyChanged();
    }
}

void EffectMakerModel::addNode(const QString &nodeQenPath)
{
    beginInsertRows({}, m_nodes.size(), m_nodes.size());
    auto *node = new CompositionNode(nodeQenPath);
    m_nodes.append(node);
    endInsertRows();

    setIsEmpty(false);

    bakeShaders();
}

void EffectMakerModel::moveNode(int fromIdx, int toIdx)
{
    if (fromIdx == toIdx)
        return;

    int toIdxAdjusted = fromIdx < toIdx ? toIdx + 1 : toIdx; // otherwise beginMoveRows() crashes
    beginMoveRows({}, fromIdx, fromIdx, {}, toIdxAdjusted);
    m_nodes.move(fromIdx, toIdx);
    endMoveRows();

    bakeShaders();
}

void EffectMakerModel::removeNode(int idx)
{
    beginRemoveRows({}, idx, idx);
    CompositionNode *node = m_nodes.at(idx);
    m_nodes.removeAt(idx);
    delete node;
    endRemoveRows();

    if (m_nodes.isEmpty())
        setIsEmpty(true);
    else
        bakeShaders();
}

void EffectMakerModel::updateBakedShaderVersions()
{
    QList<QShaderBaker::GeneratedShader> targets;
    targets.append({ QShader::SpirvShader, QShaderVersion(100) }); // Vulkan 1.0
    targets.append({ QShader::HlslShader, QShaderVersion(50) }); // Shader Model 5.0
    targets.append({ QShader::MslShader, QShaderVersion(12) }); // Metal 1.2
    targets.append({ QShader::GlslShader, QShaderVersion(300, QShaderVersion::GlslEs) }); // GLES 3.0+
    targets.append({ QShader::GlslShader, QShaderVersion(410) }); // OpenGL 4.1+
    targets.append({ QShader::GlslShader, QShaderVersion(330) }); // OpenGL 3.3
    targets.append({ QShader::GlslShader, QShaderVersion(140) }); // OpenGL 3.1
    //TODO: Do we need support for legacy shaders 100, 120?

    m_baker.setGeneratedShaders(targets);
}

QString EffectMakerModel::fragmentShader() const
{
    return m_fragmentShader;
}

void EffectMakerModel::setFragmentShader(const QString &newFragmentShader)
{
    if (m_fragmentShader == newFragmentShader)
        return;

    m_fragmentShader = newFragmentShader;
}

QString EffectMakerModel::vertexShader() const
{
    return m_vertexShader;
}

void EffectMakerModel::setVertexShader(const QString &newVertexShader)
{
    if (m_vertexShader == newVertexShader)
        return;

    m_vertexShader = newVertexShader;
}

const QString &EffectMakerModel::qmlComponentString() const
{
    return m_qmlComponentString;
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
    const QList<Uniform *> uniforms = allUniforms();
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
        QStringList errorStringList = errorMessage.split(m_spaceReg, Qt::SkipEmptyParts);
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

// Get value in QML format that used for exports
QString EffectMakerModel::valueAsString(const Uniform &uniform)
{
    if (uniform.type() == Uniform::Type::Bool) {
        return uniform.value().toBool() ? QString("true") : QString("false");
    } else if (uniform.type() == Uniform::Type::Int) {
        return QString::number(uniform.value().toInt());
    } else if (uniform.type() == Uniform::Type::Float) {
        return QString::number(uniform.value().toDouble());
    } else if (uniform.type() == Uniform::Type::Vec2) {
        QVector2D v2 = uniform.value().value<QVector2D>();
        return QString("Qt.point(%1, %2)").arg(v2.x(), v2.y());
    } else if (uniform.type() == Uniform::Type::Vec3) {
        QVector3D v3 = uniform.value().value<QVector3D>();
        return QString("Qt.vector3d(%1, %2, %3)").arg(v3.x(), v3.y(), v3.z());
    } else if (uniform.type() == Uniform::Type::Vec4) {
        QVector4D v4 = uniform.value().value<QVector4D>();
        return QString("Qt.vector4d(%1, %2, %3, %4)").arg(v4.x(), v4.y(), v4.z(), v4.w());
    } else if (uniform.type() == Uniform::Type::Color) {
        QColor c = uniform.value().value<QColor>();
        return QString("Qt.rgba(%1, %2, %3, %4)").arg(c.redF(), c.greenF(), c.blueF(), c.alphaF());
    } else if (uniform.type() == Uniform::Type::Sampler) {
        return getImageElementName(uniform);
    } else if (uniform.type() == Uniform::Type::Define) {
        return uniform.value().toString();
    } else {
        qWarning() << QString("Unhandled const variable type: %1").arg(int(uniform.type())).toLatin1();
        return QString();
    }
}

// Get value in QML binding that used for previews
QString EffectMakerModel::valueAsBinding(const Uniform &uniform)
{
    if (uniform.type() == Uniform::Type::Bool || uniform.type() == Uniform::Type::Int
        || uniform.type() == Uniform::Type::Float || uniform.type() == Uniform::Type::Define) {
        return "g_propertyData." + uniform.name();
    } else if (uniform.type() == Uniform::Type::Vec2) {
        QString sx = QString("g_propertyData.%1.x").arg(uniform.name());
        QString sy = QString("g_propertyData.%1.y").arg(uniform.name());
        return QString("Qt.point(%1, %2)").arg(sx, sy);
    } else if (uniform.type() == Uniform::Type::Vec3) {
        QString sx = QString("g_propertyData.%1.x").arg(uniform.name());
        QString sy = QString("g_propertyData.%1.y").arg(uniform.name());
        QString sz = QString("g_propertyData.%1.z").arg(uniform.name());
        return QString("Qt.vector3d(%1, %2, %3)").arg(sx, sy, sz);
    } else if (uniform.type() == Uniform::Type::Vec4) {
        QString sx = QString("g_propertyData.%1.x").arg(uniform.name());
        QString sy = QString("g_propertyData.%1.y").arg(uniform.name());
        QString sz = QString("g_propertyData.%1.z").arg(uniform.name());
        QString sw = QString("g_propertyData.%1.w").arg(uniform.name());
        return QString("Qt.vector4d(%1, %2, %3, %4)").arg(sx, sy, sz, sw);
    } else if (uniform.type() == Uniform::Type::Color) {
        QString sr = QString("g_propertyData.%1.r").arg(uniform.name());
        QString sg = QString("g_propertyData.%1.g").arg(uniform.name());
        QString sb = QString("g_propertyData.%1.b").arg(uniform.name());
        QString sa = QString("g_propertyData.%1.a").arg(uniform.name());
        return QString("Qt.rgba(%1, %2, %3, %4)").arg(sr, sg, sb, sa);
    } else if (uniform.type() == Uniform::Type::Sampler) {
        return getImageElementName(uniform);
    } else {
        qWarning() << QString("Unhandled const variable type: %1").arg(int(uniform.type())).toLatin1();
        return QString();
    }
}

// Get value in GLSL format that is used for non-exported const properties
QString EffectMakerModel::valueAsVariable(const Uniform &uniform)
{
    if (uniform.type() == Uniform::Type::Bool) {
        return uniform.value().toBool() ? QString("true") : QString("false");
    } else if (uniform.type() == Uniform::Type::Int) {
        return QString::number(uniform.value().toInt());
    } else if (uniform.type() == Uniform::Type::Float) {
        return QString::number(uniform.value().toDouble());
    } else if (uniform.type() == Uniform::Type::Vec2) {
        QVector2D v2 = uniform.value().value<QVector2D>();
        return QString("vec2(%1, %2)").arg(v2.x(), v2.y());
    } else if (uniform.type() == Uniform::Type::Vec3) {
        QVector3D v3 = uniform.value().value<QVector3D>();
        return QString("vec3(%1, %2, %3)").arg(v3.x(), v3.y(), v3.z());
    } else if (uniform.type() == Uniform::Type::Vec4) {
        QVector4D v4 = uniform.value().value<QVector4D>();
        return QString("vec4(%1, %2, %3, %4)").arg(v4.x(), v4.y(), v4.z(), v4.w());
    } else if (uniform.type() == Uniform::Type::Color) {
        QColor c = uniform.value().value<QColor>();
        return QString("vec4(%1, %2, %3, %4)").arg(c.redF(), c.greenF(), c.blueF(), c.alphaF());
    } else {
        qWarning() << QString("Unhandled const variable type: %1").arg(int(uniform.type())).toLatin1();
        return QString();
    }
}

// Return name for the image property Image element
QString EffectMakerModel::getImageElementName(const Uniform &uniform)
{
    // TODO
    Q_UNUSED(uniform)
    return {};
}

const QString EffectMakerModel::getConstVariables()
{
    const QList<Uniform *> uniforms = allUniforms();
    QString s;
    for (Uniform *uniform : uniforms) {
        // TODO: Check if uniform is already added.
        QString constValue = valueAsVariable(*uniform);
        QString type = Uniform::stringFromType(uniform->type());
        s += QString("const %1 %2 = %3;\n").arg(type, uniform->name(), constValue);
    }
    if (!s.isEmpty())
        s += '\n';

    return s;
}

const QString EffectMakerModel::getDefineProperties()
{
    const QList<Uniform *> uniforms = allUniforms();
    QString s;
    for (Uniform *uniform : uniforms) {
        // TODO: Check if uniform is already added.
        if (uniform->type() == Uniform::Type::Define) {
            QString defineValue = uniform->value().toString();
            s += QString("#define %1 %2\n").arg(uniform->name(), defineValue);
        }
    }
    if (!s.isEmpty())
        s += '\n';

    return s;
}

int EffectMakerModel::getTagIndex(const QStringList &code, const QString &tag)
{
    int index = -1;
    int line = 0;
    const QString tagString = QString("@%1").arg(tag);
    for (const QString &s : code) {
        auto st = s.trimmed();
        // Check if line or first non-space content of the line matches to tag
        static auto spaceReg = QRegularExpression("\\s");
        auto firstSpace = st.indexOf(spaceReg);
        QString firstWord = st;
        if (firstSpace > 0)
            firstWord = st.sliced(0, firstSpace);
        if (firstWord == tagString) {
            index = line;
            break;
        }
        line++;
    }
    return index;
}

QString EffectMakerModel::processVertexRootLine(const QString &line)
{
    QString output;
    QStringList lineList = line.split(m_spaceReg, Qt::SkipEmptyParts);
    if (lineList.length() > 1 && lineList.at(0) == QStringLiteral("out")) {
        lineList.removeFirst();
        QString outLine = lineList.join(' ');
        m_shaderVaryingVariables << outLine;
    } else {
        output = line + '\n';
    }
    return output;
}

QString EffectMakerModel::processFragmentRootLine(const QString &line)
{
    QString output;
    QStringList lineList = line.split(m_spaceReg, Qt::SkipEmptyParts);
    // Just skip all "in" variables. It is enough to have "out" variable in vertex.
    if (lineList.length() > 1 && lineList.at(0) == QStringLiteral("in"))
        return QString();
    output = line + '\n';
    return output;
}

QStringList EffectMakerModel::getDefaultRootVertexShader()
{
    if (m_defaultRootVertexShader.isEmpty()) {
        m_defaultRootVertexShader << "void main() {";
        m_defaultRootVertexShader << "    texCoord = qt_MultiTexCoord0;";
        m_defaultRootVertexShader << "    fragCoord = qt_Vertex.xy;";
        m_defaultRootVertexShader << "    vec2 vertCoord = qt_Vertex.xy;";
        m_defaultRootVertexShader << "    @nodes";
        m_defaultRootVertexShader << "    gl_Position = qt_Matrix * vec4(vertCoord, 0.0, 1.0);";
        m_defaultRootVertexShader << "}";
    }
    return m_defaultRootVertexShader;
}

QStringList EffectMakerModel::getDefaultRootFragmentShader()
{
    if (m_defaultRootFragmentShader.isEmpty()) {
        m_defaultRootFragmentShader << "void main() {";
        m_defaultRootFragmentShader << "    fragColor = texture(iSource, texCoord);";
        m_defaultRootFragmentShader << "    @nodes";
        m_defaultRootFragmentShader << "    fragColor = fragColor * qt_Opacity;";
        m_defaultRootFragmentShader << "}";
    }
    return m_defaultRootFragmentShader;
}

// Remove all post-processing tags ("@tag") from the code.
// Except "@nodes" tag as that is handled later.
QStringList EffectMakerModel::removeTagsFromCode(const QStringList &codeLines)
{
    QStringList s;
    for (const QString &line : codeLines) {
        const auto trimmedLine = line.trimmed();
        if (!trimmedLine.startsWith('@') || trimmedLine.startsWith("@nodes")) {
            s << line;
        } else {
            // Check if the tag is known
            bool validTag = false;
            const QList<QByteArrayView> tags = SyntaxHighlighterData::reservedTagNames();
            QString firstWord = trimmedLine.split(m_spaceReg, Qt::SkipEmptyParts).first();
            for (const QByteArrayView &tag : tags) {
                if (firstWord == QString::fromUtf8(tag)) {
                    validTag = true;
                    break;
                }
            }
            if (!validTag)
                setEffectError(QString("Unknown tag: %1").arg(trimmedLine), ErrorPreprocessor);
        }
    }
    return s;
}

QString EffectMakerModel::removeTagsFromCode(const QString &code)
{
    QStringList codeLines = removeTagsFromCode(code.split('\n'));
    return codeLines.join('\n');
}

QString EffectMakerModel::getCustomShaderVaryings(bool outState)
{
    QString output;
    QString direction = outState ? QStringLiteral("out") : QStringLiteral("in");
    int varLocation = m_shaderFeatures.enabled(ShaderFeatures::FragCoord) ? 2 : 1;
    for (const QString &var : std::as_const(m_shaderVaryingVariables)) {
        output += QString("layout(location = %1) %2 %3\n").arg(QString::number(varLocation), direction, var);
        varLocation++;
    }
    return output;
}

QString EffectMakerModel::generateVertexShader(bool includeUniforms)
{
    QString s;

    if (includeUniforms)
        s += getVSUniforms();

    // Remove tags when not generating for features check
    const bool removeTags = includeUniforms;

    s += getDefineProperties();
    // s += getConstVariables(); // Not sure yet, will check on this later

    // When the node is complete, add shader code in correct nodes order
    // split to root and main parts
    QString s_root;
    QString s_main;
    QStringList s_sourceCode;
    m_shaderVaryingVariables.clear();
    for (const CompositionNode *n : std::as_const(m_nodes)) {
        if (!n->vertexCode().isEmpty() && n->isEnabled()) {
            if (n->type() == CompositionNode::NodeType::SourceNode) {
                s_sourceCode = n->vertexCode().split('\n');
            } else if (n->type() == CompositionNode::NodeType::CustomNode) {
                const QStringList vertexCode = n->vertexCode().split('\n');
                int mainIndex = getTagIndex(vertexCode, QStringLiteral("main"));
                int line = 0;
                for (const QString &ss : vertexCode) {
                    if (mainIndex == -1 || line > mainIndex)
                        s_main += QStringLiteral("    ") + ss + '\n';
                    else if (line < mainIndex)
                        s_root += processVertexRootLine(ss);
                    line++;
                }
            }
        }
    }

    if (s_sourceCode.isEmpty()) {
        // If source nodes doesn't contain any code, use default one
        s_sourceCode << getDefaultRootVertexShader();
    }

    if (removeTags) {
        s_sourceCode = removeTagsFromCode(s_sourceCode);
        s_root = removeTagsFromCode(s_root);
        s_main = removeTagsFromCode(s_main);
    }

    s += getCustomShaderVaryings(true);
    s += s_root + '\n';

    int nodesIndex = getTagIndex(s_sourceCode, QStringLiteral("nodes"));
    int line = 0;
    for (const QString &ss : std::as_const(s_sourceCode))
        s += (line++ == nodesIndex) ? s_main : ss + '\n';

    return s;
}

QString EffectMakerModel::generateFragmentShader(bool includeUniforms)
{
    QString s;

    if (includeUniforms)
        s += getFSUniforms();

    // Remove tags when not generating for features check
    const bool removeTags = includeUniforms;

    s += getDefineProperties();
    // s += getConstVariables(); // Not sure yet, will check on this later

    // When the node is complete, add shader code in correct nodes order
    // split to root and main parts
    QString s_root;
    QString s_main;
    QStringList s_sourceCode;
    for (const CompositionNode *n : std::as_const(m_nodes)) {
        if (!n->fragmentCode().isEmpty() && n->isEnabled()) {
            if (n->type() == CompositionNode::NodeType::SourceNode) {
                s_sourceCode = n->fragmentCode().split('\n');
            } else if (n->type() == CompositionNode::NodeType::CustomNode) {
                const QStringList fragmentCode = n->fragmentCode().split('\n');
                int mainIndex = getTagIndex(fragmentCode, QStringLiteral("main"));
                int line = 0;
                for (const QString &ss : fragmentCode) {
                    if (mainIndex == -1 || line > mainIndex)
                        s_main += QStringLiteral("    ") + ss + '\n';
                    else if (line < mainIndex)
                        s_root += processFragmentRootLine(ss);
                    line++;
                }
            }
        }
    }

    if (s_sourceCode.isEmpty()) {
        // If source nodes doesn't contain any code, use default one
        s_sourceCode << getDefaultRootFragmentShader();
    }

    if (removeTags) {
        s_sourceCode = removeTagsFromCode(s_sourceCode);
        s_root = removeTagsFromCode(s_root);
        s_main = removeTagsFromCode(s_main);
    }

    s += getCustomShaderVaryings(false);
    s += s_root + '\n';

    int nodesIndex = getTagIndex(s_sourceCode, QStringLiteral("nodes"));
    int line = 0;
    for (const QString &ss : std::as_const(s_sourceCode))
        s += (line++ == nodesIndex) ? s_main : ss + '\n';

    return s;
}

// Generates string of the custom properties (uniforms) into ShaderEffect component
// Also generates QML images elements for samplers.
void EffectMakerModel::updateCustomUniforms()
{
    QString exportedRootPropertiesString;
    QString previewEffectPropertiesString;
    QString exportedEffectPropertiesString;

    const QList<Uniform *> uniforms = allUniforms();
    for (Uniform *uniform : uniforms) {
        // TODO: Check if uniform is already added.
        const bool isDefine = uniform->type() == Uniform::Type::Define;
        QString type = Uniform::typeToProperty(uniform->type());
        QString value = valueAsString(*uniform);
        QString bindedValue = valueAsBinding(*uniform);
        // When user has set custom uniform value, use it as-is
        if (uniform->useCustomValue()) {
            value = uniform->customValue();
            bindedValue = value;
        }
        // Note: Define type properties appear also as QML properties (in preview) in case QML side
        // needs to use them. This is used at least by BlurHelper BLUR_HELPER_MAX_LEVEL.
        QString propertyName = isDefine ? uniform->name().toLower() : uniform->name();
        if (!uniform->useCustomValue() && !isDefine && !uniform->description().isEmpty()) {
            // When exporting, add API documentation for properties
            const QStringList descriptionLines = uniform->description().split('\n');
            for (const QString &line : descriptionLines) {
                if (line.trimmed().isEmpty())
                    exportedRootPropertiesString += QStringLiteral("    //\n");
                else
                    exportedRootPropertiesString += QStringLiteral("    // ") + line + '\n';
            }
        }
        QString valueString = value.isEmpty() ? QString() : QString(": %1").arg(value);
        QString bindedValueString = bindedValue.isEmpty() ? QString() : QString(": %1").arg(bindedValue);
        // Custom values are not readonly, others inside the effect can be
        QString readOnly = uniform->useCustomValue() ? QString() : QStringLiteral("readonly ");
        previewEffectPropertiesString += "    " + readOnly + "property " + type + " "
                                         + propertyName + bindedValueString + '\n';
        // Define type properties are not added into exports
        if (!isDefine) {
            if (uniform->useCustomValue()) {
                // Custom values are only inside the effect, with description comments
                if (!uniform->description().isEmpty()) {
                    const QStringList descriptionLines = uniform->description().split('\n');
                    for (const QString &line : descriptionLines)
                        exportedEffectPropertiesString += QStringLiteral("        // ") + line + '\n';
                }
                exportedEffectPropertiesString += QStringLiteral("        ") + readOnly
                                                  + "property " + type + " " + propertyName
                                                  + bindedValueString + '\n';
            } else {
                // Custom values are not added into root
                exportedRootPropertiesString += "    property " + type + " " + propertyName
                                                + valueString + '\n';
                exportedEffectPropertiesString += QStringLiteral("        ")
                                                  + readOnly + "property alias " + propertyName
                                                  + ": rootItem." + uniform->name() + '\n';
            }
        }
    }

    // See if any of the properties changed
    // TODO
}

void EffectMakerModel::bakeShaders()
{
    resetEffectError(ErrorPreprocessor);
    if (m_vertexShader == generateVertexShader() && m_fragmentShader == generateFragmentShader()) {
        setShadersUpToDate(true);
        return;
    }

    setShadersUpToDate(false);

    // First update the features based on shader content
    // This will make sure that next calls to "generate" will produce correct uniforms.
    m_shaderFeatures.update(generateVertexShader(false), generateFragmentShader(false),
                            m_previewEffectPropertiesString);

    updateCustomUniforms();

    setVertexShader(generateVertexShader());
    QString vs = m_vertexShader;
    m_baker.setSourceString(vs.toUtf8(), QShader::VertexStage);
    QShader vertShader = m_baker.bake();
    if (!vertShader.isValid()) {
        qWarning() << "Shader baking failed:" << qPrintable(m_baker.errorMessage());
        setEffectError(m_baker.errorMessage().split('\n').first(), ErrorVert);
    } else {
        QString filename = m_vertexShaderFile.fileName();
        writeToFile(vertShader.serialized(), filename, FileType::Binary);
        resetEffectError(ErrorVert);
    }

    setFragmentShader(generateFragmentShader());
    QString fs = m_fragmentShader;
    m_baker.setSourceString(fs.toUtf8(), QShader::FragmentStage);

    QShader fragShader = m_baker.bake();
    if (!fragShader.isValid()) {
        qWarning() << "Shader baking failed:" << qPrintable(m_baker.errorMessage());
        setEffectError(m_baker.errorMessage().split('\n').first(), ErrorFrag);
    } else {
        QString filename = m_fragmentShaderFile.fileName();
        writeToFile(fragShader.serialized(), filename, FileType::Binary);
        resetEffectError(ErrorFrag);
    }

    if (vertShader.isValid() && fragShader.isValid()) {
        Q_EMIT shadersBaked();
        setShadersUpToDate(true);
    }

    // TODO: Mark shaders as baked, required by export later
}

bool EffectMakerModel::shadersUpToDate() const
{
    return m_shadersUpToDate;
}

void EffectMakerModel::setShadersUpToDate(bool UpToDate)
{
    if (m_shadersUpToDate == UpToDate)
        return;
    m_shadersUpToDate = UpToDate;
    emit shadersUpToDateChanged();
}

QString EffectMakerModel::getQmlImagesString(bool localFiles)
{
    Q_UNUSED(localFiles)

    // TODO
    return QString();
}

QString EffectMakerModel::getQmlComponentString(bool localFiles)
{
    auto addProperty = [localFiles](const QString &name, const QString &var,
                                    const QString &type, bool blurHelper = false)
    {
        if (localFiles) {
            const QString parent = blurHelper ? QString("blurHelper.") : QString("rootItem.");
            return QString("readonly property alias %1: %2%3\n").arg(name, parent, var);
        } else {
            const QString parent = blurHelper ? "blurHelper." : QString();
            return QString("readonly property %1 %2: %3%4\n").arg(type, name, parent, var);
        }
    };

    QString customImagesString = getQmlImagesString(localFiles);
    QString vertexShaderFilename = "file:///" + m_fragmentShaderFile.fileName();
    QString fragmentShaderFilename = "file:///" + m_vertexShaderFile.fileName();
    QString s;
    QString l1 = localFiles ? QStringLiteral("    ") : QStringLiteral("");
    QString l2 = localFiles ? QStringLiteral("        ") : QStringLiteral("    ");
    QString l3 = localFiles ? QStringLiteral("            ") : QStringLiteral("        ");

    if (!localFiles)
        s += "import QtQuick\n";
    s += l1 + "ShaderEffect {\n";
    if (m_shaderFeatures.enabled(ShaderFeatures::Source))
        s += l2 + addProperty("iSource", "source", "Item");
    if (m_shaderFeatures.enabled(ShaderFeatures::Time))
        s += l2 + addProperty("iTime", "animatedTime", "real");
    if (m_shaderFeatures.enabled(ShaderFeatures::Frame))
        s += l2 + addProperty("iFrame", "animatedFrame", "int");
    if (m_shaderFeatures.enabled(ShaderFeatures::Resolution)) {
        // Note: Pixel ratio is currently always 1.0
        s += l2 + "readonly property vector3d iResolution: Qt.vector3d(width, height, 1.0)\n";
    }
    if (m_shaderFeatures.enabled(ShaderFeatures::Mouse)) { // Do we need interactive effects?
        s += l2 + "readonly property vector4d iMouse: Qt.vector4d(rootItem._effectMouseX, rootItem._effectMouseY,\n";
        s += l2 + "                                               rootItem._effectMouseZ, rootItem._effectMouseW)\n";
    }
    if (m_shaderFeatures.enabled(ShaderFeatures::BlurSources)) {
        s += l2 + addProperty("iSourceBlur1", "blurSrc1", "Item", true);
        s += l2 + addProperty("iSourceBlur2", "blurSrc2", "Item", true);
        s += l2 + addProperty("iSourceBlur3", "blurSrc3", "Item", true);
        s += l2 + addProperty("iSourceBlur4", "blurSrc4", "Item", true);
        s += l2 + addProperty("iSourceBlur5", "blurSrc5", "Item", true);
    }
    // When used in preview component, we need property with value
    // and when in exported component, property with binding to root value.
    s += localFiles ? m_exportedEffectPropertiesString : m_previewEffectPropertiesString;
    if (!customImagesString.isEmpty())
        s += '\n' + customImagesString;

    s += '\n';
    s += l2 + "vertexShader: '" + vertexShaderFilename + "'\n";
    s += l2 + "fragmentShader: '" + fragmentShaderFilename + "'\n";
    s += l2 + "anchors.fill: parent\n";
    if (m_shaderFeatures.enabled(ShaderFeatures::GridMesh)) {
        QString gridSize = QString("%1, %2").arg(m_shaderFeatures.gridMeshWidth()).arg(m_shaderFeatures.gridMeshHeight());
        s += l2 + "mesh: GridMesh {\n";
        s += l3 + QString("resolution: Qt.size(%1)\n").arg(gridSize);
        s += l2 + "}\n";
    }
    s += l1 + "}\n";
    return s;
}

void EffectMakerModel::updateQmlComponent()
{
    // Clear possible QML runtime errors
    resetEffectError(ErrorQMLRuntime);
    m_qmlComponentString = getQmlComponentString(false);
}

} // namespace EffectMaker

