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

#ifndef FEATUREPROVIDER_H
#define FEATUREPROVIDER_H

#include "core_global.h"

#include <coreplugin/id.h>

#include <QSet>
#include <QStringList>

namespace Core {

class CORE_EXPORT FeatureSet;

class CORE_EXPORT IFeatureProvider
{
public:
    IFeatureProvider() {}
    virtual ~IFeatureProvider() {}
    virtual FeatureSet availableFeatures(const QString &platform) const = 0;
    virtual QStringList availablePlatforms() const = 0;
    virtual QString displayNameForPlatform(const QString &string) const = 0;
};

class CORE_EXPORT Feature : public Id
{
public:
    Feature() = default;
    template <int N> Feature(const char(&ch)[N]) : Id(ch) { }

    static Feature fromString(const QString &str) { return Feature(Id::fromString(str)); }
    static Feature fromName(const QByteArray &ba) { return Feature(Id::fromName(ba)); }

    static Feature versionedFeature(const QByteArray &prefix, int major = -1, int minor = -1);

private:
    explicit Feature(const Id id) : Id(id) { }
};

class CORE_EXPORT FeatureSet : private QSet<Feature>
{
public:
    explicit FeatureSet(Feature id)
    {
        if (id.isValid())
            insert(id);
    }

    FeatureSet() = default;
    FeatureSet(const FeatureSet &other) = default;
    FeatureSet &operator=(const FeatureSet &other) = default;

    static FeatureSet versionedFeatures(const QByteArray &prefix, int major, int minor = -1);

    using QSet<Feature>::isEmpty;

    bool contains(const Feature &feature) const
    {
        return QSet<Feature>::contains(feature);
    }

    bool contains(const FeatureSet &features) const
    {
        return QSet<Feature>::contains(features);
    }

    void remove(const Feature &feature)
    {
        QSet<Feature>::remove(feature);
    }

    void remove(const FeatureSet &features)
    {
        QSet<Feature>::subtract(features);
    }

    FeatureSet operator|(const Feature &feature) const
    {
        FeatureSet copy = *this;
        if (feature.isValid())
            copy.insert(feature);
        return copy;
    }

    FeatureSet operator|(const FeatureSet &features) const
    {
        FeatureSet copy = *this;
        if (!features.isEmpty())
            copy.unite(features);
        return copy;
    }

    FeatureSet &operator|=(const Feature &feature)
    {
        if (feature.isValid())
           insert(feature);
        return *this;
    }

    FeatureSet &operator|=(const FeatureSet &features)
    {
        if (!features.isEmpty())
            unite(features);
        return *this;
    }

    QStringList toStringList() const
    {
        QStringList stringList;
        foreach (const Feature &feature, QSet<Feature>(*this))
            stringList.append(feature.toString());
        return stringList;
    }

    static FeatureSet fromStringList(const QStringList &list)
    {
        FeatureSet features;
        foreach (const QString &i, list)
            features |= Feature::fromString(i);
        return features;
    }
};

} // namespace Core

/*
The following operators have to be defined in the global namespace!
Otherwise "using namespace Core" would hide other | operators
defined in the global namespace (e. g. QFlags).
*/

inline Core::FeatureSet operator |(Core::Feature feature1, Core::Feature feature2)
{ return Core::FeatureSet(feature1) | feature2; }

inline Core::FeatureSet operator|(Core::Feature feature1, Core::FeatureSet feature2)
{ return feature2 | feature1; }


#endif // FEATUREPROVIDER_H
