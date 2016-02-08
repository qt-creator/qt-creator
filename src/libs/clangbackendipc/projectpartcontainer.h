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

#ifndef CLANGBACKEND_PROJECTCONTAINER_H
#define CLANGBACKEND_PROJECTCONTAINER_H

#include <clangbackendipc_global.h>

#include <utf8stringvector.h>

namespace ClangBackEnd {

class CMBIPC_EXPORT ProjectPartContainer
{
    friend CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const ProjectPartContainer &container);
    friend CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, ProjectPartContainer &container);
    friend CMBIPC_EXPORT bool operator==(const ProjectPartContainer &first, const ProjectPartContainer &second);
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
CMBIPC_EXPORT bool operator==(const ProjectPartContainer &first, const ProjectPartContainer &second);

QDebug operator<<(QDebug debug, const ProjectPartContainer &container);
void PrintTo(const ProjectPartContainer &container, ::std::ostream* os);

} // namespace ClangBackEnd

#endif // CLANGBACKEND_PROJECTCONTAINER_H
