// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "codegensettings.h"

#include <coreplugin/icore.h>
#include <utils/qtcsettings.h>

#include <QSettings>

using namespace Utils;

namespace QtSupport {

const char CODE_GEN_GROUP[] = "FormClassWizardPage";
const char TRANSLATION_KEY[] = "RetranslationSupport";
const char EMBEDDING_KEY[] = "Embedding";
const char INCLUDE_QT_MODULE_KEY[] = "IncludeQtModule";
const char ADD_QT_VERSION_CHECK_KEY[] = "AddQtVersionCheck";

const bool retranslationSupportDefault = false;
const CodeGenSettings::UiClassEmbedding embeddingDefault
    = CodeGenSettings::PointerAggregatedUiClass;
const bool includeQtModuleDefault = false;
const bool addQtVersionCheckDefault = false;

CodeGenSettings::CodeGenSettings()
    : embedding(embeddingDefault)
    , retranslationSupport(retranslationSupportDefault)
    , includeQtModule(includeQtModuleDefault)
    , addQtVersionCheck(addQtVersionCheckDefault)
{
}

bool CodeGenSettings::equals(const CodeGenSettings &rhs) const
{
    return embedding == rhs.embedding
            && retranslationSupport == rhs.retranslationSupport
            && includeQtModule == rhs.includeQtModule
            && addQtVersionCheck == rhs.addQtVersionCheck;
}

void CodeGenSettings::fromSettings(const QSettings *settings)
{
    QString group = QLatin1String(CODE_GEN_GROUP) + '/';

    retranslationSupport = settings->value(group + TRANSLATION_KEY, retranslationSupportDefault)
                               .toBool();
    embedding = static_cast<UiClassEmbedding>(
        settings->value(group + EMBEDDING_KEY, int(embeddingDefault)).toInt());
    includeQtModule = settings->value(group + INCLUDE_QT_MODULE_KEY, includeQtModuleDefault).toBool();
    addQtVersionCheck = settings->value(group + ADD_QT_VERSION_CHECK_KEY, addQtVersionCheckDefault).toBool();
}

void CodeGenSettings::toSettings(QSettings *settings) const
{
    settings->beginGroup(CODE_GEN_GROUP);
    QtcSettings::setValueWithDefault(settings,
                                     TRANSLATION_KEY,
                                     retranslationSupport,
                                     retranslationSupportDefault);
    QtcSettings::setValueWithDefault(settings, EMBEDDING_KEY, int(embedding), int(embeddingDefault));
    QtcSettings::setValueWithDefault(settings,
                                     INCLUDE_QT_MODULE_KEY,
                                     includeQtModule,
                                     includeQtModuleDefault);
    QtcSettings::setValueWithDefault(settings,
                                     ADD_QT_VERSION_CHECK_KEY,
                                     addQtVersionCheck,
                                     addQtVersionCheckDefault);
    settings->endGroup();

}

} // namespace QtSupport
