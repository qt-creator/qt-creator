/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPTOOLSJSEXTENSION_H
#define CPPTOOLSJSEXTENSION_H

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

#endif // CPPTOOLSJSEXTENSION_H
