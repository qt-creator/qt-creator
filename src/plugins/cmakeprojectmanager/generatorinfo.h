/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef CMAKEPROJECTMANAGER_INTERNAL_GENERATORINFO_H
#define CMAKEPROJECTMANAGER_INTERNAL_GENERATORINFO_H

#include "cmakeprojectmanager.h"

#include <projectexplorer/kit.h>

#include <QCoreApplication>
#include <QMetaType>

namespace CMakeProjectManager {
namespace Internal {

class GeneratorInfo
{
    Q_DECLARE_TR_FUNCTIONS(CMakeProjectManager::Internal::GeneratorInfo)
public:
    enum Ninja { NoNinja, OfferNinja, ForceNinja };
    static QList<GeneratorInfo> generatorInfosFor(ProjectExplorer::Kit *k, Ninja n, bool preferNinja, bool hasCodeBlocks);

    GeneratorInfo();
    explicit GeneratorInfo(ProjectExplorer::Kit *kit, bool ninja = false);

    ProjectExplorer::Kit *kit() const;
    bool isNinja() const;

    QString displayName() const;
    QByteArray generatorArgument() const;
    QByteArray generator() const;

private:
    ProjectExplorer::Kit *m_kit;
    bool m_isNinja;
};

} // namespace Internal
} // namespace CMakeProjectManager

Q_DECLARE_METATYPE(CMakeProjectManager::Internal::GeneratorInfo)

#endif // CMAKEPROJECTMANAGER_INTERNAL_GENERATORINFO_H
