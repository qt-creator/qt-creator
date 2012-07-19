/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
    SymbianQtVersion(const Utils::FileName &path, bool isAutodetected = false, const QString &autodetectionSource = QString());
    SymbianQtVersion *clone() const;
    ~SymbianQtVersion();

    bool equals(BaseQtVersion *other);

    QString type() const;

    bool isValid() const;
    QString invalidReason() const;

    void restoreLegacySettings(QSettings *s);
    void fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

    QList<ProjectExplorer::Abi> detectQtAbis() const;

    QString description() const;

    bool supportsShadowBuilds() const;
    bool supportsBinaryDebuggingHelper() const;
    void addToEnvironment(const ProjectExplorer::Profile *p, Utils::Environment &env) const;
    QList<ProjectExplorer::HeaderPath> systemHeaderPathes(const ProjectExplorer::Profile *p) const;

    ProjectExplorer::IOutputParser *createOutputParser() const;

    bool isBuildWithSymbianSbsV2() const;

    QString sbsV2Directory() const;
    void setSbsV2Directory(const QString &directory);

    QtSupport::QtConfigWidget *createConfigurationWidget() const;

    Core::FeatureSet availableFeatures() const;
    QString platformName() const;
    QString platformDisplayName() const;

protected:
    void parseMkSpec(ProFileEvaluator *) const;
private:
    QString m_sbsV2Directory;
    mutable bool m_isBuildUsingSbsV2;
};

class SymbianQtConfigWidget : public QtSupport::QtConfigWidget
{
    Q_OBJECT

public:
    SymbianQtConfigWidget(SymbianQtVersion *version);

public slots:
    void updateCurrentSbsV2Directory(const QString &path);

private:
    SymbianQtVersion *m_version;
};

}
}

#endif // SYMBIANQTVERSION_H
