// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectcomposermodel.h"

#include "compositionnode.h"
#include "effectcomposertr.h"
#include "effectcomposernodesmodel.h"
#include "effectshaderscodeeditor.h"
#include "effectutils.h"
#include "propertyhandler.h"
#include "syntaxhighlighterdata.h"
#include "uniform.h"

#include <asset.h>
#include <designdocument.h>
#include <modelnodeoperations.h>
#include <qmldesignerplugin.h>
#include <uniquename.h>

#include <coreplugin/icore.h>

#include <projectexplorer/projecttree.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QByteArrayView>
#include <QFileDialog>
#include <QLibraryInfo>
#include <QStandardPaths>
#include <QVector2D>

using namespace Qt::StringLiterals;

namespace EffectComposer {

static constexpr int INVALID_CODE_EDITOR_INDEX = -1;
static constexpr int MAIN_CODE_EDITOR_INDEX = -2;

EffectComposerModel::EffectComposerModel(QObject *parent)
    : QAbstractListModel{parent}
    , m_effectComposerNodesModel{new EffectComposerNodesModel(this)}
    , m_codeEditorIndex(INVALID_CODE_EDITOR_INDEX)
    , m_shaderDir(QDir::tempPath() + "/qds_ec_XXXXXX")
    , m_currentPreviewColor("#dddddd")
{
    m_rebakeTimer.setSingleShot(true);
    connect(&m_rebakeTimer, &QTimer::timeout, this, &EffectComposerModel::bakeShaders);
    m_currentPreviewImage = defaultPreviewImage();
    connectCodeEditor();
}

EffectComposerNodesModel *EffectComposerModel::effectComposerNodesModel() const
{
    return m_effectComposerNodesModel.data();
}

QHash<int, QByteArray> EffectComposerModel::roleNames() const
{
    static const QHash<int, QByteArray> roles = {
        {NameRole, "nodeName"},
        {EnabledRole, "nodeEnabled"},
        {UniformsRole, "nodeUniformsModel"},
        {Dependency, "isDependency"},
        {Custom, "isCustom"}
    };
    return roles;
}

int EffectComposerModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return m_nodes.count();
}

QVariant EffectComposerModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.isValid() && isValidIndex(index.row()), return {});
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
    const QStringList reservedNames = nodeNames();
    QString newName = QmlDesigner::UniqueName::generate(node->name(), [&reservedNames] (const QString &checkName) -> bool {
        return reservedNames.contains(checkName, Qt::CaseInsensitive);
    });
    node->setName(newName);

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

    int oldCodeEditorIdx = m_codeEditorIndex;

    int toIdxAdjusted = fromIdx < toIdx ? toIdx + 1 : toIdx; // otherwise beginMoveRows() crashes
    beginMoveRows({}, fromIdx, fromIdx, {}, toIdxAdjusted);
    m_nodes.move(fromIdx, toIdx);
    endMoveRows();

    setHasUnsavedChanges(true);
    bakeShaders();

    // Adjust codeEditorIndex after move
    if (oldCodeEditorIdx > -1) {
        int newCodeEditorIndex = oldCodeEditorIdx;
        if (oldCodeEditorIdx == fromIdx)
            newCodeEditorIndex = toIdx;
        if (fromIdx < oldCodeEditorIdx && toIdx >= oldCodeEditorIdx)
            --newCodeEditorIndex;
        if (fromIdx > oldCodeEditorIdx && toIdx <= oldCodeEditorIdx)
            ++newCodeEditorIndex;
        setCodeEditorIndex(newCodeEditorIndex);
    }
}

void EffectComposerModel::removeNode(int idx)
{
    beginResetModel();
    m_rebakeTimer.stop();
    CompositionNode *node = m_nodes.takeAt(idx);

    // Invalidate codeEditorIndex only if the index is the same as current index
    // Then after the model reset, the nearest code editor should be opened.
    const bool switchCodeEditorNode = m_codeEditorIndex == idx;
    if (switchCodeEditorNode)
        setCodeEditorIndex(INVALID_CODE_EDITOR_INDEX);

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

    if (switchCodeEditorNode)
        openNearestAvailableCodeEditor(idx);

    if (m_nodes.isEmpty())
        setIsEmpty(true);
    else
        bakeShaders();

    setHasUnsavedChanges(true);
    emit nodesChanged();
}

// Returns false if new name was generated
bool EffectComposerModel::changeNodeName(int nodeIndex, const QString &name)
{
    QTC_ASSERT(isValidIndex(nodeIndex), return false);

    QString trimmedName = name.trimmed();
    const QString oldName = m_nodes[nodeIndex]->name();

    if (trimmedName.isEmpty())
        trimmedName = oldName;

    const QStringList reservedNames = nodeNames();

    // Matching is done case-insensitive as section headers are shown in all uppercase
    QString newName = QmlDesigner::UniqueName::generate(trimmedName, [&] (const QString &checkName) -> bool {
        return oldName != checkName && (reservedNames.contains(checkName, Qt::CaseInsensitive)
                                        || m_effectComposerNodesModel->nodeExists(checkName));
    });

    if (newName != oldName) {
        m_nodes[nodeIndex]->setName(newName);

        emit dataChanged(index(nodeIndex), index(nodeIndex), {NameRole});
    }

    return newName == trimmedName;
}

void EffectComposerModel::clear(bool clearName)
{
    beginResetModel();
    m_rebakeTimer.stop();
    qDeleteAll(m_nodes);
    m_nodes.clear();
    endResetModel();

    if (clearName) {
        setCurrentComposition("");
        setCompositionPath("");
        resetRootFragmentShader();
        resetRootVertexShader();
    }

    resetEffectError();
    setHasUnsavedChanges(!m_currentComposition.isEmpty());
    setCodeEditorIndex(INVALID_CODE_EDITOR_INDEX);

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
    const QString path = !m_compositionPath.isEmpty() ? m_compositionPath.parentDir().pathAppended("%1.qep").toUrlishString()
                                                      : effectsDir + '/' + "%1" + ".qep";

    return QmlDesigner::UniqueName::generate("Effect01", [&] (const QString &effectName) {
        return QFile::exists(path.arg(effectName));
    });
}

