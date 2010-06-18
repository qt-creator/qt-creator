/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (c) 2010 Denis Mingulov.
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

#ifndef IMAGEVIEWERACTIONHANDLER_H
#define IMAGEVIEWERACTIONHANDLER_H

#include <QObject>
#include <QtCore/QScopedPointer>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QKeySequence)

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
                               const QList<int> &context, const QKeySequence &key);

private:
    QScopedPointer<struct ImageViewerActionHandlerPrivate> d_ptr;
};

} // namespace Internal
} // namespace ImageViewer


#endif // IMAGEVIEWERACTIONHANDLER_H
