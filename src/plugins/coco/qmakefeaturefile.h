// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "modificationfile.h"

#include <utils/fileutils.h>

#include <QString>
#include <QStringList>

namespace Coco::Internal {

class QMakeFeatureFile : public ModificationFile
{
public:
    QMakeFeatureFile();

    void read();
    void write() const;

private:
    QString fromFileLine(const QString &line) const;
    QString toFileLine(const QString &option) const;
};

} // namespace Coco::Internal
