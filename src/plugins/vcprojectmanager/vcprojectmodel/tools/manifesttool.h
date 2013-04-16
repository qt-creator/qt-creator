/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkvoic
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef VCPROJECTMANAGER_INTERNAL_MANIFESTTOOL_H
#define VCPROJECTMANAGER_INTERNAL_MANIFESTTOOL_H

#include "tool.h"
#include "../../widgets/vcnodewidget.h"

class QComboBox;
class QLineEdit;

namespace VcProjectManager {
namespace Internal {

class ManifestTool : public Tool
{
public:
    typedef QSharedPointer<ManifestTool>    Ptr;

    ManifestTool();
    ManifestTool(const ManifestTool &tool);
    ~ManifestTool();

    QString nodeWidgetName() const;
    VcNodeWidget* createSettingsWidget();
    Tool::Ptr clone() const;

    // General
    bool suppressStartupBanner() const;
    bool suppressStartupBannerDefault() const;
    void setSuppressStartupBanner(bool suppress);
    bool verboseOutput() const;
    bool verboseOutputDefault() const;
    void setVerboseOutput(bool enable);
    QString assemblyIdentity() const;
    QString assemblyIdentityDefault() const;
    void setAssemblyIdentity(const QString &identity);
    bool useUnicodeResponseFiles() const;
    bool useUnicodeResponseFilesDefault() const;
    void setUseUnicodeResponseFiles(bool enable);
    bool useFAT32Workaround() const;
    bool useFAT32WorkaroundDefault() const;
    void setUseFAT32Workaround(bool enable);

    // Input and Output
    QString additionalManifestFiles() const;
    QString additionalManifestFilesDefault() const;
    void setAdditionalManifestFiles(const QString &files);
    QString inputResourceManifests() const;
    QString inputResourceManifestsDefault() const;
    void setInputResourceManifests(const QString &manifests);
    bool embedManifest() const;
    bool embedManifestDefault() const;
    void setEmbedManifest(bool enable);
    QString outputManifestFile() const;
    QString outputManifestFileDefault() const;
    void setOutputManifestFile(const QString &file);
    QString manifestResourceFile() const;
    QString manifestResourceFileDefault() const;
    void setManifestResourceFile(const QString &file);
    bool generateCatalogFiles() const;
    bool generateCatalogFilesDefault() const;
    void setGenerateCatalogFiles(bool generate);
    QString dependencyInformationFile() const;
    QString dependencyInformationFileDefault() const;
    void setDependencyInformationFile(const QString &file);

    // Isolated COM
    QString typeLibraryFile() const;
    QString typeLibraryFileDefault() const;
    void setTypeLibraryFile(const QString &file);
    QString registrarScriptFile() const;
    QString registrarScriptFileDefault() const;
    void setRegistrarScriptFile(const QString &file);
    QString componentFileName() const;
    QString componentFileNameDefault() const;
    void setComponentFileName(const QString &fileName);
    QString replacementsFile() const;
    QString replacementsFileDefault() const;
    void setReplacementsFile(const QString &file);

    // Advanced
    bool updateFileHashes() const;
    bool updateFileHashesDefault() const;
    void setUpdateFileHashes(bool update);
    QString updateFileHashesSearchPath() const;
    QString updateFileHashesSearchPathDefault() const;
    void setUpdateFileHashesSearchPath(const QString &searchPath);
};

class ManifestToolWidget : public VcNodeWidget
{
public:
    explicit ManifestToolWidget(ManifestTool::Ptr tool);
    ~ManifestToolWidget();
    void saveData();

private:
    QWidget* createGeneralWidget();
    QWidget* createInputAndOutputWidget();
    QWidget* createIsolatedCOMWidget();
    QWidget* createAdvancedWidget();

    ManifestTool::Ptr m_tool;

    // General
    QComboBox *m_suppStartBannerComboBox;
    QComboBox *m_verboseOutputComboBox;
    QLineEdit *m_asmIdentityLineEdit;
    QComboBox *m_unicodeRespFilesComboBox;
    QComboBox *m_useFATWorkaroundComboBox;

    // Input and Output
    QLineEdit *m_additManifestFilesLineEdit;
    QLineEdit *m_inputResManifestsLineEdit;
    QComboBox *m_embedManifestComboBox;
    QLineEdit *m_outputManifestFileLineEdit;
    QLineEdit *m_manifestResFile;
    QComboBox *m_genCatFilesComboBox;
    QLineEdit *m_depInfoFileLineEdit;

    // Isolated
    QLineEdit *m_typeLibFileLineEdit;
    QLineEdit *m_regScriptFileLineEdit;
    QLineEdit *m_compFileNameLineEdit;
    QLineEdit *m_replFileLineEdit;

    // Advanced
    QComboBox *m_updateFileHashComboBox;
    QLineEdit *m_updtFileHashSearchLineEdit;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_MANIFESTTOOL_H
