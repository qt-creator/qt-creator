// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "configcontroller.h"

#include "stereotypedefinitionparser.h"
#include "textscanner.h"
#include "stringtextsource.h"

#include "qmt/stereotype/stereotypecontroller.h"

#include <QDebug>
#include <QFile>

namespace qmt {

class ConfigController::ConfigControllerPrivate
{
public:
    StereotypeController *m_stereotypeController = nullptr;
};

ConfigController::ConfigController(QObject *parent)
    : QObject(parent)
    , d(new ConfigControllerPrivate)
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

void ConfigController::readStereotypeDefinitions(const Utils::FilePath &path)
{
    if (path.isEmpty()) {
        // TODO add error handling
        return;
    }

    StereotypeDefinitionParser parser;
    connect(&parser, &StereotypeDefinitionParser::iconParsed,
            this, &ConfigController::onStereotypeIconParsed);
    connect(&parser, &StereotypeDefinitionParser::relationParsed,
            this, &ConfigController::onRelationParsed);
    connect(&parser, &StereotypeDefinitionParser::toolbarParsed,
            this, &ConfigController::onToolbarParsed);

    Utils::FilePaths paths;
    if (path.isFile()) {
        paths.append(path);
    } else if (path.isDir()) {
        paths = path.dirEntries({ { "*.def" } });
    } else {
        // TODO add error handling
        return;
    }
    for (const auto &filePath : std::as_const(paths)) {
        QFile file(filePath.toString());
        if (file.open(QIODevice::ReadOnly)) {
            QString text = QString::fromUtf8(file.readAll());
            file.close();
            StringTextSource source;
            source.setSourceId(1);
            source.setText(text);
            try {
                parser.parse(&source);
            } catch (const StereotypeDefinitionParserError &x) {
                // TODO add error handling
                qDebug() << x.errorMessage() << "in line" << x.sourcePos().lineNumber();
            } catch (const TextScannerError &x) {
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

void ConfigController::onRelationParsed(const CustomRelation &customRelation)
{
    d->m_stereotypeController->addCustomRelation(customRelation);
}

void ConfigController::onToolbarParsed(const Toolbar &toolbar)
{
    d->m_stereotypeController->addToolbar(toolbar);
}

} // namespace qmt
