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

#ifndef XUIFILEDIALOG_H
#define XUIFILEDIALOG_H

#include <QString>
#include <QStringList>

#include <QFileDialog>

namespace QmlDesigner {

class XUIFileDialog
{
public:
    static void runOpenFileDialog(const QString& path, QWidget* parent, QObject* receiver, const char* member);
    static void runSaveFileDialog(const QString& path, QWidget* parent, QObject* receiver, const char* member);

    static QStringList fileNameFilters();
    static QString defaultFolder();
};

namespace Internal {
class SignalEmitter: public QObject
{
    Q_OBJECT

public:
    void emitFileNameSelected(const QString& fileName) { emit fileNameSelected(fileName); }

signals:
    void fileNameSelected(const QString&);
};

} // namespace Internal
} // namespace QmlDesigner

#endif // XUIFILEDIALOG_H
