// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/infrastructure/qmt_global.h"

#include <utils/filepath.h>

#include <QString>

QT_BEGIN_NAMESPACE
class QXmlStreamWriter;
QT_END_NAMESPACE

namespace qmt {

class Project;

class QMT_EXPORT ProjectSerializer
{
public:
    ProjectSerializer();
    ~ProjectSerializer();

    void save(const Utils::FilePath &fileName, const Project *project);
    QByteArray save(const Project *project);
    void load(const Utils::FilePath &fileName, Project *project);

private:
    void write(QXmlStreamWriter *writer, const Project *project);

};

} // namespace qmt
