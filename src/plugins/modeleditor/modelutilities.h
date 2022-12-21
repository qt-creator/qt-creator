// Copyright (C) 2018 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace qmt {
class MObject;
class MPackage;
}

namespace ModelEditor {
namespace Internal {

class ModelUtilities : public QObject
{
    Q_OBJECT
public:
    explicit ModelUtilities(QObject *parent = nullptr);
    ~ModelUtilities();

    bool haveDependency(const qmt::MObject *source, const qmt::MObject *target,
                        bool inverted = false);
    bool haveDependency(const qmt::MObject *source, const QList<qmt::MPackage *> &targets);
};

} // namespace Internal
} // namespace ModelEditor
