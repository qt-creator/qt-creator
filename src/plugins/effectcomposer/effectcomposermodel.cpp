// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectcomposermodel.h"

#include "compositionnode.h"
#include "effectutils.h"
#include "propertyhandler.h"
#include "syntaxhighlighterdata.h"
#include "uniform.h"

#include <projectexplorer/projecttree.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <modelnodeoperations.h>

#include <QByteArrayView>
#include <QLibraryInfo>
#include <QTemporaryDir>
#include <QVector2D>

namespace EffectComposer {

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

EffectComposerModel::EffectComposerModel(QObject *parent)
    : QAbstractListModel{parent}
    , m_shaderDir(QDir::tempPath() + "/qds_ec_XXXXXX")
{
    m_rebakeTimer.setSingleShot(true);
    connect(&m_rebakeTimer, &QTimer::timeout, this, &EffectComposerModel::bakeShaders);
}

QHash<int, QByteArray> EffectComposerModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "nodeName";
    roles[EnabledRole] = "nodeEnabled";
    roles[UniformsRole] = "nodeUniformsModel";
    roles[Dependency] = "isDependency";
    return roles;
}

int EffectComposerModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return m_nodes.count();
}

QVariant EffectComposerModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.isValid() && index.row() < m_nodes.size(), return {});
    QTC_ASSERT(roleNames().contains(role), return {});

    return m_nodes.at(index.row())->property(roleNames().value(role));
}

bool EffectComposerModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || !roleNames().contains(role))
        return false;

    if (role == EnabledRole) {
        m_nodes.at(index.row())->setIsEnabled(value.toBool());
        bakeShaders();
        setHasUnsavedChanges(true);

        emit dataChanged(index, index, {role});
    }

    return true;
}

void EffectComposerModel::setEffectsTypePrefix(const QString &prefix)
{
    m_effectTypePrefix = prefix;
}

void EffectComposerModel::setIsEmpty(bool val)
{
    if (m_isEmpty != val) {
        m_isEmpty = val;
        emit isEmptyChanged();

        if (m_isEmpty)
            bakeShaders();
    }
}

void EffectComposerModel::addNode(const QString &nodeQenPath)
{
    beginResetModel();
    auto *node = new CompositionNode({}, nodeQenPath);
    connectCompositionNode(node);

    const QList<QString> requiredNodes = node->requiredNodes();
    if (requiredNodes.size() > 0) {
        for (const QString &requiredId : requiredNodes) {
            if (auto reqNode = findNodeById(requiredId)) {
                reqNode->incRefCount();
                continue;
            }

            const QString path = EffectUtils::nodesSourcesPath() + "/common/" + requiredId + ".qen";
            auto requiredNode = new CompositionNode({}, path);
            connectCompositionNode(requiredNode);
            requiredNode->setRefCount(1);
            m_nodes.prepend(requiredNode);
        }
    }
    m_nodes.append(node);
    endResetModel();

    setIsEmpty(false);

    bakeShaders();
    setHasUnsavedChanges(true);

    emit nodesChanged();
}

CompositionNode *EffectComposerModel::findNodeById(const QString &id) const
{
    for (CompositionNode *node : std::as_const(m_nodes)) {
        if (node->id() == id)
            return node;
    }
    return {};
}

void EffectComposerModel::moveNode(int fromIdx, int toIdx)
{
    if (fromIdx == toIdx)
        return;

    int toIdxAdjusted = fromIdx < toIdx ? toIdx + 1 : toIdx; // otherwise beginMoveRows() crashes
    beginMoveRows({}, fromIdx, fromIdx, {}, toIdxAdjusted);
    m_nodes.move(fromIdx, toIdx);
    endMoveRows();

    setHasUnsavedChanges(true);
    bakeShaders();
}

void EffectComposerModel::removeNode(int idx)
{
    beginResetModel();
    m_rebakeTimer.stop();
    CompositionNode *node = m_nodes.takeAt(idx);

    const QStringList reqNodes = node->requiredNodes();
    for (const QString &reqId : reqNodes) {
        CompositionNode *depNode = findNodeById(reqId);
        if (depNode && depNode->decRefCount() <= 0) {
            m_nodes.removeOne(depNode);
            delete depNode;
        }
    }

    delete node;
    endResetModel();

    if (m_nodes.isEmpty())
        setIsEmpty(true);
    else
        bakeShaders();

    setHasUnsavedChanges(true);
    emit nodesChanged();
}

void EffectComposerModel::clear(bool clearName)
{
    beginResetModel();
    m_rebakeTimer.stop();
    qDeleteAll(m_nodes);
    m_nodes.clear();
    endResetModel();

    if (clearName)
        setCurrentComposition("");

    setHasUnsavedChanges(!m_currentComposition.isEmpty());

    setIsEmpty(true);
    emit nodesChanged();
}

void EffectComposerModel::assignToSelected()
{
    const QString effectsAssetsDir = QmlDesigner::ModelNodeOperations::getEffectsDefaultDirectory();
    const QString path = effectsAssetsDir + '/' + m_currentComposition + ".qep";
    emit assignToSelectedTriggered(path);
}

QString EffectComposerModel::getUniqueEffectName() const
{
    const QString effectsDir = QmlDesigner::ModelNodeOperations::getEffectsDefaultDirectory();
    const QString path = effectsDir + '/' + "Effect%1.qep";

    int num = 0;

    while (QFile::exists(path.arg(++num, 2, 10, QChar('0'))))
        ; // empty body

    return QString("Effect%1").arg(num, 2, 10, QChar('0'));
}

bool EffectComposerModel::nameExists(const QString &name) const
{
    const QString effectsDir = QmlDesigner::ModelNodeOperations::getEffectsDefaultDirectory();
    const QString path = effectsDir + '/' + "%1" + ".qep";

    return QFile::exists(path.arg(name));
}

QString EffectComposerModel::fragmentShader() const
{
    return m_fragmentShader;
}

void EffectComposerModel::setFragmentShader(const QString &newFragmentShader)
{
    if (m_fragmentShader == newFragmentShader)
        return;

    m_fragmentShader = newFragmentShader;
}

QString EffectComposerModel::vertexShader() const
{
    return m_vertexShader;
}

void EffectComposerModel::setVertexShader(const QString &newVertexShader)
{
    if (m_vertexShader == newVertexShader)
        return;

    m_vertexShader = newVertexShader;
}

QString EffectComposerModel::qmlComponentString() const
{
    return m_qmlComponentString;
}

const QList<Uniform *> EffectComposerModel::allUniforms() const
{
    QList<Uniform *> uniforms = {};
    for (const auto &node : std::as_const(m_nodes))
        uniforms.append(static_cast<EffectComposerUniformsModel *>(node->uniformsModel())->uniforms());
    return uniforms;
}

