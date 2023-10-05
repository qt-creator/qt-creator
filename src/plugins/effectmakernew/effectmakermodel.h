// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "shaderfeatures.h"

#include <QMap>
#include <QRegularExpression>
#include <QStandardItemModel>
#include <QTemporaryFile>

#include <QtShaderTools/private/qshaderbaker_p.h>

namespace EffectMaker {

class CompositionNode;
class Uniform;

struct EffectError {
    Q_GADGET
    Q_PROPERTY(QString message MEMBER m_message)
    Q_PROPERTY(int line MEMBER m_line)
    Q_PROPERTY(int type MEMBER m_type)

public:
    QString m_message;
    int m_line = -1;
    int m_type = -1;
};

class EffectMakerModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool isEmpty MEMBER m_isEmpty NOTIFY isEmptyChanged)
    Q_PROPERTY(int selectedIndex MEMBER m_selectedIndex NOTIFY selectedIndexChanged)
    Q_PROPERTY(bool shadersUpToDate READ shadersUpToDate WRITE setShadersUpToDate NOTIFY shadersUpToDateChanged)
    Q_PROPERTY(QString qmlComponentString READ qmlComponentString)


public:
    EffectMakerModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    bool isEmpty() const { return m_isEmpty; }
    void setIsEmpty(bool val);

    void addNode(const QString &nodeQenPath);

    Q_INVOKABLE void moveNode(int fromIdx, int toIdx);
    Q_INVOKABLE void removeNode(int idx);

    bool shadersUpToDate() const;
    void setShadersUpToDate(bool newShadersUpToDate);

    QString fragmentShader() const;
    void setFragmentShader(const QString &newFragmentShader);
    QString vertexShader() const;
    void setVertexShader(const QString &newVertexShader);

    const QString &qmlComponentString() const;
    void setQmlComponentString(const QString &string);

    Q_INVOKABLE void updateQmlComponent();

signals:
    void isEmptyChanged();
    void selectedIndexChanged(int idx);
    void effectErrorChanged();
    void shadersUpToDateChanged();
    void shadersBaked();

private:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        EnabledRole,
        UniformsRole
    };

    enum ErrorTypes {
        ErrorCommon = -1,
        ErrorQMLParsing,
        ErrorVert,
        ErrorFrag,
        ErrorQMLRuntime,
        ErrorPreprocessor
    };

    bool isValidIndex(int idx) const;

    const QList<Uniform *> allUniforms();

    const QString getBufUniform();
    const QString getVSUniforms();
    const QString getFSUniforms();

    void updateBakedShaderVersions();
    QString detectErrorMessage(const QString &errorMessage);
    EffectError effectError() const;
    void setEffectError(const QString &errorMessage, int type = -1, int lineNumber = -1);
    void resetEffectError(int type);

    QString valueAsString(const Uniform &uniform);
    QString valueAsBinding(const Uniform &uniform);
    QString valueAsVariable(const Uniform &uniform);
    QString getImageElementName(const Uniform &uniform);
    const QString getConstVariables();
    const QString getDefineProperties();
    int getTagIndex(const QStringList &code, const QString &tag);
    QString processVertexRootLine(const QString &line);
    QString processFragmentRootLine(const QString &line);
    QStringList getDefaultRootVertexShader();
    QStringList getDefaultRootFragmentShader();
    QStringList removeTagsFromCode(const QStringList &codeLines);
    QString removeTagsFromCode(const QString &code);
    QString getCustomShaderVaryings(bool outState);
    QString generateVertexShader(bool includeUniforms = true);
    QString generateFragmentShader(bool includeUniforms = true);
    void updateCustomUniforms();
    void bakeShaders();

    QString getQmlImagesString(bool localFiles);
    QString getQmlComponentString(bool localFiles);

    QList<CompositionNode *> m_nodes;

    int m_selectedIndex = -1;
    bool m_isEmpty = true;
    // True when shaders haven't changed since last baking
    bool m_shadersUpToDate = true;
    QMap<int, EffectError> m_effectErrors;
    ShaderFeatures m_shaderFeatures;
    QStringList m_shaderVaryingVariables;
    QString m_fragmentShader;
    QString m_vertexShader;
    QStringList m_defaultRootVertexShader;
    QStringList m_defaultRootFragmentShader;
    QShaderBaker m_baker;
    QTemporaryFile m_fragmentShaderFile;
    QTemporaryFile m_vertexShaderFile;
    // Used in exported QML, at root of the file
    QString m_exportedRootPropertiesString;
    // Used in exported QML, at ShaderEffect component of the file
    QString m_exportedEffectPropertiesString;
    // Used in preview QML, at ShaderEffect component of the file
    QString m_previewEffectPropertiesString;
    QString m_qmlComponentString;

    const QRegularExpression m_spaceReg = QRegularExpression("\\s+");
};

} // namespace EffectMaker

