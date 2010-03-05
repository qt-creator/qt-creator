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

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QObject>
#include <QtCore/QCoreApplication>

#include "xuifiledialog.h"

namespace QmlDesigner {

void XUIFileDialog::runOpenFileDialog(const QString& path, QWidget* parent, QObject* receiver, const char* member)
{
    QString dir = path;
    if (dir.isNull())
        dir = XUIFileDialog::defaultFolder();

    QString caption = QCoreApplication::translate("QmlDesigner::XUIFileDialog", "Open file");
    QString fileName = QFileDialog::getOpenFileName(parent, caption, dir, XUIFileDialog::fileNameFilters().join(";;"), 0, QFileDialog::ReadOnly);

    QmlDesigner::Internal::SignalEmitter emitter;
    QObject::connect(&emitter, SIGNAL(fileNameSelected(QString)), receiver, member);
    emitter.emitFileNameSelected(fileName);
}

void XUIFileDialog::runSaveFileDialog(const QString& path, QWidget* parent, QObject* receiver, const char* member)
{
    QString dir = path;
    if (dir.isNull())
        dir = XUIFileDialog::defaultFolder();

#ifdef Q_WS_MAC
    QFileDialog *dialog = new QFileDialog(parent, Qt::Sheet);
    dialog->setFileMode(QFileDialog::AnyFile);
    dialog->setAcceptMode(QFileDialog::AcceptSave);
    dialog->setNameFilters(XUIFileDialog::fileNameFilters());
    dialog->setDirectory(dir);
    dialog->open(receiver, member);
#else // !Q_WS_MAC
    QString caption = QCoreApplication::translate("QmlDesigner::XUIFileDialog", "Save file");
    QString fileName = QFileDialog::getSaveFileName(parent, caption, dir, XUIFileDialog::fileNameFilters().join(";;"));

    QmlDesigner::Internal::SignalEmitter emitter;
    QObject::connect(&emitter, SIGNAL(fileNameSelected(QString)), receiver, member);
    emitter.emitFileNameSelected(fileName);
#endif // Q_WS_MAC
}

QStringList XUIFileDialog::fileNameFilters()
{
    QStringList filters;

    filters
            << QCoreApplication::translate("QmlDesigner::XUIFileDialog", "Declarative UI files (*.qml)")
            << QCoreApplication::translate("QmlDesigner::XUIFileDialog", "All files (*)");

    return filters;
}

QString XUIFileDialog::defaultFolder()
{
    return QDir::currentPath();
}

} // namespace QmlDesigner
