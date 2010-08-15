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
    return path(MainQml);
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
    return path(SymbianSvgIconOrigin);
}

void QmlStandaloneApp::setSymbianTargetUid(const QString &uid)
{
    m_symbianTargetUid = uid;
}

QString QmlStandaloneApp::symbianTargetUid() const
{
    return !m_symbianTargetUid.isEmpty() ? m_symbianTargetUid
        : symbianUidForPath(path(AppProfile));
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

QString QmlStandaloneApp::path(Path path) const
{
    const QString qmlSubDir = QLatin1String("qml/")
                              + (useExistingMainQml() ? m_mainQmlFile.dir().dirName() : m_projectName)
                              + QLatin1Char('/');
    const QString originsRoot = templatesRoot();
    const QString cppOriginsSubDir = QLatin1String("cpp/");
    const QString cppTargetSubDir = cppOriginsSubDir;
    const QString qmlExtension = QLatin1String(".qml");
    const QString appPriFileName = QLatin1String("qmlapplication.pri");
    const QString mainCppFileName = QLatin1String("main.cpp");
    const QString appViewCppFileName = QLatin1String("qmlapplicationview.cpp");
    const QString appViewHFileName = QLatin1String("qmlapplicationview.h");
    const QString symbianIconFileName = QLatin1String("symbianicon.svg");
    const char* const errorMessage = "QmlStandaloneApp::path() needs more work";

    const QString pathBase = m_projectPath.absoluteFilePath() + QLatin1Char('/')
                             + m_projectName + QLatin1Char('/');
    const QDir appProFilePath(pathBase);

    switch (path) {
        case MainQml:                       return useExistingMainQml() ? m_mainQmlFile.canonicalFilePath()
                                                : pathBase + qmlSubDir + m_projectName + qmlExtension;
        case MainQmlDeployed:               return useExistingMainQml() ? qmlSubDir + m_mainQmlFile.fileName()
                                                : QString(qmlSubDir + m_projectName + qmlExtension);
        case MainQmlOrigin:                 return originsRoot + QLatin1String("qml/app/app.qml");
        case MainCpp:                       return pathBase + cppTargetSubDir + mainCppFileName;
        case MainCppOrigin:                 return originsRoot + cppOriginsSubDir + mainCppFileName;
        case MainCppProFileRelative:        return cppTargetSubDir + mainCppFileName;
        case AppProfile:                    return pathBase + m_projectName + QLatin1String(".pro");
        case AppProfileOrigin:              return originsRoot + QLatin1String("app.pro");
        case AppProfilePath:                return pathBase;
        case AppPri:                        return pathBase + appPriFileName;
        case AppPriOrigin:                  return originsRoot + appPriFileName;
        case AppViewerCpp:                  return pathBase + cppTargetSubDir + appViewCppFileName;
        case AppViewerCppOrigin:            return originsRoot + cppOriginsSubDir + appViewCppFileName;
        case AppViewerCppProFileRelative:   return cppTargetSubDir + appViewCppFileName;
        case AppViewerH:                    return pathBase + cppTargetSubDir + appViewHFileName;
        case AppViewerHOrigin:              return originsRoot + cppOriginsSubDir + appViewHFileName;
        case AppViewerHProFileRelative:     return cppTargetSubDir + appViewHFileName;
        case SymbianSvgIcon:                return pathBase + cppTargetSubDir + symbianIconFileName;
        case SymbianSvgIconOrigin:          return !m_symbianSvgIcon.isEmpty() ? m_symbianSvgIcon
                                                : originsRoot + cppOriginsSubDir + symbianIconFileName;
        case SymbianSvgIconProFileRelative: return cppTargetSubDir + symbianIconFileName;
        case QmlDir:                        return pathBase + qmlSubDir;
        case QmlDirProFileRelative:         return useExistingMainQml() ? appProFilePath.relativeFilePath(m_mainQmlFile.canonicalPath())
                                                : QString(qmlSubDir).remove(qmlSubDir.length() - 1, 1);
        default:                            qFatal(errorMessage);
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

    QFile sourceFile(path(MainCppOrigin));
    sourceFile.open(QIODevice::ReadOnly);
    Q_ASSERT(sourceFile.isOpen());
    QTextStream in(&sourceFile);

    QByteArray mainCppContent;
    QTextStream out(&mainCppContent, QIODevice::WriteOnly);

    QString line;
    do {
        line = in.readLine();
        if (line.contains(QLatin1String("// MAINQML"))) {
            line = insertParameter(line, QLatin1Char('"') + path(MainQmlDeployed) + QLatin1Char('"'));
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
    QFile proFile(path(AppProfileOrigin));
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

        if (line.contains(QLatin1String("# TARGETUID3"))) {
            valueOnNextLine = symbianTargetUid();
        } else if (line.contains(QLatin1String("# DEPLOYMENTFOLDERS"))) {
            // Eat lines
            do {
                line = in.readLine();
            } while (!(line.isNull() || line.contains(QLatin1String("# DEPLOYMENTFOLDERS_END"))));
            out << "folder_01.source = " << path(QmlDirProFileRelative) << endl;
            out << "folder_01.target = qml" << endl;
            out << "DEPLOYMENTFOLDERS = folder_01" << endl;
        } else if (line.contains(QLatin1String("# ORIENTATIONLOCK")) && m_orientation == QmlStandaloneApp::Auto) {
            uncommentNextLine = true;
        } else if (line.contains(QLatin1String("# NETWORKACCESS")) && !m_networkEnabled) {
            uncommentNextLine = true;
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

    Core::GeneratedFile generatedProFile(path(AppProfile));
    generatedProFile.setContents(generateProFile(errorMessage));
    generatedProFile.setAttributes(Core::GeneratedFile::OpenProjectAttribute);
    files.append(generatedProFile);
    files.append(generateFileCopy(path(AppPriOrigin), path(AppPri)));

    if (!useExistingMainQml())
        files.append(generateFileCopy(path(MainQmlOrigin), path(MainQml), true));

    Core::GeneratedFile generatedMainCppFile(path(MainCpp));
    generatedMainCppFile.setContents(generateMainCpp(errorMessage));
    files.append(generatedMainCppFile);

    files.append(generateFileCopy(path(AppViewerCppOrigin), path(AppViewerCpp)));
    files.append(generateFileCopy(path(AppViewerHOrigin), path(AppViewerH)));
    files.append(generateFileCopy(path(SymbianSvgIconOrigin), path(SymbianSvgIcon)));

    return files;
}
#endif // CREATORLESSTEST

bool QmlStandaloneApp::useExistingMainQml() const
{
    return !m_mainQmlFile.filePath().isEmpty();
}

} // namespace Internal
} // namespace QmlProjectManager
