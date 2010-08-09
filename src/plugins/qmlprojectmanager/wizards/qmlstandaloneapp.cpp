/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmlstandaloneapp.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QRegExp>
#include <QtCore/QTextStream>

#ifndef CREATORLESSTEST
#include <coreplugin/icore.h>
#endif // CREATORLESSTEST

namespace QmlProjectManager {
namespace Internal {

QmlStandaloneApp::QmlStandaloneApp()
    : m_loadDummyData(false)
    , m_orientation(Auto)
    , m_networkEnabled(false)
{
}

QString QmlStandaloneApp::symbianUidForPath(const QString &path)
{
    quint32 hash = 5381;
    for (int i = 0; i < path.size(); ++i) {
        const char c = path.at(i).toAscii();
        hash ^= c + ((c - i) << i % 20) + ((c + i) << (i + 5) % 20) + ((c - 2 * i) << (i + 10) % 20) + ((c + 2 * i) << (i + 15) % 20);
    }
    return QString::fromLatin1("0xE")
            + QString::fromLatin1("%1").arg(hash, 7, 16, QLatin1Char('0')).right(7);
}

void QmlStandaloneApp::setMainQmlFile(const QString &qmlFile)
{
    m_mainQmlFile.setFile(qmlFile);
}

QString QmlStandaloneApp::mainQmlFile() const
{
    return path(MainQml, Target);
}

void QmlStandaloneApp::setOrientation(Orientation orientation)
{
    m_orientation = orientation;
}

QmlStandaloneApp::Orientation QmlStandaloneApp::orientation() const
{
    return m_orientation;
}

void QmlStandaloneApp::setProjectName(const QString &name)
{
    m_projectName = name;
}

QString QmlStandaloneApp::projectName() const
{
    return m_projectName;
}

void QmlStandaloneApp::setProjectPath(const QString &path)
{
    m_projectPath.setFile(path);
}

void QmlStandaloneApp::setSymbianSvgIcon(const QString &icon)
{
    m_symbianSvgIcon = icon;
}

QString QmlStandaloneApp::symbianSvgIcon() const
{
    return path(SymbianSvgIcon, Source);
}

void QmlStandaloneApp::setSymbianTargetUid(const QString &uid)
{
    m_symbianTargetUid = uid;
}

QString QmlStandaloneApp::symbianTargetUid() const
{
    return !m_symbianTargetUid.isEmpty() ? m_symbianTargetUid
        : symbianUidForPath(path(AppProfile, Target));
}

void QmlStandaloneApp::setLoadDummyData(bool loadIt)
{
    m_loadDummyData = loadIt;
}

bool QmlStandaloneApp::loadDummyData() const
{
    return m_loadDummyData;
}

void QmlStandaloneApp::setNetworkEnabled(bool enabled)
{
    m_networkEnabled = enabled;
}

bool QmlStandaloneApp::networkEnabled() const
{
    return m_networkEnabled;
}

QString QmlStandaloneApp::path(Path path, Location location) const
{
    const QString qmlRootFolder = QLatin1String("qml/")
                                  + (useExistingMainQml() ? m_mainQmlFile.dir().dirName() : m_projectName)
                                  + QLatin1Char('/');
    const QString templatesRoot(this->templatesRoot());
    const QString cppSourceSubDir = QLatin1String("cpp/");
    const QString cppTargetSubDir = cppSourceSubDir;
    const QString qmlExtension = QLatin1String(".qml");
    const QString mainCpp = QLatin1String("main.cpp");
    const QString appViewCpp = QLatin1String("qmlapplicationview.cpp");
    const QString appViewH = QLatin1String("qmlapplicationview.h");
    const QString symbianIcon = QLatin1String("symbianicon.svg");
    const char* const errorMessage = "QmlStandaloneApp::path() needs more work";

    switch (location) {
        case Source: {
            switch (path) {
                case MainQml:           return templatesRoot + QLatin1String("qml/app/app.qml");
                case AppProfile:        return templatesRoot + QLatin1String("app.pro");
                case MainCpp:           return templatesRoot + cppSourceSubDir + mainCpp;
                case AppViewerCpp:      return templatesRoot + cppSourceSubDir + appViewCpp;
                case AppViewerH:        return templatesRoot + cppSourceSubDir + appViewH;
                case SymbianSvgIcon:    return !m_symbianSvgIcon.isEmpty() ? m_symbianSvgIcon
                                                    : templatesRoot + cppSourceSubDir + symbianIcon;
                default:                qFatal(errorMessage);
            }
        }
        case Target: {
            const QString pathBase = m_projectPath.absoluteFilePath() + QLatin1Char('/')
                                     + m_projectName + QLatin1Char('/');
            switch (path) {
                case MainQml:           return useExistingMainQml() ? m_mainQmlFile.canonicalFilePath()
                                                    : pathBase + qmlRootFolder + m_projectName + qmlExtension;
                case AppProfile:        return pathBase + m_projectName + QLatin1String(".pro");
                case AppProfilePath:    return pathBase;
                case MainCpp:           return pathBase + cppTargetSubDir + mainCpp;
                case AppViewerCpp:      return pathBase + cppTargetSubDir + appViewCpp;
                case AppViewerH:        return pathBase + cppTargetSubDir + appViewH;
                case SymbianSvgIcon:    return pathBase + cppTargetSubDir + symbianIcon;
                case QmlDir:            return pathBase + qmlRootFolder;
                default:                qFatal(errorMessage);
            }
        }
        case AppProfileRelative: {
            const QDir appProFilePath(this->path(AppProfilePath, Target));
            switch (path) {
                case MainQml:           return useExistingMainQml() ? appProFilePath.relativeFilePath(m_mainQmlFile.canonicalFilePath())
                                                        : qmlRootFolder + m_projectName + qmlExtension;
                case MainCpp:           return cppTargetSubDir + mainCpp;
                case AppViewerCpp:      return cppTargetSubDir + appViewCpp;
                case AppViewerH:        return cppTargetSubDir + appViewH;
                case SymbianSvgIcon:    return cppTargetSubDir + symbianIcon;
                case QmlDir:            return useExistingMainQml() ? appProFilePath.relativeFilePath(m_mainQmlFile.canonicalPath())
                                                        : QString(qmlRootFolder).remove(qmlRootFolder.length() - 1, 1);
                default:                qFatal(errorMessage);
            }
        }
        default: { /* case MainCppRelative: */
            switch (path) {
                case MainQml:           return useExistingMainQml() ? qmlRootFolder + m_mainQmlFile.fileName()
                                                        : QString(qmlRootFolder + m_projectName + qmlExtension);
                default:                qFatal(errorMessage);
            }
        }
    }
    return QString();
}


static QString insertParameter(const QString &line, const QString &parameter)
{
    return QString(line).replace(QRegExp(QLatin1String("\\([^()]+\\)")),
                                 QLatin1Char('(') + parameter + QLatin1Char(')'));
}

QByteArray QmlStandaloneApp::generateMainCpp(const QString *errorMessage) const
{
    Q_UNUSED(errorMessage)

    QFile sourceFile(path(MainCpp, Source));
    sourceFile.open(QIODevice::ReadOnly);
    Q_ASSERT(sourceFile.isOpen());
    QTextStream in(&sourceFile);

    QByteArray mainCppContent;
    QTextStream out(&mainCppContent, QIODevice::WriteOnly);

    QString line;
    do {
        line = in.readLine();
        if (line.contains(QLatin1String("// MAINQML"))) {
            line = insertParameter(line, QLatin1Char('"')
                                   + path(MainQml, MainCppRelative) + QLatin1Char('"'));
        } else if (line.contains(QLatin1String("// IMPORTPATHSLIST"))) {
            continue;
        } else if (line.contains(QLatin1String("// SETIMPORTPATHLIST"))) {
            continue;
        } else if (line.contains(QLatin1String("// ORIENTATION"))) {
            if (m_orientation == Auto)
                continue;
            else
                line = insertParameter(line, QLatin1String("QmlApplicationView::")
                                       + QLatin1String(m_orientation == LockLandscape ?
                                                       "LockLandscape" : "LockPortrait"));
        } else if (line.contains(QLatin1String("// LOADDUMMYDATA"))) {
            continue;
        }
        const int commentIndex = line.indexOf(QLatin1String(" //"));
        if (commentIndex != -1)
            line.truncate(commentIndex);
        out << line << endl;
    } while (!line.isNull());

    return mainCppContent;
}

QByteArray QmlStandaloneApp::generateProFile(const QString *errorMessage) const
{
    Q_UNUSED(errorMessage)

    const QChar comment = QLatin1Char('#');
    QFile proFile(path(AppProfile, Source));
    proFile.open(QIODevice::ReadOnly);
    Q_ASSERT(proFile.isOpen());
    QTextStream in(&proFile);

    QByteArray proFileContent;
    QTextStream out(&proFileContent, QIODevice::WriteOnly);

    QString line;
    QString valueOnNextLine;
    bool uncommentNextLine = false;
    do {
        line = in.readLine();

        if (line.contains(QLatin1String("# MAINQML"))) {
            valueOnNextLine = path(MainQml, AppProfileRelative);
        } else if (line.contains(QLatin1String("# TARGETUID3"))) {
            valueOnNextLine = symbianTargetUid();
        } else if (line.contains(QLatin1String("# DEPLOYMENTFOLDERS"))) {
            // Eat lines
            do {
                line = in.readLine();
            } while (!(line.isNull() || line.contains(QLatin1String("# DEPLOYMENTFOLDERS_END"))));
            out << "folder_01.source = " << path(QmlDir, AppProfileRelative) << endl;
            out << "folder_01.target = qml" << endl;
            out << "DEPLOYMENTFOLDERS = folder_01" << endl;
        } else if (line.contains(QLatin1String("# ORIENTATIONLOCK")) && m_orientation == QmlStandaloneApp::Auto) {
            uncommentNextLine = true;
        } else if (line.contains(QLatin1String("# NETWORKACCESS")) && !m_networkEnabled) {
            uncommentNextLine = true;
        } else if (line.contains(QLatin1String("# QMLINSPECTOR_PATH"))) {
            valueOnNextLine = qmlInspectorSourcesRoot();
        } else if (line.contains(QLatin1String("# QMLINSPECTOR"))) {
            // ### disabled for now; figure out the private headers problem first.
            //uncommentNextLine = true;
        }

        // Remove all marker comments
        if (line.trimmed().startsWith(comment)
            && line.trimmed().endsWith(comment))
            continue;

        if (!valueOnNextLine.isEmpty()) {
            out << line.left(line.indexOf(QLatin1Char('=')) + 2)
                << QDir::fromNativeSeparators(valueOnNextLine) << endl;
            valueOnNextLine.clear();
            continue;
        }

        if (uncommentNextLine) {
            out << comment << line << endl;
            uncommentNextLine = false;
            continue;
        }
        out << line << endl;
    } while (!line.isNull());

    return proFileContent;
}

#ifndef CREATORLESSTEST
QString QmlStandaloneApp::templatesRoot()
{
    return Core::ICore::instance()->resourcePath() + QLatin1String("/templates/qmlapp/");
}

QString QmlStandaloneApp::qmlInspectorSourcesRoot()
{
    return Core::ICore::instance()->resourcePath() + QLatin1String("/qmljsdebugger");
}

static Core::GeneratedFile generateFileCopy(const QString &source,
                                            const QString &target,
                                            bool openEditor = false)
{
    QFile sourceFile(source);
    sourceFile.open(QIODevice::ReadOnly);
    Q_ASSERT(sourceFile.isOpen());
    Core::GeneratedFile generatedFile(target);
    generatedFile.setBinary(true);
    generatedFile.setBinaryContents(sourceFile.readAll());
    if (openEditor)
        generatedFile.setAttributes(Core::GeneratedFile::OpenEditorAttribute);
    return generatedFile;
}

Core::GeneratedFiles QmlStandaloneApp::generateFiles(QString *errorMessage) const
{
    Core::GeneratedFiles files;

    Core::GeneratedFile generatedProFile(path(AppProfile, Target));
    generatedProFile.setContents(generateProFile(errorMessage));
    generatedProFile.setAttributes(Core::GeneratedFile::OpenProjectAttribute);
    files.append(generatedProFile);

    if (!useExistingMainQml())
        files.append(generateFileCopy(path(MainQml, Source), path(MainQml, Target), true));

    Core::GeneratedFile generatedMainCppFile(path(MainCpp, Target));
    generatedMainCppFile.setContents(generateMainCpp(errorMessage));
    files.append(generatedMainCppFile);

    files.append(generateFileCopy(path(AppViewerCpp, Source), path(AppViewerCpp, Target)));
    files.append(generateFileCopy(path(AppViewerH, Source), path(AppViewerH, Target)));
    files.append(generateFileCopy(path(SymbianSvgIcon, Source), path(SymbianSvgIcon, Target)));

    return files;
}
#endif // CREATORLESSTEST

bool QmlStandaloneApp::useExistingMainQml() const
{
    return !m_mainQmlFile.filePath().isEmpty();
}

} // namespace Internal
} // namespace QmlProjectManager
