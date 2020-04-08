/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>

#include <designersupportdelegate.h>

QT_BEGIN_NAMESPACE
class QQuickView;
class QQuickItem;
QT_END_NAMESPACE

class IconRenderer : public QObject
{
    Q_OBJECT

public:
    explicit IconRenderer(int size, const QString &filePath, const QString &source);

    void setupRender();

private:
    void render(const QString &fileName);

    int m_size = 16;
    QString m_filePath;
    QString m_source;
    QQuickView *m_quickView = nullptr;
    QQuickItem *m_contentItem = nullptr;
    DesignerSupport m_designerSupport;
};
