// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace QmlDesigner {

class Asset
{
public:
    enum Type { Unknown, Image, FragmentShader, Font, Audio, Video, Texture3D, Effect, Shader };

    Asset(const QString &filePath);

    static const QStringList &supportedImageSuffixes();
    static const QStringList &supportedFragmentShaderSuffixes();
    static const QStringList &supportedShaderSuffixes();
    static const QStringList &supportedFontSuffixes();
    static const QStringList &supportedAudioSuffixes();
    static const QStringList &supportedVideoSuffixes();
    static const QStringList &supportedTexture3DSuffixes();
    static const QStringList &supportedEffectMakerSuffixes();
    static const QSet<QString> &supportedSuffixes();

    const QString suffix() const;
    const QString id() const;
    bool hasSuffix() const;

    Type type() const;
    bool isImage() const;
    bool isFragmentShader() const;
    bool isShader() const;
    bool isFont() const;
    bool isAudio() const;
    bool isVideo() const;
    bool isTexture3D() const;
    bool isEffect() const;
    bool isSupported() const;

private:
    QString m_filePath;
    QString m_suffix;
};

} // namespace QmlDesigner
