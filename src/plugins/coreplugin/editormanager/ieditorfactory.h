/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <coreplugin/core_global.h>
#include <coreplugin/id.h>

#include <QObject>
#include <QStringList>

namespace Core {

class IEditor;

class CORE_EXPORT IEditorFactory : public QObject
{
    Q_OBJECT

public:
    IEditorFactory(QObject *parent = 0);
    QString displayName() const { return m_displayName; }
    void setDisplayName(const QString &displayName) { m_displayName = displayName; }

    Id id() const { return m_id; }
    void setId(Id id) { m_id = id; }

    virtual IEditor *createEditor() = 0;

    QStringList mimeTypes() const { return m_mimeTypes; }
    void setMimeTypes(const QStringList &mimeTypes) { m_mimeTypes = mimeTypes; }
    void addMimeType(const char *mimeType) { m_mimeTypes.append(QLatin1String(mimeType)); }
    void addMimeType(const QString &mimeType) { m_mimeTypes.append(mimeType); }
private:
    Id m_id;
    QString m_displayName;
    QStringList m_mimeTypes;
};

} // namespace Core