const QString EffectComposerModel::getBufUniform()
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
            QString type = Uniform::stringFromType(uniform->type(), true);
            QString props = "    " + type + " " + uniform->name() + ";\n";
            s += props;
        }
    }
    s += "};\n";
    return s;
}

const QString EffectComposerModel::getVSUniforms()
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

const QString EffectComposerModel::getFSUniforms()
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
QString EffectComposerModel::detectErrorMessage(const QString &errorMessage)
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
EffectError EffectComposerModel::effectError() const
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
void EffectComposerModel::setEffectError(const QString &errorMessage, int type, int lineNumber)
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
    qWarning() << QString("Effect error (line: %2): %1").arg(error.m_message).arg(error.m_line);
    Q_EMIT effectErrorChanged();
}

QString variantAsDataString(const Uniform::Type type, const Uniform::Type controlType, const QVariant &variant)
{
    QString s;
    switch (type) {
    case Uniform::Type::Bool:
        s = variant.toBool() ? QString("true") : QString("false");
        break;
    case Uniform::Type::Int:
    case Uniform::Type::Channel:
        s = QString::number(variant.toInt());
        break;
    case Uniform::Type::Float:
        s = QString::number(variant.toDouble());
        break;
    case Uniform::Type::Vec2: {
        QStringList list;
        QVector2D v2 = variant.value<QVector2D>();
        list << QString::number(v2.x());
        list << QString::number(v2.y());
        s = list.join(", ");
        break;
    }
    case Uniform::Type::Vec3: {
        QStringList list;
        QVector3D v3 = variant.value<QVector3D>();
        list << QString::number(v3.x());
        list << QString::number(v3.y());
        list << QString::number(v3.z());
        s = list.join(", ");
        break;
    }
    case Uniform::Type::Vec4: {
        QStringList list;
        QVector4D v4 = variant.value<QVector4D>();
        list << QString::number(v4.x());
        list << QString::number(v4.y());
        list << QString::number(v4.z());
        list << QString::number(v4.w());
        s = list.join(", ");
        break;
    }
    case Uniform::Type::Color: {
        QStringList list;
        QColor c = variant.value<QColor>();
        list << QString::number(c.redF(), 'g', 3);
        list << QString::number(c.greenF(), 'g', 3);
        list << QString::number(c.blueF(), 'g', 3);
        list << QString::number(c.alphaF(), 'g', 3);
        s = list.join(", ");
        break;
    }
    case Uniform::Type::Sampler:
    case Uniform::Type::Define: {
        if (controlType == Uniform::Type::Int)
            s = QString::number(variant.toInt());
        else if (controlType == Uniform::Type::Bool)
            s = variant.toBool() ? QString("true") : QString("false");
        else
            s = variant.toString();
        break;
    }
    }
    return s;
}

QJsonObject nodeToJson(const CompositionNode &node)
{
    QJsonObject nodeObject;
    nodeObject.insert("name", node.name());
    if (!node.description().isEmpty())
        nodeObject.insert("description", node.description());
    nodeObject.insert("enabled", node.isEnabled());
    nodeObject.insert("version", 1);
    nodeObject.insert("id", node.id());
    if (node.extraMargin())
        nodeObject.insert("extraMargin", node.extraMargin());

    // Add properties
    QJsonArray propertiesArray;
    const QList<Uniform *> uniforms = node.uniforms();
    for (const Uniform *uniform : uniforms) {
        QJsonObject uniformObject;
        uniformObject.insert("name", QString(uniform->name()));
        QString type = Uniform::stringFromType(uniform->type());

        uniformObject.insert("type", type);

        QString controlType = Uniform::stringFromType(uniform->controlType());
        if (controlType != type)
            uniformObject.insert("controlType", controlType);

        if (!uniform->displayName().isEmpty())
            uniformObject.insert("displayName", QString(uniform->displayName()));

        QString value = variantAsDataString(uniform->type(), uniform->controlType(),
                                            uniform->value());
        if (uniform->type() == Uniform::Type::Sampler)
            value = QFileInfo(value).fileName();
        uniformObject.insert("value", value);

        QString defaultValue = variantAsDataString(uniform->type(), uniform->controlType(),
                                                   uniform->defaultValue());
        if (uniform->type() == Uniform::Type::Sampler) {
            defaultValue = QFileInfo(value).fileName();
            if (uniform->enableMipmap())
                uniformObject.insert("enableMipmap", uniform->enableMipmap());
        }
        uniformObject.insert("defaultValue", defaultValue);
        if (!uniform->description().isEmpty())
            uniformObject.insert("description", uniform->description());
        if (uniform->type() == Uniform::Type::Float
            || (uniform->type() == Uniform::Type::Int
                && uniform->controlType() != Uniform::Type::Channel)
            || uniform->type() == Uniform::Type::Vec2
            || uniform->type() == Uniform::Type::Vec3
            || uniform->type() == Uniform::Type::Vec4
            || uniform->controlType() == Uniform::Type::Int) {
            uniformObject.insert("minValue", variantAsDataString(uniform->type(),
                                                                 uniform->controlType(),
                                                                 uniform->minValue()));
            uniformObject.insert("maxValue", variantAsDataString(uniform->type(),
                                                                 uniform->controlType(),
                                                                 uniform->maxValue()));
        }
        if (!uniform->customValue().isEmpty())
            uniformObject.insert("customValue", uniform->customValue());
        if (uniform->useCustomValue())
            uniformObject.insert("useCustomValue", true);

        propertiesArray.append(uniformObject);
    }
    if (!propertiesArray.isEmpty())
        nodeObject.insert("properties", propertiesArray);

    // Add shaders
    if (!node.fragmentCode().trimmed().isEmpty()) {
        QJsonArray fragmentCodeArray;
        const QStringList fsLines = node.fragmentCode().split('\n');
        for (const QString &line : fsLines)
            fragmentCodeArray.append(line);

        if (!fragmentCodeArray.isEmpty())
            nodeObject.insert("fragmentCode", fragmentCodeArray);
    }
    if (!node.vertexCode().trimmed().isEmpty()) {
        QJsonArray vertexCodeArray;
        const QStringList vsLines = node.vertexCode().split('\n');
        for (const QString &line : vsLines)
            vertexCodeArray.append(line);

        if (!vertexCodeArray.isEmpty())
            nodeObject.insert("vertexCode", vertexCodeArray);
    }

    return nodeObject;
}

QString EffectComposerModel::getGeneratedMessage() const
{
    QString s;

    QString header {
R"(
// Created with Qt Design Studio (version %1), %2
// Do not manually edit this file, it will be overwritten if effect is modified in Qt Design Studio.
)"
    };

    s += header.arg(qApp->applicationVersion(), QDateTime::currentDateTime().toString());
    return s;
}

