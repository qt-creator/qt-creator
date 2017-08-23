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

#include "fileutils.h"
#include "utils_global.h"

#include <QVariant>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT PersistentSettingsReader
{
public:
    PersistentSettingsReader();
    QVariant restoreValue(const QString &variable, const QVariant &defaultValue = QVariant()) const;
    QVariantMap restoreValues() const;
    bool load(const FileName &fileName);

private:
    QMap<QString, QVariant> m_valueMap;
};

class QTCREATOR_UTILS_EXPORT PersistentSettingsWriter
{
public:
    PersistentSettingsWriter(const FileName &fileName, const QString &docType);
    ~PersistentSettingsWriter();

    bool save(const QVariantMap &data, QString *errorString) const;
#ifdef QT_GUI_LIB
    bool save(const QVariantMap &data, QWidget *parent) const;
#endif

    FileName fileName() const;

private:
    bool write(const QVariantMap &data, QString *errorString) const;

    const FileName m_fileName;
    const QString m_docType;
    mutable QMap<QString, QVariant> m_savedData;
};

} // namespace Utils
