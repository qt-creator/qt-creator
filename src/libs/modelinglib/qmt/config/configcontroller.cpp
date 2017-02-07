/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "configcontroller.h"

#include "stereotypedefinitionparser.h"
#include "textscanner.h"
#include "stringtextsource.h"

#include "qmt/stereotype/stereotypecontroller.h"

#include <QDir>
#include <QFileInfo>
#include <QFile>

#include <QDebug>

namespace qmt {

class ConfigController::ConfigControllerPrivate
{
public:
    StereotypeController *m_stereotypeController = 0;
};

ConfigController::ConfigController(QObject *parent)
    : QObject(parent),
      d(new ConfigControllerPrivate)
{
}

ConfigController::~ConfigController()
{
    delete d;
}

void ConfigController::setStereotypeController(StereotypeController *stereotypeController)
{
    d->m_stereotypeController = stereotypeController;
}

void ConfigController::readStereotypeDefinitions(const QString &path)
{
    if (path.isEmpty()) {
        // TODO add error handling
        return;
    }

    StereotypeDefinitionParser parser;
    connect(&parser, &StereotypeDefinitionParser::iconParsed,
            this, &ConfigController::onStereotypeIconParsed);
    connect(&parser, &StereotypeDefinitionParser::toolbarParsed,
            this, &ConfigController::onToolbarParsed);

    QStringList fileNames;
    QDir dir;
    QFileInfo fileInfo(path);
    if (fileInfo.isFile()) {
        dir.setPath(fileInfo.path());
        fileNames.append(fileInfo.fileName());
    } else if (fileInfo.isDir()) {
        dir.setPath(path);
        dir.setNameFilters(QStringList("*.def"));
        fileNames = dir.entryList(QDir::Files);
    } else {
        // TODO add error handling
        return;
    }
    foreach (const QString &fileName, fileNames) {
        QFile file(QFileInfo(dir, fileName).absoluteFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            QString text = QString::fromUtf8(file.readAll());
            file.close();
            StringTextSource source;
            source.setSourceId(1);
            source.setText(text);
            try {
                parser.parse(&source);
            } catch (StereotypeDefinitionParserError x) {
                // TODO add error handling
                qDebug() << x.errorMessage() << "in line" << x.sourcePos().lineNumber();
            } catch (TextScannerError x) {
                // TODO add error handling
                qDebug() << x.errorMessage() << "in line" << x.sourcePos().lineNumber();
            } catch (...) {
                // TODO add error handling
            }
        }
    }
}

void ConfigController::onStereotypeIconParsed(const StereotypeIcon &stereotypeIcon)
{
    d->m_stereotypeController->addStereotypeIcon(stereotypeIcon);
}

void ConfigController::onToolbarParsed(const Toolbar &toolbar)
{
    d->m_stereotypeController->addToolbar(toolbar);
}

} // namespace qmt