QString EffectComposerModel::getDesignerSpecifics() const
{
    QString s;

    s += getGeneratedMessage();

    s +=
R"(
import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme

Column {
)";

    if (m_shaderFeatures.enabled(ShaderFeatures::Time)
        || m_shaderFeatures.enabled(ShaderFeatures::Frame)) {
        QString animSec =
R"(
    Section {
        caption: "%1"
        width: parent.width

        SectionLayout {
            PropertyLabel {
                text: "%2"
                tooltip: "%3"
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.timeRunning.valueToString
                    backendValue: backendValues.timeRunning
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
                }
                ExpandingSpacer {}
            }
)";
        s += animSec.arg(tr("Animation"), tr("Running"), tr("Set this property to animate the effect."));

        if (m_shaderFeatures.enabled(ShaderFeatures::Time)) {
            QString timeProp =
R"(
            PropertyLabel {
                text: "%1"
                tooltip: "%2"
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 9999999
                    decimals: 2
                    stepSize: .01
                    backendValue: backendValues.animatedTime
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }
                ExpandingSpacer {}
            }
)";
            s += timeProp.arg(tr("Time"), tr("This property allows explicit control of current animation time."));
        }

        if (m_shaderFeatures.enabled(ShaderFeatures::Frame)) {
            QString frameProp =
R"(
            PropertyLabel {
                text: "%1"
                tooltip: "%2"
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 99999999
                    decimals: 0
                    stepSize: 1
                    backendValue: backendValues.animatedFrame
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }
                ExpandingSpacer {}
            }
)";
            s += frameProp.arg(tr("Frame"), tr("This property allows explicit control of current animation frame."));
        }

        s += "        }\n";
        s += "    }\n";
    }

    if (m_shaderFeatures.enabled(ShaderFeatures::Source) && m_extraMargin) {
        QString generalSection =
            R"(
    Section {
        caption: "%1"
        width: parent.width

        SectionLayout {
            PropertyLabel {
                text: "%2"
                tooltip: "%3"
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 1000
                    decimals: 0
                    stepSize: 1
                    sliderIndicatorVisible: true
                    backendValue: backendValues.extraMargin
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }
                ExpandingSpacer {}
            }
        }
    }
)";
        s += generalSection.arg(tr("General"), tr("Extra Margin"),
                                tr("This property specifies how much of extra space is reserved for the effect outside the parent geometry."));
    }

    for (const auto &node : std::as_const(m_nodes)) {
        const QList<Uniform *> uniforms = static_cast<EffectComposerUniformsModel *>(
                                              node->uniformsModel())->uniforms();
        QString secStr =
R"(
    Section {
        caption: "%1"
        width: parent.width

        SectionLayout {
)";
        secStr = secStr.arg(node->name());

        const QString oldSecStr = secStr;

        for (Uniform *uniform : uniforms)
            secStr += uniform->getDesignerSpecifics();

        // Only add the section if it has actual content
        if (oldSecStr != secStr) {
            secStr += "        }\n";
            secStr += "    }\n";
            s += secStr;
        }
    }

    s += "}\n";

    return s;
}

QString EffectComposerModel::getQmlEffectString()
{
    QString s;

    s += getGeneratedMessage();

    // _isEffectItem is type var to hide it from property view
    QString header {
R"(
import QtQuick

Item {
    id: rootItem

    // Use visible property to show and hide the effect.
    visible: true

    // This is an internal property used by tooling to identify effect items. Do not modify.
    property bool _isEffectItem

    // This is an internal property used to manage the effect. Do not modify.
    property Item _oldParent: null
)"
    };

    s += header;

    if (m_shaderFeatures.enabled(ShaderFeatures::Source)) {
        QString sourceStr{
R"(
    // This is the main source for the effect. Set internally to the current parent item. Do not modify.
    property Item source: null
)"
        };

        QString extraMarginStr{
R"(
    // This property specifies how much of extra space is reserved for the effect outside the parent geometry.
    // It should be sufficient for most use cases but if the application uses extreme values it may be necessary to
    // increase this value.
    property int extraMargin: %1

    onExtraMarginChanged: setupSourceRect()

    function setupSourceRect() {
        if (source) {
            var w = source.width + extraMargin * 2
            var h = source.height + extraMargin * 2
            source.layer.sourceRect = Qt.rect(-extraMargin, -extraMargin, w, h)
        }
    }

    function connectSource(enable) {
        if (source) {
            if (enable) {
                source.widthChanged.connect(setupSourceRect)
                source.heightChanged.connect(setupSourceRect)
            } else {
                source.widthChanged.disconnect(setupSourceRect)
                source.heightChanged.disconnect(setupSourceRect)
            }
        }
    }
)"
        };
        s += sourceStr;
        if (m_extraMargin)
            s += extraMarginStr.arg(m_extraMargin);
    }
    if (m_shaderFeatures.enabled(ShaderFeatures::Time)
        || m_shaderFeatures.enabled(ShaderFeatures::Frame)) {
        s += "    // Enable this to animate iTime property\n";
        s += "    property bool timeRunning: true\n";
    }
    if (m_shaderFeatures.enabled(ShaderFeatures::Time)) {
        s += "    // When timeRunning is false, this can be used to control iTime manually\n";
        s += "    property real animatedTime: frameAnimation.elapsedTime\n";
    }
    if (m_shaderFeatures.enabled(ShaderFeatures::Frame)) {
        s += "    // When timeRunning is false, this can be used to control iFrame manually\n";
        s += "    property int animatedFrame: frameAnimation.currentFrame\n";
    }

    QString parentChanged{
R"(
    function setupParentLayer()
    {
        if (_oldParent && _oldParent !== parent) {
            _oldParent.layer.enabled = false
            _oldParent.layer.effect = null
            %7
            %4%2
            _oldParent.update()
            _oldParent = null
        }
        if (parent) {
            _oldParent = parent
            if (visible) {
                parent.layer.enabled = true
                parent.layer.effect = effectComponent
                %6
                %4%1%5%3
            } else {
                parent.layer.enabled = false
                parent.layer.effect = null
                %8
                %4%2
            }
            parent.update()
        }
    }

    onParentChanged: setupParentLayer()

    onVisibleChanged: setupParentLayer()

)"
    };

    QString mipmap1;
    QString mipmap2;
    QString mipmap3;
    if (m_shaderFeatures.enabled(ShaderFeatures::Mipmap)) {
        mipmap1 = QString {
            R"(parent.layer.smooth = true
            parent.layer.mipmap = true)"
        };
        mipmap2 = QString {
            R"(_oldParent.layer.smooth = false
            _oldParent.layer.mipmap = false)"
        };
        mipmap3 = QString {
            R"(parent.layer.smooth = false
            parent.layer.mipmap = false)"
        };
    }

    if (m_shaderFeatures.enabled(ShaderFeatures::Source)) {
        parentChanged = parentChanged.arg(QString("source = parent"),
                                          QString("source = null"),
                                          m_extraMargin ? QString("            setupSourceRect()") : QString(),
                                          m_extraMargin ? QString("connectSource(false)\n            ") : QString(),
                                          m_extraMargin ? QString("\n            connectSource(true)\n") : QString(),
                                          mipmap1,
                                          mipmap2,
                                          mipmap3);
    } else {
        parentChanged = parentChanged.arg(QString(), QString(), QString());
    }
    s += parentChanged;

    // Custom properties
    if (!m_exportedRootPropertiesString.isEmpty()) {
        s += m_exportedRootPropertiesString;
        s += '\n';
    }
    if (m_shaderFeatures.enabled(ShaderFeatures::Time)
        || m_shaderFeatures.enabled(ShaderFeatures::Frame)) {
        s += "    FrameAnimation {\n";
        s += "        id: frameAnimation\n";
        s += "        running: rootItem.timeRunning\n";
        s += "    }\n";
        s += '\n';
    }

    QString customImagesString = getQmlImagesString(true);
    if (!customImagesString.isEmpty())
        s += customImagesString;

    s += "    Component {\n";
    s += "        id: effectComponent\n";
    s += getQmlComponentString(true);
    s += "    }\n";
    s += "}\n";
    return s;
}

