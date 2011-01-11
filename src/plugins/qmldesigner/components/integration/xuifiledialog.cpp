/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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

    QString caption = QCoreApplication::translate("QmlDesigner::XUIFileDialog", "Open File");
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
    QString caption = QCoreApplication::translate("QmlDesigner::XUIFileDialog", "Save File");
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
