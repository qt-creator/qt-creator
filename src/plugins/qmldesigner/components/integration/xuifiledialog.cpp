/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "xuifiledialog.h"
#include <utils/hostosinfo.h>

#include <QDebug>
#include <QDir>
#include <QObject>
#include <QCoreApplication>

namespace QmlDesigner {

void XUIFileDialog::runOpenFileDialog(const QString& path, QWidget* parent, QObject* receiver, const char* member)
{
    QString dir = path;
    if (dir.isNull())
        dir = XUIFileDialog::defaultFolder();

    QString caption = QCoreApplication::translate("QmlDesigner::XUIFileDialog", "Open File");
    QString fileName = QFileDialog::getOpenFileName(parent, caption, dir, XUIFileDialog::fileNameFilters().join(QLatin1String(";;")), 0, QFileDialog::ReadOnly);

    QmlDesigner::Internal::SignalEmitter emitter;
    QObject::connect(&emitter, SIGNAL(fileNameSelected(QString)), receiver, member);
    emitter.emitFileNameSelected(fileName);
}

void XUIFileDialog::runSaveFileDialog(const QString& path, QWidget* parent, QObject* receiver, const char* member)
{
    QString dir = path;
    if (dir.isNull())
        dir = XUIFileDialog::defaultFolder();

    if (Utils::HostOsInfo::isMacHost()) {
        QFileDialog *dialog = new QFileDialog(parent, Qt::Sheet);
        dialog->setFileMode(QFileDialog::AnyFile);
        dialog->setAcceptMode(QFileDialog::AcceptSave);
        dialog->setNameFilters(XUIFileDialog::fileNameFilters());
        dialog->setDirectory(dir);
        dialog->open(receiver, member);
    } else {
        QString caption = QCoreApplication::translate("QmlDesigner::XUIFileDialog", "Save File");
        QString fileName = QFileDialog::getSaveFileName(parent, caption, dir, XUIFileDialog::fileNameFilters().join(QLatin1String(";;")));

        QmlDesigner::Internal::SignalEmitter emitter;
        QObject::connect(&emitter, SIGNAL(fileNameSelected(QString)), receiver, member);
        emitter.emitFileNameSelected(fileName);
    }
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