void EffectComposerModel::saveComposition(const QString &name)
{
    if (name.isEmpty() || name.size() < 3 || name[0].isLower()) {
        QString error = QString("Error: Couldn't save composition '%1', name is invalid").arg(name);
        qWarning() << error;
        return;
    }

    const QString effectsAssetsDir = QmlDesigner::ModelNodeOperations::getEffectsDefaultDirectory();
    const QString path = effectsAssetsDir + '/' + name + ".qep";
    auto saveFile = QFile(path);
    if (!saveFile.open(QIODevice::WriteOnly)) {
        QString error = QString("Error: Couldn't save composition file: '%1'").arg(path);
        qWarning() << error;
        return;
    }

    updateExtraMargin();

    QJsonObject json;
    // File format version
    json.insert("version", 1);

    // Add nodes
    QJsonArray nodesArray;
    for (const CompositionNode *node : std::as_const(m_nodes)) {
        QJsonObject nodeObject = nodeToJson(*node);
        nodesArray.append(nodeObject);
    }

    if (!nodesArray.isEmpty())
        json.insert("nodes", nodesArray);

    QJsonObject rootJson;
    rootJson.insert("QEP", json);
    QJsonDocument jsonDoc(rootJson);

    saveFile.write(jsonDoc.toJson());
    saveFile.close();
    setCurrentComposition(name);

    saveResources(name);
    setHasUnsavedChanges(false);
}

void EffectComposerModel::openComposition(const QString &path)
{
    clear(true);

    const QString effectName = QFileInfo(path).baseName();
    setCurrentComposition(effectName);

    QFile compFile(path);
    if (!compFile.open(QIODevice::ReadOnly)) {
        QString error = QString("Couldn't open composition file: '%1'").arg(path);
        qWarning() << qPrintable(error);
        setEffectError(error);
        return;
    }

    QByteArray data = compFile.readAll();

    if (data.isEmpty())
        return;

    QJsonParseError parseError;
    QJsonDocument jsonDoc(QJsonDocument::fromJson(data, &parseError));
    if (parseError.error != QJsonParseError::NoError) {
        QString error = QString("Error parsing the project file: %1").arg(parseError.errorString());
        qWarning() << error;
        setEffectError(error);
        return;
    }
    QJsonObject rootJson = jsonDoc.object();
    if (!rootJson.contains("QEP")) {
        QString error = QStringLiteral("Error: Invalid project file");
        qWarning() << error;
        setEffectError(error);
        return;
    }

    QJsonObject json = rootJson["QEP"].toObject();

    int version = -1;
    if (json.contains("version"))
        version = json["version"].toInt(-1);

    if (version != 1) {
        QString error = QString("Error: Unknown project version (%1)").arg(version);
        qWarning() << error;
        setEffectError(error);
        return;
    }

    if (json.contains("nodes") && json["nodes"].isArray()) {
        beginResetModel();
        QHash<QString, int> refCounts;
        const QJsonArray nodesArray = json["nodes"].toArray();

        for (const auto &nodeElement : nodesArray) {
            auto *node = new CompositionNode(effectName, {}, nodeElement.toObject());
            connectCompositionNode(node);
            m_nodes.append(node);
            const QStringList reqIds = node->requiredNodes();
            for (const QString &reqId : reqIds)
                ++refCounts[reqId];
        }

        for (auto it = refCounts.cbegin(), end = refCounts.cend(); it != end; ++it) {
            CompositionNode *depNode = findNodeById(it.key());
            if (depNode)
                depNode->setRefCount(it.value());
        }

        endResetModel();

        setIsEmpty(m_nodes.isEmpty());
        bakeShaders();
    }

    setHasUnsavedChanges(false);
    emit nodesChanged();
}

