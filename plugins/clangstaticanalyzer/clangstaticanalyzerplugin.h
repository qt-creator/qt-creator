/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Enterprise Qt Quick Profiler Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#ifndef CLANGSTATICANALYZERPLUGIN_H
#define CLANGSTATICANALYZERPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace ClangStaticAnalyzer {
namespace Internal {

class ClangStaticAnalyzerTool;
class ClangStaticAnalyzerSettings;

class ClangStaticAnalyzerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ClangStaticAnalyzer.json")

public:
    ClangStaticAnalyzerPlugin();
    ~ClangStaticAnalyzerPlugin();

    bool initialize(const QStringList &arguments, QString *errorString);
    bool initializeEnterpriseFeatures(const QStringList &arguments, QString *errorString);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

private:
    ClangStaticAnalyzerTool *m_analyzerTool;
};

} // namespace Internal
} // namespace ClangStaticAnalyzerPlugin

#endif // CLANGSTATICANALYZERPLUGIN_H

