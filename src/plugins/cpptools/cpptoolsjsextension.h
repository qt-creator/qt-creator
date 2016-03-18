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

#include <QObject>

#include <QStringList>

namespace CppTools {
namespace Internal {

/**
 * This class extends the JS features in our macro expander.
 */
class CppToolsJsExtension : public QObject
{
    Q_OBJECT

public:
    CppToolsJsExtension(QObject *parent = 0) : QObject(parent) { }

    // Generate header guard:
    Q_INVOKABLE QString headerGuard(const QString &in) const;

    // Fix the filename casing as configured in C++/File Naming:
    Q_INVOKABLE QString fileName(const QString &path, const QString &extension) const;

    // Work with classes:
    Q_INVOKABLE QStringList namespaces(const QString &klass) const;
    Q_INVOKABLE QString className(const QString &klass) const;
    Q_INVOKABLE QString classToFileName(const QString &klass,
                                        const QString &extension) const;
    Q_INVOKABLE QString classToHeaderGuard(const QString &klass, const QString &extension) const;
    Q_INVOKABLE QString openNamespaces(const QString &klass) const;
    Q_INVOKABLE QString closeNamespaces(const QString &klass) const;
};

} // namespace Internal
} // namespace CppTools
