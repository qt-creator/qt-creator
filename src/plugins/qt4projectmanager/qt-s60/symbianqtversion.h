/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef SYMBIANQTVERSION_H
#define SYMBIANQTVERSION_H

#include <qtsupport/baseqtversion.h>

namespace Qt4ProjectManager {
namespace Internal {

class SymbianQtVersion : public QtSupport::BaseQtVersion
{
public:
    SymbianQtVersion();
    SymbianQtVersion(const QString &path, bool isAutodetected = false, const QString &autodetectionSource = QString());
    SymbianQtVersion *clone() const;
    ~SymbianQtVersion();

    virtual bool equals(BaseQtVersion *other);

    QString type() const;

    virtual bool isValid() const;
    virtual QString invalidReason() const;

    virtual bool toolChainAvailable(const QString &id) const;

    virtual void restoreLegacySettings(QSettings *s);
    virtual void fromMap(const QVariantMap &map);
    virtual QVariantMap toMap() const;

    virtual QList<ProjectExplorer::Abi> qtAbis() const;

    virtual bool supportsTargetId(const QString &id) const;
    virtual QSet<QString> supportedTargetIds() const;

    virtual QString description() const;

    virtual bool supportsShadowBuilds() const;
    virtual bool supportsBinaryDebuggingHelper() const;
    virtual void addToEnvironment(Utils::Environment &env) const;
    virtual QList<ProjectExplorer::HeaderPath> systemHeaderPathes() const;

    virtual ProjectExplorer::IOutputParser *createOutputParser() const;

    virtual QString systemRoot() const;
    void setSystemRoot(const QString &);

    bool isBuildWithSymbianSbsV2() const;

    QString sbsV2Directory() const;
    void setSbsV2Directory(const QString &directory);

    virtual QtSupport::QtConfigWidget *createConfigurationWidget() const;

protected:
    QList<ProjectExplorer::Task> reportIssuesImpl(const QString &proFile, const QString &buildDir);
    void parseMkSpec(ProFileEvaluator *) const;
private:
    QString m_sbsV2Directory;
    QString m_systemRoot;
    mutable bool m_validSystemRoot;
    mutable bool m_isBuildUsingSbsV2;
};

class SymbianQtConfigWidget : public QtSupport::QtConfigWidget
{
    Q_OBJECT
public:
    SymbianQtConfigWidget(SymbianQtVersion *version);
public slots:
    void updateCurrentSbsV2Directory(const QString &path);
    void updateCurrentS60SDKDirectory(const QString &path);
private:
    SymbianQtVersion *m_version;
};

}
}

#endif // SYMBIANQTVERSION_H
