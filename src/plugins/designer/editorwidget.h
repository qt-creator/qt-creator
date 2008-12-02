/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#ifndef DESIGNER_EDITORWIDGET_H
#define DESIGNER_EDITORWIDGET_H

#include "designerconstants.h"

#include <coreplugin/minisplitter.h>

#include <QtCore/QPointer>
#include <QtCore/QList>
#include <QtCore/QVariant>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QTabWidget;
class QVBoxLayout;
QT_END_NAMESPACE

namespace Designer {
namespace Internal {

    /* A widget that shares its embedded sub window with others. For example,
     * the designer editors need to share the widget box, etc. */
    class SharedSubWindow : public QWidget {
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

    /** State of the editor window (splitter sizes)
      * Shared as a global struct between the instances and stored
      * in QSettings. */
    struct EditorWidgetState {
        QVariant toVariant() const; // API to conveniently store in QSettings
        bool fromVariant(const QVariant &v);

        QList<int> horizontalSizes;
        QList<int> centerVerticalSizes;
        QList<int> rightVerticalSizes;
    };

    /* Form editor splitter used as editor window. Contains the shared designer
     * windows. */
    class EditorWidget : public Core::MiniSplitter {
        Q_OBJECT
        Q_DISABLE_COPY(EditorWidget)
    public:
        explicit EditorWidget(QWidget *formWindow);

        virtual bool event(QEvent * e);

        EditorWidgetState save() const;
        bool restore(const EditorWidgetState &s);

        // Get/Set the shared splitter state of all editors of that type for
        // settings
        static EditorWidgetState state();
        static void setState(const EditorWidgetState&st);

    public slots:
        void activate();
        void toolChanged(int);

    private:
        void setInitialSizes();

        SharedSubWindow* m_designerSubWindows[Designer::Constants::DesignerSubWindowCount];
        QSplitter *m_centerVertSplitter;
        QTabWidget *m_bottomTab;
        QSplitter *m_rightVertSplitter;
    };
}
}

#endif
