// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectnode.h"

#include "compositionnode.h"
#include "effectutils.h"
#include "uniform.h"

#include <QDir>
#include <QFileInfo>

namespace EffectComposer {

EffectNode::EffectNode(const QString &qenPath, bool isBuiltIn)
    : m_qenPath(qenPath)
{
    const QFileInfo fileInfo = QFileInfo(qenPath);

    CompositionNode node({}, qenPath);

    m_name = node.name();
    m_description = node.description();
    m_isCustom = node.isCustom();
    m_canBeRemoved = !isBuiltIn;

    QString iconPath = QStringLiteral("%1/icon/%2.svg").arg(fileInfo.absolutePath(),
                                                            fileInfo.baseName());
    if (!QFileInfo::exists(iconPath)) {
        QString iconFileName = m_isCustom ? QString{"user_custom_effect.svg"}
                                          : QString{"placeholder.svg"};
        iconPath = QStringLiteral("%1/%2").arg(EffectUtils::nodesSourcesPath(), iconFileName);
    }
    m_iconPath = QUrl::fromLocalFile(iconPath);

    const QList<Uniform *> uniforms = node.uniforms();
    for (const Uniform *uniform : uniforms) {
        m_uniformNames.insert(uniform->name());
        if (uniform->type() == Uniform::Type::Sampler) {
            m_defaultImagesHash.insert(
                uniform->name(), uniform->defaultValue().toString());
        }
    }
}

QString EffectNode::name() const
{
    return m_name;
}

QString EffectNode::description() const
{
    return m_description;
}

QString EffectNode::qenPath() const
{
    return m_qenPath;
}

void EffectNode::setCanBeAdded(bool enabled)
{
    if (enabled != m_canBeAdded) {
        m_canBeAdded = enabled;
        emit canBeAddedChanged();
    }
}

bool EffectNode::hasUniform(const QString &name)
{
    return m_uniformNames.contains(name);
}

} // namespace EffectComposer

