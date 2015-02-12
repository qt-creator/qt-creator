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

#include "codegensettings.h"

#include <coreplugin/icore.h>

#include <QSettings>

static const char CODE_GEN_GROUP[] = "FormClassWizardPage";
static const char TRANSLATION_KEY[] = "RetranslationSupport";
static const char EMBEDDING_KEY[] = "Embedding";
static const char INCLUDE_QT_MODULE_KEY[] = "IncludeQtModule";
static const char ADD_QT_VERSION_CHECK_KEY[] = "AddQtVersionCheck";

static const bool retranslationSupportDefault = false;

namespace QtSupport {

CodeGenSettings::CodeGenSettings() :
    embedding(PointerAggregatedUiClass),
    retranslationSupport(retranslationSupportDefault),
    includeQtModule(false),
    addQtVersionCheck(false)
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
    QString group = QLatin1String(CODE_GEN_GROUP) + QLatin1Char('/');

    retranslationSupport = settings->value(group + QLatin1String(TRANSLATION_KEY), retranslationSupportDefault).toBool();
    embedding =  static_cast<UiClassEmbedding>(settings->value(group + QLatin1String(EMBEDDING_KEY), int(PointerAggregatedUiClass)).toInt());
    includeQtModule = settings->value(group + QLatin1String(INCLUDE_QT_MODULE_KEY), false).toBool();
    addQtVersionCheck = settings->value(group + QLatin1String(ADD_QT_VERSION_CHECK_KEY), false).toBool();
}

void CodeGenSettings::toSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(CODE_GEN_GROUP));
    settings->setValue(QLatin1String(TRANSLATION_KEY), retranslationSupport);
    settings->setValue(QLatin1String(EMBEDDING_KEY), embedding);
    settings->setValue(QLatin1String(INCLUDE_QT_MODULE_KEY), includeQtModule);
    settings->setValue(QLatin1String(ADD_QT_VERSION_CHECK_KEY), addQtVersionCheck);
    settings->endGroup();

}

} // namespace QtSupport
