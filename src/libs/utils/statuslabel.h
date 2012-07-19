/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef UTILS_STATUSLABEL_H
#define UTILS_STATUSLABEL_H

#include "utils_global.h"

QT_FORWARD_DECLARE_CLASS(QTimer)

#include <QLabel>

namespace Utils {

class QTCREATOR_UTILS_EXPORT StatusLabel : public QLabel
{
    Q_OBJECT
public:
    explicit StatusLabel(QWidget *parent = 0);

public slots:
    void showStatusMessage(const QString &message, int timeoutMS = 5000);
    void clearStatusMessage();

private slots:
    void slotTimeout();

private:
    void stopTimer();

    QTimer *m_timer;
    QString m_lastPermanentStatusMessage;
};

} // namespace Utils

#endif // UTILS_STATUSLABEL_H