void EffectComposerModel::saveResources(const QString &name)
{
    // Make sure that uniforms are up-to-date
    updateCustomUniforms();

    QString qmlFilename = name + ".qml";
    QString vsFilename = name + ".vert.qsb";
    QString fsFilename = name + ".frag.qsb";

    // Shaders should be all lowercase
    vsFilename = vsFilename.toLower();
    fsFilename = fsFilename.toLower();

    // Get effects dir
    const Utils::FilePath effectsResDir = QmlDesigner::ModelNodeOperations::getEffectsImportDirectory();
    const QString effectsResPath = effectsResDir.pathAppended(name).toString() + '/';
    Utils::FilePath effectPath = Utils::FilePath::fromString(effectsResPath);

    // Create the qmldir for effects
    QString qmldirFileName("qmldir");
    Utils::FilePath qmldirPath = effectsResDir.resolvePath(qmldirFileName);
    QString qmldirContent = QString::fromUtf8(qmldirPath.fileContents().value_or(QByteArray()));
    if (qmldirContent.isEmpty()) {
        qmldirContent.append(QString("module %1\n").arg(m_effectTypePrefix));
        qmldirPath.writeFileContents(qmldirContent.toUtf8());
    }

    Utils::FilePaths oldFiles;
    QStringList newFileNames;

    // Create effect folder if not created
    if (!effectPath.exists())
        effectPath.createDir();
    else
        oldFiles = effectPath.dirEntries(QDir::Files);

    // Create effect qmldir
    newFileNames.append(qmldirFileName);
    qmldirPath = effectPath.resolvePath(qmldirFileName);
    qmldirContent = QString::fromUtf8(qmldirPath.fileContents().value_or(QByteArray()));
    if (qmldirContent.isEmpty()) {
        qmldirContent.append(QString("module %1.").arg(m_effectTypePrefix));
        qmldirContent.append(name);
        qmldirContent.append('\n');
        qmldirContent.append(name);
        qmldirContent.append(" 1.0 ");
        qmldirContent.append(name);
        qmldirContent.append(".qml\n");
        qmldirPath.writeFileContents(qmldirContent.toUtf8());
    }

    // Create designer folder if not created
    const Utils::FilePath designerPath = effectPath.pathAppended("designer");
    if (!designerPath.exists())
        designerPath.createDir();

    // Create designer property sheet
    // Since this is in subdir, no need to add it to newFileNames
    QString specContent = getDesignerSpecifics();
    QString specFileName("%1SpecificsDynamic.qml");
    specFileName = specFileName.arg(name);
    Utils::FilePath specPath = designerPath.resolvePath(specFileName);
    specPath.writeFileContents(specContent.toUtf8());

    // Create the qml file
    QString qmlComponentString = getQmlEffectString();
    QStringList qmlStringList = qmlComponentString.split('\n');

    // Replace shaders with local versions
    for (int i = 1; i < qmlStringList.size(); i++) {
        QString line = qmlStringList.at(i).trimmed();
        if (line.startsWith("vertexShader")) {
            QString vsLine = "            vertexShader: '" + vsFilename + "'";
            qmlStringList[i] = vsLine;
        } else if (line.startsWith("fragmentShader")) {
            QString fsLine = "            fragmentShader: '" + fsFilename + "'";
            qmlStringList[i] = fsLine;
        }
    }

    const QString qmlString = qmlStringList.join('\n');
    QString qmlFilePath = effectsResPath + qmlFilename;

    // Get exposed properties from the old qml file if it exists
    QSet<QByteArray> oldExposedProps;
    Utils::FilePath oldQmlFile = Utils::FilePath::fromString(qmlFilePath);
    if (oldQmlFile.exists()) {
        const QByteArray oldQmlContent = oldQmlFile.fileContents().value();
        oldExposedProps = getExposedProperties(oldQmlContent);
    }

    const QByteArray qmlUtf8 = qmlString.toUtf8();
    if (!oldExposedProps.isEmpty()) {
        const QSet<QByteArray> newExposedProps = getExposedProperties(qmlUtf8);
        oldExposedProps.subtract(newExposedProps);
        if (!oldExposedProps.isEmpty()) {
            // If there were exposed properties that are no longer exposed, those
            // need to be removed from any instances of the effect in the scene
            emit removePropertiesFromScene(oldExposedProps, name);
        }
    }

    writeToFile(qmlUtf8, qmlFilePath, FileType::Text);
    newFileNames.append(qmlFilename);

    // Save shaders and images
    QStringList sources = {m_vertexShaderFilename, m_fragmentShaderFilename};
    QStringList dests = {vsFilename, fsFilename};

    QHash<QString, Uniform *> fileNameToUniformHash;
    const QList<Uniform *> uniforms = allUniforms();
    bool hasSampler = false;
    for (Uniform *uniform : uniforms) {
        if (uniform->type() == Uniform::Type::Sampler && !uniform->value().toString().isEmpty()) {
            QString imagePath = uniform->value().toString();
            QFileInfo fi(imagePath);
            QString imageFilename = fi.fileName();
            if (imagePath.startsWith("file:")) {
                QUrl url(imagePath);
                imagePath = url.toLocalFile();
            }
            sources.append(imagePath);
            dests.append(imageFilename);
            fileNameToUniformHash.insert(imageFilename, uniform);
            hasSampler = true;
        }
    }

    if (m_shaderFeatures.enabled(ShaderFeatures::BlurSources)) {
        QString blurHelperFilename("BlurHelper.qml");
        QString blurFsFilename("bluritems.frag.qsb");
        QString blurVsFilename("bluritems.vert.qsb");
        QString blurHelperPath(EffectUtils::nodesSourcesPath() + "/common/");
        sources.append(blurHelperPath + blurHelperFilename);
        sources.append(blurHelperPath + blurFsFilename);
        sources.append(blurHelperPath + blurVsFilename);
        dests.append(blurHelperFilename);
        dests.append(blurFsFilename);
        dests.append(blurVsFilename);
    }

    for (int i = 0; i < sources.count(); ++i) {
        Utils::FilePath source = Utils::FilePath::fromString(sources[i]);
        Utils::FilePath target = Utils::FilePath::fromString(effectsResPath + dests[i]);
        newFileNames.append(target.fileName());
        if (target.exists() && source.fileName() != target.fileName())
            target.removeFile(); // Remove existing file for update
        if (!source.copyFile(target))
            qWarning() << __FUNCTION__ << " Failed to copy file: " << source;

        if (fileNameToUniformHash.contains(dests[i])) {
            Uniform *uniform = fileNameToUniformHash[dests[i]];
            const QVariant newValue = target.toString();
            uniform->setDefaultValue(newValue);
            uniform->setValue(newValue);
        }
    }

    // Delete old content that was not overwritten
    // We ignore subdirectories, as currently subdirs only contain fixed content
    for (const Utils::FilePath &oldFile : oldFiles) {
        if (!newFileNames.contains(oldFile.fileName()))
            oldFile.removeFile();
    }

    // Refresh UI to update sampler UrlChoosers
    if (hasSampler) {
        beginResetModel();
        endResetModel();
    }

    emit resourcesSaved(QString("%1.%2.%2").arg(m_effectTypePrefix, name).toUtf8(), effectPath);
}

void EffectComposerModel::resetEffectError(int type)
{
    if (m_effectErrors.contains(type)) {
        m_effectErrors.remove(type);
        Q_EMIT effectErrorChanged();
    }
}

// Get value in QML format that used for exports
QString EffectComposerModel::valueAsString(const Uniform &uniform)
{
    if (uniform.type() == Uniform::Type::Bool) {
        return uniform.value().toBool() ? QString("true") : QString("false");
    } else if (uniform.type() == Uniform::Type::Int) {
        return QString::number(uniform.value().toInt());
    } else if (uniform.type() == Uniform::Type::Float) {
        return QString::number(uniform.value().toDouble());
    } else if (uniform.type() == Uniform::Type::Vec2) {
        QVector2D v2 = uniform.value().value<QVector2D>();
        return QString("Qt.point(%1, %2)").arg(v2.x()).arg(v2.y());
    } else if (uniform.type() == Uniform::Type::Vec3) {
        QVector3D v3 = uniform.value().value<QVector3D>();
        return QString("Qt.vector3d(%1, %2, %3)").arg(v3.x()).arg(v3.y()).arg(v3.z());
    } else if (uniform.type() == Uniform::Type::Vec4) {
        QVector4D v4 = uniform.value().value<QVector4D>();
        return QString("Qt.vector4d(%1, %2, %3, %4)").arg(v4.x()).arg(v4.y()).arg(v4.z()).arg(v4.w());
    } else if (uniform.type() == Uniform::Type::Sampler) {
        return getImageElementName(uniform, true);
    } else if (uniform.type() == Uniform::Type::Color) {
        return QString("\"%1\"").arg(uniform.value().toString());
    } else if (uniform.type() == Uniform::Type::Channel) {
        return QString::number(uniform.value().toInt());
    } else if (uniform.type() == Uniform::Type::Define) {
        if (uniform.controlType() == Uniform::Type::Int)
            return QString::number(uniform.value().toInt());
        else if (uniform.controlType() == Uniform::Type::Bool)
            return uniform.value().toBool() ? QString("1") : QString("0");
        return uniform.value().toString();
    } else {
        qWarning() << QString("Unhandled const variable type: %1").arg(int(uniform.type())).toLatin1();
        return QString();
    }
}

