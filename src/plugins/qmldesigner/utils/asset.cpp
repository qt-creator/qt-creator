// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QImageReader>

#include "asset.h"

namespace QmlDesigner {

Asset::Asset(const QString &filePath)
    : m_filePath(filePath)
{
    const QStringList split = filePath.split('.');
    if (split.size() > 1)
        m_suffix = "*." + split.last().toLower();

    resolveType();
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

const QStringList &Asset::supportedVertexShaderSuffixes()
{
    static const QStringList retList {"*.vert", "*.glsl", "*.glslv", "*.vsh"};
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

bool Asset::isSupported(const QString &path)
{
    return supportedSuffixes().contains(path);
}

Asset::Type Asset::type() const
{
    return m_type;
}

bool Asset::isImage() const
{
    return m_type == Asset::Type::Image;
}

bool Asset::isFragmentShader() const
{
    return m_type == Asset::Type::FragmentShader;
}

bool Asset::isVertexShader() const
{
    return m_type == Asset::Type::VertexShader;
}

bool Asset::isShader() const
{
    return isFragmentShader() || isVertexShader();
}

bool Asset::isFont() const
{
    return m_type == Asset::Type::Font;
}

bool Asset::isAudio() const
{
    return m_type == Asset::Type::Audio;
}

bool Asset::isVideo() const
{
    return m_type == Asset::Type::Video;
}

bool Asset::isTexture3D() const
{
    return m_type == Asset::Type::Texture3D;
}

bool Asset::isHdrFile() const
{
    return m_suffix == "*.hdr";
}

bool Asset::isKtxFile() const
{
    return m_suffix == "*.ktx";
}

bool Asset::isEffect() const
{
    return m_type == Asset::Type::Effect;
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
    return m_type != Asset::Type::Unknown;
}

bool Asset::isValidTextureSource()
{
    return isImage() || isTexture3D();
}

void Asset::resolveType()
{
    if (m_suffix.isEmpty())
        return;

    if (supportedImageSuffixes().contains(m_suffix))
        m_type = Asset::Type::Image;
    else if (supportedFragmentShaderSuffixes().contains(m_suffix))
        m_type = Asset::Type::FragmentShader;
    else if (supportedVertexShaderSuffixes().contains(m_suffix))
        m_type = Asset::Type::VertexShader;
    else if (supportedFontSuffixes().contains(m_suffix))
        m_type = Asset::Type::Font;
    else if (supportedAudioSuffixes().contains(m_suffix))
        m_type = Asset::Type::Audio;
    else if (supportedVideoSuffixes().contains(m_suffix))
        m_type = Asset::Type::Video;
    else if (supportedTexture3DSuffixes().contains(m_suffix))
        m_type = Asset::Type::Texture3D;
    else if (supportedEffectMakerSuffixes().contains(m_suffix))
        m_type = Asset::Type::Effect;
}

bool Asset::hasSuffix() const
{
    return !m_suffix.isEmpty();
}

} // namespace QmlDesigner
