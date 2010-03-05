/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef DESIGNER_EDITORWIDGET_H
#define DESIGNER_EDITORWIDGET_H

#include "designerconstants.h"

#include <utils/fancymainwindow.h>

#include <QtCore/QPointer>
#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QVariant>
#include <QtCore/QSettings>
#include <QtGui/QWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QDockWidget>
#include <QtGui/QHideEvent>

namespace Designer {
namespace Internal {

/* A widget that shares its embedded sub window with others. For example,
 * the designer editors need to share the widget box, etc. */
class SharedSubWindow : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(SharedSubWindow)

public:
    SharedSubWindow(QWidget *shared, QWidget *parent = 0);
    virtual ~SharedSubWindow();

public slots:
    // Takes the shared widget off the current parent and adds it to its
    // layout
    void activate();

private:
    QPointer <QWidget> m_shared;
    QVBoxLayout *m_layout;
};

/* Form editor splitter used as editor window. Contains the shared designer
 * windows. */
class EditorWidget : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(EditorWidget)
public:
    explicit EditorWidget(QWidget *formWindow);


    void resetToDefaultLayout();
    QDockWidget* const* dockWidgets() const { return m_designerDockWidgets; }
    bool isLocked() const { return m_mainWindow->isLocked(); }
    void setLocked(bool locked) { m_mainWindow->setLocked(locked); }

    static void saveState(QSettings *settings);
    static void restoreState(QSettings *settings);

public slots:
    void activate();
    void toolChanged(int);

protected:
    void hideEvent(QHideEvent * e);

private:
    SharedSubWindow* m_designerSubWindows[Designer::Constants::DesignerSubWindowCount];
    QDockWidget *m_designerDockWidgets[Designer::Constants::DesignerSubWindowCount];
    Utils::FancyMainWindow *m_mainWindow;
    bool m_initialized;

    static QHash<QString, QVariant> m_globalState;
};

} // namespace Internal
} // namespace Designer

#endif // DESIGNER_EDITORWIDGET_H