QString EffectComposerModel::getUniqueDisplayName(const QStringList reservedNames) const
{
    return QmlDesigner::UniqueName::generate(
        Tr::tr("New Property"),
        [&reservedNames](const QString &name) { return reservedNames.contains(name); });
}

bool EffectComposerModel::nameExists(const QString &name) const
{
    const QString effectsDir = QmlDesigner::ModelNodeOperations::getEffectsDefaultDirectory();
    const QString path = !m_compositionPath.isEmpty() ? m_compositionPath.parentDir().pathAppended("%1.qep").toUrlishString()
                                                      : effectsDir + '/' + "%1" + ".qep";

    return QFile::exists(path.arg(name));
}

void EffectComposerModel::chooseCustomPreviewImage()
{
    QTimer::singleShot(0, this, [&]() {
        using Utils::FilePath;
        static FilePath lastDir;
        const QStringList &suffixes = QmlDesigner::Asset::supportedImageSuffixes();
        QmlDesigner::DesignDocument *document = QmlDesigner::QmlDesignerPlugin::instance()->currentDesignDocument();
        const FilePath currentDir = lastDir.isEmpty() ? document->fileName().parentDir()
                                                             : lastDir;
        const QStringList fileNames = QFileDialog::getOpenFileNames(
            Core::ICore::dialogParent(),
            Tr::tr("Select Custom Effect Background Image"),
            currentDir.toFSPathString(),
            Tr::tr("Image Files (%1)").arg(suffixes.join(" ")));

        if (!fileNames.isEmpty()) {
            FilePath imageFile = FilePath::fromString(fileNames.first());
            lastDir = imageFile.absolutePath();
            if (imageFile.exists()) {
                FilePath imagesDir = customPreviewImagesPath();
                if (!imagesDir.exists())
                    imagesDir.createDir();
                FilePath targetFile = imagesDir.pathAppended(imageFile.fileName());
                if (!targetFile.exists())
                    imageFile.copyFile(targetFile);
                if (targetFile.exists()) {
                    QUrl imgUrl = QUrl::fromLocalFile(targetFile.toFSPathString());
                    if (!m_customPreviewImages.contains(imgUrl))
                        m_customPreviewImages.append(imgUrl);
                    m_currentPreviewImage = imgUrl;

                    setHasUnsavedChanges(true);

                    emit currentPreviewImageChanged();
                    emit previewImagesChanged();
                    emit customPreviewImageCountChanged();
                }
            }
        }
    });
}

void EffectComposerModel::previewComboAboutToOpen()
{
    m_customPreviewImages.clear();
    const Utils::FilePaths imagePaths = customPreviewImagesPath().dirEntries(QDir::Files);
    for (const auto &imagePath : imagePaths) {
        QmlDesigner::Asset asset(imagePath.toFSPathString());
        if (asset.isImage())
            m_customPreviewImages.append(imagePath.toUrl());
    }

    emit previewImagesChanged();
    emit customPreviewImageCountChanged();

    if (!previewImages().contains(m_currentPreviewImage))
        setCurrentPreviewImage({});
}

