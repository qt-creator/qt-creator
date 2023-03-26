// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QSizeF;
QT_END_NAMESPACE

namespace qmt {

class MDiagram;
class DElement;
class IMoveable;
class IResizable;

class ISceneInspector
{
public:
    virtual ~ISceneInspector() { }

    virtual QSizeF rasterSize() const = 0;
    virtual QSizeF minimalSize(const DElement *element, const MDiagram *diagram) const = 0;
    virtual IMoveable *moveable(const DElement *element, const MDiagram *diagram) const = 0;
    virtual IResizable *resizable(const DElement *element, const MDiagram *diagram) const = 0;
};

} // namespace qmt
