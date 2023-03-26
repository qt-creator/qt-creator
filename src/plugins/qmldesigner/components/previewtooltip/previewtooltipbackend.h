// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <imagecacheauxiliarydata.h>

#include <QObject>
#include <QQmlEngine>

#include <memory>

namespace QmlDesigner {

class PreviewImageTooltip;
class AsynchronousImageCache;

class PreviewTooltipBackend : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(QString info READ info WRITE setInfo NOTIFY infoChanged)
    Q_PROPERTY(QString extraId READ extraId WRITE setExtraId NOTIFY extraIdChanged)
    Q_PROPERTY(bool scaleImage READ scaleImage WRITE setScaleImage NOTIFY scaleImageChanged)

public:
    PreviewTooltipBackend(AsynchronousImageCache &cache);
    ~PreviewTooltipBackend();

    Q_INVOKABLE void showTooltip();
    Q_INVOKABLE void hideTooltip();
    Q_INVOKABLE void reposition();

    QString name() const;
    void setName(const QString &name);
    QString path() const;
    void setPath(const QString &path);
    QString info() const;
    void setInfo(const QString &info);
    QString extraId() const;
    void setExtraId(const QString &extraId);
    bool scaleImage() const;
    void setScaleImage(bool scale);

    bool isVisible() const;

    void setAuxiliaryData(ImageCache::AuxiliaryData auxiliaryData)
    {
        m_auxiliaryData = std::move(auxiliaryData);
    }

signals:
    void nameChanged();
    void pathChanged();
    void infoChanged();
    void extraIdChanged();
    void scaleImageChanged();

private:
    QString m_name;
    QString m_path;
    QString m_info;
    QString m_extraId;
    bool m_scaleImage = true;
    std::unique_ptr<PreviewImageTooltip> m_tooltip;
    ImageCache::AuxiliaryData m_auxiliaryData;
    AsynchronousImageCache &m_cache;
};

}

QML_DECLARE_TYPE(QmlDesigner::PreviewTooltipBackend)
