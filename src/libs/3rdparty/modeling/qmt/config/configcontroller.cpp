/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

struct ConfigController::ConfigControllerPrivate
{
    ConfigControllerPrivate()
        : m_stereotypeController(0)
    {
    }

    StereotypeController *m_stereotypeController;
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
    StereotypeDefinitionParser parser;
    connect(&parser, SIGNAL(iconParsed(StereotypeIcon)), this, SLOT(onStereotypeIconParsed(StereotypeIcon)));
    connect(&parser, SIGNAL(toolbarParsed(Toolbar)), this, SLOT(onToolbarParsed(Toolbar)));

    QDir dir(path);
    dir.setNameFilters(QStringList() << QStringLiteral("*.def"));
    foreach (const QString &fileName, dir.entryList(QDir::Files)) {
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
                qDebug() << x.getErrorMsg() << "in line" << x.getSourcePos().getLineNumber();
            } catch (TextScannerError x) {
                qDebug() << x.getErrorMsg() << "in line" << x.getSourcePos().getLineNumber();
            } catch (...) {
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
