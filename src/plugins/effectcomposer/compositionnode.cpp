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
#include <QRegularExpression>
#include <QRegularExpressionMatch>

namespace {

struct CodeRename
{
    CodeRename(const QString &oldName, const QString &newName)
        : m_newName(newName)
        , m_regex{QString(R"(\b%1\b)").arg(oldName)}
    {}

    QString operator()(QString code) const { return code.replace(m_regex, m_newName); }

    void operator()(QTextDocument *document) const
    {
        QTextCursor docCursor(document);
        bool documentChanged = false;
        QTextBlock block = document->lastBlock();
        while (block.isValid()) {
            QString blockText = block.text();
            QRegularExpressionMatch match = m_regex.match(blockText);
            if (match.hasMatch()) {
                if (!documentChanged) {
                    docCursor.beginEditBlock();
                    documentChanged = true;
                }
                blockText.replace(m_regex, m_newName);
                QTextCursor blockCursor(block);
                blockCursor.movePosition(QTextCursor::StartOfBlock);
                blockCursor.insertText(blockText);
                blockCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                blockCursor.removeSelectedText();
            }
            block = block.previous();
        }
        if (documentChanged)
            docCursor.endEditBlock();
    }

    const QString m_newName;
    const QRegularExpression m_regex;
};

} // namespace

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

    connect(
        &m_uniformsModel,
        &QAbstractItemModel::rowsRemoved,
        this,
        &CompositionNode::rebakeRequested);

    connect(
        &m_uniformsModel,
        &EffectComposerUniformsModel::uniformRenamed,
        this,
        &CompositionNode::onUniformRenamed);
}

CompositionNode::~CompositionNode()
{
    EffectShadersCodeEditor::instance()->cleanFromData(m_shaderEditorData.get());
};

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
    return &m_uniformsModel;
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

bool CompositionNode::isCustom() const
{
    return m_isCustom;
}

void CompositionNode::setCustom(bool enable)
{
    if (enable != m_isCustom) {
        m_isCustom = enable;
        emit isCustomChanged();
    }
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

    if (json.contains("custom"))
        m_isCustom = json["custom"].toBool();

    m_id = json.value("id").toString();
    if (m_id.isEmpty() && !qenPath.isEmpty()) {
        QString fileName = qenPath.split('/').last();
        fileName.chop(4); // remove ".qen"
        m_id = fileName;
    }

    // parse properties
    const QJsonArray &jsonProps = json.value("properties").toArray();
    for (const QJsonValueConstRef &prop : jsonProps) {
        const auto uniform = new Uniform(effectName, prop.toObject(), qenPath);
        m_uniformsModel.addUniform(uniform);
        g_propertyData()->insert(uniform->name(), uniform->value());
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

void CompositionNode::ensureCodeEditorData()
{
    using TextEditor::TextDocument;
    if (m_shaderEditorData)
        return;

    m_shaderEditorData.reset(EffectShadersCodeEditor::instance()
                                 ->createEditorData(fragmentCode(), vertexCode(), &m_uniformsModel));

    connect(m_shaderEditorData->fragmentDocument.get(), &TextDocument::contentsChanged, this, [this] {
        setFragmentCode(m_shaderEditorData->fragmentDocument->plainText());
    });

    connect(m_shaderEditorData->vertexDocument.get(), &TextDocument::contentsChanged, this, [this] {
        setVertexCode(m_shaderEditorData->vertexDocument->plainText());
    });
}

void CompositionNode::requestRebakeIfLiveUpdateMode()
{
    if (EffectShadersCodeEditor::instance()->liveUpdate())
        emit rebakeRequested();
}

QList<Uniform *> CompositionNode::uniforms() const
{
    return m_uniformsModel.uniforms();
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
    m_InUseCheckNeeded = true;
    emit fragmentCodeChanged();

    requestRebakeIfLiveUpdateMode();
}

void CompositionNode::setVertexCode(const QString &vertexCode)
{
    if (m_vertexCode == vertexCode)
        return;

    m_vertexCode = vertexCode;
    m_InUseCheckNeeded = true;
    emit vertexCodeChanged();

    requestRebakeIfLiveUpdateMode();
}

void CompositionNode::markAsSaved()
{
    if (!m_shaderEditorData)
        return;

    m_shaderEditorData->fragmentDocument->document()->setModified(false);
    m_shaderEditorData->vertexDocument->document()->setModified(false);
}

void CompositionNode::openCodeEditor()
{
    auto editor = EffectShadersCodeEditor::instance();
    ensureCodeEditorData();
    editor->setupShader(m_shaderEditorData.get());
    editor->showWidget();
}

void CompositionNode::addUniform(const QVariantMap &data)
{
    const auto uniform = new Uniform({}, QJsonObject::fromVariantMap(data), {});
    g_propertyData()->insert(uniform->name(), uniform->value());
    m_uniformsModel.addUniform(uniform);
    updateAreUniformsInUse(true);
}

void CompositionNode::updateUniform(int index, const QVariantMap &data)
{
    QTC_ASSERT(index < uniforms().size() && index >= 0, return);

    const auto uniform = new Uniform({}, QJsonObject::fromVariantMap(data), {});

    g_propertyData()->insert(uniform->name(), uniform->value());
    m_uniformsModel.updateUniform(index, uniform);
    updateAreUniformsInUse(true);
}

void CompositionNode::updateAreUniformsInUse(bool force)
{
    if (force || m_InUseCheckNeeded) {
        const QString matchTemplate("\\b%1\\b");
        const QList<Uniform *> uniList = uniforms();

        // Some of the uniforms may only be used by customValue properties
        QString customValues;
        for (const Uniform *u : uniList) {
            if (!u->customValue().isEmpty()) {
                customValues.append(u->customValue());
                customValues.append(' ');
            }
        }

        for (int i = 0; i < uniList.size(); ++i) {
            Uniform *u = uniList[i];
            QString pattern = matchTemplate.arg(QRegularExpression::escape(u->name()));
            QRegularExpression regex(pattern);
            bool found = false;
            found = regex.match(m_fragmentCode).hasMatch();
            if (!found)
                found = regex.match(m_vertexCode).hasMatch();
            if (!found && !customValues.isEmpty())
                found = regex.match(customValues).hasMatch();
            m_uniformsModel.setData(m_uniformsModel.index(i), found,
                                    EffectComposerUniformsModel::IsInUse);
        }
        m_InUseCheckNeeded = false;
    }
}

void CompositionNode::onUniformRenamed(const QString &oldName, const QString &newName)
{
    CodeRename codeRename{oldName, newName};
    if (m_shaderEditorData) {
        codeRename(m_shaderEditorData->vertexDocument->document());
        codeRename(m_shaderEditorData->fragmentDocument->document());
    } else {
        setVertexCode(codeRename(vertexCode()));
        setFragmentCode(codeRename(fragmentCode()));
    }
}

QString CompositionNode::name() const
{
    return m_name;
}

void CompositionNode::setName(const QString &name)
{
    if (m_name == name)
        return;

    m_name = name;
    emit nameChanged();
}

} // namespace EffectComposer

