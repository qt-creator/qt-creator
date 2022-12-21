// Copyright (C) 2018 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace qmt {
class MObject;
class ModelController;
}

namespace ModelEditor {
namespace Internal {

class ModelUtilities;

class PackageViewController : public QObject
{
    Q_OBJECT
    class PackageViewControllerPrivate;

public:
    explicit PackageViewController(QObject *parent = nullptr);
    ~PackageViewController();

    void setModelController(qmt::ModelController *modelController);
    void setModelUtilities(ModelUtilities *modelUtilities);

    void createAncestorDependencies(qmt::MObject *object1, qmt::MObject *object2);

private:
    bool haveMatchingStereotypes(const qmt::MObject *object1, const qmt::MObject *object2);

    PackageViewControllerPrivate *d;
};

} // namespace Internal
} // namespace ModelEditor
