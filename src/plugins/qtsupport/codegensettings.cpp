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

#include "codegensettings.h"

#include <coreplugin/icore.h>
#include <utils/qtcsettings.h>

#include <QSettings>

static const char CODE_GEN_GROUP[] = "FormClassWizardPage";
static const char TRANSLATION_KEY[] = "RetranslationSupport";
static const char EMBEDDING_KEY[] = "Embedding";
static const char INCLUDE_QT_MODULE_KEY[] = "IncludeQtModule";
static const char ADD_QT_VERSION_CHECK_KEY[] = "AddQtVersionCheck";

static const bool retranslationSupportDefault = false;
static const QtSupport::CodeGenSettings::UiClassEmbedding embeddingDefault
    = QtSupport::CodeGenSettings::PointerAggregatedUiClass;
static const bool includeQtModuleDefault = false;
static const bool addQtVersionCheckDefault = false;

using namespace Utils;

namespace QtSupport {

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