// Get value in QML binding that used for previews
QString EffectComposerModel::valueAsBinding(const Uniform &uniform)
{
    if (uniform.type() == Uniform::Type::Bool
        || uniform.type() == Uniform::Type::Int
        || uniform.type() == Uniform::Type::Float
        || uniform.type() == Uniform::Type::Color
        || uniform.type() == Uniform::Type::Channel
        || uniform.type() == Uniform::Type::Define) {
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
    } else if (uniform.type() == Uniform::Type::Sampler) {
        return getImageElementName(uniform, false);
    } else {
        qWarning() << QString("Unhandled const variable type: %1").arg(int(uniform.type())).toLatin1();
        return QString();
    }
}

// Get value in GLSL format that is used for non-exported const properties
QString EffectComposerModel::valueAsVariable(const Uniform &uniform)
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
    } else if (uniform.type() == Uniform::Type::Channel) {
        return QString::number(uniform.value().toInt());
    } else {
        qWarning() << QString("Unhandled const variable type: %1").arg(int(uniform.type())).toLatin1();
        return QString();
    }
}

// Return name for the image property Image element
QString EffectComposerModel::getImageElementName(const Uniform &uniform, bool localFiles)
{
    if (localFiles && uniform.value().toString().isEmpty())
        return QStringLiteral("null");
    QString simplifiedName = uniform.name().simplified();
    simplifiedName = simplifiedName.remove(' ');
    return QStringLiteral("imageItem") + simplifiedName;
}

const QString EffectComposerModel::getConstVariables()
{
    const QList<Uniform *> uniforms = allUniforms();
    QString s;
    for (Uniform *uniform : uniforms) {
        // TODO: Check if uniform is already added.
        QString constValue = valueAsVariable(*uniform);
        QString type = Uniform::stringFromType(uniform->type(), true);
        s += QString("const %1 %2 = %3;\n").arg(type, uniform->name(), constValue);
    }
    if (!s.isEmpty())
        s += '\n';

    return s;
}

const QString EffectComposerModel::getDefineProperties()
{
    const QList<Uniform *> uniforms = allUniforms();
    QString s;
    for (Uniform *uniform : uniforms) {
        // TODO: Check if uniform is already added.
        if (uniform->type() == Uniform::Type::Define)
            s += QString("#define %1 %2\n").arg(uniform->name(), valueAsString(*uniform));
    }
    if (!s.isEmpty())
        s += '\n';

    return s;
}

int EffectComposerModel::getTagIndex(const QStringList &code, const QString &tag)
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

QString EffectComposerModel::processVertexRootLine(const QString &line)
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

QString EffectComposerModel::processFragmentRootLine(const QString &line)
{
    QString output;
    QStringList lineList = line.split(m_spaceReg, Qt::SkipEmptyParts);
    // Just skip all "in" variables. It is enough to have "out" variable in vertex.
    if (lineList.length() > 1 && lineList.at(0) == QStringLiteral("in"))
        return QString();
    output = line + '\n';
    return output;
}

QStringList EffectComposerModel::getDefaultRootVertexShader()
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

QStringList EffectComposerModel::getDefaultRootFragmentShader()
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
QStringList EffectComposerModel::removeTagsFromCode(const QStringList &codeLines)
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

QString EffectComposerModel::removeTagsFromCode(const QString &code)
{
    QStringList codeLines = removeTagsFromCode(code.split('\n'));
    return codeLines.join('\n');
}

QString EffectComposerModel::getCustomShaderVaryings(bool outState)
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

