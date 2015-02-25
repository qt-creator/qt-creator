/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DIFFVIEW_H
#define DIFFVIEW_H

#include <coreplugin/id.h>

#include <QIcon>
#include <QString>
#include <QWidget>

namespace DiffEditor {
namespace Internal {

class DiffEditorGuiController;
class SideBySideDiffEditorWidget;
class UnifiedDiffEditorWidget;

const char SIDE_BY_SIDE_VIEW_ID[] = "SideBySide";
const char UNIFIED_VIEW_ID[] = "Unified";

class IDiffView
{
public:
    IDiffView() { }
    virtual ~IDiffView() { }

    QIcon icon() const;
    QString toolTip() const;

    Core::Id id() const;
    virtual QWidget *widget() = 0;
    virtual void setDiffEditorGuiController(DiffEditorGuiController *controller) = 0;

protected:
    void setIcon(const QIcon &icon);
    void setToolTip(const QString &toolTip);
    void setId(const Core::Id &id);

private:
    QIcon m_icon;
    QString m_toolTip;
    Core::Id m_id;
};

class UnifiedView : public IDiffView {
public:
    UnifiedView();

    QWidget *widget();
    void setDiffEditorGuiController(DiffEditorGuiController *controller);

private:
    UnifiedDiffEditorWidget *m_widget;
};

class SideBySideView : public IDiffView {
public:
    SideBySideView();

    QWidget *widget();
    void setDiffEditorGuiController(DiffEditorGuiController *controller);

private:
    SideBySideDiffEditorWidget *m_widget;
};

} // namespace Internal
} // namespace DiffEditor

#endif // DIFFVIEW_H
