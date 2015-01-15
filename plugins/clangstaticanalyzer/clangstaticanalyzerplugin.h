/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise Qt Quick Profiler Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
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