QString EffectComposerModel::generateVertexShader(bool includeUniforms)
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
            const QStringList vertexCode = n->vertexCode().split('\n');
            int mainIndex = getTagIndex(vertexCode, "main");
            int line = 0;
            for (const QString &ss : vertexCode) {
                if (mainIndex == -1 || line > mainIndex)
                    s_main += "    " + ss + '\n';
                else if (line < mainIndex)
                    s_root += processVertexRootLine(ss);
                line++;
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

QString EffectComposerModel::generateFragmentShader(bool includeUniforms)
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

void EffectComposerModel::handleQsbProcessExit(Utils::Process *qsbProcess, const QString &shader, bool preview)
{
    --m_remainingQsbTargets;

    const QString errStr = qsbProcess->errorString();
    const QByteArray errStd = qsbProcess->readAllRawStandardError();
    QString previewStr;
    if (preview)
        previewStr = QStringLiteral("preview");

    if (!errStr.isEmpty()) {
        qWarning() << QString("Failed to generate %3 QSB file for: %1 %2")
                          .arg(shader, errStr, previewStr);
    }

    if (!errStd.isEmpty()) {
        qWarning() << QString("Failed to generate %3 QSB file for: %1 %2")
                          .arg(shader, QString::fromUtf8(errStd), previewStr);
    }

    if (m_remainingQsbTargets <= 0) {
        Q_EMIT shadersBaked();
        setShadersUpToDate(true);

        // TODO: Mark shaders as baked, required by export later
    }

    qsbProcess->deleteLater();
}

// Generates string of the custom properties (uniforms) into ShaderEffect component
// Also generates QML images elements for samplers.
void EffectComposerModel::updateCustomUniforms()
{
    QString exportedRootPropertiesString;
    QString previewEffectPropertiesString;
    QString exportedEffectPropertiesString;

    const QList<Uniform *> uniforms = allUniforms();
    for (Uniform *uniform : uniforms) {
        // TODO: Check if uniform is already added.
        const bool isDefine = uniform->type() == Uniform::Type::Define;
        QString propertyType = Uniform::typeToProperty(uniform->type());
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
        QString boundValueString = bindedValue.isEmpty() ? QString() : QString(": %1").arg(bindedValue);
        // Custom values are not readonly, others inside the effect can be
        QString readOnly = uniform->useCustomValue() ? QString() : QStringLiteral("readonly ");
        previewEffectPropertiesString += "    " + readOnly + "property " + propertyType + " "
                                         + propertyName + boundValueString + '\n';
        // Define type properties are not added into exports
        if (!isDefine) {
            if (uniform->useCustomValue()) {
                // Custom values are only inside the effect, with description comments
                if (!uniform->description().isEmpty()) {
                    const QStringList descriptionLines = uniform->description().split('\n');
                    for (const QString &line : descriptionLines)
                        exportedEffectPropertiesString += QStringLiteral("            // ") + line + '\n';
                }
                exportedEffectPropertiesString += QStringLiteral("            ") + readOnly
                                                  + "property " + propertyType + " " + propertyName
                                                  + boundValueString + '\n';
            } else {
                // Custom values are not added into root
                exportedRootPropertiesString += "    property " + propertyType + " " + propertyName
                                                + valueString + '\n';
                exportedEffectPropertiesString += QStringLiteral("            ")
                                                  + readOnly + "property " + propertyType + " " + propertyName
                                                  + ": rootItem." + uniform->name() + '\n';
            }
        }
    }

    // See if any of the properties changed
    m_exportedRootPropertiesString = exportedRootPropertiesString;
    m_previewEffectPropertiesString = previewEffectPropertiesString;
    m_exportedEffectPropertiesString = exportedEffectPropertiesString;
}

void EffectComposerModel::initShaderDir()
{
    static int count = 0;
    static const QString fileNameTemplate = "%1_%2.%3";
    const QString countStr = QString::number(count);

    auto resetFile = [&countStr, this](QString &fileName, const QString prefix, const QString suffix) {
        // qsb generation is done in separate process, so it is not guaranteed all of the old files
        // get deleted here, as they may not exist yet. Any dangling files will be deleted at
        // application shutdown, when the temporary directory is destroyed.
        if (!fileName.isEmpty()) {
            QFile file(fileName);
            if (file.exists())
                file.remove();
        }

        fileName = m_shaderDir.filePath(fileNameTemplate.arg(prefix, countStr, suffix));
    };

    resetFile(m_vertexSourceFilename, "source", "vert");
    resetFile(m_fragmentSourceFilename, "source", "frag");
    resetFile(m_vertexShaderFilename, "compiled", "vert.qsb");
    resetFile(m_fragmentShaderFilename, "compiled", "frag.qsb");
    resetFile(m_vertexShaderPreviewFilename, "compiled_prev", "vert.qsb");
    resetFile(m_fragmentShaderPreviewFilename, "compiled_prev", "frag.qsb");

    ++count;
}

void EffectComposerModel::bakeShaders()
{
    const QString failMessage = "Shader baking failed: ";

    const ProjectExplorer::Target *target = ProjectExplorer::ProjectTree::currentTarget();
    if (!target) {
        qWarning() << failMessage << "Target not found";
        return;
    }

    initShaderDir();

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
    writeToFile(vs.toUtf8(), m_vertexSourceFilename, FileType::Text);

    setFragmentShader(generateFragmentShader());
    QString fs = m_fragmentShader;
    writeToFile(fs.toUtf8(), m_fragmentSourceFilename, FileType::Text);

    QtSupport::QtVersion *qtVer = QtSupport::QtKitAspect::qtVersion(target->kit());
    if (!qtVer) {
        qWarning() << failMessage << "Qt version not found";
        return;
    }

    Utils::FilePath qsbPath = qtVer->binPath().pathAppended("qsb").withExecutableSuffix();
    if (!qsbPath.exists()) {
        qWarning() << failMessage << "QSB tool for target kit not found";
        return;
    }

    Utils::FilePath binPath = Utils::FilePath::fromString(
        QLibraryInfo::path(QLibraryInfo::BinariesPath));
    Utils::FilePath qsbPrevPath = binPath.pathAppended("qsb").withExecutableSuffix();
    if (!qsbPrevPath.exists()) {
        qWarning() << failMessage << "QSB tool for preview shaders not found";
        return;
    }

    m_remainingQsbTargets = 2; // We only have 2 shaders
    const QStringList srcPaths = {m_vertexSourceFilename, m_fragmentSourceFilename};
    const QStringList outPaths = {m_vertexShaderFilename, m_fragmentShaderFilename};
    const QStringList outPrevPaths = {m_vertexShaderPreviewFilename, m_fragmentShaderPreviewFilename};

    auto runQsb = [this, srcPaths](const Utils::FilePath &qsbPath, const QStringList &outPaths, bool preview) {
        for (int i = 0; i < 2; ++i) {
            const auto workDir = Utils::FilePath::fromString(outPaths[i]);
            // TODO: Optional legacy glsl support like standalone effect maker needs to add "100es,120"
            QStringList args = {"-s", "--glsl", "300es,140,330,410", "--hlsl", "50", "--msl", "12"};
            args << "-o" << outPaths[i] << srcPaths[i];

            auto qsbProcess = new Utils::Process(this);
            connect(qsbProcess, &Utils::Process::done, this,
                    [this, qsbProcess, path = srcPaths[i], preview] {
                handleQsbProcessExit(qsbProcess, path, preview);
            });
            qsbProcess->setWorkingDirectory(workDir.absolutePath());
            qsbProcess->setCommand({qsbPath, args});
            qsbProcess->start();
        }
    };

    runQsb(qsbPath, outPaths, false);
    runQsb(qsbPrevPath, outPrevPaths, true);

}

bool EffectComposerModel::shadersUpToDate() const
{
    return m_shadersUpToDate;
}

void EffectComposerModel::setShadersUpToDate(bool UpToDate)
{
    if (m_shadersUpToDate == UpToDate)
        return;
    m_shadersUpToDate = UpToDate;
    emit shadersUpToDateChanged();
}

bool EffectComposerModel::isEnabled() const
{
    return m_isEnabled;
}

void EffectComposerModel::setIsEnabled(bool enabled)
{
    if (m_isEnabled == enabled)
        return;
    m_isEnabled = enabled;
    emit isEnabledChanged();
}

bool EffectComposerModel::hasValidTarget() const
{
    return m_hasValidTarget;
}

void EffectComposerModel::setHasValidTarget(bool validTarget)
{
    if (m_hasValidTarget == validTarget)
        return;
    m_hasValidTarget = validTarget;
    emit hasValidTargetChanged();
}

QString EffectComposerModel::getQmlImagesString(bool localFiles)
{
    QString imagesString;
    const QList<Uniform *> uniforms = allUniforms();
    for (Uniform *uniform : uniforms) {
        if (uniform->type() == Uniform::Type::Sampler) {
            QString imagePath = uniform->value().toString();
            // For preview, generate image element even if path is empty, as changing uniform values
            // will not trigger qml code regeneration
            if (localFiles) {
                if (imagePath.isEmpty())
                    continue;
                QFileInfo fi(imagePath);
                imagePath = fi.fileName();
                imagesString += QString("    property url %1Url: \"%2\"\n")
                                    .arg(uniform->name(), imagePath);
            }
            imagesString += "    Image {\n";
            QString simplifiedName = getImageElementName(*uniform, localFiles);
            imagesString += QString("        id: %1\n").arg(simplifiedName);
            imagesString += "        anchors.fill: parent\n";
            // File paths are absolute, return as local when requested
            if (localFiles) {
                imagesString += QString("        source: rootItem.%1Url\n").arg(uniform->name());
            } else {
                imagesString += QString("        source: g_propertyData.%1\n").arg(uniform->name());

                if (uniform->enableMipmap())
                    imagesString += "        mipmap: true\n";
            }

            imagesString += "        visible: false\n";
            imagesString += "    }\n";
        }
    }
    return imagesString;
}

QString EffectComposerModel::getQmlComponentString(bool localFiles)
{
    auto addProperty = [localFiles](const QString &name, const QString &var,
                                    const QString &type, bool blurHelper = false)
    {
        if (localFiles) {
            const QString parent = blurHelper ? QString("blurHelper.") : QString("rootItem.");
            return QString("readonly property %1 %2: %3%4\n").arg(type, name, parent, var);
        } else {
            const QString parent = blurHelper ? "blurHelper." : QString();
            return QString("readonly property %1 %2: %3%4\n").arg(type, name, parent, var);
        }
    };

    QString s;
    QString l1 = localFiles ? QStringLiteral("        ") : QStringLiteral("");
    QString l2 = localFiles ? QStringLiteral("            ") : QStringLiteral("    ");
    QString l3 = localFiles ? QStringLiteral("                ") : QStringLiteral("        ");

    if (!localFiles)
        s += "import QtQuick\n";
    s += l1 + "ShaderEffect {\n";

    if (localFiles) {
        // Explicit "source" property is required for render puppet to detect effect correctly
        s += l2 + "property Item source: null\n";
    }
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

    if (!localFiles) {
        QString customImagesString = getQmlImagesString(false);
        if (!customImagesString.isEmpty())
            s += '\n' + customImagesString;
    }

    s += '\n';
    const QString vertFile = localFiles ? m_vertexShaderFilename : m_vertexShaderPreviewFilename;
    const QString fragFile = localFiles ? m_fragmentShaderFilename : m_fragmentShaderPreviewFilename;
    s += l2 + "vertexShader: 'file:///" + vertFile + "'\n";
    s += l2 + "fragmentShader: 'file:///" + fragFile + "'\n";
    s += l2 + "anchors.fill: " + (localFiles ? "rootItem.source" : "parent") + "\n";
    if (localFiles) {
        if (m_extraMargin)
            s += l2 + "anchors.margins: -rootItem.extraMargin\n";
    } else {
        s += l2 + "anchors.margins: -root.extraMargin\n";
    }
    if (m_shaderFeatures.enabled(ShaderFeatures::GridMesh)) {
        QString gridSize = QString("%1, %2").arg(m_shaderFeatures.gridMeshWidth())
                                            .arg(m_shaderFeatures.gridMeshHeight());
        s += l2 + "mesh: GridMesh {\n";
        s += l3 + QString("resolution: Qt.size(%1)\n").arg(gridSize);
        s += l2 + "}\n";
    }
    if (localFiles && m_shaderFeatures.enabled(ShaderFeatures::BlurSources)) {
        s += l2 + "BlurHelper {\n";
        s += l3 + "id: blurHelper\n";
        s += l3 + "source: rootItem.source\n";
        int blurMax = 32;
        if (g_propertyData.contains("BLUR_HELPER_MAX_LEVEL"))
            blurMax = g_propertyData["BLUR_HELPER_MAX_LEVEL"].toInt();
        s += l3 + QString("property int blurMax: %1\n").arg(blurMax);
        s += l3 + "property real blurMultiplier: rootItem.blurMultiplier\n";
        s += l2 + "}\n";
    }

    s += l1 + "}\n";
    return s;
}

void EffectComposerModel::connectCompositionNode(CompositionNode *node)
{
    connect(qobject_cast<EffectComposerUniformsModel *>(node->uniformsModel()),
            &EffectComposerUniformsModel::dataChanged, this, [this] {
                setHasUnsavedChanges(true);
            });
    connect(node, &CompositionNode::rebakeRequested, this, [this] {
        // This can come multiple times in a row in response to property changes, so let's buffer it
        m_rebakeTimer.start(200);
    });
}

void EffectComposerModel::updateExtraMargin()
{
    m_extraMargin = 0;
    for (CompositionNode *node : std::as_const(m_nodes))
        m_extraMargin = qMax(node->extraMargin(), m_extraMargin);
}

QSet<QByteArray> EffectComposerModel::getExposedProperties(const QByteArray &qmlContent)
{
    QSet<QByteArray> returnSet;
    const QByteArrayList lines = qmlContent.split('\n');
    const QByteArray propertyTag {"    property"}; // Match only toplevel exposed properties
    for (const QByteArray &line : lines) {
        if (line.startsWith(propertyTag)) {
            QByteArrayList words = line.trimmed().split(' ');
            if (words.size() >= 3) {
                QByteArray propName = words[2];
                if (propName.endsWith(':'))
                    propName.chop(1);
                returnSet.insert(propName);
            }
        }
    }
    return returnSet;
}

QString EffectComposerModel::currentComposition() const
{
    return m_currentComposition;
}

void EffectComposerModel::setCurrentComposition(const QString &newCurrentComposition)
{
    if (m_currentComposition == newCurrentComposition)
        return;

    m_currentComposition = newCurrentComposition;
    emit currentCompositionChanged();
}

bool EffectComposerModel::hasUnsavedChanges() const
{
    return m_hasUnsavedChanges;
}

void EffectComposerModel::setHasUnsavedChanges(bool val)
{
    if (m_hasUnsavedChanges == val)
        return;

    m_hasUnsavedChanges = val;
    emit hasUnsavedChangesChanged();
}

QStringList EffectComposerModel::uniformNames() const
{
    QStringList usedList;
    const QList<Uniform *> uniforms = allUniforms();
    for (const auto uniform : uniforms)
        usedList.append(uniform->name());
    return usedList;
}

bool EffectComposerModel::isDependencyNode(int index) const
{
    if (m_nodes.size() > index)
        return m_nodes[index]->isDependency();
    return false;
}

void EffectComposerModel::updateQmlComponent()
{
    // Clear possible QML runtime errors
    resetEffectError(ErrorQMLRuntime);
    m_qmlComponentString = getQmlComponentString(false);
}

// Removes "file:" from the URL path.
// So e.g. "file:///C:/myimages/steel1.jpg" -> "C:/myimages/steel1.jpg"
QString EffectComposerModel::stripFileFromURL(const QString &urlString) const
{
    QUrl url(urlString);
    QString filePath = (url.scheme() == QStringLiteral("file")) ? url.toLocalFile() : url.toString();
    return filePath;
}

} // namespace EffectComposer
