// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include "qmt/infrastructure/qmt_global.h"

namespace qmt {

class CustomRelation;
class StereotypeController;
class StereotypeIcon;
class Toolbar;

class QMT_EXPORT ConfigController : public QObject
{
    Q_OBJECT
    class ConfigControllerPrivate;

public:
    explicit ConfigController(QObject *parent = nullptr);
    ~ConfigController() override;

    void setStereotypeController(StereotypeController *stereotypeController);

    void readStereotypeDefinitions(const QString &path);

private:
    void onStereotypeIconParsed(const StereotypeIcon &stereotypeIcon);
    void onRelationParsed(const CustomRelation &customRelation);
    void onToolbarParsed(const Toolbar &toolbar);

    ConfigControllerPrivate *d;
};

} // namespace qmt
