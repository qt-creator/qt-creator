// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QImageReader>

#include "asset.h"

namespace QmlDesigner {

Asset::Asset(const QString &filePath)
    : m_filePath(filePath)
{
    m_suffix = "*." + filePath.split('.').last().toLower();
}


const QStringList &Asset::supportedImageSuffixes()
{
    static QStringList retList;
    if (retList.isEmpty()) {
        const QList<QByteArray> suffixes = QImageReader::supportedImageFormats();
        for (const QByteArray &suffix : suffixes)
            retList.append("*." + QString::fromUtf8(suffix));
    }
    return retList;
}

const QStringList &Asset::supportedFragmentShaderSuffixes()
{
    static const QStringList retList {"*.frag", "*.glsl", "*.glslf", "*.fsh"};
    return retList;
}

const QStringList &Asset::supportedShaderSuffixes()
{
    static const QStringList retList {"*.frag", "*.vert",
                                      "*.glsl", "*.glslv", "*.glslf",
                                      "*.vsh", "*.fsh"};
    return retList;
}

const QStringList &Asset::supportedFontSuffixes()
{
    static const QStringList retList {"*.ttf", "*.otf"};
    return retList;
}

const QStringList &Asset::supportedAudioSuffixes()
{
    static const QStringList retList {"*.wav", "*.mp3"};
    return retList;
}

const QStringList &Asset::supportedVideoSuffixes()
{
    static const QStringList retList {"*.mp4"};
    return retList;
}

const QStringList &Asset::supportedTexture3DSuffixes()
{
    // These are file types only supported by 3D textures
    static QStringList retList {"*.hdr", "*.ktx"};
    return retList;
}

const QStringList &Asset::supportedEffectMakerSuffixes()
{
    // These are file types only supported by Effect Maker
    static QStringList retList {"*.qep"};
    return retList;
}

const QSet<QString> &Asset::supportedSuffixes()
{
    static QSet<QString> allSuffixes;
    if (allSuffixes.isEmpty()) {
        auto insertSuffixes = [](const QStringList &suffixes) {
            for (const auto &suffix : suffixes)
                allSuffixes.insert(suffix);
        };
        insertSuffixes(supportedImageSuffixes());
        insertSuffixes(supportedShaderSuffixes());
        insertSuffixes(supportedFontSuffixes());
        insertSuffixes(supportedAudioSuffixes());
        insertSuffixes(supportedVideoSuffixes());
        insertSuffixes(supportedTexture3DSuffixes());
        insertSuffixes(supportedEffectMakerSuffixes());
    }
    return allSuffixes;
}

Asset::Type Asset::type() const
{
    if (supportedImageSuffixes().contains(m_suffix))
        return Asset::Type::Image;

    if (supportedFragmentShaderSuffixes().contains(m_suffix))
        return Asset::Type::FragmentShader;

    if (supportedShaderSuffixes().contains(m_suffix))
        return Asset::Type::Shader;

    if (supportedFontSuffixes().contains(m_suffix))
        return Asset::Type::Font;

    if (supportedAudioSuffixes().contains(m_suffix))
        return Asset::Type::Audio;

    if (supportedVideoSuffixes().contains(m_suffix))
        return Asset::Type::Video;

    if (supportedTexture3DSuffixes().contains(m_suffix))
        return Asset::Type::Texture3D;

    if (supportedEffectMakerSuffixes().contains(m_suffix))
        return Asset::Type::Effect;

    return Asset::Type::Unknown;
}

bool Asset::isImage() const
{
    return type() == Asset::Type::Image;
}

bool Asset::isFragmentShader() const
{
    return type() == Asset::Type::FragmentShader;
}

bool Asset::isShader() const
{
    return type() == Asset::Type::Shader;
}

bool Asset::isFont() const
{
    return type() == Asset::Type::Font;
}

bool Asset::isAudio() const
{
    return type() == Asset::Type::Audio;
}

bool Asset::isVideo() const
{
    return type() == Asset::Type::Video;
}

bool Asset::isTexture3D() const
{
    return type() == Asset::Type::Texture3D;
}

bool Asset::isEffect() const
{
    return type() == Asset::Type::Effect;
}

const QString Asset::suffix() const
{
    return m_suffix;
}

const QString Asset::id() const
{
    return m_filePath;
}

bool Asset::isSupported() const
{
    return supportedSuffixes().contains(m_filePath);
}

bool Asset::hasSuffix() const
{
    return !m_suffix.isEmpty();
}

} // namespace QmlDesigner
