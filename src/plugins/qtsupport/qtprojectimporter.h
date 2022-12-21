// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qtsupport_global.h"

#include <projectexplorer/projectimporter.h>

namespace QtSupport {

class QtVersion;

// Documentation inside.
class QTSUPPORT_EXPORT QtProjectImporter : public ProjectExplorer::ProjectImporter
{
public:
    QtProjectImporter(const Utils::FilePath &path);

    class QtVersionData
    {
    public:
        QtVersion *qt = nullptr;
        bool isTemporary = true;
    };

protected:
    QtVersionData findOrCreateQtVersion(const Utils::FilePath &qmakePath) const;
    ProjectExplorer::Kit *createTemporaryKit(const QtVersionData &versionData,
                                             const KitSetupFunction &setup) const;

private:
    void cleanupTemporaryQt(ProjectExplorer::Kit *k, const QVariantList &vl);
    void persistTemporaryQt(ProjectExplorer::Kit *k, const QVariantList &vl);
};

} // namespace QmakeProjectManager
