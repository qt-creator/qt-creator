/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

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
