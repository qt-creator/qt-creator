/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QObject>
#include <QQmlEngine>

#include <memory>

namespace QmlDesigner {

class PreviewImageTooltip;
class ImageCache;

class PreviewTooltipBackend : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString componentPath READ componentPath WRITE setComponentPath NOTIFY componentPathChanged)
    Q_PROPERTY(QString componentName READ componentName WRITE setComponentName NOTIFY componentNameChanged)

public:
    PreviewTooltipBackend(ImageCache &cache);
    ~PreviewTooltipBackend();

    Q_INVOKABLE void showTooltip();
    Q_INVOKABLE void hideTooltip();

    QString componentPath() const;
    void setComponentPath(const QString &path);

    QString componentName() const;
    void setComponentName(const QString &path);

signals:
    void componentPathChanged();
    void componentNameChanged();

private:
    QString m_componentPath;
    QString m_componentName;
    std::unique_ptr<PreviewImageTooltip> m_tooltip;
    ImageCache &m_cache;
};

}

QML_DECLARE_TYPE(QmlDesigner::PreviewTooltipBackend)
