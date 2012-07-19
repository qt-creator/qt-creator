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

#ifndef INFOBAR_H
#define INFOBAR_H

#include "core_global.h"

#include <QObject>

QT_BEGIN_NAMESPACE
class QBoxLayout;
QT_END_NAMESPACE

namespace Core {

class InfoBar;
class InfoBarDisplay;

class CORE_EXPORT InfoBarEntry
{
public:
    InfoBarEntry(const QString &_id, const QString &_infoText);
    InfoBarEntry(const InfoBarEntry &other) { *this = other; }
    void setCustomButtonInfo(const QString &_buttonText, QObject *_object, const char *_member);
    void setCancelButtonInfo(QObject *_object, const char *_member);
    void setCancelButtonInfo(const QString &_cancelButtonText, QObject *_object, const char *_member);

private:
    QString id;
    QString infoText;
    QString buttonText;
    QObject *object;
    const char *buttonPressMember;
    QString cancelButtonText;
    QObject *cancelObject;
    const char *cancelButtonPressMember;
    friend class InfoBar;
    friend class InfoBarDisplay;
};

class CORE_EXPORT InfoBar : public QObject
{
    Q_OBJECT

public:
    void addInfo(const InfoBarEntry &info);
    void removeInfo(const QString &id);
    bool containsInfo(const QString &id) const;
    void clear();

signals:
    void changed();

private:
    QList<InfoBarEntry> m_infoBarEntries;
    friend class InfoBarDisplay;
};

class CORE_EXPORT InfoBarDisplay : public QObject
{
    Q_OBJECT

public:
    InfoBarDisplay(QObject *parent = 0);
    void setTarget(QBoxLayout *layout, int index);
    void setInfoBar(InfoBar *infoBar);

private slots:
    void cancelButtonClicked();
    void update();
    void infoBarDestroyed();
    void widgetDestroyed();

private:
    QList<QWidget *> m_infoWidgets;
    InfoBar *m_infoBar;
    QBoxLayout *m_boxLayout;
    int m_boxIndex;
};

} // namespace Core

#endif // INFOBAR_H
