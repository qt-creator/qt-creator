// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <utils/styledbar.h>
#include <seekerslider.h>

QT_BEGIN_NAMESPACE
class QToolBar;
QT_END_NAMESPACE

namespace Utils { class CrumblePath; }

namespace QmlDesigner {

class ToolBox : public Utils::StyledBar
{
public:
    ToolBox(SeekerSlider *seeker, QWidget *parentWidget);
    void setLeftSideActions(const QList<QAction*> &actions);
    void setRightSideActions(const QList<QAction*> &actions);
    void addLeftSideAction(QAction *action);
    void addRightSideAction(QAction *action);
    QList<QAction*> actions() const;
    SeekerSlider *seeker() const;

private:
    QToolBar *m_leftToolBar;
    QToolBar *m_rightToolBar;
    SeekerSlider *m_seeker;
};

} // namespace QmlDesigner