void EffectComposerModel::removeCustomPreviewImage(const QUrl &url)
{
    int count = m_customPreviewImages.size();
    m_customPreviewImages.removeOne(url);
    if (url.isLocalFile()) {
        Utils::FilePath img = Utils::FilePath::fromUrl(url);
        if (img.exists())
            img.removeFile();
    }
    if (m_customPreviewImages.size() != count) {
        emit previewImagesChanged();
        emit customPreviewImageCountChanged();
        if (url == m_currentPreviewImage) {
            m_currentPreviewImage = defaultPreviewImage();
            emit currentPreviewImageChanged();
        }
    }
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

void EffectComposerModel::setRootFragmentShader(const QString &shader)
{
    m_rootFragmentShader = shader;
}

void EffectComposerModel::resetRootFragmentShader()
{
    static const QString defaultRootFragmentShader = {
        "void main() {\n"
        "    fragColor = texture(iSource, texCoord);\n"
        "    @nodes\n"
        "    fragColor = fragColor * qt_Opacity;\n"
        "}\n"};
    setRootFragmentShader(defaultRootFragmentShader);
}

void EffectComposerModel::setRootVertexShader(const QString &shader)
{
    m_rootVertexShader = shader;
}

void EffectComposerModel::resetRootVertexShader()
{
    static const QString defaultRootVertexShader = {
        "void main() {\n"
        "    texCoord = qt_MultiTexCoord0;\n"
        "    fragCoord = qt_Vertex.xy;\n"
        "    vec2 vertCoord = qt_Vertex.xy;\n"
        "    @nodes\n"
        "    gl_Position = qt_Matrix * vec4(vertCoord, 0.0, 1.0);\n"
        "}\n"};
    setRootVertexShader(defaultRootVertexShader);
}

QString EffectComposerModel::qmlComponentString() const
{
    return m_qmlComponentString;
}

const QList<Uniform *> EffectComposerModel::allUniforms() const
{
    QList<Uniform *> uniforms;
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

void EffectComposerModel::resetEffectError(int type, bool notify)
{
    if (type < 0 && !m_effectErrors.isEmpty()) {
        m_effectErrors.clear();
        if (notify)
            Q_EMIT effectErrorsChanged();
    } else if (m_effectErrors.contains(type)) {
        m_effectErrors.remove(type);
        if (notify)
            Q_EMIT effectErrorsChanged();
    }
}

// Set the effect error message with optional type and lineNumber.
// Type comes from ErrorTypes, defaulting to common errors (-1).
void EffectComposerModel::setEffectError(const QString &errorMessage, int type,
                                         bool notify, int lineNumber)
{
    EffectError error;
    error.m_type = type;
    if (type == 2) {
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
    QList<EffectError> &errors = m_effectErrors[type];
    errors.append(error);

    qWarning() << QString("Effect error (line: %2): %1").arg(error.m_message).arg(error.m_line);

    if (notify)
        Q_EMIT effectErrorsChanged();
}

QString EffectComposerModel::effectErrors() const
{
    static const QStringList errorTypeStrings{
        Tr::tr("Common error: %1"),
        Tr::tr("QML parsing error: %1"),
        Tr::tr("Shader error: %1"),
        Tr::tr("Preprocessor error: %1")};

    QString retval;
    for (const QList<EffectError> &errors : std::as_const(m_effectErrors)) {
        for (const EffectError &e : errors) {
            if (!e.m_message.isEmpty()) {
                int index = e.m_type;
                if (index < 0 || index >= errorTypeStrings.size())
                    index = 0;
                retval.append(errorTypeStrings[index].arg(e.m_message) + "\n");
            }
        }
    }

    if (!retval.isEmpty())
        retval.chop(1); // Remove last newline

    return retval;
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
    nodeObject.insert("custom", node.isCustom());
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
        if (uniform->userAdded())
            uniformObject.insert("userAdded", true);

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
                    id: timeRunningCheckBox
                    text: backendValues.timeRunning.valueToString
                    backendValue: backendValues.timeRunning
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
                }
                ExpandingSpacer {}
            }
)";
        s += animSec.arg(
            Tr::tr("Animation"),
            Tr::tr("Running"),
            Tr::tr("Set this property to animate the effect."));

        if (m_shaderFeatures.enabled(ShaderFeatures::Time)) {
            QString timeProp =
R"(
            PropertyLabel {
                text: "%1"
                tooltip: "%2"
            }

            SecondColumnLayout {
                SpinBox {
                    enabled: !timeRunningCheckBox.checked
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
            s += timeProp.arg(
                Tr::tr("Time"),
                //: do not translate "Running"
                Tr::tr("This property allows explicit control of current animation time when "
                       "Running property is false."));
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
                    enabled: !timeRunningCheckBox.checked
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
            s += frameProp.arg(
                Tr::tr("Frame"),
                //: do not translate "Running"
                Tr::tr("This property allows explicit control of current animation frame when "
                       "Running property is false."));
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
        s += generalSection.arg(
            Tr::tr("General"),
            Tr::tr("Extra Margin"),
            Tr::tr("This property specifies how much of extra space is reserved for the effect "
                   "outside the parent geometry."));
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
        s += "    property real animatedTime: 0\n";
    }
    if (m_shaderFeatures.enabled(ShaderFeatures::Frame)) {
        s += "    // When timeRunning is false, this can be used to control iFrame manually\n";
        s += "    property int animatedFrame: 0\n";
    }

    QString imageFixerTag{"___ecImagefixer___"};
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
%9
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
                                          mipmap3,
                                          imageFixerTag);
    } else {
        parentChanged = parentChanged.arg(QString(), QString(), QString(),
                                          QString(), QString(), QString(),
                                          QString(), QString(), QString());
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

    QString imageFixerStr;
    QString customImagesString = getQmlImagesString(true, imageFixerStr);
    if (!customImagesString.isEmpty())
        s += customImagesString;

    s += "    Component {\n";
    s += "        id: effectComponent\n";
    s += getQmlComponentString(true);
    s += "    }\n";
    s += "}\n";

    s.replace(imageFixerTag, imageFixerStr);

    return s;
}

void EffectComposerModel::connectCodeEditor()
{
    EffectShadersCodeEditor *editor = EffectShadersCodeEditor::instance();
    editor->setCompositionsModel(this);

    connect(this, &QObject::destroyed, editor, &QObject::deleteLater);

    connect(
        editor,
        &EffectShadersCodeEditor::rebakeRequested,
        this,
        &EffectComposerModel::startRebakeTimer);

    connect(editor, &EffectShadersCodeEditor::openedChanged, this, [this](bool visible) {
        if (!visible)
            setCodeEditorIndex(INVALID_CODE_EDITOR_INDEX);
    });
}

void EffectComposerModel::createCodeEditorData()
{
    using TextEditor::TextDocument;
    if (m_shaderEditorData)
        return;

    m_shaderEditorData.reset(
        EffectShadersCodeEditor::instance()
            ->createEditorData(m_rootFragmentShader, m_rootVertexShader, nullptr));

    connect(m_shaderEditorData->fragmentDocument.get(), &TextDocument::contentsChanged, this, [this] {
        setRootFragmentShader(m_shaderEditorData->fragmentDocument->plainText());
        setHasUnsavedChanges(true);
        rebakeIfLiveUpdateMode();
    });

    connect(m_shaderEditorData->vertexDocument.get(), &TextDocument::contentsChanged, this, [this] {
        setRootVertexShader(m_shaderEditorData->vertexDocument->plainText());
        setHasUnsavedChanges(true);
        rebakeIfLiveUpdateMode();
    });
}

void EffectComposerModel::writeComposition(const QString &name)
{
    resetEffectError(ErrorCommon);
    resetEffectError(ErrorQMLParsing);

    if (name.isEmpty() || name.size() < 3 || name[0].isLower()) {
        setEffectError(QString("Failed to save composition '%1', name is invalid").arg(name));
        return;
    }

    const QString effectsAssetsDir = QmlDesigner::ModelNodeOperations::getEffectsDefaultDirectory();

    const QString path = !m_compositionPath.isEmpty()
                             ? m_compositionPath.parentDir().pathAppended(name + ".qep").toUrlishString()
                             : effectsAssetsDir + '/' + name + ".qep";

    auto saveFile = QFile(path);
    if (!saveFile.open(QIODevice::WriteOnly)) {
        setEffectError(QString("Failed to save composition file: '%1'").arg(path));
        return;
    }

    const Utils::FilePath compositionPath = Utils::FilePath::fromString(path);
    const Utils::FilePath compositionDir = compositionPath.absolutePath();

    updateExtraMargin();

    QJsonObject json;
    // File format version
    json.insert("version", 1);
    json.insert("tool", "EffectComposer");

    QString previewStr;
    Utils::FilePath previewPath = Utils::FilePath::fromUrl(m_currentPreviewImage);
    if (m_currentPreviewImage.isLocalFile())
        previewStr = previewPath.fileName();
    else
        previewStr = m_currentPreviewImage.toString();
    json.insert("previewImage", previewStr);
    json.insert("previewColor", m_currentPreviewColor.name());

    // Add nodes
    QJsonArray nodesArray;
    for (const CompositionNode *node : std::as_const(m_nodes)) {
        QJsonObject nodeObject = nodeToJson(*node);
        nodesArray.append(nodeObject);
    }

    auto toJsonArray = [](const QString &code) -> QJsonArray {
        if (code.isEmpty())
            return {};
        return QJsonArray::fromStringList(code.split('\n'));
    };

    if (!nodesArray.isEmpty())
        json.insert("nodes", nodesArray);

    json.insert("vertexCode", toJsonArray(m_rootVertexShader));
    json.insert("fragmentCode", toJsonArray(m_rootFragmentShader));

    QJsonObject rootJson;
    rootJson.insert("QEP", json);
    QJsonDocument jsonDoc(rootJson);

    saveFile.write(jsonDoc.toJson());
    saveFile.close();

    setCurrentComposition(name);
    setCompositionPath(compositionPath);

    saveResources(name);
    setHasUnsavedChanges(false);
}

void EffectComposerModel::saveComposition(const QString &name)
{
    writeComposition(name);
    m_pendingSaveBakeCounter = m_currentBakeCounter + 1; // next bake counter
    startRebakeTimer();
}

void EffectComposerModel::openCodeEditor(int idx)
{
    if (idx == MAIN_CODE_EDITOR_INDEX)
        return openMainCodeEditor();

    if (idx < 0 || idx >= m_nodes.size())
        return;

    CompositionNode *node = m_nodes.at(idx);
    node->openCodeEditor();

    setCodeEditorIndex(idx);
}

void EffectComposerModel::openMainCodeEditor()
{
    createCodeEditorData();

    auto editor = EffectShadersCodeEditor::instance();
    editor->setupShader(m_shaderEditorData.get());
    editor->showWidget();

    setCodeEditorIndex(MAIN_CODE_EDITOR_INDEX);
}

bool EffectComposerModel::canAddNodeToLibrary(int idx)
{
    QTC_ASSERT(isValidIndex(idx), return false);
    CompositionNode *node = m_nodes.at(idx);

    if (m_effectComposerNodesModel->isBuiltIn(node->name()))
        return !m_effectComposerNodesModel->nodeExists(node->name());

    return true;
}

bool EffectComposerModel::nodeExists(int idx)
{
    QTC_ASSERT(isValidIndex(idx), return false);
    CompositionNode *node = m_nodes.at(idx);

    return m_effectComposerNodesModel->nodeExists(node->name());
}

QString EffectComposerModel::addNodeToLibraryNode(int idx)
{
    const QString errorTag{"@ERROR@"};
    QTC_ASSERT(isValidIndex(idx) && canAddNodeToLibrary(idx), return errorTag);

    CompositionNode *node = m_nodes.at(idx);

    // Adding node to library changes it to a custom one, as we can't overwrite builtin nodes
    node->setCustom(true);

    QString fileNameBase = EffectUtils::nodeNameToFileName(node->name());
    Utils::FilePath nodePath = Utils::FilePath::fromString(EffectUtils::nodeLibraryPath())
                                   .pathAppended(fileNameBase);
    Utils::FilePath qenFile = nodePath.pathAppended(fileNameBase + ".qen");

    QSet<QString> newFileNames = {qenFile.fileName()};
    Utils::FilePaths oldFiles;
    bool update = nodePath.exists();
    if (update)
        oldFiles = nodePath.dirEntries(QDir::Files);
    else
        nodePath.createDir();

    QJsonObject rootObj;
    QJsonObject nodeObject = nodeToJson(*node);
    rootObj.insert("QEN", nodeObject);
    QJsonDocument jsonDoc(rootObj);
    Utils::Result<qint64> result = qenFile.writeFileContents(jsonDoc.toJson());
    if (!result)
        return errorTag + Tr::tr("Failed to write QEN file for effect:\n%1").arg(qenFile.fileName());

    QList<Utils::FilePath> sources;
    QStringList dests;
    const QList<Uniform *> uniforms = node->uniforms();
    for (Uniform *uniform : uniforms) {
        const QString valueStr = uniform->value().toString();
        if (uniform->type() == Uniform::Type::Sampler && !valueStr.isEmpty()) {
            Utils::FilePath imagePath = Utils::FilePath::fromString(valueStr);
            QString imageFilename = imagePath.fileName();
            sources.append(imagePath);
            dests.append(imageFilename);
        }
    }

    for (int i = 0; i < sources.count(); ++i) {
        Utils::FilePath source = sources[i];
        Utils::FilePath target = nodePath.pathAppended(dests[i]);
        newFileNames.insert(target.fileName());
        if (target.exists() && source.fileName() != target.fileName())
            target.removeFile(); // Remove existing file for update
        if (!target.exists() && !source.copyFile(target))
            return errorTag + Tr::tr("Failed to copy effect resource:\n%1").arg(source.fileName());
    }

    // Delete old content that was not overwritten
    for (const Utils::FilePath &oldFile : std::as_const(oldFiles)) {
        if (!newFileNames.contains(oldFile.fileName()))
            oldFile.removeFile();
    }

    m_effectComposerNodesModel->loadCustomNodes();

    return update ? Tr::tr("Effect was updated in effect library.")
                  : Tr::tr("Effect was added to effect library.");
}

QVariant EffectComposerModel::valueLimit(const QString &type, bool max) const
{
    static const int intMin = std::numeric_limits<int>::lowest();
    static const int intMax = std::numeric_limits<int>::max();
    static const float floatMin = std::numeric_limits<float>::lowest();
    static const float floatMax = std::numeric_limits<float>::max();

    if (type == "float")
        return max ? floatMax : floatMin;
    if (type == "int")
        return max ? intMax : intMin;

    qWarning() << __FUNCTION__ << "Invalid type for limit:" << type;

    return {};
}

void EffectComposerModel::openComposition(const QString &path)
{
    clear(true);

    Utils::FilePath effectPath = Utils::FilePath::fromString(path);
    const QString effectName = effectPath.baseName();

    setCurrentComposition(effectName);
    setCompositionPath(effectPath);

    QFile compFile(path);
    if (!compFile.open(QIODevice::ReadOnly)) {
        setEffectError(QString("Failed to open composition file: '%1'").arg(path));
        return;
    }

    QByteArray data = compFile.readAll();

    if (data.isEmpty())
        return;

    QJsonParseError parseError;
    QJsonDocument jsonDoc(QJsonDocument::fromJson(data, &parseError));
    if (parseError.error != QJsonParseError::NoError) {
        setEffectError(QString("Failed to parse the project file: %1").arg(parseError.errorString()));
        return;
    }
    QJsonObject rootJson = jsonDoc.object();
    if (!rootJson.contains("QEP")) {
        setEffectError(QStringLiteral("Invalid project file"));
        return;
    }

    QJsonObject json = rootJson["QEP"].toObject();

    const QString toolName = json.contains("tool")   ? json["tool"].toString()
                             : json.contains("QQEM") ? "QQEM"_L1
                                                     : ""_L1;

    if (!toolName.isEmpty() && toolName != "EffectComposer") {
        setEffectError(QString("'%1' effects are not compatible with 'Effect Composer'").arg(toolName));
        return;
    }

    int version = -1;
    if (json.contains("version"))
        version = json["version"].toInt(-1);

    if (version != 1) {
        setEffectError(QString("Unknown project version (%1)").arg(version));
        return;
    }

    auto toCodeBlock = [](const QJsonValue &jsonValue) -> QString {
        if (!jsonValue.isArray())
            return {};

        QString code;
        const QJsonArray array = jsonValue.toArray();
        for (const QJsonValue &lineValue : array) {
            if (lineValue.isString())
                code += lineValue.toString() + '\n';
        }

        if (!code.isEmpty())
            code.chop(1);

        return code;
    };

    if (json.contains("vertexCode"))
        setRootVertexShader(toCodeBlock(json["vertexCode"]));
    else
        resetRootVertexShader();

    if (json.contains("fragmentCode"))
        setRootFragmentShader(toCodeBlock(json["fragmentCode"]));
    else
        resetRootFragmentShader();

    m_currentPreviewImage = defaultPreviewImage();
    if (json.contains("previewImage")) {
        const QString imageStr = json["previewImage"].toString();
        if (!imageStr.isEmpty()) {
            if (imageStr.startsWith("images/preview")) {// built-in preview image
                m_currentPreviewImage = QUrl(imageStr);
            } else {
                Utils::FilePath prevPath = customPreviewImagesPath().pathAppended(imageStr);
                if (prevPath.exists())
                    m_currentPreviewImage = prevPath.toUrl();
            }
        }
    }

    if (json.contains("previewColor"))
        m_currentPreviewColor = QColor::fromString(json["previewColor"].toString());

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
    emit currentPreviewImageChanged();
    emit currentPreviewColorChanged();
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
    const QString effectsResPath = effectsResDir.pathAppended(name).toUrlishString() + '/';
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
    QSet<QString> newFileNames;

    // Create effect folder if not created
    if (!effectPath.exists())
        effectPath.createDir();
    else
        oldFiles = effectPath.dirEntries(QDir::Files);

    // Create effect qmldir
    newFileNames.insert(qmldirFileName);
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
    newFileNames.insert(qmlFilename);

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
        newFileNames.insert(target.fileName());
        if (target.exists() && source.fileName() != target.fileName())
            target.removeFile(); // Remove existing file for update
        if (!target.exists() && !source.copyFile(target))
            setEffectError(QString("Failed to copy file: %1").arg(source.toFSPathString()));

        if (fileNameToUniformHash.contains(dests[i])) {
            Uniform *uniform = fileNameToUniformHash[dests[i]];
            const QVariant newValue = target.toUrlishString();
            uniform->setDefaultValue(newValue);
            uniform->setValue(newValue);
        }
    }

    // Delete old content that was not overwritten
    // We ignore subdirectories, as currently subdirs only contain fixed content
    for (const Utils::FilePath &oldFile : std::as_const(oldFiles)) {
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

void EffectComposerModel::openNearestAvailableCodeEditor(int idx)
{
    int nearestIdx = idx;

    if (nearestIdx >= m_nodes.size())
        nearestIdx = m_nodes.size() - 1;

    while (nearestIdx >= 0) {
        CompositionNode *node = m_nodes.at(nearestIdx);
        if (!node->isDependency())
            return openCodeEditor(nearestIdx);

        --nearestIdx;
    }

    openMainCodeEditor();
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
        return getImageElementName(uniform);
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
        setEffectError(QString("Unhandled const variable type: %1").arg(int(uniform.type())),
                       ErrorQMLParsing);
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
        return getImageElementName(uniform);
    } else {
        setEffectError(QString("Unhandled const variable type: %1").arg(int(uniform.type())),
                       ErrorQMLParsing);
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
        setEffectError(QString("Unhandled const variable type: %1").arg(int(uniform.type())),
                       ErrorQMLParsing);
        return QString();
    }
}

// Return name for the image property Image element
QString EffectComposerModel::getImageElementName(const Uniform &uniform) const
{
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
    QStringList s_sourceCode = m_rootVertexShader.split('\n');
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
    QStringList s_sourceCode = m_rootFragmentShader.split('\n');
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

void EffectComposerModel::handleQsbProcessExit(
    Utils::Process *qsbProcess, const QString &shader, bool preview, int bakeCounter)
{
    if (bakeCounter == m_currentBakeCounter) {
        if (!m_qsbFirstProcessIsDone) {
            m_qsbFirstProcessIsDone = true;
            resetEffectError(ErrorShader, false);
        }

        --m_remainingQsbTargets;

        const QString errStr = qsbProcess->errorString();
        const QByteArray errStd = qsbProcess->readAllRawStandardError();
        QString previewStr;
        if (preview)
            previewStr = QStringLiteral("preview");

        if (!errStr.isEmpty() || !errStd.isEmpty()) {
            const QString failMessage = "Failed to generate %3 QSB file for: %1\n%2";
            QString error;
            if (!errStr.isEmpty())
                error = failMessage.arg(shader, errStr, previewStr);
            if (!errStd.isEmpty())
                error = failMessage.arg(shader, QString::fromUtf8(errStd), previewStr);
            setEffectError(error, ErrorShader, false);
        }

        if (m_remainingQsbTargets <= 0) {
            Q_EMIT shadersBaked();
            setShadersUpToDate(true);
            Q_EMIT effectErrorsChanged();

            // TODO: Mark shaders as baked, required by export later
        }
    }

    if (bakeCounter == m_pendingSaveBakeCounter && !preview)
        copyProcessTargetToEffectDir(qsbProcess);

    qsbProcess->deleteLater();
}

void EffectComposerModel::copyProcessTargetToEffectDir(Utils::Process *qsbProcess)
{
    auto getShaderFormat = [](const Utils::FilePath &path) -> QString {
        static const QStringList acceptedFormats = {"frag.qsb", "vert.qsb"};
        QString shaderFormat = path.completeSuffix();
        for (const QString &checkFormat : acceptedFormats) {
            if (shaderFormat.endsWith(checkFormat))
                return checkFormat;
        }
        return {};
    };

    auto findOutputFileFromArgs = [](Utils::Process *qsbProcess) -> Utils::FilePath {
        QStringList qsbProcessArgs = qsbProcess->commandLine().splitArguments();
        int outputIndex = qsbProcessArgs.indexOf("-o", 0, Qt::CaseInsensitive);
        if (outputIndex > 0) {
            if (++outputIndex < qsbProcessArgs.size())
                return Utils::FilePath::fromString(qsbProcessArgs.at(outputIndex));
        }
        return {};
    };

    const Utils::FilePath &outputFile = findOutputFileFromArgs(qsbProcess);
    const QString &shaderFormat = getShaderFormat(outputFile);
    const QString &effectName = currentComposition();
    const Utils::FilePath &effectsResDir
        = QmlDesigner::ModelNodeOperations::getEffectsImportDirectory();
    Utils::FilePath destFile = effectsResDir.pathAppended(
        "/" + effectName + "/" + effectName + "." + shaderFormat);

    if (destFile.exists())
        destFile.removeFile();

    outputFile.copyFile(destFile);
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
    static const QString fileNameTemplate = "%1_%2.%3";
    const QString countStr = QString::number(m_currentBakeCounter);

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

    ++m_currentBakeCounter;
}

void EffectComposerModel::bakeShaders()
{
    initShaderDir();

    if (Utils::FilePath::fromString(m_vertexSourceFilename).exists()
        && Utils::FilePath::fromString(m_fragmentSourceFilename).exists()
        && m_vertexShader == generateVertexShader()
        && m_fragmentShader == generateFragmentShader()) {
        setShadersUpToDate(true);
        return;
    }

    const QString failMessage = "Shader baking failed: %1";
    // Don't reset shader errors yet to avoid UI flicker
    resetEffectError(ErrorCommon);
    resetEffectError(ErrorQMLParsing);
    resetEffectError(ErrorPreprocessor);

    const ProjectExplorer::Kit *kit = ProjectExplorer::activeKitForCurrentProject();
    if (!kit) {
        setEffectError(failMessage.arg("Target not found"));
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

    QtSupport::QtVersion *qtVer = QtSupport::QtKitAspect::qtVersion(kit);
    if (!qtVer) {
        setEffectError(failMessage.arg("Qt version not found"));
        return;
    }

    Utils::FilePath qsbPath = qtVer->binPath().pathAppended("qsb").withExecutableSuffix();
    if (!qsbPath.exists()) {
        setEffectError(failMessage.arg("QSB tool for target kit not found"));
        return;
    }

    Utils::FilePath binPath = Utils::FilePath::fromString(
        QLibraryInfo::path(QLibraryInfo::BinariesPath));
    Utils::FilePath qsbPrevPath = binPath.pathAppended("qsb").withExecutableSuffix();
    if (!qsbPrevPath.exists()) {
        setEffectError(failMessage.arg("QSB tool for preview shaders not found"));
        return;
    }

    m_remainingQsbTargets = 0;
    const QStringList srcPaths = {m_vertexSourceFilename, m_fragmentSourceFilename};
    const QStringList outPaths = {m_vertexShaderFilename, m_fragmentShaderFilename};
    const QStringList outPrevPaths = {m_vertexShaderPreviewFilename, m_fragmentShaderPreviewFilename};

    auto runQsb = [this, srcPaths](
                      const Utils::FilePath &qsbPath, const QStringList &outPaths, bool preview) {
        for (int i = 0; i < 2; ++i) {
            const auto workDir = Utils::FilePath::fromString(outPaths[i]);
            // TODO: Optional legacy glsl support like standalone effect maker needs to add "100es,120"
            QStringList args = {"-s", "--glsl", "300es,140,330,410", "--hlsl", "50", "--msl", "12"};
            args << "-o" << outPaths[i] << srcPaths[i];

            ++m_remainingQsbTargets;

            auto qsbProcess = new Utils::Process(this);
            connect(
                qsbProcess,
                &Utils::Process::done,
                this,
                std::bind(
                    &EffectComposerModel::handleQsbProcessExit,
                    this,
                    qsbProcess,
                    srcPaths[i],
                    preview,
                    std::move(m_currentBakeCounter)));
            qsbProcess->setWorkingDirectory(workDir.absolutePath());
            qsbProcess->setCommand({qsbPath, args});
            qsbProcess->start();
        }
    };

    m_qsbFirstProcessIsDone = false;
    runQsb(qsbPath, outPaths, false);
    runQsb(qsbPrevPath, outPrevPaths, true);

    for (CompositionNode *node : std::as_const(m_nodes))
        node->updateAreUniformsInUse();
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

QString EffectComposerModel::getQmlImagesString(bool localFiles, QString &outImageFixerStr)
{
    const QString imageItemChanged{
R"(
    property var old%2: null
    function %3
    {
        if (old%2) {
            old%2.layer.enabled = false
            old%2 = null
        }
        if (%1 != imageItem%1) {
            %1.layer.enabled = true
            old%2 = %1
        }
    }
    on%2Changed: %3
)"
    };

    QString imagesString;
    const QList<Uniform *> uniforms = allUniforms();
    for (Uniform *uniform : uniforms) {
        if (uniform->type() == Uniform::Type::Sampler) {
            QString imagePath = uniform->value().toString();
            if (localFiles) {
                QString capitalName = uniform->name();
                if (!capitalName.isEmpty())
                    capitalName[0] = capitalName[0].toUpper();
                QString funcName = "setupLayer_" + uniform->name() + "()";
                outImageFixerStr += "\n        " + funcName;
                imagesString += imageItemChanged.arg(uniform->name(), capitalName, funcName);
                QFileInfo fi(imagePath);
                imagePath = fi.fileName();
                imagesString += QString("    property url %1Url: \"%2\"\n")
                                    .arg(uniform->name(), imagePath);
            }
            imagesString += "    Image {\n";
            QString simplifiedName = getImageElementName(*uniform);
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
    using namespace Qt::StringLiterals;
    auto addProperty = [localFiles](const QString &name, const QString &var,
                                    const QString &type, const QString &condition = {},
                                    bool blurHelper = false)
    {
        QString parent;
        if (blurHelper)
            parent = "blurHelper.";
        else if (localFiles)
            parent = "rootItem.";

        return QString("readonly property %1 %2: %5%3%4\n").arg(type, name, parent, var, condition);
    };

    QString s;
    const QString l1 = localFiles ? "        "_L1 : ""_L1;
    const QString l2 = localFiles ? "            "_L1 : "    "_L1;
    const QString l3 = localFiles ? "                "_L1 : "        "_L1;

    if (!localFiles)
        s += "import QtQuick\n";
    s += l1 + "ShaderEffect {\n";

    if (localFiles) {
        // Explicit "source" property is required for rendering QML Puppet to detect effects correctly
        s += l2 + "property Item source: null\n";
    }
    if (m_shaderFeatures.enabled(ShaderFeatures::Source))
        s += l2 + addProperty("iSource", "source", "Item");
    if (m_shaderFeatures.enabled(ShaderFeatures::Time))
        s += l2 + addProperty("iTime", "animatedTime", "real", "frameAnimation.running ? frameAnimation.elapsedTime : ");
    if (m_shaderFeatures.enabled(ShaderFeatures::Frame))
        s += l2 + addProperty("iFrame", "animatedFrame", "int", "frameAnimation.running ? frameAnimation.currentFrame : ");
    if (m_shaderFeatures.enabled(ShaderFeatures::Resolution)) {
        // Note: Pixel ratio is currently always 1.0
        s += l2 + "readonly property vector3d iResolution: Qt.vector3d(width, height, 1.0)\n";
    }
    if (m_shaderFeatures.enabled(ShaderFeatures::Mouse)) { // Do we need interactive effects?
        s += l2 + "readonly property vector4d iMouse: Qt.vector4d(rootItem._effectMouseX, rootItem._effectMouseY,\n";
        s += l2 + "                                               rootItem._effectMouseZ, rootItem._effectMouseW)\n";
    }
    if (m_shaderFeatures.enabled(ShaderFeatures::BlurSources)) {
        s += l2 + addProperty("iSourceBlur1", "blurSrc1", "Item", "", true);
        s += l2 + addProperty("iSourceBlur2", "blurSrc2", "Item", "", true);
        s += l2 + addProperty("iSourceBlur3", "blurSrc3", "Item", "", true);
        s += l2 + addProperty("iSourceBlur4", "blurSrc4", "Item", "", true);
        s += l2 + addProperty("iSourceBlur5", "blurSrc5", "Item", "", true);
    }
    // When used in preview component, we need property with value
    // and when in exported component, property with binding to root value.
    s += localFiles ? m_exportedEffectPropertiesString : m_previewEffectPropertiesString;

    if (!localFiles) {
        QString dummyStr;
        QString customImagesString = getQmlImagesString(false, dummyStr);
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
        if (g_propertyData()->contains("BLUR_HELPER_MAX_LEVEL"))
            blurMax = g_propertyData()->value("BLUR_HELPER_MAX_LEVEL").toInt();
        s += l3 + QString("property int blurMax: %1\n").arg(blurMax);
        s += l3 + "property real blurMultiplier: rootItem.blurMultiplier\n";
        s += l2 + "}\n";
    }

    s += l1 + "}\n";
    return s;
}

void EffectComposerModel::connectCompositionNode(CompositionNode *node)
{
    auto setUnsaved = std::bind(&EffectComposerModel::setHasUnsavedChanges, this, true);
    connect(
        qobject_cast<EffectComposerUniformsModel *>(node->uniformsModel()),
        &EffectComposerUniformsModel::dataChanged,
        this,
        setUnsaved);

    connect(node, &CompositionNode::rebakeRequested, this, &EffectComposerModel::startRebakeTimer);
    connect(node, &CompositionNode::fragmentCodeChanged, this, setUnsaved);
    connect(node, &CompositionNode::vertexCodeChanged, this, setUnsaved);
}

void EffectComposerModel::updateExtraMargin()
{
    m_extraMargin = 0;
    for (CompositionNode *node : std::as_const(m_nodes))
        m_extraMargin = qMax(node->extraMargin(), m_extraMargin);
}

void EffectComposerModel::startRebakeTimer()
{
    // This can come multiple times in a row in response to property changes, so let's buffer it
    m_rebakeTimer.start(200);
}

void EffectComposerModel::rebakeIfLiveUpdateMode()
{
    if (EffectShadersCodeEditor::instance()->liveUpdate())
        startRebakeTimer();
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

void EffectComposerModel::setCodeEditorIndex(int index)
{
    if (m_codeEditorIndex == index)
        return;

    m_codeEditorIndex = index;
    emit codeEditorIndexChanged(m_codeEditorIndex);
}

Utils::FilePath EffectComposerModel::customPreviewImagesPath() const
{
    QStandardPaths::StandardLocation location = QStandardPaths::DocumentsLocation;

    return Utils::FilePath::fromString(QStandardPaths::writableLocation(location))
        .pathAppended("QtDesignStudio/effect_composer/preview_images");
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

    auto editor = EffectShadersCodeEditor::instance();
    editor->close();
    editor->cleanFromData(m_shaderEditorData.get());

    m_shaderEditorData.reset();
}

QList<QUrl> EffectComposerModel::defaultPreviewImages() const
{
    static QList<QUrl> defaultImages = {
        QUrl("images/preview0.png"),
        QUrl("images/preview1.png"),
        QUrl("images/preview2.png"),
        QUrl("images/preview3.png"),
        QUrl("images/preview4.png")
    };

    return defaultImages;
}

QUrl EffectComposerModel::defaultPreviewImage() const
{
    const QList<QUrl> &defaultImages = defaultPreviewImages();
    return defaultImages.first();
}

QList<QUrl> EffectComposerModel::previewImages() const
{
    return m_customPreviewImages + defaultPreviewImages();
}

QColor EffectComposerModel::currentPreviewColor() const
{
    return m_currentPreviewColor;
}

void EffectComposerModel::setCurrentPreviewColor(const QColor &color)
{
    if (m_currentPreviewColor == color)
        return;

    m_currentPreviewColor = color;

    setHasUnsavedChanges(true);

    emit currentPreviewColorChanged();
}

QUrl EffectComposerModel::currentPreviewImage() const
{
    return m_currentPreviewImage;
}

void EffectComposerModel::setCurrentPreviewImage(const QUrl &path)
{
    if (m_currentPreviewImage == path)
        return;

    if (previewImages().contains(path))
        m_currentPreviewImage = path;
    else
        m_currentPreviewImage = defaultPreviewImage();

    setHasUnsavedChanges(true);

    emit currentPreviewImageChanged();
}

int EffectComposerModel::customPreviewImageCount() const
{
    return m_customPreviewImages.size();
}

int EffectComposerModel::mainCodeEditorIndex() const
{
    return MAIN_CODE_EDITOR_INDEX;
}

Utils::FilePath EffectComposerModel::compositionPath() const
{
    return m_compositionPath;
}

void EffectComposerModel::setCompositionPath(const Utils::FilePath &newCompositionPath)
{
    if (m_compositionPath == newCompositionPath)
        return;

    m_compositionPath = newCompositionPath;
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

    if (!m_hasUnsavedChanges) {
        for (CompositionNode *node : std::as_const(m_nodes))
            node->markAsSaved();
    }
}

QStringList EffectComposerModel::uniformNames() const
{
    QStringList usedList;
    const QList<Uniform *> uniforms = allUniforms();
    for (const auto uniform : uniforms)
        usedList.append(uniform->name());
    return usedList;
}

const QStringList EffectComposerModel::nodeNames() const
{
    QStringList names;
    for (const auto &node : std::as_const(m_nodes))
        names.append(node->name());
    return names;
}

QString EffectComposerModel::generateUniformName(const QString &nodeName,
                                                 const QString &propertyName,
                                                 const QString &oldName) const
{
    const QStringList allNames = uniformNames();
    QString uniformName = nodeName;
    if (!propertyName.isEmpty()) {
        QString fixedPropName = propertyName;
        fixedPropName[0] = fixedPropName[0].toUpper();
        uniformName.append(fixedPropName);
    }

    return QmlDesigner::UniqueName::generateId(uniformName, [&] (const QString &name) {
        return allNames.contains(name) && name != oldName;
    });
}

bool EffectComposerModel::isDependencyNode(int index) const
{
    if (m_nodes.size() > index)
        return m_nodes[index]->isDependency();
    return false;
}

bool EffectComposerModel::hasCustomNode() const
{
    for (const auto *node : std::as_const(m_nodes)) {
        if (node->isCustom())
            return true;
    }
    return false;
}

void EffectComposerModel::updateQmlComponent()
{
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

void EffectComposerModel::addOrUpdateNodeUniform(int idx, const QVariantMap &data, int updateIndex)
{
    QTC_ASSERT(isValidIndex(idx), return);

    // Convert values to Uniform digestible strings
    auto fixedData = data;
    auto type = Uniform::typeFromString(data["type"].toString());
    auto controlType = Uniform::typeFromString(data["controlType"].toString());
    fixedData["defaultValue"] = variantAsDataString(type, controlType, data["defaultValue"]);
    fixedData["minValue"] = variantAsDataString(type, controlType, data["minValue"]);
    fixedData["maxValue"] = variantAsDataString(type, controlType, data["maxValue"]);

    if (updateIndex >= 0)
        m_nodes[idx]->updateUniform(updateIndex, fixedData);
    else
        m_nodes[idx]->addUniform(fixedData);

    setHasUnsavedChanges(true);
    startRebakeTimer();
}

bool EffectComposerModel::writeToFile(
    const QByteArray &buf, const QString &fileName, [[maybe_unused]] FileType fileType)
{
    Utils::FilePath fp = Utils::FilePath::fromString(fileName);
    fp.absolutePath().createDir();
    if (!fp.writeFileContents(buf)) {
        setEffectError(QString("Failed to open file for writing: %1").arg(fileName));
        return false;
    }
    return true;
}

bool EffectComposerModel::isValidIndex(int idx) const
{
    return m_nodes.size() > idx && idx >= 0;
}

} // namespace EffectComposer
