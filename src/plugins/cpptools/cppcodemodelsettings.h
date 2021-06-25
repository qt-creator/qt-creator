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

#include <utils/fileutils.h>

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
    Utils::Id clangDiagnosticConfigId() const;
    void setClangDiagnosticConfigId(const Utils::Id &configId);
    static Utils::Id defaultClangDiagnosticConfigId() ;
    const ClangDiagnosticConfig clangDiagnosticConfig() const;

    ClangDiagnosticConfigs clangCustomDiagnosticConfigs() const;
    void setClangCustomDiagnosticConfigs(const ClangDiagnosticConfigs &configs);

    bool enableLowerClazyLevels() const;
    void setEnableLowerClazyLevels(bool yesno);

    PCHUsage pchUsage() const;
    void setPCHUsage(PCHUsage pchUsage);

    bool interpretAmbigiousHeadersAsCHeaders() const;
    void setInterpretAmbigiousHeadersAsCHeaders(bool yesno);

    bool skipIndexingBigFiles() const;
    void setSkipIndexingBigFiles(bool yesno);

    int indexerFileSizeLimitInMb() const;
    void setIndexerFileSizeLimitInMb(int sizeInMB);

    void setCategorizeFindReferences(bool categorize) { m_categorizeFindReferences = categorize; }
    bool categorizeFindReferences() const { return m_categorizeFindReferences; }

signals:
    void clangDiagnosticConfigsInvalidated(const QVector<Utils::Id> &configId);
    void changed();

private:
    PCHUsage m_pchUsage = PchUse_BuildSystem;
    bool m_interpretAmbigiousHeadersAsCHeaders = false;
    bool m_skipIndexingBigFiles = true;
    int m_indexerFileSizeLimitInMB = 5;
    ClangDiagnosticConfigs m_clangCustomDiagnosticConfigs;
    Utils::Id m_clangDiagnosticConfigId;
    bool m_enableLowerClazyLevels = true; // For UI behavior only
    bool m_categorizeFindReferences = false; // Ephemeral!
};

class CPPTOOLS_EXPORT ClangdSettings
{
public:
    class Data
    {
    public:
        bool useClangd = false;
        bool enableIndexing = true;
        Utils::FilePath executableFilePath;
    };

    static bool useClangd() { return instance().m_data.useClangd; }

    static void setDefaultClangdPath(const Utils::FilePath &filePath);
    static Utils::FilePath clangdFilePath();
    static bool indexingEnabled() { return instance().m_data.enableIndexing; }

    static void setData(const Data &data);
    static Data data() { return instance().m_data; }

#ifdef WITH_TESTS
    static void setUseClangd(bool use);
    static void setClangdFilePath(const Utils::FilePath &filePath);
#endif

private:
    ClangdSettings() { loadSettings(); }
    static ClangdSettings &instance();

    void loadSettings();
    void saveSettings();

    Data m_data;
};

} // namespace CppTools
