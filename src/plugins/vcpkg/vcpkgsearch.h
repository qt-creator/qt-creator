// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>
#include <utils/aspects.h>

#include <QUrl>

namespace Vcpkg::Internal::Search {

struct VcpkgManifest
{
    QString name;
    QString version;
    QString license;
    QStringList dependencies;
    QString shortDescription;
    QStringList description;
    QUrl homepage;
};

VcpkgManifest parseVcpkgManifest(const QByteArray &vcpkgManifestJsonData, bool *ok = nullptr);
VcpkgManifest showVcpkgPackageSearchDialog(const VcpkgManifest &projectManifest,
                                           QWidget *parent = nullptr);

} // namespace Vcpkg::Internal::Search
