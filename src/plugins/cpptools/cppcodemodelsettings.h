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

#pragma once

#include "cpptools_global.h"

#include "clangdiagnosticconfigsmodel.h"

#include <QObject>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace CppTools {

class CPPTOOLS_EXPORT CppCodeModelSettings : public QObject
{
    Q_OBJECT

public:
    enum PCHUsage {
        PchUse_None = 1,
        PchUse_BuildSystem = 2
    };

public:
    void fromSettings(QSettings *s);
    void toSettings(QSettings *s);

public:
    Core::Id clangDiagnosticConfigId() const;
    void setClangDiagnosticConfigId(const Core::Id &configId);
    const ClangDiagnosticConfig clangDiagnosticConfig() const;

    ClangDiagnosticConfigs clangCustomDiagnosticConfigs() const;
    void setClangCustomDiagnosticConfigs(const ClangDiagnosticConfigs &configs);

    PCHUsage pchUsage() const;
    void setPCHUsage(PCHUsage pchUsage);

    bool interpretAmbigiousHeadersAsCHeaders() const;
    void setInterpretAmbigiousHeadersAsCHeaders(bool yesno);

    bool skipIndexingBigFiles() const;
    void setSkipIndexingBigFiles(bool yesno);

    int indexerFileSizeLimitInMb() const;
    void setIndexerFileSizeLimitInMb(int sizeInMB);

public: // for tests
    void emitChanged();

signals:
    void changed();
    void clangDiagnosticConfigIdChanged();

private:
    PCHUsage m_pchUsage = PchUse_None;
    bool m_interpretAmbigiousHeadersAsCHeaders = false;
    bool m_skipIndexingBigFiles = true;
    int m_indexerFileSizeLimitInMB = 5;
    ClangDiagnosticConfigs m_clangCustomDiagnosticConfigs;
    Core::Id m_clangDiagnosticConfigId;
};

} // namespace CppTools
