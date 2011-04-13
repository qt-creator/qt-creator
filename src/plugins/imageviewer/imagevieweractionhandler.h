/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (c) 2010 Denis Mingulov.
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef IMAGEVIEWERACTIONHANDLER_H
#define IMAGEVIEWERACTIONHANDLER_H

#include "coreplugin/icontext.h"

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>

QT_BEGIN_NAMESPACE
class QAction;
class QKeySequence;
QT_END_NAMESPACE

namespace ImageViewer {
namespace Internal {

class ImageViewerActionHandler : public QObject
{
    Q_OBJECT
public:
    explicit ImageViewerActionHandler(QObject *parent = 0);
    ~ImageViewerActionHandler();

    void createActions();

signals:

public slots:
    void actionTriggered(int supportedAction);

protected:
    /*!
      \brief Create a new action and register this action in the action manager.
      \param actionId Action's internal id
      \param id Command id
      \param title Action's title
      \param context Current context
      \param key Key sequence for the command
      \return Created and registered action, 0 if something goes wrong
     */
    QAction *registerNewAction(int actionId, const QString &id, const QString &title,
                               const Core::Context &context, const QKeySequence &key);

private:
    QScopedPointer<struct ImageViewerActionHandlerPrivate> d_ptr;
};

} // namespace Internal
} // namespace ImageViewer


#endif // IMAGEVIEWERACTIONHANDLER_H
