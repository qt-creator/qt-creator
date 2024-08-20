// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignerutils_global.h"

#include <QSize>
#include <QString>

QT_FORWARD_DECLARE_CLASS(QPixmap)

namespace QmlDesigner {

class QMLDESIGNERUTILS_EXPORT Asset
{
public:
    enum Type { Unknown,
                Image,
                MissingImage,
                FragmentShader,
                VertexShader,
                Font,
                Audio,
                Video,
                Texture3D,
                Effect };

    Asset(const QString &filePath);

    static const QStringList &supportedImageSuffixes();
    static const QStringList &supportedFragmentShaderSuffixes();
    static const QStringList &supportedVertexShaderSuffixes();
    static const QStringList &supportedShaderSuffixes();
    static const QStringList &supportedFontSuffixes();
    static const QStringList &supportedAudioSuffixes();
    static const QStringList &supportedVideoSuffixes();
    static const QStringList &supportedTexture3DSuffixes();
    static const QStringList &supportedEffectComposerSuffixes();
    static const QSet<QString> &supportedSuffixes();
    static bool isSupported(const QString &path);

    const QString suffix() const;
    const QString id() const;
    const QString fileName() const;
    bool hasSuffix() const;
    QPixmap pixmap(const QSize &size = {}) const;

    Type type() const;
    bool isImage() const;
    bool isFragmentShader() const;
    bool isVertexShader() const;
    bool isShader() const;
    bool isFont() const;
    bool isAudio() const;
    bool isVideo() const;
    bool isTexture3D() const;
    bool isHdrFile() const;
    bool isKtxFile() const;
    bool isEffect() const;
    bool isSupported() const;
    bool isValidTextureSource();

private:
    void resolveType();

    QString m_filePath;
    QString m_fileName;
    QString m_suffix;
    Type m_type = Unknown;
};

} // namespace QmlDesigner
