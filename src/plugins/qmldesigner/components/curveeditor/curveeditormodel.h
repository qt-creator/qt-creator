/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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

#pragma once

#include "detail/treemodel.h"

#include <vector>

QT_BEGIN_NAMESPACE
class QPointF;
QT_END_NAMESPACE

namespace DesignTools {

struct CurveEditorStyle;

class AnimationCurve;
class PropertyTreeItem;
class TreeItem;

class CurveEditorModel : public TreeModel
{
    Q_OBJECT

signals:
    void currentFrameChanged(int frame);

    void startFrameChanged(int frame);

    void endFrameChanged(int frame);

    void updateStartFrame(int frame);

    void updateEndFrame(int frame);

    void curveChanged(PropertyTreeItem *item);

public:
    virtual double minimumTime() const = 0;

    virtual double maximumTime() const = 0;

    virtual CurveEditorStyle style() const = 0;

public:
    CurveEditorModel(double minTime, double maxTime, QObject *parent = nullptr);

    ~CurveEditorModel() override;

    void setCurrentFrame(int frame);

    void setMinimumTime(double time, bool internal);

    void setMaximumTime(double time, bool internal);

    void setCurve(unsigned int id, const AnimationCurve &curve);

    void reset(const std::vector<TreeItem *> &items);

protected:
    double m_minTime = 0.;

    double m_maxTime = 0.;
};

} // End namespace DesignTools.
