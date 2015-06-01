/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CODEMODELBACKEND_PROJECTCONTAINER_H
#define CODEMODELBACKEND_PROJECTCONTAINER_H

#include <QMetaType>

#include <utf8stringvector.h>

#include <codemodelbackendipc_global.h>

namespace CodeModelBackEnd {

class CMBIPC_EXPORT ProjectPartContainer
{
    friend CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const ProjectPartContainer &container);
    friend CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, ProjectPartContainer &container);
    friend CMBIPC_EXPORT bool operator == (const ProjectPartContainer &first, const ProjectPartContainer &second);
    friend CMBIPC_EXPORT bool operator < (const ProjectPartContainer &first, const ProjectPartContainer &second);
public:
    ProjectPartContainer() = default;
    ProjectPartContainer(const Utf8String &projectPartId,
                     const Utf8StringVector &arguments = Utf8StringVector());

    const Utf8String &projectPartId() const;
    const Utf8StringVector &arguments() const;

private:
    Utf8String projectPartId_;
    Utf8StringVector arguments_;
};

CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const ProjectPartContainer &container);
CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, ProjectPartContainer &container);
CMBIPC_EXPORT bool operator == (const ProjectPartContainer &first, const ProjectPartContainer &second);
CMBIPC_EXPORT bool operator < (const ProjectPartContainer &first, const ProjectPartContainer &second);

CMBIPC_EXPORT QDebug operator <<(QDebug debug, const ProjectPartContainer &container);
void PrintTo(const ProjectPartContainer &container, ::std::ostream* os);

} // namespace CodeModelBackEnd

Q_DECLARE_METATYPE(CodeModelBackEnd::ProjectPartContainer)

#endif // CODEMODELBACKEND_PROJECTCONTAINER_H
