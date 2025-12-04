// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectstorage.h"

#include "projectstoragetracing.h"

#include <predicate.h>
#include <sqlitedatabase.h>

#include <concepts>

namespace QmlDesigner {

using NanotraceHR::keyValue;
using ProjectStorageTracing::category;
using Storage::Synchronization::EnumerationDeclaration;
using Storage::Synchronization::Type;
using Storage::Synchronization::TypeAnnotation;

enum class SpecialIdState { Unresolved = -1 };

constexpr TypeId unresolvedTypeId = TypeId::createSpecialState(SpecialIdState::Unresolved);

namespace {
class UnresolvedTypeId : public TypeId
{
public:
    constexpr UnresolvedTypeId()
        : TypeId{TypeId::createSpecialState(SpecialIdState::Unresolved)}
    {}

    static constexpr UnresolvedTypeId create(DatabaseType idNumber)
    {
        UnresolvedTypeId id;
        id.id = idNumber;
        return id;
    }
};

auto createSingletonTypeTraitMask()
{
    Storage::TypeTraits traits;
    traits.type = 0;
    traits.isSingleton = true;

    return traits.type;
}

auto createSingletonTraitsExpression()
{
    Utils::SmallString traitsExpression = "traits & ";
    traitsExpression.append(Utils::SmallString::number(createSingletonTypeTraitMask()));

    return traitsExpression;
}

} // namespace

struct ProjectStorage::Statements
{
    Statements(Sqlite::Database &database)
        : database{database}
    {}

    Sqlite::Database &database;
    Sqlite::ReadWriteStatement<1, 2> insertTypeStatement{
        "INSERT INTO types(sourceId, name) VALUES(?1, ?2) RETURNING typeId", database};
    Sqlite::WriteStatement<3> updatePrototypeAndExtensionNameStatement{
        "UPDATE types "
        "SET prototypeNameId=?2, extensionNameId=?3 "
        "WHERE typeId=?1 AND (prototypeNameId IS NOT ?2 OR extensionNameId IS NOT ?3)",
        database};
    Sqlite::WriteStatement<2> insertBasesStatement{
        "INSERT INTO bases(typeId, baseId) VALUES(?1, ?2)", database};
    Sqlite::WriteStatement<3> updateBasesStatement{
        "UPDATE bases SET baseId=?2 WHERE typeId=?1 AND baseId=?3", database};
    Sqlite::WriteStatement<2> deleteBasesStatement{
        "DELETE FROM bases WHERE typeId=?1 and baseId=?2", database};
    Sqlite::WriteStatement<2> upsertPrototypesStatement{
        "INSERT INTO prototypes(typeId, prototypeId) "
        "VALUES(?1, ?2)  "
        "ON CONFLICT DO UPDATE SET prototypeId=excluded.prototypeId "
        "WHERE prototypeId IS NOT excluded.prototypeId",
        database};
    Sqlite::WriteStatement<1> deletePrototypesStatement{"DELETE FROM prototypes WHERE typeId=?1",
                                                        database};
    mutable Sqlite::ReadStatement<1, 1> selectPrototypeIdByTypeIdStatement{
        "SELECT prototypeId FROM prototypes WHERE typeId=?1", database};
    mutable Sqlite::ReadStatement<1, 1> selectExtensionIdByTypeIdStatement{
        "SELECT baseId FROM bases WHERE typeId=?1 AND baseId NOT IN ("
        "  SELECT prototypeId FROM prototypes WHERE typeId=?1)",
        database};
    Sqlite::WriteStatement<1> deleteAllBasesStatement{"DELETE FROM bases WHERE typeId=?1", database};
    mutable Sqlite::ReadStatement<3, 1> selectTypeBySourceIdStatement{
        "SELECT typeId, prototypeNameId, extensionNameId FROM types WHERE sourceId=?1", database};
    mutable Sqlite::ReadStatement<1, 1> selectBaseIdsStatement{
        "SELECT baseId FROM bases WHERE typeId=?1 ORDER BY baseId", database};
    mutable Sqlite::ReadStatement<1, 1> selectTypeIdByExportedNameStatement{
        "SELECT typeId FROM exportedTypeNames WHERE name=?1", database};
    mutable Sqlite::ReadStatement<1, 2> selectTypeIdByModuleIdAndExportedNameStatement{
        "SELECT typeId FROM exportedTypeNames "
        "WHERE moduleId=?1 AND name=?2 "
        "ORDER BY majorVersion DESC, minorVersion DESC "
        "LIMIT 1",
        database};
    mutable Sqlite::ReadStatement<1, 3> selectTypeIdByModuleIdAndExportedNameAndMajorVersionStatement{
        "SELECT typeId FROM exportedTypeNames "
        "WHERE moduleId=?1 AND name=?2 AND majorVersion=?3"
        "ORDER BY minorVersion DESC "
        "LIMIT 1",
        database};
    mutable Sqlite::ReadStatement<1, 4> selectTypeIdByModuleIdAndExportedNameAndVersionStatement{
        "SELECT typeId FROM exportedTypeNames "
        "WHERE moduleId=?1 AND name=?2 AND majorVersion=?3 AND minorVersion<=?4"
        "ORDER BY minorVersion DESC "
        "LIMIT 1",
        database};
    mutable Sqlite::ReadStatement<4, 1> selectPropertyDeclarationResultByPropertyDeclarationIdStatement{
        "SELECT propertyImportedTypeNameId, "
        "  propertyTypeId, "
        "  propertyDeclarationId, "
        "  propertyTraits "
        "FROM propertyDeclarations "
        "WHERE propertyDeclarationId=?1 "
        "LIMIT 1",
        database};
    mutable Sqlite::ReadStatement<6, 1> selectTypeByTypeIdStatement{
        "SELECT sourceId, t.name, t.typeId,  traits, annotationTraits, "
        "pd.name "
        "FROM types AS t LEFT JOIN propertyDeclarations AS pd ON "
        "defaultPropertyId=propertyDeclarationId "
        "WHERE t.typeId=?",
        database};
    mutable Sqlite::ReadStatement<2, 1> selectTypeNameAndSourceIdByTypeIdStatement{
        "SELECT name, sourceId FROM types WHERE typeId=?", database};
    mutable Sqlite::ReadStatement<5, 1> selectExportedTypesByTypeIdStatement{
        "SELECT moduleId, typeId, name, majorVersion, minorVersion "
        "FROM exportedTypeNames "
        "WHERE typeId=?",
        database};
    mutable Sqlite::ReadStatement<5, 2> selectExportedTypesByTypeIdAndSourceIdStatement{
        "SELECT etn.moduleId, "
        "  typeId, "
        "  name, "
        "  etn.majorVersion, "
        "  etn.minorVersion "
        "FROM exportedTypeNames AS etn "
        "JOIN documentImports USING(moduleId) "
        "WHERE typeId=?1 AND sourceId=?2",
        database};
    mutable Sqlite::ReadStatement<6> selectTypesStatement{
        "SELECT sourceId, "
        "       t.name, "
        "       t.typeId, "
        "       traits, "
        "       annotationTraits, "
        "       pd.name "
        "FROM types AS t LEFT JOIN propertyDeclarations AS pd "
        "     ON defaultPropertyId=propertyDeclarationId "
        "WHERE traits IS NOT NULL",
        database};
    mutable Sqlite::ReadStatement<1> selectTypeIdsStatement{"SELECT typeId "
                                                            "FROM types "
                                                            "WHERE traits IS NOT NULL",
                                                            database};
    Sqlite::WriteStatement<2> updateTypeTraitStatement{
        "UPDATE types SET traits = ?2 WHERE typeId=?1", database};
    Sqlite::ReadWriteStatement<1, 2> updateTypeAnnotationTraitsStatement{
        "WITH RECURSIVE "
        "  heirs(typeId) AS ("
        "      VALUES(?1) "
        "    UNION ALL "
        "      SELECT p.typeId "
        "      FROM prototypes AS p JOIN heirs AS h "
        "      WHERE prototypeId=h.typeId "
        "        AND p.typeId NOT IN (SELECT typeId FROM typeAnnotations)) "
        "UPDATE types AS t "
        "SET annotationTraits = ?2 "
        "FROM heirs h "
        "WHERE t.typeId=h.typeId "
        "RETURNING typeId",
        database};
    Sqlite::ReadStatement<2, 1> selectTypeAnnotationTraitsFromPrototypeStatement{
        "WITH RECURSIVE "
        "  typeChain(typeId, baseId) AS ( "
        "      SELECT typeId, prototypeId "
        "      FROM prototypes "
        "      WHERE typeId=?1 "
        "    UNION ALL "
        "      SELECT tc.typeId, p.prototypeId "
        "      FROM prototypes AS p JOIN typeChain AS tc "
        "      WHERE p.typeId=tc.baseId) "
        "SELECT tc.typeId, annotationTraits "
        "FROM typeChain AS tc "
        "  JOIN typeAnnotations AS ta ON(ta.typeId=tc.baseId) "
        "  JOIN types AS t ON(t.typeId=tc.baseId) "
        "LIMIT 1",
        database};
    Sqlite::ReadStatement<1, 2> selectNotUpdatedTypesInSourcesStatement{
        "SELECT DISTINCT typeId FROM types WHERE (sourceId IN carray(?1) AND typeId NOT IN "
        "carray(?2))",
        database};
    Sqlite::WriteStatement<1> deleteExportedTypeNamesByTypeIdStatement{
        "DELETE FROM exportedTypeNames WHERE typeId=?", database};
    Sqlite::WriteStatement<1> deleteEnumerationDeclarationByTypeIdStatement{
        "DELETE FROM enumerationDeclarations WHERE typeId=?", database};
    Sqlite::WriteStatement<1> deletePropertyDeclarationByTypeIdStatement{
        "DELETE FROM propertyDeclarations WHERE typeId=?", database};
    Sqlite::WriteStatement<1> deleteFunctionDeclarationByTypeIdStatement{
        "DELETE FROM functionDeclarations WHERE typeId=?", database};
    Sqlite::WriteStatement<1> deleteSignalDeclarationByTypeIdStatement{
        "DELETE FROM signalDeclarations WHERE typeId=?", database};
    Sqlite::WriteStatement<1> deletePrototypeByTypeIdStatement{
        "DELETE FROM prototypes WHERE typeId=?", database};
    Sqlite::WriteStatement<1> deleteBaseByTypeIdStatement{"DELETE FROM bases WHERE typeId=?",
                                                          database};
    Sqlite::WriteStatement<1> resetTypeStatement{"UPDATE types "
                                                 "SET traits=NULL, "
                                                 "    prototypeNameId=NULL, "
                                                 "    extensionNameId=NULL, "
                                                 "    defaultPropertyId=NULL, "
                                                 "    annotationTraits=NULL "
                                                 "WHERE typeId=?",
                                                 database};
    mutable Sqlite::ReadStatement<6, 1> selectPropertyDeclarationsByTypeIdStatement{
        "SELECT "
        "  propertyDeclarationId, "
        "  name, "
        "  propertyTypeId, "
        "  propertyTraits, "
        "  (SELECT name "
        "   FROM propertyDeclarations "
        "   WHERE propertyDeclarationId=pd.aliasPropertyDeclarationId), "
        "  typeId "
        "FROM propertyDeclarations AS pd "
        "WHERE typeId=?",
        database};
    Sqlite::ReadStatement<6, 1> selectPropertyDeclarationsForTypeIdStatement{
        "SELECT "
        "  name, "
        "  propertyTraits, "
        "  propertyTypeId, "
        "  propertyImportedTypeNameId, "
        "  propertyDeclarationId, "
        "  aliasPropertyDeclarationId "
        "FROM propertyDeclarations "
        "WHERE typeId=? "
        "ORDER BY name",
        database};
    Sqlite::ReadWriteStatement<1, 5> insertPropertyDeclarationStatement{
        "INSERT INTO propertyDeclarations("
        "  typeId, "
        "  name, "
        "  propertyTypeId, "
        "  propertyTraits, "
        "  propertyImportedTypeNameId, "
        "  aliasPropertyDeclarationId) "
        "VALUES(?1, ?2, ?3, ?4, ?5, NULL) "
        "RETURNING propertyDeclarationId",
        database};
    Sqlite::WriteStatement<4> updatePropertyDeclarationStatement{
        "UPDATE propertyDeclarations "
        "SET "
        "  propertyTypeId=?2, "
        "  propertyTraits=?3, "
        "  propertyImportedTypeNameId=?4, "
        "  aliasPropertyImportedTypeNameId=NULL, "
        "  aliasPropertyDeclarationName=NULL, "
        "  aliasPropertyDeclarationTailName=NULL, "
        "  aliasPropertyDeclarationId=NULL, "
        "  aliasPropertyDeclarationTailId=NULL "
        "WHERE propertyDeclarationId=?1",
        database};
    Sqlite::WriteStatement<2> resetAliasPropertyDeclarationStatement{
        "UPDATE propertyDeclarations "
        "SET propertyTypeId=NULL, "
        "    propertyTraits=?2, "
        "    propertyImportedTypeNameId=NULL, "
        "    aliasPropertyDeclarationId=NULL, "
        "    aliasPropertyDeclarationTailId=NULL "
        "WHERE propertyDeclarationId=?1",
        database};
    Sqlite::WriteStatement<3> updatePropertyAliasDeclarationRecursivelyWithTypeAndTraitsStatement{
        "WITH RECURSIVE "
        "  properties(aliasPropertyDeclarationId) AS ( "
        "    SELECT propertyDeclarationId FROM propertyDeclarations WHERE "
        "      aliasPropertyDeclarationId=?1 "
        "   UNION ALL "
        "     SELECT pd.propertyDeclarationId FROM "
        "       propertyDeclarations AS pd JOIN properties USING(aliasPropertyDeclarationId)) "
        "UPDATE propertyDeclarations AS pd "
        "SET propertyTypeId=?2, propertyTraits=?3 "
        "FROM properties AS p "
        "WHERE pd.propertyDeclarationId=p.aliasPropertyDeclarationId",
        database};
    Sqlite::WriteStatement<1> updatePropertyAliasDeclarationRecursivelyStatement{
        "WITH RECURSIVE "
        "  propertyValues(propertyTypeId, propertyTraits) AS ("
        "    SELECT propertyTypeId, propertyTraits FROM propertyDeclarations "
        "      WHERE propertyDeclarationId=?1), "
        "  properties(aliasPropertyDeclarationId) AS ( "
        "    SELECT propertyDeclarationId FROM propertyDeclarations WHERE "
        "      aliasPropertyDeclarationId=?1 "
        "   UNION ALL "
        "     SELECT pd.propertyDeclarationId FROM "
        "       propertyDeclarations AS pd JOIN properties USING(aliasPropertyDeclarationId)) "
        "UPDATE propertyDeclarations AS pd "
        "SET propertyTypeId=pv.propertyTypeId, propertyTraits=pv.propertyTraits "
        "FROM properties AS p, propertyValues AS pv "
        "WHERE pd.propertyDeclarationId=p.aliasPropertyDeclarationId",
        database};
    Sqlite::WriteStatement<1> deletePropertyDeclarationStatement{
        "DELETE FROM propertyDeclarations WHERE propertyDeclarationId=?", database};
    Sqlite::ReadStatement<3, 1> selectPropertyDeclarationsWithAliasForTypeIdStatement{
        "SELECT name, "
        "  propertyDeclarationId, "
        "  aliasPropertyDeclarationId "
        "FROM propertyDeclarations "
        "WHERE typeId=? AND aliasPropertyDeclarationId IS NOT NULL "
        "ORDER BY name",
        database};
    Sqlite::WriteStatement<5> updatePropertyDeclarationWithAliasAndTypeStatement{
        "UPDATE propertyDeclarations "
        "SET propertyTypeId=?2, "
        "  propertyTraits=?3, "
        "  propertyImportedTypeNameId=?4, "
        "  aliasPropertyDeclarationId=?5 "
        "WHERE propertyDeclarationId=?1",
        database};
    Sqlite::ReadWriteStatement<1, 5> insertAliasPropertyDeclarationStatement{
        "INSERT INTO propertyDeclarations("
        "  typeId, "
        "  name, "
        "  aliasPropertyImportedTypeNameId, "
        "  aliasPropertyDeclarationName, "
        "  aliasPropertyDeclarationTailName) "
        "VALUES(?1, ?2, ?3, ?4, ?5) "
        "RETURNING propertyDeclarationId",
        database};
    mutable Sqlite::ReadStatement<4, 1> selectFunctionDeclarationsForTypeIdStatement{
        "SELECT name, returnTypeName, signature, functionDeclarationId FROM "
        "functionDeclarations WHERE typeId=? ORDER BY name, signature",
        database};
    mutable Sqlite::ReadStatement<3, 1> selectFunctionDeclarationsForTypeIdWithoutSignatureStatement{
        "SELECT name, returnTypeName, functionDeclarationId FROM "
        "functionDeclarations WHERE typeId=? ORDER BY name",
        database};
    mutable Sqlite::ReadStatement<3, 1> selectFunctionParameterDeclarationsStatement{
        "SELECT json_extract(json_each.value, '$.n'), json_extract(json_each.value, '$.tn'), "
        "json_extract(json_each.value, '$.tr') FROM functionDeclarations, "
        "json_each(functionDeclarations.signature) WHERE functionDeclarationId=?",
        database};
    Sqlite::WriteStatement<4> insertFunctionDeclarationStatement{
        "INSERT INTO functionDeclarations(typeId, name, returnTypeName, signature) VALUES(?1, "
        "?2, "
        "?3, ?4)",
        database};
    Sqlite::WriteStatement<3> updateFunctionDeclarationStatement{
        "UPDATE functionDeclarations "
        "SET returnTypeName=?2, signature=?3 "
        "WHERE functionDeclarationId=?1",
        database};
    Sqlite::WriteStatement<1> deleteFunctionDeclarationStatement{
        "DELETE FROM functionDeclarations WHERE functionDeclarationId=?", database};
    mutable Sqlite::ReadStatement<3, 1> selectSignalDeclarationsForTypeIdStatement{
        "SELECT name, signature, signalDeclarationId FROM signalDeclarations WHERE typeId=? "
        "ORDER "
        "BY name, signature",
        database};
    mutable Sqlite::ReadStatement<2, 1> selectSignalDeclarationsForTypeIdWithoutSignatureStatement{
        "SELECT name, signalDeclarationId FROM signalDeclarations WHERE typeId=? ORDER BY name",
        database};
    mutable Sqlite::ReadStatement<3, 1> selectSignalParameterDeclarationsStatement{
        "SELECT json_extract(json_each.value, '$.n'), json_extract(json_each.value, '$.tn'), "
        "json_extract(json_each.value, '$.tr') FROM signalDeclarations, "
        "json_each(signalDeclarations.signature) WHERE signalDeclarationId=?",
        database};
    Sqlite::WriteStatement<3> insertSignalDeclarationStatement{
        "INSERT INTO signalDeclarations(typeId, name, signature) VALUES(?1, ?2, ?3)", database};
    Sqlite::WriteStatement<2> updateSignalDeclarationStatement{
        "UPDATE signalDeclarations SET  signature=?2 WHERE signalDeclarationId=?1", database};
    Sqlite::WriteStatement<1> deleteSignalDeclarationStatement{
        "DELETE FROM signalDeclarations WHERE signalDeclarationId=?", database};
    mutable Sqlite::ReadStatement<3, 1> selectEnumerationDeclarationsForTypeIdStatement{
        "SELECT name, enumeratorDeclarations, enumerationDeclarationId FROM "
        "enumerationDeclarations WHERE typeId=? ORDER BY name",
        database};
    mutable Sqlite::ReadStatement<2, 1> selectEnumerationDeclarationsForTypeIdWithoutEnumeratorDeclarationsStatement{
        "SELECT name, enumerationDeclarationId FROM enumerationDeclarations WHERE typeId=? "
        "ORDER "
        "BY name",
        database};
    mutable Sqlite::ReadStatement<3, 1> selectEnumeratorDeclarationStatement{
        "SELECT json_each.key, json_each.value, json_each.type!='null' FROM "
        "enumerationDeclarations, json_each(enumerationDeclarations.enumeratorDeclarations) "
        "WHERE "
        "enumerationDeclarationId=?",
        database};
    Sqlite::WriteStatement<3> insertEnumerationDeclarationStatement{
        "INSERT INTO enumerationDeclarations(typeId, name, enumeratorDeclarations) VALUES(?1, "
        "?2, "
        "?3)",
        database};
    Sqlite::WriteStatement<2> updateEnumerationDeclarationStatement{
        "UPDATE enumerationDeclarations SET  enumeratorDeclarations=?2 WHERE "
        "enumerationDeclarationId=?1",
        database};
    Sqlite::WriteStatement<1> deleteEnumerationDeclarationStatement{
        "DELETE FROM enumerationDeclarations WHERE enumerationDeclarationId=?", database};
    mutable Sqlite::ReadStatement<1, 2> selectTypeIdBySourceIdAndNameStatement{
        "SELECT typeId FROM types WHERE sourceId=?1 and name=?2", database};
    mutable Sqlite::ReadStatement<1, 3> selectTypeIdByModuleIdsAndExportedNameStatement{
        "SELECT typeId FROM exportedTypeNames WHERE moduleId IN carray(?1, ?2, 'int32') AND "
        "name=?3",
        database};
    mutable Sqlite::ReadStatement<6> selectAllDocumentImportsStatement{
        "SELECT moduleId, majorVersion, minorVersion, sourceId, contextSourceId, alias "
        "FROM documentImports ",
        database};
    mutable Sqlite::ReadStatement<7, 2> selectDocumentImportForContextSourceIdStatement{
        "SELECT importId, sourceId, moduleId, majorVersion, minorVersion, contextSourceId, alias "
        "FROM documentImports "
        "WHERE contextSourceId IN carray(?1) AND kind=?2 "
        "ORDER BY sourceId, moduleId, alias, majorVersion, minorVersion",
        database};
    Sqlite::ReadWriteStatement<1, 9> insertDocumentImportWithVersionStatement{
        "INSERT INTO documentImports(sourceId, moduleId, sourceModuleId, kind, majorVersion, "
        "  minorVersion, parentImportId, contextSourceId, alias) "
        "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, nullif(?9, '')) "
        "RETURNING importId",
        database};
    Sqlite::WriteStatement<1> deleteDocumentImportStatement{
        "DELETE FROM documentImports WHERE importId=?1", database};
    Sqlite::WriteStatement<2> deleteDocumentImportsWithParentImportIdStatement{
        "DELETE FROM documentImports WHERE sourceId=?1 AND parentImportId=?2", database};
    mutable Sqlite::ReadStatement<1, 2> selectPropertyDeclarationIdByTypeIdAndNameStatement{
        "SELECT propertyDeclarationId "
        "FROM propertyDeclarations "
        "WHERE typeId=?1 AND name=?2 "
        "LIMIT 1",
        database};
    Sqlite::WriteStatement<2> updateAliasIdPropertyDeclarationStatement{
        "UPDATE propertyDeclarations SET aliasPropertyDeclarationId=?2  WHERE "
        "aliasPropertyDeclarationId=?1",
        database};
    Sqlite::WriteStatement<2> updateAliasPropertyDeclarationByAliasPropertyDeclarationIdStatement{
        "UPDATE propertyDeclarations SET propertyTypeId=new.propertyTypeId, "
        "propertyTraits=new.propertyTraits, aliasPropertyDeclarationId=?1 FROM (SELECT "
        "propertyTypeId, propertyTraits FROM propertyDeclarations WHERE "
        "propertyDeclarationId=?1) "
        "AS new WHERE aliasPropertyDeclarationId=?2",
        database};
    Sqlite::WriteStatement<1> updateAliasPropertyDeclarationToNullStatement{
        "UPDATE propertyDeclarations SET aliasPropertyDeclarationId=NULL, propertyTypeId=NULL, "
        "propertyTraits=NULL WHERE propertyDeclarationId=? AND (aliasPropertyDeclarationId IS "
        "NOT "
        "NULL OR propertyTypeId IS NOT NULL OR propertyTraits IS NOT NULL)",
        database};
    Sqlite::ReadStatement<5, 1> selectAliasPropertiesDeclarationForPropertiesWithTypeIdStatement{
        "  SELECT alias.typeId, "
        "         alias.propertyDeclarationId, "
        "         alias.aliasPropertyImportedTypeNameId, "
        "         alias.aliasPropertyDeclarationId, "
        "         alias.aliasPropertyDeclarationTailId "
        "  FROM propertyDeclarations AS alias "
        "    JOIN propertyDeclarations AS target "
        "      ON alias.aliasPropertyDeclarationId=target.propertyDeclarationId "
        "         OR alias.aliasPropertyDeclarationTailId=target.propertyDeclarationId "
        "  WHERE alias.propertyTypeId=?1 "
        "UNION ALL "
        "  SELECT alias.typeId, "
        "         alias.propertyDeclarationId, "
        "         alias.aliasPropertyImportedTypeNameId, "
        "         alias.aliasPropertyDeclarationId, "
        "         alias.aliasPropertyDeclarationTailId "
        "  FROM propertyDeclarations AS alias "
        "    JOIN propertyDeclarations AS target "
        "      ON alias.aliasPropertyDeclarationId=target.propertyDeclarationId "
        "         OR alias.aliasPropertyDeclarationTailId=target.propertyDeclarationId "
        "  WHERE target.typeId=?1 "
        "UNION ALL "
        "  SELECT alias.typeId, "
        "         alias.propertyDeclarationId, "
        "         alias.aliasPropertyImportedTypeNameId, "
        "         alias.aliasPropertyDeclarationId, alias.aliasPropertyDeclarationTailId "
        "  FROM propertyDeclarations AS alias "
        "    JOIN propertyDeclarations AS target "
        "      ON alias.aliasPropertyDeclarationId=target.propertyDeclarationId "
        "         OR alias.aliasPropertyDeclarationTailId=target.propertyDeclarationId "
        "  WHERE alias.aliasPropertyImportedTypeNameId IN "
        "    (SELECT importedTypeNameId "
        "     FROM exportedTypeNames JOIN importedTypeNames USING(name) "
        "     WHERE typeId=?1)",
        database};
    Sqlite::ReadStatement<3, 1> selectAliasPropertiesDeclarationForPropertiesWithAliasIdStatement{
        "WITH RECURSIVE "
        "  properties(propertyDeclarationId, propertyImportedTypeNameId, typeId, "
        "    aliasPropertyDeclarationId) AS ("
        "      SELECT propertyDeclarationId, propertyImportedTypeNameId, typeId, "
        "        aliasPropertyDeclarationId FROM propertyDeclarations WHERE "
        "        aliasPropertyDeclarationId=?1"
        "    UNION ALL "
        "      SELECT pd.propertyDeclarationId, pd.propertyImportedTypeNameId, pd.typeId, "
        "        pd.aliasPropertyDeclarationId FROM propertyDeclarations AS pd JOIN properties "
        "AS "
        "        p ON pd.aliasPropertyDeclarationId=p.propertyDeclarationId)"
        "SELECT propertyDeclarationId, propertyImportedTypeNameId, aliasPropertyDeclarationId "
        "  FROM properties",
        database};
    Sqlite::ReadWriteStatement<3, 1> updatesPropertyDeclarationPropertyTypeToNullStatement{
        "UPDATE propertyDeclarations SET propertyTypeId=NULL WHERE propertyTypeId=?1 AND "
        "aliasPropertyDeclarationId IS NULL RETURNING typeId, propertyDeclarationId, "
        "propertyImportedTypeNameId",
        database};
    Sqlite::ReadWriteStatement<3, 2> selectPropertyDeclarationForPrototypeIdAndTypeNameStatement{
        "SELECT typeId, propertyDeclarationId, propertyImportedTypeNameId "
        "FROM propertyDeclarations "
        "WHERE propertyTypeId IS ?2 "
        "  AND propertyImportedTypeNameId IN (SELECT importedTypeNameId "
        "    FROM "
        "    importedTypeNames WHERE name=?1)",
        database};
    Sqlite::ReadWriteStatement<5, 2> selectAliasPropertyDeclarationForPrototypeIdAndTypeNameStatement{
        "SELECT alias.typeId, "
        "       alias.propertyDeclarationId, "
        "       alias.aliasPropertyImportedTypeNameId, "
        "       alias.aliasPropertyDeclarationId, "
        "       alias.aliasPropertyDeclarationTailId "
        "FROM propertyDeclarations AS alias "
        "  JOIN propertyDeclarations AS target "
        "  ON alias.aliasPropertyDeclarationId=target.propertyDeclarationId "
        "    OR alias.aliasPropertyDeclarationTailId=target.propertyDeclarationId "
        "WHERE alias.propertyTypeId IS ?2 "
        "  AND target.propertyImportedTypeNameId IN "
        "    (SELECT importedTypeNameId "
        "     FROM importedTypeNames "
        "     WHERE name=?1)",
        database};
    mutable Sqlite::ReadStatement<1, 1> selectPropertyNameStatement{
        "SELECT name FROM propertyDeclarations WHERE propertyDeclarationId=?", database};
    Sqlite::WriteStatement<2> updatePropertyDeclarationTypeStatement{
        "UPDATE propertyDeclarations SET propertyTypeId=?2 WHERE propertyDeclarationId=?1", database};
    Sqlite::ReadWriteStatement<1, 2> updateBasesByBaseIdStatement{
        "UPDATE bases SET baseId=?2 WHERE baseId=?1 RETURNING typeId", database};
    Sqlite::WriteStatement<2> updatePrototypesByPrototypeIdStatement{
        "UPDATE prototypes SET prototypeId=?2 WHERE prototypeId=?1", database};
    mutable Sqlite::ReadStatement<2, 1> selectBaseNamesByTypeIdStatement{
        "SELECT prototypeNameId, extensionNameId FROM types WHERE typeId=?1", database};
    Sqlite::ReadStatement<3, 2> selectTypeIdAndBaseNameIdForBaseIdAndTypeNameStatement{
        "SELECT typeId, prototypeNameId, extensionNameId "
        "FROM types JOIN bases USING(typeId) JOIN importedTypeNames AS itn "
        "WHERE baseId=?2 AND itn.name=?1 "
        "  AND (importedTypeNameId=prototypeNameId OR importedTypeNameId=extensionNameId)",
        database};
    mutable Sqlite::ReadStatement<1, 1> selectPrototypeAndExtensionIdsStatement{
        "WITH RECURSIVE "
        "  prototypes(typeId) AS ( "
        "      SELECT baseId FROM bases WHERE typeId=?1 "
        "    UNION ALL "
        "      SELECT baseId "
        "      FROM bases JOIN prototypes USING(typeId)) "
        "SELECT typeId FROM prototypes",
        database};
    mutable Sqlite::ReadStatement<1, 1> selectPrototypeIdsStatement{
        "WITH RECURSIVE "
        "  typeChain(typeId) AS ( "
        "      SELECT prototypeId FROM prototypes WHERE typeId=?1 "
        "    UNION ALL "
        "      SELECT prototypeId "
        "      FROM prototypes JOIN typeChain USING(typeId)) "
        "SELECT typeId FROM typeChain",
        database};
    Sqlite::WriteStatement<3> updatePropertyDeclarationAliasIdAndTypeNameIdStatement{
        "UPDATE propertyDeclarations "
        "SET aliasPropertyDeclarationId=?2, "
        "    propertyImportedTypeNameId=?3 "
        "WHERE propertyDeclarationId=?1",
        database};
    Sqlite::WriteStatement<1> updatePropertiesDeclarationValuesOfAliasStatement{
        "WITH RECURSIVE "
        "  properties(propertyDeclarationId, propertyTypeId, propertyTraits) AS ( "
        "      SELECT aliasPropertyDeclarationId, propertyTypeId, propertyTraits FROM "
        "       propertyDeclarations WHERE propertyDeclarationId=?1 "
        "   UNION ALL "
        "      SELECT pd.aliasPropertyDeclarationId, pd.propertyTypeId, pd.propertyTraits FROM "
        "        propertyDeclarations AS pd JOIN properties USING(propertyDeclarationId)) "
        "UPDATE propertyDeclarations AS pd SET propertyTypeId=p.propertyTypeId, "
        "  propertyTraits=p.propertyTraits "
        "FROM properties AS p "
        "WHERE pd.propertyDeclarationId=?1 AND p.propertyDeclarationId IS NULL AND "
        "  (pd.propertyTypeId IS NOT p.propertyTypeId OR pd.propertyTraits IS NOT "
        "  p.propertyTraits)",
        database};
    Sqlite::WriteStatement<1> updatePropertyDeclarationAliasIdToNullStatement{
        "UPDATE propertyDeclarations SET aliasPropertyDeclarationId=NULL  WHERE "
        "propertyDeclarationId=?1",
        database};
    mutable Sqlite::ReadStatement<1, 1> selectPropertyDeclarationIdsForAliasChainStatement{
        "WITH RECURSIVE "
        "  properties(propertyDeclarationId) AS ( "
        "    SELECT aliasPropertyDeclarationId FROM propertyDeclarations WHERE "
        "     propertyDeclarationId=?1 "
        "   UNION ALL "
        "     SELECT aliasPropertyDeclarationId FROM propertyDeclarations JOIN properties "
        "       USING(propertyDeclarationId)) "
        "SELECT propertyDeclarationId FROM properties",
        database};
    mutable Sqlite::ReadStatement<3> selectAllFileStatusesStatement{
        "SELECT sourceId, size, lastModified FROM fileStatuses ORDER BY sourceId", database};
    mutable Sqlite::ReadStatement<3, 1> selectFileStatusesForSourceIdsStatement{
        "SELECT sourceId, size, lastModified "
        "FROM fileStatuses "
        "WHERE sourceId IN carray(?1) "
        "ORDER BY sourceId",
        database};
    mutable Sqlite::ReadStatement<3, 1> selectFileStatusForSourceIdStatement{
        "SELECT sourceId, size, lastModified "
        "FROM fileStatuses "
        "WHERE sourceId=?1",
        database};
    Sqlite::WriteStatement<3> insertFileStatusStatement{
        "INSERT INTO fileStatuses(sourceId, size, lastModified) VALUES(?1, ?2, ?3)", database};
    Sqlite::WriteStatement<1> deleteFileStatusStatement{
        "DELETE FROM fileStatuses WHERE sourceId=?1", database};
    Sqlite::WriteStatement<3> updateFileStatusStatement{
        "UPDATE fileStatuses SET size=?2, lastModified=?3 WHERE sourceId=?1", database};
    Sqlite::ReadStatement<1, 1> selectTypeIdBySourceIdStatement{
        "SELECT typeId FROM types WHERE sourceId=?", database};
    mutable Sqlite::ReadStatement<1, 3> selectImportedTypeNameIdStatement{
        "SELECT importedTypeNameId FROM importedTypeNames WHERE kind=?1 AND "
        "importOrSourceId=?2 "
        "AND name=?3 LIMIT 1",
        database};
    mutable Sqlite::ReadWriteStatement<1, 3> insertImportedTypeNameIdStatement{
        "INSERT INTO importedTypeNames(kind, importOrSourceId, name) "
        "VALUES (?1, ?2, ?3) "
        "RETURNING importedTypeNameId",
        database};
    mutable Sqlite::ReadStatement<1, 2> selectImportIdBySourceIdAndAliasStatement{
        "SELECT importId "
        "FROM documentImports "
        "WHERE sourceId=?1 AND alias=?2 "
        "LIMIT 1",
        database};
    mutable Sqlite::ReadStatement<1, 5> selectImportIdBySourceIdAndModuleIdAndVersionAndAliasStatement{
        "SELECT importId "
        "FROM documentImports "
        "WHERE sourceId=?1 "
        "  AND moduleId=?2 "
        "  AND alias IS nullif(?5, '') "
        "  AND majorVersion=?3 "
        "  AND minorVersion=?4 "
        "LIMIT 1",
        database};
    mutable Sqlite::ReadStatement<1, 1> selectKindFromImportedTypeNamesStatement{
        "SELECT kind FROM importedTypeNames WHERE importedTypeNameId=?1", database};
    mutable Sqlite::ReadStatement<1, 1> selectNameFromImportedTypeNamesStatement{
        "SELECT name FROM importedTypeNames WHERE importedTypeNameId=?1", database};
    mutable Sqlite::ReadStatement<1, 1> selectTypeIdForQualifiedImportedTypeNameStatement{
        "SELECT typeId "
        "FROM importedTypeNames AS itn "
        "  JOIN documentImports AS di ON importOrSourceId=di.importId "
        "  JOIN documentImports AS di2 ON di.sourceId=di2.sourceId "
        "    AND di.moduleId=di2.sourceModuleId "
        "  JOIN exportedTypeNames AS etn ON di2.moduleId=etn.moduleId "
        "WHERE itn.kind=2 "
        "  AND importedTypeNameId=?1 "
        "  AND itn.name=etn.name "
        "  AND (di.majorVersion=0xFFFFFFFF "
        "    OR (di.majorVersion=etn.majorVersion "
        "      AND (di.minorVersion=0xFFFFFFFF OR di.minorVersion>=etn.minorVersion))) "
        "ORDER BY etn.majorVersion DESC, etn.minorVersion DESC "
        "LIMIT 1",
        database};
    mutable Sqlite::ReadStatement<1, 1> selectTypeIdForImportedTypeNameStatement{
        "SELECT typeId FROM importedTypeNames AS itn "
        "  JOIN exportedTypeNames AS etn USING(name) "
        "  JOIN documentImports AS di ON importOrSourceId=sourceId "
        "WHERE  importedTypeNameId=?1 "
        "  AND itn.kind=1 "
        "  AND etn.moduleId=di.moduleId "
        "  AND di.alias IS NULL "
        "  AND (di.majorVersion=0xFFFFFFFF "
        "    OR (di.majorVersion=etn.majorVersion "
        "      AND (di.minorVersion=0xFFFFFFFF OR di.minorVersion>=etn.minorVersion))) "
        "ORDER BY di.kind, etn.majorVersion DESC, etn.minorVersion DESC "
        "LIMIT 1",
        database};
    mutable Sqlite::ReadStatement<5, 1> selectExportedTypeNameForQualifiedImportedTypeNameStatement{
        "SELECT etn.moduleId, etn.typeId, etn.name, etn.majorVersion, etn.minorVersion "
        "FROM importedTypeNames AS itn "
        "  JOIN documentImports AS di ON importOrSourceId=di.importId "
        "  JOIN documentImports AS di2 ON di.sourceId=di2.sourceId "
        "    AND di.moduleId=di2.sourceModuleId "
        "  JOIN exportedTypeNames AS etn ON di2.moduleId=etn.moduleId "
        "WHERE itn.kind=2 "
        "  AND importedTypeNameId=?1 "
        "  AND itn.name=etn.name "
        "  AND (di.majorVersion=0xFFFFFFFF "
        "    OR (di.majorVersion=etn.majorVersion "
        "      AND (di.minorVersion=0xFFFFFFFF OR di.minorVersion>=etn.minorVersion))) "
        "ORDER BY etn.majorVersion DESC, etn.minorVersion DESC "
        "LIMIT 1",
        database};
    mutable Sqlite::ReadStatement<5, 1> selectExportedTypeNameForImportedTypeNameStatement{
        "SELECT etn.moduleId, etn.typeId, etn.name, etn.majorVersion, etn.minorVersion "
        "FROM importedTypeNames AS itn "
        "  JOIN exportedTypeNames AS etn USING(name) "
        "  JOIN documentImports AS di ON importOrSourceId=sourceId "
        "WHERE  importedTypeNameId=?1 "
        "  AND itn.kind=1 "
        "  AND etn.moduleId=di.moduleId "
        "  AND (di.majorVersion=0xFFFFFFFF "
        "    OR (di.majorVersion=etn.majorVersion "
        "      AND (di.minorVersion=0xFFFFFFFF OR di.minorVersion>=etn.minorVersion))) "
        "ORDER BY di.kind, etn.majorVersion DESC, etn.minorVersion DESC "
        "LIMIT 1",
        database};
    mutable Sqlite::ReadStatement<6, 1> selectExportedTypesForSourceIdsStatement{
        "SELECT moduleId, "
        "       name, "
        "       majorVersion, "
        "       minorVersion, "
        "       typeId,"
        "       contextSourceId "
        "FROM exportedTypeNames "
        "WHERE contextSourceId in carray(?1) "
        "ORDER BY name, moduleId, majorVersion, minorVersion",
        database};
    Sqlite::WriteStatement<6> insertExportedTypeNamesStatement{"INSERT INTO exportedTypeNames( "
                                                               "  moduleId, "
                                                               "  name, "
                                                               "  majorVersion, "
                                                               "  minorVersion, "
                                                               "  typeId, "
                                                               "  contextSourceId) "
                                                               "VALUES(?1, ?2, ?3, ?4, ?5, ?6)",
                                                               database};
    Sqlite::WriteStatement<4> deleteExportedTypeNameStatement{
        "DELETE FROM exportedTypeNames "
        "WHERE name=?2 AND moduleId=?1 AND majorVersion=?3 AND minorVersion=?4",
        database};
    Sqlite::WriteStatement<5> updateExportedTypeNameTypeIdStatement{
        "UPDATE exportedTypeNames "
        "SET typeId=?5 "
        "WHERE name=?2 AND moduleId=?1 AND majorVersion=?3 AND minorVersion=?4",
        database};
    Sqlite::WriteStatement<5> updateExportedTypeNameContextSourceIdStatement{
        "UPDATE exportedTypeNames "
        "SET contextSourceId=?5 "
        "WHERE name=?2 AND moduleId=?1 AND majorVersion=?3 AND minorVersion=?4",
        database};
    mutable Sqlite::ReadStatement<4, 1> selectProjectEntryInfosForDirectoryIdsStatement{
        "SELECT contextSourceId, sourceId, moduleId, fileType "
        "FROM projectEntryInfos "
        "WHERE contextSourceId IN carray(?1) "
        "ORDER BY contextSourceId, sourceId",
        database};
    Sqlite::WriteStatement<4> insertProjectEntryInfoStatement{
        "INSERT INTO projectEntryInfos(contextSourceId, sourceId, "
        "moduleId, fileType) VALUES(?1, ?2, ?3, ?4)",
        database};
    Sqlite::WriteStatement<2> deleteProjectEntryInfoStatement{
        "DELETE FROM projectEntryInfos WHERE contextSourceId=?1 AND sourceId=?2", database};
    Sqlite::WriteStatement<4> updateProjectEntryInfoStatement{
        "UPDATE projectEntryInfos SET moduleId=?3, fileType=?4 WHERE contextSourceId=?1 AND "
        "sourceId=?2",
        database};
    mutable Sqlite::ReadStatement<4, 1> selectProjectEntryInfosForContextSourceIdStatement{
        "SELECT contextSourceId, sourceId, moduleId, fileType "
        "FROM projectEntryInfos "
        "WHERE contextSourceId=?1",
        database};
    mutable Sqlite::ReadStatement<4, 2> selectProjectEntryInfosForContextSourceIdAndFileTypeStatement{
        "SELECT contextSourceId, sourceId, moduleId, fileType "
        "FROM projectEntryInfos "
        "WHERE contextSourceId=?1 AND fileType=?2",
        database};
    mutable Sqlite::ReadStatement<1, 2> selectProjectEntryInfosSourceIdsForContextSourceIdAndFileTypeStatement{
        "SELECT sourceId FROM projectEntryInfos WHERE contextSourceId=?1 AND fileType=?2", database};
    mutable Sqlite::ReadStatement<4, 1> selectProjectEntryInfoForSourceIdStatement{
        "SELECT contextSourceId, sourceId, moduleId, fileType FROM projectEntryInfos WHERE "
        "sourceId=?1 LIMIT 1",
        database};
    mutable Sqlite::ReadStatement<1, 1> selectTypeIdsForSourceIdsStatement{
        "SELECT typeId FROM types WHERE sourceId IN carray(?1)", database};
    mutable Sqlite::ReadStatement<6, 1> selectModuleExportedImportsForSourceIdStatement{
        "SELECT moduleExportedImportId, "
        "       moduleId, "
        "       exportedModuleId, "
        "       majorVersion, "
        "       minorVersion, "
        "       isAutoVersion "
        "FROM moduleExportedImports "
        "WHERE moduleId IN carray(?1) "
        "ORDER BY moduleId, exportedModuleId",
        database};
    Sqlite::WriteStatement<5> insertModuleExportedImportWithVersionStatement{
        "INSERT INTO moduleExportedImports(moduleId, exportedModuleId, isAutoVersion, "
        "majorVersion, minorVersion) VALUES (?1, ?2, ?3, ?4, ?5)",
        database};
    Sqlite::WriteStatement<1> deleteModuleExportedImportStatement{
        "DELETE FROM moduleExportedImports WHERE moduleExportedImportId=?1", database};
    mutable Sqlite::ReadStatement<3, 3> selectModuleExportedImportsForModuleIdStatement{
        "WITH RECURSIVE "
        "  imports(moduleId, majorVersion, minorVersion, moduleExportedImportId) AS ( "
        "      SELECT exportedModuleId, "
        "             iif(isAutoVersion=1, ?2, majorVersion), "
        "             iif(isAutoVersion=1, ?3, minorVersion), "
        "             moduleExportedImportId "
        "        FROM moduleExportedImports WHERE moduleId=?1 "
        "    UNION ALL "
        "      SELECT exportedModuleId, "
        "             iif(mei.isAutoVersion=1, i.majorVersion, mei.majorVersion), "
        "             iif(mei.isAutoVersion=1, i.minorVersion, mei.minorVersion), "
        "             mei.moduleExportedImportId "
        "        FROM moduleExportedImports AS mei JOIN imports AS i USING(moduleId)) "
        "SELECT DISTINCT moduleId, majorVersion, minorVersion "
        "FROM imports",
        database};
    mutable Sqlite::ReadStatement<1, 1> selectLocalPropertyDeclarationIdsForTypeStatement{
        "SELECT propertyDeclarationId "
        "FROM propertyDeclarations "
        "WHERE typeId=? "
        "ORDER BY propertyDeclarationId",
        database};
    mutable Sqlite::ReadStatement<1, 2> selectLocalPropertyDeclarationIdForTypeAndPropertyNameStatement{
        "SELECT propertyDeclarationId "
        "FROM propertyDeclarations "
        "WHERE typeId=?1 AND name=?2 LIMIT 1",
        database};
    mutable Sqlite::ReadStatement<4, 1> selectPropertyDeclarationForPropertyDeclarationIdStatement{
        "SELECT typeId, name, propertyTraits, propertyTypeId "
        "FROM propertyDeclarations "
        "WHERE propertyDeclarationId=?1 LIMIT 1",
        database};
    mutable Sqlite::ReadStatement<2, 1> selectPropertyDeclarationNameAndTypeIdForPropertyDeclarationIdStatement{
        "SELECT name, typeId "
        "FROM propertyDeclarations "
        "WHERE propertyDeclarationId=?1 "
        "LIMIT 1",
        database};
    mutable Sqlite::ReadStatement<1, 1> selectSignalDeclarationNamesForTypeStatement{
        "WITH RECURSIVE "
        "  prototypes(typeId) AS ( "
        "      VALUES(?1) "
        "    UNION ALL "
        "      SELECT baseId "
        "      FROM bases JOIN prototypes USING(typeId)) "
        "SELECT name FROM prototypes JOIN signalDeclarations USING(typeId) ORDER BY name",
        database};
    mutable Sqlite::ReadStatement<1, 1> selectFuncionDeclarationNamesForTypeStatement{
        "WITH RECURSIVE "
        "  prototypes(typeId) AS ( "
        "      VALUES(?1) "
        "    UNION ALL "
        "      SELECT baseId "
        "      FROM bases JOIN prototypes USING(typeId)) "
        "SELECT name FROM prototypes JOIN functionDeclarations USING(typeId) ORDER BY name",
        database};
    mutable Sqlite::ReadStatement<2> selectTypesWithDefaultPropertyStatement{
        "SELECT typeId, defaultPropertyId FROM types ORDER BY typeId", database};
    Sqlite::WriteStatement<2> updateDefaultPropertyIdStatement{
        "UPDATE types SET defaultPropertyId=?2 WHERE typeId=?1", database};
    Sqlite::WriteStatement<1> updateDefaultPropertyIdToNullStatement{
        "UPDATE types SET defaultPropertyId=NULL WHERE defaultPropertyId=?1", database};
    mutable Sqlite::ReadStatement<3, 1> selectInfoTypeByTypeIdStatement{
        "SELECT sourceId, traits, annotationTraits FROM types WHERE typeId=?", database};
    mutable Sqlite::ReadStatement<1, 1> selectSourceIdByTypeIdStatement{
        "SELECT sourceId FROM types WHERE typeId=?", database};
    mutable Sqlite::ReadStatement<1, 1> selectPrototypeAnnotationTraitsByTypeIdStatement{
        "SELECT annotationTraits "
        "FROM types "
        "WHERE typeId=(SELECT baseId FROM bases WHERE typeId=?) AND annotationTraits IS NOT NULL "
        "LIMIT 1",
        database};
    mutable Sqlite::ReadStatement<1, 1> selectDefaultPropertyDeclarationIdStatement{
        "SELECT defaultPropertyId FROM types WHERE typeId=?", database};
    Sqlite::WriteStatement<2> upsertPropertyEditorPathIdStatement{
        "INSERT INTO propertyEditorPaths(typeId, pathSourceId) VALUES(?1, ?2) ON CONFLICT DO "
        "UPDATE SET pathSourceId=excluded.pathSourceId WHERE pathSourceId IS NOT "
        "excluded.pathSourceId",
        database};
    mutable Sqlite::ReadStatement<1, 1> selectPropertyEditorPathIdStatement{
        "SELECT pathSourceId FROM propertyEditorPaths WHERE typeId=?", database};
    mutable Sqlite::ReadStatement<3, 1> selectPropertyEditorPathsForForSourceIdsStatement{
        "SELECT typeId, pathSourceId, directoryId "
        "FROM propertyEditorPaths "
        "WHERE directoryId IN carray(?1) "
        "ORDER BY typeId",
        database};
    Sqlite::WriteStatement<3> insertPropertyEditorPathStatement{
        "INSERT INTO propertyEditorPaths(typeId, pathSourceId, directoryId) VALUES (?1, ?2, "
        "?3)",
        database};
    Sqlite::WriteStatement<3> updatePropertyEditorPathsStatement{
        "UPDATE propertyEditorPaths "
        "SET pathSourceId=?2, directoryId=?3 "
        "WHERE typeId=?1",
        database};
    Sqlite::WriteStatement<1> deletePropertyEditorPathStatement{
        "DELETE FROM propertyEditorPaths WHERE typeId=?1", database};
    mutable Sqlite::ReadStatement<5, 1> selectTypeAnnotationsForSourceIdsStatement{
        "SELECT typeId, typeName, iconPath, itemLibrary, hints FROM typeAnnotations WHERE "
        "sourceId IN carray(?1) ORDER BY typeId",
        database};
    Sqlite::WriteStatement<7> insertTypeAnnotationStatement{
        "INSERT INTO "
        "  typeAnnotations(typeId, sourceId, directoryId, typeName, iconPath, itemLibrary, "
        "  hints) "
        "VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7)",
        database};
    Sqlite::WriteStatement<5> updateTypeAnnotationStatement{
        "UPDATE typeAnnotations "
        "SET typeName=?2, iconPath=?3, itemLibrary=?4, hints=?5 "
        "WHERE typeId=?1",
        database};
    Sqlite::WriteStatement<1> deleteTypeAnnotationStatement{
        "DELETE FROM typeAnnotations WHERE typeId=?1", database};
    mutable Sqlite::ReadStatement<1, 1> selectTypeIconPathStatement{
        "SELECT iconPath FROM typeAnnotations WHERE typeId=?1", database};
    mutable Sqlite::ReadStatement<2, 1> selectTypeHintsStatement{
        "SELECT hints.key, hints.value "
        "FROM typeAnnotations, json_each(typeAnnotations.hints) AS hints "
        "WHERE typeId=?1 AND hints IS NOT NULL",
        database};
    mutable Sqlite::ReadStatement<1, 1> selectTypeAnnotationSourceIdsStatement{
        "SELECT sourceId FROM typeAnnotations WHERE directoryId=?1 ORDER BY sourceId", database};
    mutable Sqlite::ReadStatement<1, 0> selectTypeAnnotationDirectoryIdsStatement{
        "SELECT DISTINCT directoryId FROM typeAnnotations ORDER BY directoryId", database};
    mutable Sqlite::ReadStatement<10> selectItemLibraryEntriesStatement{
        "SELECT typeId, typeName, i.value->>'$.name', i.value->>'$.iconPath', "
        "  i.value->>'$.category',  i.value->>'$.import', i.value->>'$.toolTip', "
        "  i.value->>'$.properties', i.value->>'$.extraFilePaths', i.value->>'$.templatePath' "
        "FROM typeAnnotations AS ta , json_each(ta.itemLibrary) AS i "
        "WHERE ta.itemLibrary IS NOT NULL",
        database};
    mutable Sqlite::ReadStatement<10, 1> selectItemLibraryEntriesByTypeIdStatement{
        "SELECT typeId, typeName, i.value->>'$.name', i.value->>'$.iconPath', "
        "  i.value->>'$.category', i.value->>'$.import', i.value->>'$.toolTip', "
        "  i.value->>'$.properties', i.value->>'$.extraFilePaths', i.value->>'$.templatePath' "
        "FROM typeAnnotations AS ta, json_each(ta.itemLibrary) AS i "
        "WHERE typeId=?1 AND ta.itemLibrary IS NOT NULL",
        database};
    mutable Sqlite::ReadStatement<10, 1> selectItemLibraryEntriesBySourceIdStatement{
        "SELECT typeId, typeName, i.value->>'$.name', i.value->>'$.iconPath', "
        "i.value->>'$.category', "
        "  i.value->>'$.import', i.value->>'$.toolTip', i.value->>'$.properties', "
        "  i.value->>'$.extraFilePaths', i.value->>'$.templatePath' "
        "FROM typeAnnotations, json_each(typeAnnotations.itemLibrary) AS i "
        "WHERE typeId IN (SELECT DISTINCT typeId "
        "                 FROM documentImports AS di JOIN exportedTypeNames "
        "                   USING(moduleId) "
        "                 WHERE di.sourceId=?)",
        database};
    mutable Sqlite::ReadStatement<4, 1> selectDirectoryImportsItemLibraryEntriesBySourceIdStatement{
        "SELECT typeId, etn.name, moduleId, t.sourceId "
        "FROM documentImports AS di "
        "  JOIN exportedTypeNames AS etn USING(moduleId) "
        "  JOIN types AS t USING(typeId)"
        "WHERE di.sourceId=?1",
        database};
    mutable Sqlite::ReadStatement<3, 1> selectItemLibraryPropertiesStatement{
        "SELECT p.value->>0, p.value->>1, p.value->>2 FROM json_each(?1) AS p", database};
    mutable Sqlite::ReadStatement<1, 1> selectItemLibraryExtraFilePathsStatement{
        "SELECT p.value FROM json_each(?1) AS p", database};
    mutable Sqlite::ReadStatement<1, 1> selectTypeIdsByModuleIdStatement{
        "SELECT DISTINCT typeId FROM exportedTypeNames WHERE moduleId=?", database};
    mutable Sqlite::ReadStatement<1, 1> selectHeirTypeIdsStatement{
        "WITH RECURSIVE "
        "  typeSelection(typeId) AS ("
        "      SELECT typeId FROM bases WHERE baseId=?1 "
        "    UNION ALL "
        "      SELECT b.typeId "
        "      FROM bases AS b JOIN typeSelection AS ts "
        "      WHERE baseId=ts.typeId) "
        "SELECT typeId FROM typeSelection",
        database};
    mutable Sqlite::ReadStatement<6, 0> selectBrokenAliasPropertyDeclarationsStatement{
        "SELECT typeId, "
        "       propertyDeclarationId, "
        "       aliasPropertyImportedTypeNameId, "
        "       aliasPropertyDeclarationName, "
        "       aliasPropertyDeclarationTailName, "
        "       sourceId "
        "FROM propertyDeclarations JOIN types USING(typeId) "
        "WHERE "
        "    aliasPropertyImportedTypeNameId IS NOT NULL "
        "  AND "
        "    propertyImportedTypeNameId IS NULL "
        "LIMIT 1",
        database};
    mutable Sqlite::ReadStatement<1, 1> selectSingletonTypeIdsBySourceIdStatement{
        "SELECT DISTINCT typeId "
        "FROM types "
        "  JOIN exportedTypeNames USING (typeId) "
        "  JOIN documentImports AS di USING(moduleId) "
        "WHERE di.sourceId=?1 AND "
            + createSingletonTraitsExpression(),
        database};
    mutable Sqlite::ReadStatement<1> selectMaxTypeIdStatement{"SELECT max(typeId) FROM types",
                                                              database};
    mutable Sqlite::ReadStatement<1, 2> selectSourceModuleIdForSourceIdAndModuleIdStatement{
        "SELECT sourceModuleId FROM documentImports WHERE sourceId=?1 AND moduleId=?2", database};
    mutable Sqlite::ReadStatement<1, 2> selectImportIdForSourceIdAndModuleIdStatement{
        "SELECT importId FROM documentImports WHERE sourceId=?1 AND moduleId=?2", database};
    mutable Sqlite::ReadStatement<1, 1> selectParentImportIdForImportIdStatement{
        "SELECT parentImportId FROM documentImports WHERE importId=?1", database};
    mutable Sqlite::ReadStatement<6, 1> selectDocumentImportByImportIdStatement{
        "SELECT moduleId, majorVersion, minorVersion, sourceId, contextSourceId, alias "
        "FROM documentImports "
        "WHERE importId=?1",
        database};
};

class ProjectStorage::Initializer
{
public:
    Initializer(Database &database, bool isInitialized)
    {
        if (!isInitialized) {
            createTypesAndePropertyDeclarationsTables(database);
            createBasesTable(database);
            createPrototypeTable(database);
            createExportedTypeNamesTable(database);
            createImportedTypeNamesTable(database);
            createEnumerationsTable(database);
            createFunctionsTable(database);
            createSignalsTable(database);
            createModuleExportedImportsTable(database);
            createDocumentImportsTable(database);
            createFileStatusesTable(database);
            createProjectEntryInfosTable(database);
            createPropertyEditorPathsTable(database);
            createTypeAnnotionsTable(database);
        }
        database.setIsInitialized(true);
    }

    void createTypesAndePropertyDeclarationsTables(Database &database)
    {
        Sqlite::StrictTable typesTable;
        typesTable.setUseIfNotExists(true);
        typesTable.setName("types");
        typesTable.addColumn("typeId", Sqlite::StrictColumnType::Integer, {Sqlite::PrimaryKey{}});
        auto &sourceIdColumn = typesTable.addColumn("sourceId", Sqlite::StrictColumnType::Integer);
        auto &typesNameColumn = typesTable.addColumn("name", Sqlite::StrictColumnType::Text);
        auto &traitsColumn = typesTable.addColumn("traits", Sqlite::StrictColumnType::Integer);
        auto &prototypeNameIdColumn = typesTable.addColumn("prototypeNameId",
                                                           Sqlite::StrictColumnType::Integer);
        auto &extensionNameIdColumn = typesTable.addColumn("extensionNameId",
                                                           Sqlite::StrictColumnType::Integer);
        auto &defaultPropertyIdColumn = typesTable.addColumn("defaultPropertyId",
                                                             Sqlite::StrictColumnType::Integer);
        typesTable.addColumn("annotationTraits", Sqlite::StrictColumnType::Integer);
        typesTable.addUniqueIndex({sourceIdColumn, typesNameColumn});
        typesTable.addIndex({defaultPropertyIdColumn});
        typesTable.addIndex({prototypeNameIdColumn});
        typesTable.addIndex({extensionNameIdColumn});
        Utils::SmallString traitsExpression = "traits & ";
        traitsExpression.append(Utils::SmallString::number(createSingletonTypeTraitMask()));
        typesTable.addIndex({traitsColumn}, traitsExpression);

        typesTable.initialize(database);

        {
            Sqlite::StrictTable propertyDeclarationTable;
            propertyDeclarationTable.setUseIfNotExists(true);
            propertyDeclarationTable.setName("propertyDeclarations");
            propertyDeclarationTable.addColumn("propertyDeclarationId",
                                               Sqlite::StrictColumnType::Integer,
                                               {Sqlite::PrimaryKey{}});
            auto &typeIdColumn = propertyDeclarationTable.addColumn("typeId");
            auto &nameColumn = propertyDeclarationTable.addColumn("name");
            auto &propertyTypeIdColumn = propertyDeclarationTable.addColumn(
                "propertyTypeId", Sqlite::StrictColumnType::Integer);
            propertyDeclarationTable.addColumn("propertyTraits", Sqlite::StrictColumnType::Integer);
            auto &propertyImportedTypeNameIdColumn = propertyDeclarationTable.addColumn(
                "propertyImportedTypeNameId", Sqlite::StrictColumnType::Integer);
            auto &aliasPropertyImportedTypeNameIdColumn = propertyDeclarationTable.addColumn(
                "aliasPropertyImportedTypeNameId", Sqlite::StrictColumnType::Integer);
            propertyDeclarationTable.addColumn("aliasPropertyDeclarationName",
                                               Sqlite::StrictColumnType::Text);
            propertyDeclarationTable.addColumn("aliasPropertyDeclarationTailName",
                                               Sqlite::StrictColumnType::Text);
            auto &aliasPropertyDeclarationIdColumn = propertyDeclarationTable.addForeignKeyColumn(
                "aliasPropertyDeclarationId",
                propertyDeclarationTable,
                Sqlite::ForeignKeyAction::NoAction,
                Sqlite::ForeignKeyAction::Restrict);
            auto &aliasPropertyDeclarationTailIdColumn = propertyDeclarationTable.addForeignKeyColumn(
                "aliasPropertyDeclarationTailId",
                propertyDeclarationTable,
                Sqlite::ForeignKeyAction::NoAction,
                Sqlite::ForeignKeyAction::Restrict);

            propertyDeclarationTable.addUniqueIndex({typeIdColumn, nameColumn});
            propertyDeclarationTable.addIndex({propertyTypeIdColumn, propertyImportedTypeNameIdColumn});
            propertyDeclarationTable.addIndex(
                {aliasPropertyImportedTypeNameIdColumn, propertyImportedTypeNameIdColumn});
            propertyDeclarationTable.addIndex({aliasPropertyDeclarationIdColumn},
                                              "aliasPropertyDeclarationId IS NOT NULL");
            propertyDeclarationTable.addIndex({aliasPropertyDeclarationTailIdColumn},
                                              "aliasPropertyDeclarationTailId IS NOT NULL");

            propertyDeclarationTable.initialize(database);
        }
    }

    void createBasesTable(Database &database)
    {
        Sqlite::StrictTable table;
        table.setUseIfNotExists(true);
        table.setUseWithoutRowId(true);
        table.setName("bases");
        auto &typeIdColumn = table.addColumn("typeId");
        auto &baseIdColumn = table.addColumn("baseId");

        table.addPrimaryKeyContraint({typeIdColumn, baseIdColumn});
        table.addIndex({baseIdColumn, typeIdColumn});

        table.initialize(database);
    }

    void createPrototypeTable(Database &database)
    {
        Sqlite::StrictTable table;
        table.setUseIfNotExists(true);
        table.setName("prototypes");
        auto &typeIdColumn = table.addColumn("typeId",
                                             Sqlite::StrictColumnType::Integer,
                                             {Sqlite::PrimaryKey{}});
        auto &prototypeIdColumn = table.addColumn("prototypeId");

        table.addIndex({typeIdColumn, prototypeIdColumn});
        table.addIndex({prototypeIdColumn, typeIdColumn});

        table.initialize(database);
    }

    void createExportedTypeNamesTable(Database &database)
    {
        Sqlite::StrictTable table;
        table.setUseIfNotExists(true);
        table.setUseWithoutRowId(true);
        table.setName("exportedTypeNames");
        auto &nameColumn = table.addColumn("name", Sqlite::StrictColumnType::Text);
        auto &moduleIdColumn = table.addColumn("moduleId", Sqlite::StrictColumnType::Integer);
        auto &typeIdColumn = table.addColumn("typeId", Sqlite::StrictColumnType::Integer);
        auto &majorVersionColumn = table.addColumn("majorVersion", Sqlite::StrictColumnType::Integer);
        auto &minorVersionColumn = table.addColumn("minorVersion", Sqlite::StrictColumnType::Integer);
        auto &contextSourceIdColumn = table.addColumn("contextSourceId",
                                                      Sqlite::StrictColumnType::Integer);

        table.addPrimaryKeyContraint(
            {nameColumn, moduleIdColumn, majorVersionColumn, minorVersionColumn});

        table.addIndex({typeIdColumn});
        table.addIndex({moduleIdColumn});
        table.addIndex({contextSourceIdColumn});

        table.initialize(database);
    }

    void createImportedTypeNamesTable(Database &database)
    {
        Sqlite::StrictTable table;
        table.setUseIfNotExists(true);
        table.setName("importedTypeNames");
        table.addColumn("importedTypeNameId",
                        Sqlite::StrictColumnType::Integer,
                        {Sqlite::PrimaryKey{}});
        auto &importOrSourceIdColumn = table.addColumn("importOrSourceId");
        auto &nameColumn = table.addColumn("name", Sqlite::StrictColumnType::Text);
        auto &kindColumn = table.addColumn("kind", Sqlite::StrictColumnType::Integer);

        table.addUniqueIndex({kindColumn, importOrSourceIdColumn, nameColumn});
        table.addIndex({nameColumn});

        table.initialize(database);
    }

    void createEnumerationsTable(Database &database)
    {
        Sqlite::StrictTable table;
        table.setUseIfNotExists(true);
        table.setName("enumerationDeclarations");
        table.addColumn("enumerationDeclarationId",
                        Sqlite::StrictColumnType::Integer,
                        {Sqlite::PrimaryKey{}});
        auto &typeIdColumn = table.addColumn("typeId", Sqlite::StrictColumnType::Integer);
        auto &nameColumn = table.addColumn("name", Sqlite::StrictColumnType::Text);
        table.addColumn("enumeratorDeclarations", Sqlite::StrictColumnType::Text);

        table.addUniqueIndex({typeIdColumn, nameColumn});

        table.initialize(database);
    }

    void createFunctionsTable(Database &database)
    {
        Sqlite::StrictTable table;
        table.setUseIfNotExists(true);
        table.setName("functionDeclarations");
        table.addColumn("functionDeclarationId",
                        Sqlite::StrictColumnType::Integer,
                        {Sqlite::PrimaryKey{}});
        auto &typeIdColumn = table.addColumn("typeId", Sqlite::StrictColumnType::Integer);
        auto &nameColumn = table.addColumn("name", Sqlite::StrictColumnType::Text);
        auto &signatureColumn = table.addColumn("signature", Sqlite::StrictColumnType::Text);
        table.addColumn("returnTypeName");

        table.addUniqueIndex({typeIdColumn, nameColumn, signatureColumn});

        table.initialize(database);
    }

    void createSignalsTable(Database &database)
    {
        Sqlite::StrictTable table;
        table.setUseIfNotExists(true);
        table.setName("signalDeclarations");
        table.addColumn("signalDeclarationId",
                        Sqlite::StrictColumnType::Integer,
                        {Sqlite::PrimaryKey{}});
        auto &typeIdColumn = table.addColumn("typeId", Sqlite::StrictColumnType::Integer);
        auto &nameColumn = table.addColumn("name", Sqlite::StrictColumnType::Text);
        auto &signatureColumn = table.addColumn("signature", Sqlite::StrictColumnType::Text);

        table.addUniqueIndex({typeIdColumn, nameColumn, signatureColumn});

        table.initialize(database);
    }

    void createModuleExportedImportsTable(Database &database)
    {
        Sqlite::StrictTable table;
        table.setUseIfNotExists(true);
        table.setName("moduleExportedImports");
        table.addColumn("moduleExportedImportId",
                        Sqlite::StrictColumnType::Integer,
                        {Sqlite::PrimaryKey{}});
        auto &moduleIdColumn = table.addColumn("moduleId", Sqlite::StrictColumnType::Integer);
        auto &sourceIdColumn = table.addColumn("exportedModuleId", Sqlite::StrictColumnType::Integer);
        table.addColumn("isAutoVersion", Sqlite::StrictColumnType::Integer);
        table.addColumn("majorVersion", Sqlite::StrictColumnType::Integer);
        table.addColumn("minorVersion", Sqlite::StrictColumnType::Integer);

        table.addUniqueIndex({sourceIdColumn, moduleIdColumn});

        table.initialize(database);
    }

    void createDocumentImportsTable(Database &database)
    {
        Sqlite::StrictTable table;
        table.setUseIfNotExists(true);
        table.setName("documentImports");
        table.addColumn("importId", Sqlite::StrictColumnType::Integer, {Sqlite::PrimaryKey{}});
        auto &sourceIdColumn = table.addColumn("sourceId", Sqlite::StrictColumnType::Integer);
        auto &contextSourceIdColumn = table.addColumn("contextSourceId",
                                                      Sqlite::StrictColumnType::Integer);
        auto &moduleIdColumn = table.addColumn("moduleId", Sqlite::StrictColumnType::Integer);
        auto &sourceModuleIdColumn = table.addColumn("sourceModuleId",
                                                     Sqlite::StrictColumnType::Integer);
        auto &kindColumn = table.addColumn("kind", Sqlite::StrictColumnType::Integer);
        auto &majorVersionColumn = table.addColumn("majorVersion", Sqlite::StrictColumnType::Integer);
        auto &minorVersionColumn = table.addColumn("minorVersion", Sqlite::StrictColumnType::Integer);
        auto &parentImportIdColumn = table.addColumn("parentImportId",
                                                     Sqlite::StrictColumnType::Integer);
        auto &aliasColumn = table.addColumn("alias", Sqlite::StrictColumnType::Text);

        table.addUniqueIndex({sourceIdColumn,
                              moduleIdColumn,
                              aliasColumn,
                              kindColumn,
                              sourceModuleIdColumn,
                              majorVersionColumn,
                              minorVersionColumn,
                              parentImportIdColumn});

        table.addUniqueIndex({sourceIdColumn, aliasColumn}, "alias IS NOT NULL");

        table.addIndex({contextSourceIdColumn, kindColumn});

        table.initialize(database);
    }

    void createFileStatusesTable(Database &database)
    {
        Sqlite::StrictTable table;
        table.setUseIfNotExists(true);
        table.setName("fileStatuses");
        table.addColumn("sourceId", Sqlite::StrictColumnType::Integer, {Sqlite::PrimaryKey{}});
        table.addColumn("size", Sqlite::StrictColumnType::Integer);
        if constexpr (sizeof(std::filesystem::file_time_type::rep) == 16)
            table.addColumn("lastModified", Sqlite::StrictColumnType::Blob);
        else
            table.addColumn("lastModified", Sqlite::StrictColumnType::Integer);

        table.initialize(database);
    }

    void createProjectEntryInfosTable(Database &database)
    {
        Sqlite::StrictTable table;
        table.setUseIfNotExists(true);
        table.setUseWithoutRowId(true);
        table.setName("projectEntryInfos");
        auto &contextSourceIdColumn = table.addColumn("contextSourceId",
                                                      Sqlite::StrictColumnType::Integer);
        auto &sourceIdColumn = table.addColumn("sourceId", Sqlite::StrictColumnType::Integer);
        table.addColumn("moduleId", Sqlite::StrictColumnType::Integer);
        auto &fileTypeColumn = table.addColumn("fileType", Sqlite::StrictColumnType::Integer);

        table.addPrimaryKeyContraint({contextSourceIdColumn, sourceIdColumn});
        table.addUniqueIndex({sourceIdColumn});
        table.addIndex({contextSourceIdColumn, fileTypeColumn});

        table.initialize(database);
    }

    void createPropertyEditorPathsTable(Database &database)
    {
        Sqlite::StrictTable table;
        table.setUseIfNotExists(true);
        table.setUseWithoutRowId(true);
        table.setName("propertyEditorPaths");
        table.addColumn("typeId", Sqlite::StrictColumnType::Integer, {Sqlite::PrimaryKey{}});
        table.addColumn("pathSourceId", Sqlite::StrictColumnType::Integer);
        auto &directoryIdColumn = table.addColumn("directoryId", Sqlite::StrictColumnType::Integer);

        table.addIndex({directoryIdColumn});

        table.initialize(database);
    }

    void createTypeAnnotionsTable(Database &database)
    {
        Sqlite::StrictTable table;
        table.setUseIfNotExists(true);
        table.setUseWithoutRowId(true);
        table.setName("typeAnnotations");
        auto &typeIdColumn = table.addColumn("typeId",
                                             Sqlite::StrictColumnType::Integer,
                                             {Sqlite::PrimaryKey{}});
        auto &sourceIdColumn = table.addColumn("sourceId", Sqlite::StrictColumnType::Integer);
        auto &directoryIdColumn = table.addColumn("directoryId", Sqlite::StrictColumnType::Integer);
        table.addColumn("typeName", Sqlite::StrictColumnType::Text);
        table.addColumn("iconPath", Sqlite::StrictColumnType::Text);
        table.addColumn("itemLibrary", Sqlite::StrictColumnType::Text);
        table.addColumn("hints", Sqlite::StrictColumnType::Text);

        table.addUniqueIndex({sourceIdColumn, typeIdColumn});
        table.addIndex({directoryIdColumn});

        table.initialize(database);
    }
};

ProjectStorage::ProjectStorage(Database &database,
                               ProjectStorageErrorNotifierInterface &errorNotifier,
                               ModulesStorage &modulesStorage,
                               bool isInitialized)
    : database{database}
    , errorNotifier{&errorNotifier}
    , exclusiveTransaction{database}
    , initializer{std::make_unique<ProjectStorage::Initializer>(database, isInitialized)}
    , modulesStorage{modulesStorage}
    , commonTypeCache_{*this, modulesStorage}
    , s{std::make_unique<ProjectStorage::Statements>(database)}
{
    NanotraceHR::Tracer tracer{"initialize", category()};

    exclusiveTransaction.commit();

    database.walCheckpointFull();

}

ProjectStorage::~ProjectStorage() = default;

void ProjectStorage::synchronize(Storage::Synchronization::SynchronizationPackage package)
{
    NanotraceHR::Tracer tracer{"synchronize", category()};

    TypeIds deletedTypeIds;
    Storage::Info::ExportedTypeNames removedExportedTypeNames;
    Storage::Info::ExportedTypeNames addedExportedTypeNames;
    ExportedTypesChanged exportedTypesChanged = ExportedTypesChanged::No;

    Sqlite::withImmediateTransaction(database, [&] {
        AliasPropertyDeclarations aliasPropertyDeclarationsToLink;

        AliasPropertyDeclarations relinkableAliasPropertyDeclarations;
        PropertyDeclarations relinkablePropertyDeclarations;
        Bases relinkableBases;
        SmallTypeIds<256> updatedPrototypeIds;

        TypeIds updatedTypeIds;
        updatedTypeIds.reserve(package.types.size());

        TypeIds typeIdsToBeDeleted;

        std::ranges::sort(package.updatedTypeSourceIds);

        synchronizeFileStatuses(package.fileStatuses, package.updatedFileStatusSourceIds);
        synchronizeImports(package.imports,
                           package.updatedImportSourceIds,
                           package.moduleDependencies,
                           package.updatedModuleDependencySourceIds,
                           package.moduleExportedImports,
                           package.updatedModuleIds,
                           relinkableBases);
        synchronizeExportedTypes(package.exportedTypes,
                                 package.updatedExportedTypeSourceIds,
                                 relinkableAliasPropertyDeclarations,
                                 relinkablePropertyDeclarations,
                                 relinkableBases,
                                 exportedTypesChanged,
                                 removedExportedTypeNames,
                                 addedExportedTypeNames);
        synchronizeTypes(package.types,
                         updatedTypeIds,
                         aliasPropertyDeclarationsToLink,
                         relinkableAliasPropertyDeclarations,
                         relinkablePropertyDeclarations,
                         relinkableBases,
                         updatedPrototypeIds);

        deleteNotUpdatedTypes(updatedTypeIds,
                              package.updatedTypeSourceIds,
                              typeIdsToBeDeleted,
                              relinkableAliasPropertyDeclarations,
                              relinkablePropertyDeclarations,
                              relinkableBases,
                              deletedTypeIds);

        relink(relinkableAliasPropertyDeclarations,
               relinkablePropertyDeclarations,
               relinkableBases,
               deletedTypeIds);

        repairBrokenAliasPropertyDeclarations();

        linkAliases(aliasPropertyDeclarationsToLink, RaiseError::Yes);

        auto updatedAnnotationTypes = synchronizeTypeAnnotations(package.typeAnnotations,
                                                                 package.updatedTypeAnnotationSourceIds);
        updateAnnotationsTypeTraitsFromPrototypes(updatedAnnotationTypes, updatedPrototypeIds);
        synchronizePropertyEditorQmlPaths(package.propertyEditorQmlPaths,
                                          package.updatedPropertyEditorQmlPathDirectoryIds);

        synchronizeProjectEntryInfos(package.projectEntryInfos,
                                     package.updatedProjectEntryInfoSourceIds);

        resetBasesCache();

        commonTypeCache_.refreshTypeIds();
    });

    callRefreshMetaInfoCallback(deletedTypeIds,
                                exportedTypesChanged,
                                removedExportedTypeNames,
                                addedExportedTypeNames);
}

void ProjectStorage::synchronizeDocumentImports(Storage::Imports imports, SourceId sourceId)
{
    NanotraceHR::Tracer tracer{"synchronize document imports",
                               category(),
                               keyValue("imports", imports),
                               keyValue("source id", sourceId)};

    Sqlite::withImmediateTransaction(database, [&] {
        AliasPropertyDeclarations relinkableAliasPropertyDeclarations;
        PropertyDeclarations relinkablePropertyDeclarations;
        Bases relinkableBases;
        TypeIds deletedTypeIds;

        synchronizeDocumentImports(imports,
                                   {sourceId},
                                   Storage::Synchronization::ImportKind::Import,
                                   Relink::Yes,
                                   relinkableBases);

        relink(relinkableAliasPropertyDeclarations,
               relinkablePropertyDeclarations,
               relinkableBases,
               deletedTypeIds);
    });
}

void ProjectStorage::addObserver(ProjectStorageObserver *observer)
{
    NanotraceHR::Tracer tracer{"add observer", category()};
    observers.push_back(observer);
}

void ProjectStorage::removeObserver(ProjectStorageObserver *observer)
{
    NanotraceHR::Tracer tracer{"remove observer", category()};
    observers.removeOne(observer);
}

TypeId ProjectStorage::typeId(ModuleId moduleId,
                              Utils::SmallStringView exportedTypeName,
                              Storage::Version version) const
{
    NanotraceHR::Tracer tracer{"get type id by exported name",
                               category(),
                               keyValue("module id", moduleId),
                               keyValue("exported type name", exportedTypeName),
                               keyValue("version", version)};

    TypeId typeId;

    if (version.minor) {
        typeId = s->selectTypeIdByModuleIdAndExportedNameAndVersionStatement.valueWithTransaction<TypeId>(
            moduleId, exportedTypeName, version.major.value, version.minor.value);

    } else if (version.major) {
        typeId = s->selectTypeIdByModuleIdAndExportedNameAndMajorVersionStatement
                     .valueWithTransaction<TypeId>(moduleId, exportedTypeName, version.major.value);

    } else {
        typeId = s->selectTypeIdByModuleIdAndExportedNameStatement
                     .valueWithTransaction<TypeId>(moduleId, exportedTypeName);
    }

    tracer.end(keyValue("type id", typeId));

    return typeId;
}

Storage::Info::ExportedTypeName ProjectStorage::exportedTypeName(ImportedTypeNameId typeNameId) const
{
    NanotraceHR::Tracer tracer{"get exported type name id by imported type name",
                               category(),
                               keyValue("imported type name id", typeNameId)};

    return Sqlite::withDeferredTransaction(database,
                                           [&] { return fetchExportedTypeName(typeNameId); });
}

QVarLengthArray<TypeId, 256> ProjectStorage::typeIds(ModuleId moduleId) const
{
    NanotraceHR::Tracer tracer{"get type ids by module id",
                               category(),
                               keyValue("module id", moduleId)};

    auto typeIds = s->selectTypeIdsByModuleIdStatement.valuesWithTransaction<SmallTypeIds<256>>(
        moduleId);

    tracer.end(keyValue("type ids", typeIds));

    return typeIds;
}

SmallTypeIds<256> ProjectStorage::singletonTypeIds(SourceId sourceId) const
{
    NanotraceHR::Tracer tracer{"get singleton type ids by source id",
                               category(),
                               keyValue("source id", sourceId)};

    auto typeIds = s->selectSingletonTypeIdsBySourceIdStatement.valuesWithTransaction<SmallTypeIds<256>>(
        sourceId);

    tracer.end(keyValue("type ids", typeIds));

    return typeIds;
}

Storage::Info::ExportedTypeNames ProjectStorage::exportedTypeNames(TypeId typeId) const
{
    NanotraceHR::Tracer tracer{"get exported type names by type id",
                               category(),
                               keyValue("type id", typeId)};

    auto exportedTypenames = s->selectExportedTypesByTypeIdStatement
                                 .valuesWithTransaction<Storage::Info::ExportedTypeName, 4>(typeId);

    tracer.end(keyValue("exported type names", exportedTypenames));

    return exportedTypenames;
}

Storage::Info::ExportedTypeNames ProjectStorage::exportedTypeNames(TypeId typeId, SourceId sourceId) const
{
    NanotraceHR::Tracer tracer{"get exported type names by source id",
                               category(),
                               keyValue("type id", typeId),
                               keyValue("source id", sourceId)};

    auto exportedTypenames = s->selectExportedTypesByTypeIdAndSourceIdStatement
                                 .valuesWithTransaction<Storage::Info::ExportedTypeName, 4>(typeId,
                                                                                            sourceId);

    tracer.end(keyValue("exported type names", exportedTypenames));

    return exportedTypenames;
}

ImportId ProjectStorage::importId(const Storage::Import &import) const
{
    NanotraceHR::Tracer tracer{"get import id by import", category(), keyValue("import", import)};

    auto importId = Sqlite::withDeferredTransaction(database, [&] {
        return fetchImportId(import.sourceId, import);
    });

    tracer.end(keyValue("import id", importId));

    return importId;
}

ImportId ProjectStorage::importId(SourceId sourceId, Utils::SmallStringView alias) const
{
    NanotraceHR::Tracer tracer{"get import id by alias",
                               category(),
                               keyValue("source id", sourceId),
                               keyValue("alias", alias)};

    auto importId = Sqlite::withDeferredTransaction(database,
                                                    [&] { return fetchImportId(sourceId, alias); });

    tracer.end(keyValue("import id", importId));

    return importId;
}

ModuleId ProjectStorage::importModuleIdForSourceIdAndModuleId(SourceId sourceId, ModuleId moduleId) const
{
    NanotraceHR::Tracer tracer{"get original import module id for source id and module id",
                               category(),
                               keyValue("source id", sourceId),
                               keyValue("module id", moduleId)};

    return s->selectSourceModuleIdForSourceIdAndModuleIdStatement
        .valueWithTransaction<ModuleId>(sourceId, moduleId);
}

Storage::Import ProjectStorage::originalImportForSourceIdAndModuleId(SourceId sourceId,
                                                                     ModuleId moduleId) const
{
    NanotraceHR::Tracer tracer{"get original import module id for source id and module id",
                               category(),
                               keyValue("source id", sourceId),
                               keyValue("module id", moduleId)};

    return Sqlite::withImplicitTransaction(database, [&] {
        auto importId = s->selectImportIdForSourceIdAndModuleIdStatement.value<ImportId>(sourceId,
                                                                                         moduleId);

        while (importId) {
            auto parentImportId = s->selectParentImportIdForImportIdStatement.value<ImportId>(importId);
            if (not parentImportId)
                break;
            importId = parentImportId;
        }

        return s->selectDocumentImportByImportIdStatement.value<Storage::Import>(importId);
    });
}

ImportedTypeNameId ProjectStorage::importedTypeNameId(ImportId importId,
                                                      Utils::SmallStringView typeName)
{
    NanotraceHR::Tracer tracer{"get imported type name id by import id",
                               category(),
                               keyValue("import id", importId),
                               keyValue("imported type name", typeName)};

    auto importedTypeNameId = Sqlite::withDeferredTransaction(database, [&] {
        return fetchImportedTypeNameId(Storage::Synchronization::TypeNameKind::QualifiedExported,
                                       importId,
                                       typeName);
    });

    tracer.end(keyValue("imported type name id", importedTypeNameId));

    return importedTypeNameId;
}

ImportedTypeNameId ProjectStorage::importedTypeNameId(SourceId sourceId,
                                                      Utils::SmallStringView typeName)
{
    NanotraceHR::Tracer tracer{"get imported type name id by source id",
                               category(),
                               keyValue("source id", sourceId),
                               keyValue("imported type name", typeName)};

    auto importedTypeNameId = Sqlite::withDeferredTransaction(database, [&] {
        return fetchImportedTypeNameId(Storage::Synchronization::TypeNameKind::Exported,
                                       sourceId,
                                       typeName);
    });

    tracer.end(keyValue("imported type name id", importedTypeNameId));

    return importedTypeNameId;
}

QVarLengthArray<PropertyDeclarationId, 128> ProjectStorage::propertyDeclarationIds(TypeId typeId) const
{
    NanotraceHR::Tracer tracer{"get property declaration ids", category(), keyValue("type id", typeId)};

    auto propertyDeclarationIds = Sqlite::withDeferredTransaction(database, [&] {
        return fetchPropertyDeclarationIds(typeId);
    });

    std::ranges::sort(propertyDeclarationIds);

    tracer.end(keyValue("property declaration ids", propertyDeclarationIds));

    return propertyDeclarationIds;
}

QVarLengthArray<PropertyDeclarationId, 128> ProjectStorage::localPropertyDeclarationIds(TypeId typeId) const
{
    NanotraceHR::Tracer tracer{"get local property declaration ids",
                               category(),
                               keyValue("type id", typeId)};

    auto propertyDeclarationIds = s->selectLocalPropertyDeclarationIdsForTypeStatement
                                      .valuesWithTransaction<QVarLengthArray<PropertyDeclarationId, 128>>(
                                          typeId);

    tracer.end(keyValue("property declaration ids", propertyDeclarationIds));

    return propertyDeclarationIds;
}

PropertyDeclarationId ProjectStorage::propertyDeclarationId(TypeId typeId,
                                                            Utils::SmallStringView propertyName) const
{
    NanotraceHR::Tracer tracer{"get property declaration id",
                               category(),
                               keyValue("type id", typeId),
                               keyValue("property name", propertyName)};

    auto propertyDeclarationId = Sqlite::withDeferredTransaction(database, [&] {
        return fetchPropertyDeclarationId(typeId, propertyName);
    });

    tracer.end(keyValue("property declaration id", propertyDeclarationId));

    return propertyDeclarationId;
}

PropertyDeclarationId ProjectStorage::localPropertyDeclarationId(TypeId typeId,
                                                                 Utils::SmallStringView propertyName) const
{
    NanotraceHR::Tracer tracer{"get local property declaration id",
                               category(),
                               keyValue("type id", typeId),
                               keyValue("property name", propertyName)};

    auto propertyDeclarationId = s->selectLocalPropertyDeclarationIdForTypeAndPropertyNameStatement
                                     .valueWithTransaction<PropertyDeclarationId>(typeId,
                                                                                  propertyName);

    tracer.end(keyValue("property declaration id", propertyDeclarationId));

    return propertyDeclarationId;
}

PropertyDeclarationId ProjectStorage::defaultPropertyDeclarationId(TypeId typeId) const
{
    NanotraceHR::Tracer tracer{"get default property declaration id",
                               category(),
                               keyValue("type id", typeId)};

    auto propertyDeclarationId = Sqlite::withDeferredTransaction(database, [&] {
        return fetchDefaultPropertyDeclarationId(typeId);
    });

    tracer.end(keyValue("property declaration id", propertyDeclarationId));

    return propertyDeclarationId;
}

std::optional<Storage::Info::PropertyDeclaration> ProjectStorage::propertyDeclaration(
    PropertyDeclarationId propertyDeclarationId) const
{
    NanotraceHR::Tracer tracer{"get property declaration",
                               category(),
                               keyValue("property declaration id", propertyDeclarationId)};

    auto propertyDeclaration = s->selectPropertyDeclarationForPropertyDeclarationIdStatement
                                   .optionalValueWithTransaction<Storage::Info::PropertyDeclaration>(
                                       propertyDeclarationId);

    tracer.end(keyValue("property declaration", propertyDeclaration));

    return propertyDeclaration;
}

std::optional<Storage::Info::Type> ProjectStorage::type(TypeId typeId) const
{
    NanotraceHR::Tracer tracer{"get type", category(), keyValue("type id", typeId)};

    auto type = s->selectInfoTypeByTypeIdStatement.optionalValueWithTransaction<Storage::Info::Type>(
        typeId);

    tracer.end(keyValue("type", type));

    return type;
}

Utils::PathString ProjectStorage::typeIconPath(TypeId typeId) const
{
    NanotraceHR::Tracer tracer{"get type icon path", category(), keyValue("type id", typeId)};

    auto typeIconPath = s->selectTypeIconPathStatement.valueWithTransaction<Utils::PathString>(typeId);

    tracer.end(keyValue("type icon path", typeIconPath));

    return typeIconPath;
}

Storage::Info::TypeHints ProjectStorage::typeHints(TypeId typeId) const
{
    NanotraceHR::Tracer tracer{"get type hints", category(), keyValue("type id", typeId)};

    auto typeHints = s->selectTypeHintsStatement.valuesWithTransaction<Storage::Info::TypeHints, 4>(
        typeId);

    tracer.end(keyValue("type hints", typeHints));

    return typeHints;
}

SmallSourceIds<4> ProjectStorage::typeAnnotationSourceIds(DirectoryPathId directoryId) const
{
    NanotraceHR::Tracer tracer{"get type annotaion source ids",
                               category(),
                               keyValue("source id", directoryId)};

    auto sourceIds = s->selectTypeAnnotationSourceIdsStatement.valuesWithTransaction<SmallSourceIds<4>>(
        directoryId);

    tracer.end(keyValue("source ids", sourceIds));

    return sourceIds;
}

SmallDirectoryPathIds<64> ProjectStorage::typeAnnotationDirectoryIds() const
{
    NanotraceHR::Tracer tracer{"get type annotaion source ids", category()};

    auto sourceIds = s->selectTypeAnnotationDirectoryIdsStatement
                         .valuesWithTransaction<SmallDirectoryPathIds<64>>();

    tracer.end(keyValue("source ids", sourceIds));

    return sourceIds;
}

Storage::Info::ItemLibraryEntries ProjectStorage::itemLibraryEntries(TypeId typeId) const
{
    NanotraceHR::Tracer tracer{"get item library entries  by type id",
                               category(),
                               keyValue("type id", typeId)};

    using Storage::Info::ItemLibraryProperties;
    Storage::Info::ItemLibraryEntries entries;

    auto callback = [&](TypeId typeId_,
                        Utils::SmallStringView typeName,
                        Utils::SmallStringView name,
                        Utils::SmallStringView iconPath,
                        Utils::SmallStringView category,
                        Utils::SmallStringView import,
                        Utils::SmallStringView toolTip,
                        Utils::SmallStringView properties,
                        Utils::SmallStringView extraFilePaths,
                        Utils::SmallStringView templatePath) {
        auto &last = entries.emplace_back(
            typeId_, typeName, name, iconPath, category, import, toolTip, templatePath);
        if (properties.size())
            s->selectItemLibraryPropertiesStatement.readTo(last.properties, properties);
        if (extraFilePaths.size())
            s->selectItemLibraryExtraFilePathsStatement.readTo(last.extraFilePaths, extraFilePaths);
    };

    s->selectItemLibraryEntriesByTypeIdStatement.readCallbackWithTransaction(callback, typeId);

    tracer.end(keyValue("item library entries", entries));

    return entries;
}

Storage::Info::ItemLibraryEntries ProjectStorage::itemLibraryEntries(ImportId importId) const
{
    NanotraceHR::Tracer tracer{"get item library entries  by import id",
                               category(),
                               keyValue("import id", importId)};

    using Storage::Info::ItemLibraryProperties;
    Storage::Info::ItemLibraryEntries entries;

    auto callback = [&](TypeId typeId_,
                        Utils::SmallStringView typeName,
                        Utils::SmallStringView name,
                        Utils::SmallStringView iconPath,
                        Utils::SmallStringView category,
                        Utils::SmallStringView import,
                        Utils::SmallStringView toolTip,
                        Utils::SmallStringView properties,
                        Utils::SmallStringView extraFilePaths,
                        Utils::SmallStringView templatePath) {
        auto &last = entries.emplace_back(
            typeId_, typeName, name, iconPath, category, import, toolTip, templatePath);
        if (properties.size())
            s->selectItemLibraryPropertiesStatement.readTo(last.properties, properties);
        if (extraFilePaths.size())
            s->selectItemLibraryExtraFilePathsStatement.readTo(last.extraFilePaths, extraFilePaths);
    };

    s->selectItemLibraryEntriesByTypeIdStatement.readCallbackWithTransaction(callback, importId);

    tracer.end(keyValue("item library entries", entries));

    return entries;
}

namespace {
bool isCapitalLetter(char c)
{
    return c >= 'A' && c <= 'Z';
}
} // namespace

Storage::Info::ItemLibraryEntries ProjectStorage::itemLibraryEntries(SourceId sourceId) const
{
    NanotraceHR::Tracer tracer{"get item library entries by source id",
                               category(),
                               keyValue("source id", sourceId)};

    using Storage::Info::ItemLibraryProperties;
    Storage::Info::ItemLibraryEntries entries;

    auto callback = [&](TypeId typeId,
                        Utils::SmallStringView typeName,
                        Utils::SmallStringView name,
                        Utils::SmallStringView iconPath,
                        Utils::SmallStringView category,
                        Utils::SmallStringView import,
                        Utils::SmallStringView toolTip,
                        Utils::SmallStringView properties,
                        Utils::SmallStringView extraFilePaths,
                        Utils::SmallStringView templatePath) {
        auto &last = entries.emplace_back(
            typeId, typeName, name, iconPath, category, import, toolTip, templatePath);
        if (properties.size())
            s->selectItemLibraryPropertiesStatement.readTo(last.properties, properties);
        if (extraFilePaths.size())
            s->selectItemLibraryExtraFilePathsStatement.readTo(last.extraFilePaths, extraFilePaths);
    };

    s->selectItemLibraryEntriesBySourceIdStatement.readCallbackWithTransaction(callback, sourceId);

    tracer.end(keyValue("item library entries", entries));

    return entries;
}

Storage::Info::ItemLibraryEntries ProjectStorage::allItemLibraryEntries() const
{
    NanotraceHR::Tracer tracer{"get all item library entries", category()};

    using Storage::Info::ItemLibraryProperties;
    Storage::Info::ItemLibraryEntries entries;

    auto callback = [&](TypeId typeId,
                        Utils::SmallStringView typeName,
                        Utils::SmallStringView name,
                        Utils::SmallStringView iconPath,
                        Utils::SmallStringView category,
                        Utils::SmallStringView import,
                        Utils::SmallStringView toolTip,
                        Utils::SmallStringView properties,
                        Utils::SmallStringView extraFilePaths,
                        Utils::SmallStringView templatePath) {
        auto &last = entries.emplace_back(
            typeId, typeName, name, iconPath, category, import, toolTip, templatePath);
        if (properties.size())
            s->selectItemLibraryPropertiesStatement.readTo(last.properties, properties);
        if (extraFilePaths.size())
            s->selectItemLibraryExtraFilePathsStatement.readTo(last.extraFilePaths, extraFilePaths);
    };

    s->selectItemLibraryEntriesStatement.readCallbackWithTransaction(callback);

    tracer.end(keyValue("item library entries", entries));

    return entries;
}

Storage::Info::ItemLibraryEntries ProjectStorage::directoryImportsItemLibraryEntries(SourceId sourceId) const
{
    NanotraceHR::Tracer tracer{"get directory import item library entries",
                               category(),
                               keyValue("source id", sourceId)};

    using Storage::Info::ItemLibraryProperties;
    Storage::Info::ItemLibraryEntries entries;

    auto callback = [&](TypeId typeId,
                        Utils::SmallStringView typeName,
                        ModuleId moduleId,
                        SourceId componentSourceId) {
        if (!isCapitalLetter(typeName.front()))
            return;

        auto module = modulesStorage.module(moduleId);
        if (module.kind != Storage::ModuleKind::PathLibrary)
            return;

        auto &last = entries.emplace_back(typeId, typeName, typeName, "", module.name);
        last.moduleKind = Storage::ModuleKind::PathLibrary;
        last.componentSourceId = componentSourceId;
    };

    s->selectDirectoryImportsItemLibraryEntriesBySourceIdStatement
        .readCallbackWithTransaction(callback, sourceId);

    tracer.end(keyValue("item library entries", entries));

    return entries;
}

std::vector<Utils::SmallString> ProjectStorage::signalDeclarationNames(TypeId typeId) const
{
    NanotraceHR::Tracer tracer{"get signal names", category(), keyValue("type id", typeId)};

    auto signalDeclarationNames = s->selectSignalDeclarationNamesForTypeStatement
                                      .valuesWithTransaction<Utils::SmallString, 32>(typeId);

    tracer.end(keyValue("signal names", signalDeclarationNames));

    return signalDeclarationNames;
}

std::vector<Utils::SmallString> ProjectStorage::functionDeclarationNames(TypeId typeId) const
{
    NanotraceHR::Tracer tracer{"get function names", category(), keyValue("type id", typeId)};

    auto functionDeclarationNames = s->selectFuncionDeclarationNamesForTypeStatement
                                        .valuesWithTransaction<Utils::SmallString, 32>(typeId);

    tracer.end(keyValue("function names", functionDeclarationNames));

    return functionDeclarationNames;
}

std::optional<Utils::SmallString> ProjectStorage::propertyName(
    PropertyDeclarationId propertyDeclarationId) const
{
    NanotraceHR::Tracer tracer{"get property name",
                               category(),
                               keyValue("property declaration id", propertyDeclarationId)};

    auto propertyName = s->selectPropertyNameStatement.optionalValueWithTransaction<Utils::SmallString>(
        propertyDeclarationId);

    tracer.end(keyValue("property name", propertyName));

    return propertyName;
}

SmallTypeIds<16> ProjectStorage::prototypeIds(TypeId type) const
{
    NanotraceHR::Tracer tracer{"get prototypes", category(), keyValue("type id", type)};

    auto prototypeIds = s->selectPrototypeIdsStatement.valuesWithTransaction<SmallTypeIds<16>>(type);

    tracer.end(keyValue("type ids", prototypeIds));

    return prototypeIds;
}

SmallTypeIds<16> ProjectStorage::prototypeAndSelfIds(TypeId typeId) const
{
    NanotraceHR::Tracer tracer{"get prototypes and self", category()};

    SmallTypeIds<16> prototypeAndSelfIds;
    prototypeAndSelfIds.push_back(typeId);

    s->selectPrototypeIdsStatement.readToWithTransaction(prototypeAndSelfIds, typeId);

    tracer.end(keyValue("type ids", prototypeAndSelfIds));

    return prototypeAndSelfIds;
}

SmallTypeIds<64> ProjectStorage::heirIds(TypeId typeId) const
{
    NanotraceHR::Tracer tracer{"get heirs", category()};

    auto heirIds = s->selectHeirTypeIdsStatement.valuesWithTransaction<SmallTypeIds<64>>(typeId);

    tracer.end(keyValue("type ids", heirIds));

    return heirIds;
}

TypeId ProjectStorage::basedOn(TypeId) const
{
    return TypeId{};
}

TypeId ProjectStorage::basedOn(TypeId typeId, TypeId id1) const
{
    return basedOn_(typeId, id1);
}

TypeId ProjectStorage::basedOn(TypeId typeId, TypeId id1, TypeId id2) const
{
    return basedOn_(typeId, id1, id2);
}

TypeId ProjectStorage::basedOn(TypeId typeId, TypeId id1, TypeId id2, TypeId id3) const
{
    return basedOn_(typeId, id1, id2, id3);
}

TypeId ProjectStorage::basedOn(TypeId typeId, TypeId id1, TypeId id2, TypeId id3, TypeId id4) const
{
    return basedOn_(typeId, id1, id2, id3, id4);
}

TypeId ProjectStorage::basedOn(TypeId typeId, TypeId id1, TypeId id2, TypeId id3, TypeId id4, TypeId id5) const
{
    return basedOn_(typeId, id1, id2, id3, id4, id5);
}

TypeId ProjectStorage::basedOn(
    TypeId typeId, TypeId id1, TypeId id2, TypeId id3, TypeId id4, TypeId id5, TypeId id6) const
{
    return basedOn_(typeId, id1, id2, id3, id4, id5, id6);
}

TypeId ProjectStorage::basedOn(
    TypeId typeId, TypeId id1, TypeId id2, TypeId id3, TypeId id4, TypeId id5, TypeId id6, TypeId id7) const
{
    return basedOn_(typeId, id1, id2, id3, id4, id5, id6, id7);
}

TypeId ProjectStorage::basedOn(TypeId typeId,
                               TypeId id1,
                               TypeId id2,
                               TypeId id3,
                               TypeId id4,
                               TypeId id5,
                               TypeId id6,
                               TypeId id7,
                               TypeId id8) const
{
    return basedOn_(typeId, id1, id2, id3, id4, id5, id6, id7, id8);
}

TypeId ProjectStorage::basedOn(TypeId typeId,
                               TypeId id1,
                               TypeId id2,
                               TypeId id3,
                               TypeId id4,
                               TypeId id5,
                               TypeId id6,
                               TypeId id7,
                               TypeId id8,
                               TypeId id9) const
{
    return basedOn_(typeId, id1, id2, id3, id4, id5, id6, id7, id8, id9);
}

TypeId ProjectStorage::basedOn(TypeId typeId,
                               TypeId id1,
                               TypeId id2,
                               TypeId id3,
                               TypeId id4,
                               TypeId id5,
                               TypeId id6,
                               TypeId id7,
                               TypeId id8,
                               TypeId id9,
                               TypeId id10) const
{
    return basedOn_(typeId, id1, id2, id3, id4, id5, id6, id7, id8, id9, id10);
}

TypeId ProjectStorage::basedOn(TypeId typeId,
                               TypeId id1,
                               TypeId id2,
                               TypeId id3,
                               TypeId id4,
                               TypeId id5,
                               TypeId id6,
                               TypeId id7,
                               TypeId id8,
                               TypeId id9,
                               TypeId id10,
                               TypeId id11) const
{
    return basedOn_(typeId, id1, id2, id3, id4, id5, id6, id7, id8, id9, id10, id11);
}

TypeId ProjectStorage::basedOn(TypeId typeId,
                               TypeId id1,
                               TypeId id2,
                               TypeId id3,
                               TypeId id4,
                               TypeId id5,
                               TypeId id6,
                               TypeId id7,
                               TypeId id8,
                               TypeId id9,
                               TypeId id10,
                               TypeId id11,
                               TypeId id12) const
{
    return basedOn_(typeId, id1, id2, id3, id4, id5, id6, id7, id8, id9, id10, id11, id12);
}

TypeId ProjectStorage::fetchTypeIdByExportedName(Utils::SmallStringView name) const
{
    NanotraceHR::Tracer tracer{"is based on", category(), keyValue("exported type name", name)};

    auto typeId = s->selectTypeIdByExportedNameStatement.valueWithTransaction<TypeId>(name);

    tracer.end(keyValue("type id", typeId));

    return typeId;
}

TypeId ProjectStorage::fetchTypeIdByModuleIdsAndExportedName(ModuleIds moduleIds,
                                                             Utils::SmallStringView name) const
{
    NanotraceHR::Tracer tracer{"fetch type id by module ids and exported name",
                               category(),
                               keyValue("module ids", NanotraceHR::array(moduleIds)),
                               keyValue("exported type name", name)};
    auto typeId = s->selectTypeIdByModuleIdsAndExportedNameStatement.valueWithTransaction<TypeId>(
        static_cast<void *>(moduleIds.data()), static_cast<long long>(moduleIds.size()), name);

    tracer.end(keyValue("type id", typeId));

    return typeId;
}

TypeId ProjectStorage::fetchTypeIdByName(SourceId sourceId, Utils::SmallStringView name)
{
    NanotraceHR::Tracer tracer{"fetch type id by name",
                               category(),
                               keyValue("source id", sourceId),
                               keyValue("internal type name", name)};

    auto typeId = s->selectTypeIdBySourceIdAndNameStatement.valueWithTransaction<TypeId>(sourceId,
                                                                                         name);

    tracer.end(keyValue("type id", typeId));

    return typeId;
}

Storage::Synchronization::Type ProjectStorage::fetchTypeByTypeId(TypeId typeId)
{
    NanotraceHR::Tracer tracer{"fetch type by type id", category(), keyValue("type id", typeId)};

    auto type = Sqlite::withDeferredTransaction(database, [&] {
        auto type = s->selectTypeByTypeIdStatement.value<Storage::Synchronization::Type>(typeId);

        type.prototypeId = fetchPrototypeId(typeId);
        type.extensionId = fetchExtensionId(typeId);
        type.typeId = typeId;

        return type;
    });

    tracer.end(keyValue("type", type));

    return type;
}

Storage::Synchronization::Types ProjectStorage::fetchTypes()
{
    NanotraceHR::Tracer tracer{"fetch types", category()};

    auto types = Sqlite::withDeferredTransaction(database, [&] {
        auto types = s->selectTypesStatement.values<Storage::Synchronization::Type, 64>();

        for (Storage::Synchronization::Type &type : types) {
            type.prototypeId = fetchPrototypeId(type.typeId);
            type.extensionId = fetchExtensionId(type.typeId);
        }

        return types;
    });

    tracer.end(keyValue("type", types));

    return types;
}

SmallTypeIds<256> ProjectStorage::fetchTypeIds() const
{
    return s->selectTypeIdsStatement.valuesWithTransaction<SmallTypeIds<256>>();
}

FileStatuses ProjectStorage::fetchAllFileStatuses() const
{
    NanotraceHR::Tracer tracer{"fetch all file statuses", category()};

    return s->selectAllFileStatusesStatement.valuesWithTransaction<FileStatus>();
}

FileStatus ProjectStorage::fetchFileStatus(SourceId sourceId) const
{
    NanotraceHR::Tracer tracer{"fetch file status", category(), keyValue("source id", sourceId)};

    auto fileStatus = s->selectFileStatusForSourceIdStatement.valueWithTransaction<FileStatus>(
        sourceId);

    tracer.end(keyValue("file status", fileStatus));

    return fileStatus;
}

std::optional<Storage::Synchronization::ProjectEntryInfo> ProjectStorage::fetchProjectEntryInfo(
    SourceId sourceId) const
{
    NanotraceHR::Tracer tracer{"fetch directory info", category(), keyValue("source id", sourceId)};

    auto projectEntryInfo = s->selectProjectEntryInfoForSourceIdStatement
                                .optionalValueWithTransaction<Storage::Synchronization::ProjectEntryInfo>(
                                    sourceId);

    tracer.end(keyValue("directory info", projectEntryInfo));

    return projectEntryInfo;
}

Storage::Synchronization::ProjectEntryInfos ProjectStorage::fetchProjectEntryInfos(
    SourceId contextSourceId) const
{
    NanotraceHR::Tracer tracer{"fetch directory infos by directory id",
                               category(),
                               keyValue("source id", contextSourceId)};

    auto projectEntryInfos = s->selectProjectEntryInfosForContextSourceIdStatement
                                 .valuesWithTransaction<Storage::Synchronization::ProjectEntryInfo, 1024>(
                                     contextSourceId);

    tracer.end(keyValue("directory infos", projectEntryInfos));

    return projectEntryInfos;
}

Storage::Synchronization::ProjectEntryInfos ProjectStorage::fetchProjectEntryInfos(
    SourceId contextSourceId, Storage::Synchronization::FileType fileType) const
{
    NanotraceHR::Tracer tracer{"fetch directory infos by source id and file type",
                               category(),
                               keyValue("source context id", contextSourceId),
                               keyValue("file type", fileType)};

    auto projectEntryInfos = s->selectProjectEntryInfosForContextSourceIdAndFileTypeStatement
                                 .valuesWithTransaction<Storage::Synchronization::ProjectEntryInfo, 16>(
                                     contextSourceId, fileType);

    tracer.end(keyValue("directory infos", projectEntryInfos));

    return projectEntryInfos;
}

Storage::Synchronization::ProjectEntryInfos ProjectStorage::fetchProjectEntryInfos(
    const SourceIds &contextSourceIds) const
{
    NanotraceHR::Tracer tracer{"fetch directory infos by source ids",
                               category(),
                               keyValue("directory ids", contextSourceIds)};

    auto projectEntryInfos = s->selectProjectEntryInfosForDirectoryIdsStatement
                                 .valuesWithTransaction<Storage::Synchronization::ProjectEntryInfo, 64>(
                                     Sqlite::toIntegers(contextSourceIds));

    tracer.end(keyValue("directory infos", projectEntryInfos));

    return projectEntryInfos;
}

SmallDirectoryPathIds<32> ProjectStorage::fetchSubdirectoryIds(DirectoryPathId directoryId) const
{
    NanotraceHR::Tracer tracer{"fetch subdirectory source ids",
                               category(),
                               keyValue("directory id", directoryId)};

    auto sourceIds = s->selectProjectEntryInfosSourceIdsForContextSourceIdAndFileTypeStatement
                         .rangeWithTransaction<SourceId>(SourceId::create(directoryId),
                                                         Storage::Synchronization::FileType::Directory);

    SmallDirectoryPathIds<32> directoryIds;
    for (SourceId sourceId : sourceIds)
        directoryIds.push_back(sourceId.directoryPathId());

    tracer.end(keyValue("directory ids", directoryIds));

    return directoryIds;
}

void ProjectStorage::setPropertyEditorPathId(TypeId typeId, SourceId pathId)
{
    Sqlite::ImmediateTransaction transaction{database};

    s->upsertPropertyEditorPathIdStatement.write(typeId, pathId);

    transaction.commit();
}

SourceId ProjectStorage::propertyEditorPathId(TypeId typeId) const
{
    NanotraceHR::Tracer tracer{"property editor path id", category(), keyValue("type id", typeId)};

    auto sourceId = s->selectPropertyEditorPathIdStatement.valueWithTransaction<SourceId>(typeId);

    tracer.end(keyValue("source id", sourceId));

    return sourceId;
}

Storage::Imports ProjectStorage::fetchDocumentImports() const
{
    NanotraceHR::Tracer tracer{"fetch document imports", category()};

    return s->selectAllDocumentImportsStatement.valuesWithTransaction<Storage::Imports>();
}

void ProjectStorage::resetForTestsOnly()
{
    database.clearAllTablesForTestsOnly();
    commonTypeCache_.clearForTestsOnly();
    observers.clear();
}

Storage::Synchronization::FunctionDeclarations ProjectStorage::fetchFunctionDeclarationsForTestOnly(
    TypeId typeId) const
{
    return Sqlite::withDeferredTransaction(
        database, std::bind_front(&ProjectStorage::fetchFunctionDeclarations, this, typeId));
}

Storage::Synchronization::SignalDeclarations ProjectStorage::fetchSignalDeclarationsForTestOnly(
    TypeId typeId) const
{
    return Sqlite::withDeferredTransaction(database,
                                           std::bind_front(&ProjectStorage::fetchSignalDeclarations,
                                                           this,
                                                           typeId));
}

Storage::Synchronization::EnumerationDeclarations ProjectStorage::fetchEnumerationDeclarationsForTestOnly(
    TypeId typeId) const
{
    return Sqlite::withDeferredTransaction(
        database, std::bind_front(&ProjectStorage::fetchEnumerationDeclarations, this, typeId));
}

void ProjectStorage::callRefreshMetaInfoCallback(
    TypeIds &deletedTypeIds,
    ExportedTypesChanged exportedTypesChanged,
    const Storage::Info::ExportedTypeNames &removedExportedTypeNames,
    const Storage::Info::ExportedTypeNames &addedExportedTypeNames)
{
    NanotraceHR::Tracer tracer{"call refresh meta info callback",
                               category(),
                               keyValue("type ids", deletedTypeIds)};

    if (deletedTypeIds.size()) {
        std::ranges::sort(deletedTypeIds);

        for (ProjectStorageObserver *observer : observers)
            observer->removedTypeIds(deletedTypeIds);
    }

    if (exportedTypesChanged == ExportedTypesChanged::Yes) {
        for (ProjectStorageObserver *observer : observers) {
            observer->exportedTypeNamesChanged(addedExportedTypeNames, removedExportedTypeNames);
        }
    }
}

SourceIds ProjectStorage::filterSourceIdsWithoutType(const SourceIds &updatedSourceIds,
                                                     SourceIds &sourceIdsOfTypes)
{
    std::ranges::sort(sourceIdsOfTypes);

    SourceIds sourceIdsWithoutTypeSourceIds;
    sourceIdsWithoutTypeSourceIds.reserve(updatedSourceIds.size());
    std::ranges::set_difference(updatedSourceIds,
                                sourceIdsOfTypes,
                                std::back_inserter(sourceIdsWithoutTypeSourceIds));

    return sourceIdsWithoutTypeSourceIds;
}

TypeIds ProjectStorage::fetchTypeIds(const SourceIds &sourceIds)
{
    NanotraceHR::Tracer tracer{"fetch type ids", category(), keyValue("source ids", sourceIds)};

    return s->selectTypeIdsForSourceIdsStatement.values<TypeId, 128>(Sqlite::toIntegers(sourceIds));
}

void ProjectStorage::unique(SourceIds &sourceIds)
{
    std::ranges::sort(sourceIds);
    auto removed = std::ranges::unique(sourceIds);
    sourceIds.erase(removed.begin(), removed.end());
}

void ProjectStorage::updateAnnotationTypeTraitsInHeirs(TypeId typeId,
                                                       Storage::TypeTraits traits,
                                                       SmallTypeIds<256> &updatedTypes)
{
    NanotraceHR::Tracer tracer{"synchronize type traits",
                               category(),
                               keyValue("type id", typeId),
                               keyValue("type traits", traits)};

    s->updateTypeAnnotationTraitsStatement.readTo(updatedTypes, typeId, traits.annotation);
}

void ProjectStorage::updateTypeIdInTypeAnnotations(Storage::Synchronization::TypeAnnotations &typeAnnotations)
{
    NanotraceHR::Tracer tracer{"update type id in type annotations", category()};

    for (auto &annotation : typeAnnotations) {
        annotation.typeId = fetchTypeIdByModuleIdAndExportedName(annotation.moduleId,
                                                                 annotation.typeName);
    }

    std::erase_if(typeAnnotations, is_null(&TypeAnnotation::typeId));
}

void ProjectStorage::updateAnnotationsTypeTraitsFromPrototypes(SmallTypeIds<256> &alreadyUpdatedTypes,
                                                               SmallTypeIds<256> &updatedPrototypeIds)
{
    NanotraceHR::Tracer tracer{"update annotations type traits from prototypes", category()};

    std::ranges::sort(updatedPrototypeIds);
    std::ranges::sort(alreadyUpdatedTypes);

    SmallTypeIds<256> typesToUpdate;

    std::ranges::set_difference(updatedPrototypeIds,
                                alreadyUpdatedTypes,
                                std::back_inserter(typesToUpdate));

    auto callback = [&](TypeId typeId, long long traits) {
        NanotraceHR::Tracer tracer{"update annotation type traits from prototype",
                                   category(),
                                   keyValue("type id", typeId),
                                   keyValue("annotation traits", traits)};

        s->updateTypeAnnotationTraitsStatement.write(typeId, traits);
    };

    for (TypeId typeId : typesToUpdate)
        s->selectTypeAnnotationTraitsFromPrototypeStatement.readCallback(callback, typeId);
}

SmallTypeIds<256> ProjectStorage::synchronizeTypeAnnotations(
    Storage::Synchronization::TypeAnnotations &typeAnnotations,
    const SourceIds &updatedTypeAnnotationSourceIds)
{
    NanotraceHR::Tracer tracer{"synchronize type annotations", category()};

    updateTypeIdInTypeAnnotations(typeAnnotations);

    auto compareKey = [](auto &&first, auto &&second) { return first.typeId <=> second.typeId; };

    std::ranges::sort(typeAnnotations, {}, &TypeAnnotation::typeId);

    auto range = s->selectTypeAnnotationsForSourceIdsStatement.range<TypeAnnotationView>(
        Sqlite::toIntegers(updatedTypeAnnotationSourceIds));

    SmallTypeIds<256> updatedTypes;

    auto insert = [&](const TypeAnnotation &annotation) {
        if (!annotation.sourceId)
            throw TypeAnnotationHasInvalidSourceId{};

        NanotraceHR::Tracer tracer{"insert type annotations",
                                   category(),
                                   keyValue("type annotation", annotation)};

        s->insertTypeAnnotationStatement.write(annotation.typeId,
                                               annotation.sourceId,
                                               annotation.directoryId,
                                               annotation.typeName,
                                               annotation.iconPath,
                                               createEmptyAsNull(annotation.itemLibraryJson),
                                               createEmptyAsNull(annotation.hintsJson));

        updateAnnotationTypeTraitsInHeirs(annotation.typeId, annotation.traits, updatedTypes);
    };

    auto update = [&](const TypeAnnotationView &annotationFromDatabase,
                      const TypeAnnotation &annotation) {
        if (annotationFromDatabase.typeName != annotation.typeName
            || annotationFromDatabase.iconPath != annotation.iconPath
            || annotationFromDatabase.itemLibraryJson != annotation.itemLibraryJson
            || annotationFromDatabase.hintsJson != annotation.hintsJson) {
            NanotraceHR::Tracer tracer{"update type annotations",
                                       category(),
                                       keyValue("type annotation from database",
                                                annotationFromDatabase),
                                       keyValue("type annotation", annotation)};

            s->updateTypeAnnotationStatement.write(annotation.typeId,
                                                   annotation.typeName,
                                                   annotation.iconPath,
                                                   createEmptyAsNull(annotation.itemLibraryJson),
                                                   createEmptyAsNull(annotation.hintsJson));

            updateAnnotationTypeTraitsInHeirs(annotation.typeId, annotation.traits, updatedTypes);

            return Sqlite::UpdateChange::Update;
        }

        updateAnnotationTypeTraitsInHeirs(annotation.typeId, annotation.traits, updatedTypes);

        return Sqlite::UpdateChange::No;
    };

    auto remove = [&](const TypeAnnotationView &annotationFromDatabase) {
        NanotraceHR::Tracer tracer{"remove type annotations",
                                   category(),
                                   keyValue("type annotation", annotationFromDatabase)};

        auto prototypeAnnotationTraits = s->selectPrototypeAnnotationTraitsByTypeIdStatement
                                             .value<long long>(annotationFromDatabase.typeId);
        s->deleteTypeAnnotationStatement.write(annotationFromDatabase.typeId);

        s->updateTypeAnnotationTraitsStatement.write(annotationFromDatabase.typeId,
                                                     prototypeAnnotationTraits);
    };

    Sqlite::insertUpdateDelete(range, typeAnnotations, compareKey, insert, update, remove);

    return updatedTypes;
}

void ProjectStorage::synchronizeTypeTrait(const Storage::Synchronization::Type &type)
{
    s->updateTypeTraitStatement.write(type.typeId, type.traits.type);
}

void ProjectStorage::synchronizeTypes(Storage::Synchronization::Types &types,
                                      TypeIds &updatedTypeIds,
                                      AliasPropertyDeclarations &aliasPropertyDeclarationsToLink,
                                      AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                                      PropertyDeclarations &relinkablePropertyDeclarations,
                                      Bases &relinkableBases,
                                      SmallTypeIds<256> &updatedPrototypeIds)
{
    NanotraceHR::Tracer tracer{"synchronize types", category()};

    for (auto &type : types) {
        if (!type.sourceId)
            throw TypeHasInvalidSourceId{};

        type.typeId = declareType(type.typeName, type.sourceId);
        synchronizeTypeTrait(type);
        updatedTypeIds.push_back(type.typeId);
    }

    std::ranges::sort(types, {}, &Type::typeId);

    syncPrototypesAndExtensions(types, relinkableBases, updatedPrototypeIds);
    resetDefaultPropertiesIfChanged(types);
    resetRemovedAliasPropertyDeclarationsToNull(types, relinkableAliasPropertyDeclarations);
    syncDeclarations(types, aliasPropertyDeclarationsToLink, relinkablePropertyDeclarations);
    syncDefaultProperties(types);
}

void ProjectStorage::synchronizeProjectEntryInfos(
    Storage::Synchronization::ProjectEntryInfos &projectEntryInfos,
    const SourceIds &updatedProjectEntryInfoSourceIds)
{
    NanotraceHR::Tracer tracer{"synchronize directory infos", category()};

    auto compareKey = [](auto &&first, auto &&second) {
        return std::tie(first.contextSourceId, first.sourceId)
               <=> std::tie(second.contextSourceId, second.sourceId);
    };

    std::ranges::sort(projectEntryInfos, [&](auto &&first, auto &&second) {
        return std::tie(first.contextSourceId, first.sourceId)
               < std::tie(second.contextSourceId, second.sourceId);
    });

    auto range = s->selectProjectEntryInfosForDirectoryIdsStatement
                     .range<Storage::Synchronization::ProjectEntryInfo>(
                         Sqlite::toIntegers(updatedProjectEntryInfoSourceIds));

    auto insert = [&](const Storage::Synchronization::ProjectEntryInfo &projectEntryInfo) {
        NanotraceHR::Tracer tracer{"insert directory info",
                                   category(),
                                   keyValue("directory info", projectEntryInfo)};

        if (!projectEntryInfo.contextSourceId)
            throw ProjectEntryInfoHasInvalidProjectSourceId{};
        if (!projectEntryInfo.sourceId)
            throw ProjectEntryInfoHasInvalidSourceId{};

        s->insertProjectEntryInfoStatement.write(projectEntryInfo.contextSourceId,
                                                 projectEntryInfo.sourceId,
                                                 projectEntryInfo.moduleId,
                                                 projectEntryInfo.fileType);
    };

    auto update = [&](const Storage::Synchronization::ProjectEntryInfo &projectEntryInfoFromDatabase,
                      const Storage::Synchronization::ProjectEntryInfo &projectEntryInfo) {
        if (projectEntryInfoFromDatabase.fileType != projectEntryInfo.fileType
            || !compareInvalidAreTrue(projectEntryInfoFromDatabase.moduleId,
                                      projectEntryInfo.moduleId)) {
            NanotraceHR::Tracer tracer{"update directory info",
                                       category(),
                                       keyValue("directory info", projectEntryInfo),
                                       keyValue("directory info from database",
                                                projectEntryInfoFromDatabase)};

            s->updateProjectEntryInfoStatement.write(projectEntryInfo.contextSourceId,
                                                     projectEntryInfo.sourceId,
                                                     projectEntryInfo.moduleId,
                                                     projectEntryInfo.fileType);
            return Sqlite::UpdateChange::Update;
        }

        return Sqlite::UpdateChange::No;
    };

    auto remove = [&](const Storage::Synchronization::ProjectEntryInfo &projectEntryInfo) {
        NanotraceHR::Tracer tracer{"remove directory info",
                                   category(),
                                   keyValue("directory info", projectEntryInfo)};

        s->deleteProjectEntryInfoStatement.write(projectEntryInfo.contextSourceId,
                                                 projectEntryInfo.sourceId);
    };

    Sqlite::insertUpdateDelete(range, projectEntryInfos, compareKey, insert, update, remove);
}

void ProjectStorage::synchronizeFileStatuses(FileStatuses &fileStatuses,
                                             const SourceIds &updatedSourceIds)
{
    NanotraceHR::Tracer tracer{"synchronize file statuses", category()};

    auto compareKey = [](auto &&first, auto &&second) { return first.sourceId <=> second.sourceId; };

    std::ranges::sort(fileStatuses, {}, &FileStatus::sourceId);

    auto range = s->selectFileStatusesForSourceIdsStatement.range<FileStatus>(
        Sqlite::toIntegers(updatedSourceIds));

    auto insert = [&](const FileStatus &fileStatus) {
        NanotraceHR::Tracer tracer{"insert file status",
                                   category(),
                                   keyValue("file status", fileStatus)};

        if (!fileStatus.sourceId)
            throw FileStatusHasInvalidSourceId{};
        s->insertFileStatusStatement.write(fileStatus.sourceId,
                                           fileStatus.size,
                                           fileStatus.lastModified);
    };

    auto update = [&](const FileStatus &fileStatusFromDatabase, const FileStatus &fileStatus) {
        if (fileStatusFromDatabase.lastModified != fileStatus.lastModified
            || fileStatusFromDatabase.size != fileStatus.size) {
            NanotraceHR::Tracer tracer{"update file status",
                                       category(),
                                       keyValue("file status", fileStatus),
                                       keyValue("file status from database", fileStatusFromDatabase)};

            s->updateFileStatusStatement.write(fileStatus.sourceId,
                                               fileStatus.size,
                                               fileStatus.lastModified);
            return Sqlite::UpdateChange::Update;
        }

        return Sqlite::UpdateChange::No;
    };

    auto remove = [&](const FileStatus &fileStatus) {
        NanotraceHR::Tracer tracer{"remove file status",
                                   category(),
                                   keyValue("file status", fileStatus)};

        s->deleteFileStatusStatement.write(fileStatus.sourceId);
    };

    Sqlite::insertUpdateDelete(range, fileStatuses, compareKey, insert, update, remove);
}

void ProjectStorage::synchronizeImports(Storage::Imports &imports,
                                        const SourceIds &updatedSourceIds,
                                        Storage::Imports &moduleDependencies,
                                        const SourceIds &updatedModuleDependencySourceIds,
                                        Storage::Synchronization::ModuleExportedImports &moduleExportedImports,
                                        const ModuleIds &updatedModuleIds,
                                        Bases &relinkableBases)
{
    NanotraceHR::Tracer tracer{"synchronize imports", category()};

    synchronizeModuleExportedImports(moduleExportedImports, updatedModuleIds);
    NanotraceHR::Tracer importTracer{"synchronize qml document imports", category()};
    synchronizeDocumentImports(imports,
                               updatedSourceIds,
                               Storage::Synchronization::ImportKind::Import,
                               Relink::No,
                               relinkableBases);
    importTracer.end();
    NanotraceHR::Tracer moduleDependenciesTracer{"synchronize module depdencies", category()};
    synchronizeDocumentImports(moduleDependencies,
                               updatedModuleDependencySourceIds,
                               Storage::Synchronization::ImportKind::ModuleDependency,
                               Relink::Yes,
                               relinkableBases);
    moduleDependenciesTracer.end();
}

void ProjectStorage::synchronizeModuleExportedImports(
    Storage::Synchronization::ModuleExportedImports &moduleExportedImports,
    const ModuleIds &updatedModuleIds)
{
    NanotraceHR::Tracer tracer{"synchronize module exported imports", category()};
    std::ranges::sort(moduleExportedImports, [](auto &&first, auto &&second) {
        return std::tie(first.moduleId, first.exportedModuleId)
               < std::tie(second.moduleId, second.exportedModuleId);
    });

    auto range = s->selectModuleExportedImportsForSourceIdStatement
                     .range<Storage::Synchronization::ModuleExportedImportView>(
                         Sqlite::toIntegers(updatedModuleIds));

    auto compareKey = [](const Storage::Synchronization::ModuleExportedImportView &view,
                         const Storage::Synchronization::ModuleExportedImport &import) {
        return std::tie(view.moduleId, view.exportedModuleId)
               <=> std::tie(import.moduleId, import.exportedModuleId);
    };

    auto insert = [&](const Storage::Synchronization::ModuleExportedImport &import) {
        NanotraceHR::Tracer tracer{"insert module exported import",
                                   category(),
                                   keyValue("module exported import", import),
                                   keyValue("module id", import.moduleId)};
        tracer.tick("exported module", keyValue("module id", import.exportedModuleId));

        s->insertModuleExportedImportWithVersionStatement.write(import.moduleId,
                                                                import.exportedModuleId,
                                                                import.isAutoVersion,
                                                                import.version.major.value,
                                                                import.version.minor.value);
    };

    auto update = [](const Storage::Synchronization::ModuleExportedImportView &,
                     const Storage::Synchronization::ModuleExportedImport &) {
        return Sqlite::UpdateChange::No;
    };

    auto remove = [&](const Storage::Synchronization::ModuleExportedImportView &view) {
        NanotraceHR::Tracer tracer{"remove module exported import",
                                   category(),
                                   keyValue("module exported import view", view),
                                   keyValue("module id", view.moduleId)};
        tracer.tick("exported module", keyValue("module id", view.exportedModuleId));

        s->deleteModuleExportedImportStatement.write(view.moduleExportedImportId);
    };

    Sqlite::insertUpdateDelete(range, moduleExportedImports, compareKey, insert, update, remove);
}

void ProjectStorage::handleAliasPropertyDeclarationsWithPropertyType(
    TypeId typeId, AliasPropertyDeclarations &relinkableAliasPropertyDeclarations)
{
    NanotraceHR::Tracer tracer{"handle alias property declarations with property type",
                               category(),
                               keyValue("type id", typeId),
                               keyValue("relinkable alias property declarations",
                                        relinkableAliasPropertyDeclarations)};

    auto callback = [&](TypeId typeId_,
                        PropertyDeclarationId propertyDeclarationId,
                        ImportedTypeNameId aliasPropertyImportedTypeNameId,
                        PropertyDeclarationId aliasPropertyDeclarationId,
                        PropertyDeclarationId aliasPropertyDeclarationTailId) {
        auto aliasPropertyName = s->selectPropertyNameStatement.value<Utils::SmallString>(
            aliasPropertyDeclarationId);
        Utils::SmallString aliasPropertyNameTail;
        if (aliasPropertyDeclarationTailId)
            aliasPropertyNameTail = s->selectPropertyNameStatement.value<Utils::SmallString>(
                aliasPropertyDeclarationTailId);

        relinkableAliasPropertyDeclarations.emplace_back(TypeId{typeId_},
                                                         propertyDeclarationId,
                                                         aliasPropertyImportedTypeNameId,
                                                         std::move(aliasPropertyName),
                                                         std::move(aliasPropertyNameTail),
                                                         fetchTypeSourceId(typeId_));

        s->updateAliasPropertyDeclarationToNullStatement.write(propertyDeclarationId);
    };

    s->selectAliasPropertiesDeclarationForPropertiesWithTypeIdStatement.readCallback(callback, typeId);
}

void ProjectStorage::handlePropertyDeclarationWithPropertyType(
    TypeId typeId, PropertyDeclarations &relinkablePropertyDeclarations)
{
    NanotraceHR::Tracer tracer{"handle property declarations with property type",
                               category(),
                               keyValue("type id", typeId),
                               keyValue("relinkable property declarations",
                                        relinkablePropertyDeclarations)};

    s->updatesPropertyDeclarationPropertyTypeToNullStatement.readTo(relinkablePropertyDeclarations,
                                                                    typeId);
}

void ProjectStorage::handlePropertyDeclarationsWithExportedTypeNameAndTypeId(
    Utils::SmallStringView exportedTypeName,
    TypeId typeId,
    PropertyDeclarations &relinkablePropertyDeclarations)
{
    NanotraceHR::Tracer tracer{"handle property declarations with exported type name and type id",
                               category(),
                               keyValue("type name", exportedTypeName),
                               keyValue("type id", typeId),
                               keyValue("relinkable property declarations",
                                        relinkablePropertyDeclarations)};

    s->selectPropertyDeclarationForPrototypeIdAndTypeNameStatement.readTo(relinkablePropertyDeclarations,
                                                                          exportedTypeName,
                                                                          typeId);
}

void ProjectStorage::handleAliasPropertyDeclarationsWithExportedTypeNameAndTypeId(
    Utils::SmallStringView exportedTypeName,
    TypeId typeId,
    AliasPropertyDeclarations &relinkableAliasPropertyDeclarations)
{
    NanotraceHR::Tracer tracer{
        "handle alias property declarations with exported type name and type id",
        category(),
        keyValue("type name", exportedTypeName),
        keyValue("type id", typeId),
        keyValue("relinkable alias property declarations", relinkableAliasPropertyDeclarations)};

    auto callback = [&](TypeId typeId_,
                        PropertyDeclarationId propertyDeclarationId,
                        ImportedTypeNameId aliasPropertyImportedTypeNameId,
                        PropertyDeclarationId aliasPropertyDeclarationId,
                        PropertyDeclarationId aliasPropertyDeclarationTailId) {
        auto aliasPropertyName = s->selectPropertyNameStatement.value<Utils::SmallString>(
            aliasPropertyDeclarationId);
        Utils::SmallString aliasPropertyNameTail;
        if (aliasPropertyDeclarationTailId)
            aliasPropertyNameTail = s->selectPropertyNameStatement.value<Utils::SmallString>(
                aliasPropertyDeclarationTailId);

        relinkableAliasPropertyDeclarations.emplace_back(TypeId{typeId_},
                                                         propertyDeclarationId,
                                                         aliasPropertyImportedTypeNameId,
                                                         std::move(aliasPropertyName),
                                                         std::move(aliasPropertyNameTail),
                                                         fetchTypeSourceId(typeId_));
    };

    s->selectAliasPropertyDeclarationForPrototypeIdAndTypeNameStatement.readCallback(callback,
                                                                                     exportedTypeName,
                                                                                     typeId);
}

void ProjectStorage::handleBases(TypeId baseId, Bases &relinkableBases)
{
    NanotraceHR::Tracer tracer{"handle bases", category(), keyValue("type id", baseId)};

    QVarLengthArray<TypeId, 256> deletedBases;

    s->updateBasesByBaseIdStatement.readTo(deletedBases, baseId, unresolvedTypeId);
    s->updatePrototypesByPrototypeIdStatement.write(baseId, unresolvedTypeId);

    std::ranges::sort(deletedBases);
    auto removed = std::ranges::unique(deletedBases);
    deletedBases.erase(removed.begin(), removed.end());

    struct NameIds
    {
        NameIds() = default;

        NameIds(ImportedTypeNameId prototypeNameId, ImportedTypeNameId extensionNameId)
            : prototypeNameId{prototypeNameId}
            , extensionNameId{extensionNameId}
        {}

        ImportedTypeNameId prototypeNameId;
        ImportedTypeNameId extensionNameId;
    };

    for (TypeId typeId : deletedBases) {
        auto nameIds = s->selectBaseNamesByTypeIdStatement.value<NameIds>(typeId);

        if (nameIds.prototypeNameId or nameIds.extensionNameId) {
            tracer.tick("add relinkable base", keyValue("type id", typeId));

            relinkableBases.emplace_back(typeId, nameIds.prototypeNameId, nameIds.extensionNameId);
        }
    }
}

void ProjectStorage::handleBasesWithExportedTypeNameAndTypeId(Utils::SmallStringView exportedTypeName,
                                                              TypeId typeId,
                                                              Bases &relinkableBases)
{
    NanotraceHR::Tracer tracer{"handle invalid bases with exported type name and type id",
                               category(),
                               keyValue("type id", typeId),
                               keyValue("exported type name", exportedTypeName)};

    auto callback =
        [&](TypeId typeId, ImportedTypeNameId prototypeNameId, ImportedTypeNameId extensionNameId) {
            tracer.tick("add relinkable base", keyValue("type id", typeId));

            relinkableBases.emplace_back(typeId, prototypeNameId, extensionNameId);
        };

    s->selectTypeIdAndBaseNameIdForBaseIdAndTypeNameStatement.readCallback(callback,
                                                                           exportedTypeName,
                                                                           typeId);
}

void ProjectStorage::deleteType(TypeId typeId,
                                AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                                PropertyDeclarations &relinkablePropertyDeclarations,
                                Bases &relinkableBases)
{
    NanotraceHR::Tracer tracer{"delete type", category(), keyValue("type id", typeId)};

    handlePropertyDeclarationWithPropertyType(typeId, relinkablePropertyDeclarations);
    handleAliasPropertyDeclarationsWithPropertyType(typeId, relinkableAliasPropertyDeclarations);
    handleBases(typeId, relinkableBases);
    s->deleteEnumerationDeclarationByTypeIdStatement.write(typeId);
    s->deletePropertyDeclarationByTypeIdStatement.write(typeId);
    s->deleteFunctionDeclarationByTypeIdStatement.write(typeId);
    s->deleteSignalDeclarationByTypeIdStatement.write(typeId);
    s->deletePrototypeByTypeIdStatement.write(typeId);
    s->deleteBaseByTypeIdStatement.write(typeId);
    s->resetTypeStatement.write(typeId);
}

void ProjectStorage::relinkAliasPropertyDeclarations(AliasPropertyDeclarations &aliasPropertyDeclarations,
                                                     const TypeIds &deletedTypeIds)
{
    NanotraceHR::Tracer tracer{"relink alias properties",
                               category(),
                               keyValue("alias property declarations", aliasPropertyDeclarations),
                               keyValue("deleted type ids", deletedTypeIds)};

    std::ranges::sort(aliasPropertyDeclarations);
    // todo remove duplicates

    auto relink = [&](const AliasPropertyDeclaration &alias) {
        auto typeId = fetchTypeId(alias.aliasImportedTypeNameId);

        if (typeId) {
            auto propertyDeclaration = fetchPropertyDeclarationByTypeIdAndNameUngarded(
                typeId, alias.aliasPropertyName);
            if (propertyDeclaration) {
                auto [propertyImportedTypeNameId, propertyTypeId, aliasId, propertyTraits] = *propertyDeclaration;

                s->updatePropertyDeclarationWithAliasAndTypeStatement.write(alias.propertyDeclarationId,
                                                                            propertyTypeId,
                                                                            propertyTraits,
                                                                            propertyImportedTypeNameId,
                                                                            aliasId);
                return;
            }
        }

        errorNotifier->typeNameCannotBeResolved(fetchImportedTypeName(alias.aliasImportedTypeNameId),
                                                fetchTypeSourceId(alias.typeId));
        s->resetAliasPropertyDeclarationStatement.write(alias.propertyDeclarationId,
                                                        Storage::PropertyDeclarationTraits{});
    };

    Utils::set_greedy_difference(aliasPropertyDeclarations,
                                 deletedTypeIds,
                                 relink,
                                 {},
                                 &AliasPropertyDeclaration::typeId);
}

void ProjectStorage::relinkPropertyDeclarations(PropertyDeclarations &relinkablePropertyDeclaration,
                                                const TypeIds &deletedTypeIds)
{
    NanotraceHR::Tracer tracer{"relink property declarations",
                               category(),
                               keyValue("relinkable property declarations",
                                        relinkablePropertyDeclaration),
                               keyValue("deleted type ids", deletedTypeIds)};

    std::ranges::sort(relinkablePropertyDeclaration);
    relinkablePropertyDeclaration.erase(std::unique(relinkablePropertyDeclaration.begin(),
                                                    relinkablePropertyDeclaration.end()),
                                        relinkablePropertyDeclaration.end());

    Utils::set_greedy_difference(
        relinkablePropertyDeclaration,
        deletedTypeIds,
        [&](const PropertyDeclaration &property) {
            TypeId propertyTypeId = fetchTypeId(property.importedTypeNameId);

            if (!propertyTypeId) {
                errorNotifier->typeNameCannotBeResolved(fetchImportedTypeName(
                                                            property.importedTypeNameId),
                                                        fetchTypeSourceId(property.typeId));
                propertyTypeId = TypeId{};
            }

            s->updatePropertyDeclarationTypeStatement.write(property.propertyDeclarationId,
                                                            propertyTypeId);
        },
        {},
        &PropertyDeclaration::typeId);
}

void ProjectStorage::relinkBases(Bases &relinkableBases, const TypeIds &deletedTypeIds)
{
    NanotraceHR::Tracer tracer{"relink bases", category()};

    std::ranges::sort(relinkableBases);
    auto [begin, end] = std::ranges::unique(relinkableBases);
    relinkableBases.erase(begin, end);

    Utils::set_greedy_difference(
        relinkableBases,
        deletedTypeIds,
        [&](const Base &base) {
            NanotraceHR::Tracer tracer{"relink base", category(), keyValue("type id", base.typeId)};

            auto getBaseId = [&](ImportedTypeNameId baseNameId) {
                if (baseNameId) {
                    TypeId baseId = fetchTypeId(baseNameId);
                    if (!baseId)
                        errorNotifier->typeNameCannotBeResolved(fetchImportedTypeName(baseNameId),
                                                                fetchTypeSourceId(base.typeId));
                    return baseId;
                }
                return TypeId{};
            };

            TypeId prototypeId = getBaseId(base.prototypeNameId);

            TypeId extensionId = getBaseId(base.extensionNameId);

            auto changeBases = updateBases(base.typeId, prototypeId, extensionId);
            updatePrototypes(base.typeId, prototypeId);

            if (changeBases)
                checkForPrototypeChainCycle(base.typeId);
            tracer.end(keyValue("prototype id", prototypeId), keyValue("extension id", extensionId));
        },
        {},
        &Base::typeId);
}

void ProjectStorage::deleteNotUpdatedTypes(const TypeIds &updatedTypeIds,
                                           const SourceIds &updatedTypeSourceIds,
                                           const TypeIds &typeIdsToBeDeleted,
                                           AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                                           PropertyDeclarations &relinkablePropertyDeclarations,
                                           Bases &relinkableBases,
                                           TypeIds &deletedTypeIds)
{
    NanotraceHR::Tracer tracer{"delete not updated types", category()};

    auto callback = [&](TypeId typeId) {
        deletedTypeIds.push_back(typeId);
        deleteType(typeId,
                   relinkableAliasPropertyDeclarations,
                   relinkablePropertyDeclarations,
                   relinkableBases);
    };

    s->selectNotUpdatedTypesInSourcesStatement.readCallback(callback,
                                                            Sqlite::toIntegers(updatedTypeSourceIds),
                                                            Sqlite::toIntegers(updatedTypeIds));
    for (TypeId typeIdToBeDeleted : typeIdsToBeDeleted)
        callback(typeIdToBeDeleted);
}

void ProjectStorage::relink(AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                            PropertyDeclarations &relinkablePropertyDeclarations,
                            Bases &relinkableBases,
                            TypeIds &deletedTypeIds)
{
    NanotraceHR::Tracer tracer{"relink", category()};

    std::ranges::sort(deletedTypeIds);

    relinkBases(relinkableBases, deletedTypeIds);
    relinkPropertyDeclarations(relinkablePropertyDeclarations, deletedTypeIds);
    relinkAliasPropertyDeclarations(relinkableAliasPropertyDeclarations, deletedTypeIds);
}

PropertyDeclarationId ProjectStorage::fetchAliasId(TypeId aliasTypeId,
                                                   Utils::SmallStringView aliasPropertyName,
                                                   Utils::SmallStringView aliasPropertyNameTail)
{
    NanotraceHR::Tracer tracer{"fetch alias id",
                               category(),
                               keyValue("alias type id", aliasTypeId),
                               keyValue("alias property name", aliasPropertyName),
                               keyValue("alias property name tail", aliasPropertyNameTail)};

    if (aliasPropertyNameTail.empty())
        return fetchPropertyDeclarationIdByTypeIdAndNameUngarded(aliasTypeId, aliasPropertyName);

    auto stemAlias = fetchPropertyDeclarationByTypeIdAndNameUngarded(aliasTypeId, aliasPropertyName);

    if (!stemAlias)
        return PropertyDeclarationId{};

    return fetchPropertyDeclarationIdByTypeIdAndNameUngarded(stemAlias->propertyTypeId,
                                                             aliasPropertyNameTail);
}

void ProjectStorage::linkAliasPropertyDeclarationAliasIds(
    const AliasPropertyDeclarations &aliasDeclarations, RaiseError raiseError)
{
    NanotraceHR::Tracer tracer{"link alias property declarations alias ids",
                               category(),
                               keyValue("alias property declarations", aliasDeclarations)};

    for (const auto &aliasDeclaration : aliasDeclarations) {
        auto aliasTypeId = fetchTypeId(aliasDeclaration.aliasImportedTypeNameId);

        if (aliasTypeId) {
            auto aliasId = fetchAliasId(aliasTypeId,
                                        aliasDeclaration.aliasPropertyName,
                                        aliasDeclaration.aliasPropertyNameTail);

            if (aliasId) {
                s->updatePropertyDeclarationAliasIdAndTypeNameIdStatement
                    .write(aliasDeclaration.propertyDeclarationId,
                           aliasId,
                           aliasDeclaration.aliasImportedTypeNameId);
            } else {
                s->resetAliasPropertyDeclarationStatement.write(aliasDeclaration.propertyDeclarationId,
                                                                Storage::PropertyDeclarationTraits{});
                s->updatePropertyAliasDeclarationRecursivelyWithTypeAndTraitsStatement
                    .write(aliasDeclaration.propertyDeclarationId,
                           TypeId{},
                           Storage::PropertyDeclarationTraits{});

                errorNotifier->propertyNameDoesNotExists(aliasDeclaration.composedPropertyName(),
                                                         aliasDeclaration.sourceId);
            }
        } else if (raiseError == RaiseError::Yes) {
            errorNotifier->typeNameCannotBeResolved(fetchImportedTypeName(
                                                        aliasDeclaration.aliasImportedTypeNameId),
                                                    aliasDeclaration.sourceId);
            s->resetAliasPropertyDeclarationStatement.write(aliasDeclaration.propertyDeclarationId,
                                                            Storage::PropertyDeclarationTraits{});
        }
    }
}

void ProjectStorage::updateAliasPropertyDeclarationValues(const AliasPropertyDeclarations &aliasDeclarations)
{
    NanotraceHR::Tracer tracer{"update alias property declarations",
                               category(),
                               keyValue("alias property declarations", aliasDeclarations)};

    for (const auto &aliasDeclaration : aliasDeclarations) {
        s->updatePropertiesDeclarationValuesOfAliasStatement.write(
            aliasDeclaration.propertyDeclarationId);
        s->updatePropertyAliasDeclarationRecursivelyStatement.write(
            aliasDeclaration.propertyDeclarationId);
    }
}

void ProjectStorage::checkAliasPropertyDeclarationCycles(const AliasPropertyDeclarations &aliasDeclarations)
{
    NanotraceHR::Tracer tracer{"check alias property declarations cycles",
                               category(),
                               keyValue("alias property declarations", aliasDeclarations)};
    for (const auto &aliasDeclaration : aliasDeclarations)
        checkForAliasChainCycle(aliasDeclaration.propertyDeclarationId);
}

void ProjectStorage::linkAliases(const AliasPropertyDeclarations &aliasPropertyDeclarationsToLink,
                                 RaiseError raiseError)
{
    NanotraceHR::Tracer tracer{"link aliases", category()};

    linkAliasPropertyDeclarationAliasIds(aliasPropertyDeclarationsToLink, raiseError);

    checkAliasPropertyDeclarationCycles(aliasPropertyDeclarationsToLink);

    updateAliasPropertyDeclarationValues(aliasPropertyDeclarationsToLink);
}

void ProjectStorage::repairBrokenAliasPropertyDeclarations()
{
    NanotraceHR::Tracer tracer{"repair broken alias property declarations", category()};

    auto brokenAliasPropertyDeclarations = s->selectBrokenAliasPropertyDeclarationsStatement
                                               .values<AliasPropertyDeclaration>();

    linkAliases(brokenAliasPropertyDeclarations, RaiseError::No);
}

void ProjectStorage::synchronizeExportedTypes(
    Storage::Synchronization::ExportedTypes &exportedTypes,
    const SourceIds &updatedExportedTypeSourceIds,
    AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
    PropertyDeclarations &relinkablePropertyDeclarations,
    Bases &relinkableBases,
    ExportedTypesChanged &exportedTypesChanged,
    Storage::Info::ExportedTypeNames &removedExportedTypeNames,
    Storage::Info::ExportedTypeNames &addedExportedTypeNames)
{
    NanotraceHR::Tracer tracer{"synchronize exported types", category()};

    removedExportedTypeNames.reserve(exportedTypes.size());
    addedExportedTypeNames.reserve(exportedTypes.size());

    std::ranges::sort(exportedTypes, [](auto &&first, auto &&second) {
        return std::tie(first.name, first.moduleId, first.version.major, first.version.minor)
               < std::tie(second.name, second.moduleId, second.version.major, second.version.minor);
    });

    auto range = s->selectExportedTypesForSourceIdsStatement
                     .range<Storage::Synchronization::ExportedTypeView>(
                         Sqlite::toIntegers(updatedExportedTypeSourceIds));

    auto compareKey = [](const Storage::Synchronization::ExportedTypeView &view,
                         const Storage::Synchronization::ExportedType &type) {
        return std::tie(view.name, view.moduleId, view.version.major.value, view.version.minor.value)
               <=> std::tie(type.name, type.moduleId, type.version.major.value, type.version.minor.value);
    };

    auto insert = [&](const Storage::Synchronization::ExportedType &type) {
        NanotraceHR::Tracer tracer{"insert exported type",
                                   category(),
                                   keyValue("exported type", type),
                                   keyValue("module id", type.moduleId)};
        if (!type.moduleId)
            throw QmlDesigner::ModuleDoesNotExists{};

        auto typeId = declareType(type.typeIdName, type.typeSourceId);

        try {
            s->insertExportedTypeNamesStatement.write(type.moduleId,
                                                      type.name,
                                                      type.version.major.value,
                                                      type.version.minor.value,
                                                      typeId,
                                                      type.contextSourceId);

        } catch (const Sqlite::ConstraintPreventsModification &) {
            errorNotifier->exportedTypeNameIsDuplicate(type.moduleId, type.name);
            throw QmlDesigner::ExportedTypeCannotBeInserted{type.name};
        }

        handlePropertyDeclarationsWithExportedTypeNameAndTypeId(type.name,
                                                                TypeId{},
                                                                relinkablePropertyDeclarations);
        handleAliasPropertyDeclarationsWithExportedTypeNameAndTypeId(type.name,
                                                                     TypeId{},
                                                                     relinkableAliasPropertyDeclarations);
        handleBasesWithExportedTypeNameAndTypeId(type.name, unresolvedTypeId, relinkableBases);

        addedExportedTypeNames.emplace_back(type.moduleId, typeId, type.name, type.version);

        exportedTypesChanged = ExportedTypesChanged::Yes;
    };

    auto update = [&](const Storage::Synchronization::ExportedTypeView &view,
                      const Storage::Synchronization::ExportedType &type) {
        auto typeId = declareType(type.typeIdName, type.typeSourceId);

        Sqlite::UpdateChange update = Sqlite::UpdateChange::No;

        if (view.typeId != typeId) {
            NanotraceHR::Tracer tracer{"update exported type id", category()};

            handlePropertyDeclarationWithPropertyType(view.typeId, relinkablePropertyDeclarations);
            handleAliasPropertyDeclarationsWithPropertyType(view.typeId,
                                                            relinkableAliasPropertyDeclarations);
            handleBases(view.typeId, relinkableBases);

            s->updateExportedTypeNameTypeIdStatement.write(view.moduleId,
                                                           view.name,
                                                           view.version.major.value,
                                                           view.version.minor.value,
                                                           typeId);

            exportedTypesChanged = ExportedTypesChanged::Yes;

            addedExportedTypeNames.emplace_back(type.moduleId, typeId, type.name, type.version);
            removedExportedTypeNames.emplace_back(view.moduleId, view.typeId, view.name, view.version);

            update = Sqlite::UpdateChange::Update;
        }

        if (view.contextSourceId != type.contextSourceId) {
            NanotraceHR::Tracer tracer{"update exported context source id", category()};

            s->updateExportedTypeNameContextSourceIdStatement.write(view.moduleId,
                                                                    view.name,
                                                                    view.version.major.value,
                                                                    view.version.minor.value,
                                                                    type.contextSourceId);
            update = Sqlite::UpdateChange::Update;
        }

        return update;
    };

    auto remove = [&](const Storage::Synchronization::ExportedTypeView &view) {
        NanotraceHR::Tracer tracer{"remove exported type",
                                   category(),
                                   keyValue("exported type", view),
                                   keyValue("type id", view.typeId),
                                   keyValue("module id", view.moduleId)};

        handlePropertyDeclarationWithPropertyType(view.typeId, relinkablePropertyDeclarations);
        handleAliasPropertyDeclarationsWithPropertyType(view.typeId,
                                                        relinkableAliasPropertyDeclarations);
        handleBases(view.typeId, relinkableBases);

        s->deleteExportedTypeNameStatement.write(view.moduleId,
                                                 view.name,
                                                 view.version.major.value,
                                                 view.version.minor.value);

        removedExportedTypeNames.emplace_back(view.moduleId, view.typeId, view.name, view.version);

        exportedTypesChanged = ExportedTypesChanged::Yes;
    };

    Sqlite::insertUpdateDelete(range, exportedTypes, compareKey, insert, update, remove);
}

void ProjectStorage::synchronizePropertyDeclarationsInsertAlias(
    AliasPropertyDeclarations &aliasPropertyDeclarationsToLink,
    const Storage::Synchronization::PropertyDeclaration &value,
    SourceId sourceId,
    TypeId typeId)
{
    NanotraceHR::Tracer tracer{"insert property declaration to alias",
                               category(),
                               keyValue("property declaration", value)};

    auto [propertyImportedTypeNameId, _] = fetchImportedTypeNameId(value.typeName, sourceId);

    auto callback = [&](PropertyDeclarationId propertyDeclarationId) {
        aliasPropertyDeclarationsToLink.emplace_back(typeId,
                                                     propertyDeclarationId,
                                                     propertyImportedTypeNameId,
                                                     value.aliasPropertyName,
                                                     value.aliasPropertyNameTail,
                                                     sourceId);
        return Sqlite::CallbackControl::Abort;
    };

    s->insertAliasPropertyDeclarationStatement.readCallback(callback,
                                                            typeId,
                                                            value.name,
                                                            propertyImportedTypeNameId,
                                                            value.aliasPropertyName,
                                                            value.aliasPropertyNameTail);
}

QVarLengthArray<PropertyDeclarationId, 128> ProjectStorage::fetchPropertyDeclarationIds(
    TypeId baseTypeId) const
{
    QVarLengthArray<PropertyDeclarationId, 128> propertyDeclarationIds;

    s->selectLocalPropertyDeclarationIdsForTypeStatement.readTo(propertyDeclarationIds, baseTypeId);

    auto range = s->selectPrototypeAndExtensionIdsStatement.range<TypeId>(baseTypeId);

    for (TypeId prototype : range) {
        s->selectLocalPropertyDeclarationIdsForTypeStatement.readTo(propertyDeclarationIds, prototype);
    }

    return propertyDeclarationIds;
}

PropertyDeclarationId ProjectStorage::fetchNextPropertyDeclarationId(
    TypeId baseTypeId, Utils::SmallStringView propertyName) const
{
    auto range = s->selectPrototypeAndExtensionIdsStatement.range<TypeId>(baseTypeId);

    for (TypeId prototype : range) {
        auto propertyDeclarationId = s->selectPropertyDeclarationIdByTypeIdAndNameStatement
                                         .value<PropertyDeclarationId>(prototype, propertyName);

        if (propertyDeclarationId)
            return propertyDeclarationId;
    }

    return PropertyDeclarationId{};
}

PropertyDeclarationId ProjectStorage::fetchPropertyDeclarationId(TypeId typeId,
                                                                 Utils::SmallStringView propertyName) const
{
    auto propertyDeclarationId = s->selectPropertyDeclarationIdByTypeIdAndNameStatement
                                     .value<PropertyDeclarationId>(typeId, propertyName);

    if (propertyDeclarationId)
        return propertyDeclarationId;

    return fetchNextPropertyDeclarationId(typeId, propertyName);
}

PropertyDeclarationId ProjectStorage::fetchNextDefaultPropertyDeclarationId(TypeId baseTypeId) const
{
    auto range = s->selectPrototypeAndExtensionIdsStatement.range<TypeId>(baseTypeId);

    for (TypeId prototype : range) {
        auto propertyDeclarationId = s->selectDefaultPropertyDeclarationIdStatement
                                         .value<PropertyDeclarationId>(prototype);

        if (propertyDeclarationId)
            return propertyDeclarationId;
    }

    return PropertyDeclarationId{};
}

PropertyDeclarationId ProjectStorage::fetchDefaultPropertyDeclarationId(TypeId typeId) const
{
    auto propertyDeclarationId = s->selectDefaultPropertyDeclarationIdStatement
                                     .value<PropertyDeclarationId>(typeId);

    if (propertyDeclarationId)
        return propertyDeclarationId;

    return fetchNextDefaultPropertyDeclarationId(typeId);
}

void ProjectStorage::synchronizePropertyDeclarationsInsertProperty(
    const Storage::Synchronization::PropertyDeclaration &value, SourceId sourceId, TypeId typeId)
{
    NanotraceHR::Tracer tracer{"insert property declaration",
                               category(),
                               keyValue("property declaration", value)};

    auto [propertyImportedTypeNameId, typeNameKind] = fetchImportedTypeNameId(value.typeName,
                                                                              sourceId);
    auto propertyTypeId = fetchTypeId(propertyImportedTypeNameId, typeNameKind);

    if (!propertyTypeId) {
        auto typeName = std::visit([](auto &&importedTypeName) { return importedTypeName.name; },
                                   value.typeName);
        errorNotifier->typeNameCannotBeResolved(typeName, sourceId);
        propertyTypeId = TypeId{};
    }

    auto propertyDeclarationId = s->insertPropertyDeclarationStatement.value<PropertyDeclarationId>(
        typeId, value.name, propertyTypeId, value.traits, propertyImportedTypeNameId);

    auto nextPropertyDeclarationId = fetchNextPropertyDeclarationId(typeId, value.name);
    if (nextPropertyDeclarationId) {
        s->updateAliasIdPropertyDeclarationStatement.write(nextPropertyDeclarationId,
                                                           propertyDeclarationId);
        s->updatePropertyAliasDeclarationRecursivelyWithTypeAndTraitsStatement
            .write(propertyDeclarationId, propertyTypeId, value.traits);
    }
}

void ProjectStorage::synchronizePropertyDeclarationsUpdateAlias(
    AliasPropertyDeclarations &aliasPropertyDeclarationsToLink,
    const Storage::Synchronization::PropertyDeclarationView &view,
    const Storage::Synchronization::PropertyDeclaration &value,
    SourceId sourceId)
{
    NanotraceHR::Tracer tracer{"update property declaration to alias",
                               category(),
                               keyValue("property declaration", value),
                               keyValue("property declaration view", view)};

    auto [importedTypeName, _] = fetchImportedTypeNameId(value.typeName, sourceId);

    aliasPropertyDeclarationsToLink.emplace_back(view.propertyTypeId,
                                                 view.id,
                                                 importedTypeName,
                                                 value.aliasPropertyName,
                                                 value.aliasPropertyNameTail,
                                                 sourceId,
                                                 view.aliasId);
}

Sqlite::UpdateChange ProjectStorage::synchronizePropertyDeclarationsUpdateProperty(
    const Storage::Synchronization::PropertyDeclarationView &view,
    const Storage::Synchronization::PropertyDeclaration &value,
    SourceId sourceId,
    PropertyDeclarationIds &propertyDeclarationIds)
{
    NanotraceHR::Tracer tracer{"update property declaration",
                               category(),
                               keyValue("property declaration", value),
                               keyValue("property declaration view", view)};

    auto [propertyImportedTypeNameId, typeNameKind] = fetchImportedTypeNameId(value.typeName,
                                                                              sourceId);

    auto propertyTypeId = fetchTypeId(propertyImportedTypeNameId, typeNameKind);

    if (!propertyTypeId) {
        auto typeName = std::visit([](auto &&importedTypeName) { return importedTypeName.name; },
                                   value.typeName);
        errorNotifier->typeNameCannotBeResolved(typeName, sourceId);
        propertyTypeId = TypeId{};
        propertyDeclarationIds.push_back(view.id);
    }

    if (view.traits == value.traits && compareId(propertyTypeId, view.propertyTypeId)
        && propertyImportedTypeNameId == view.typeNameId)
        return Sqlite::UpdateChange::No;

    s->updatePropertyDeclarationStatement.write(view.id,
                                                propertyTypeId,
                                                value.traits,
                                                propertyImportedTypeNameId);
    s->updatePropertyAliasDeclarationRecursivelyWithTypeAndTraitsStatement.write(view.id,
                                                                                 propertyTypeId,
                                                                                 value.traits);
    propertyDeclarationIds.push_back(view.id);

    tracer.end(keyValue("updated", "yes"));

    return Sqlite::UpdateChange::Update;
}

void ProjectStorage::synchronizePropertyDeclarations(
    TypeId typeId,
    Storage::Synchronization::PropertyDeclarations &propertyDeclarations,
    SourceId sourceId,
    AliasPropertyDeclarations &aliasPropertyDeclarationsToLink,
    PropertyDeclarationIds &propertyDeclarationIds)
{
    NanotraceHR::Tracer tracer{"synchronize property declaration", category()};

    std::ranges::sort(propertyDeclarations, [](auto &&first, auto &&second) {
        return Sqlite::compare(first.name, second.name) < 0;
    });

    auto range = s->selectPropertyDeclarationsForTypeIdStatement
                     .range<Storage::Synchronization::PropertyDeclarationView>(typeId);

    auto compareKey = [](const Storage::Synchronization::PropertyDeclarationView &view,
                         const Storage::Synchronization::PropertyDeclaration &value) {
        return view.name <=> value.name;
    };

    auto insert = [&](const Storage::Synchronization::PropertyDeclaration &value) {
        if (value.kind == Storage::Synchronization::PropertyKind::Alias) {
            synchronizePropertyDeclarationsInsertAlias(aliasPropertyDeclarationsToLink,
                                                       value,
                                                       sourceId,
                                                       typeId);
        } else {
            synchronizePropertyDeclarationsInsertProperty(value, sourceId, typeId);
        }
    };

    auto update = [&](const Storage::Synchronization::PropertyDeclarationView &view,
                      const Storage::Synchronization::PropertyDeclaration &value) {
        if (value.kind == Storage::Synchronization::PropertyKind::Alias) {
            synchronizePropertyDeclarationsUpdateAlias(aliasPropertyDeclarationsToLink,
                                                       view,
                                                       value,
                                                       sourceId);
            propertyDeclarationIds.push_back(view.id);
        } else {
            return synchronizePropertyDeclarationsUpdateProperty(view,
                                                                 value,
                                                                 sourceId,
                                                                 propertyDeclarationIds);
        }

        return Sqlite::UpdateChange::No;
    };

    auto remove = [&](const Storage::Synchronization::PropertyDeclarationView &view) {
        NanotraceHR::Tracer tracer{"remove property declaration",
                                   category(),
                                   keyValue("property declaratio viewn", view)};

        auto nextPropertyDeclarationId = fetchNextPropertyDeclarationId(typeId, view.name);

        if (nextPropertyDeclarationId) {
            s->updateAliasPropertyDeclarationByAliasPropertyDeclarationIdStatement
                .write(nextPropertyDeclarationId, view.id);
        }

        s->updateDefaultPropertyIdToNullStatement.write(view.id);
        s->deletePropertyDeclarationStatement.write(view.id);
        propertyDeclarationIds.push_back(view.id);
    };

    Sqlite::insertUpdateDelete(range, propertyDeclarations, compareKey, insert, update, remove);
}

void ProjectStorage::resetRemovedAliasPropertyDeclarationsToNull(
    Storage::Synchronization::Type &type, PropertyDeclarationIds &propertyDeclarationIds)
{
    NanotraceHR::Tracer tracer{"reset removed alias property declaration to null", category()};

    Storage::Synchronization::PropertyDeclarations &aliasDeclarations = type.propertyDeclarations;

    std::ranges::sort(aliasDeclarations, {}, &Storage::Synchronization::PropertyDeclaration::name);

    auto range = s->selectPropertyDeclarationsWithAliasForTypeIdStatement
                     .range<AliasPropertyDeclarationView>(type.typeId);

    auto compareKey = [](const AliasPropertyDeclarationView &view,
                         const Storage::Synchronization::PropertyDeclaration &value) {
        return view.name <=> value.name;
    };

    auto insert = [&](const Storage::Synchronization::PropertyDeclaration &) {};

    auto update = [&](const AliasPropertyDeclarationView &,
                      const Storage::Synchronization::PropertyDeclaration &) {
        return Sqlite::UpdateChange::No;
    };

    auto remove = [&](const AliasPropertyDeclarationView &view) {
        NanotraceHR::Tracer tracer{"reset removed alias property declaration to null",
                                   category(),
                                   keyValue("alias property declaration view", view)};

        s->updatePropertyDeclarationAliasIdToNullStatement.write(view.id);
        propertyDeclarationIds.push_back(view.id);
    };

    Sqlite::insertUpdateDelete(range, aliasDeclarations, compareKey, insert, update, remove);
}

void ProjectStorage::resetRemovedAliasPropertyDeclarationsToNull(
    Storage::Synchronization::Types &types,
    AliasPropertyDeclarations &relinkableAliasPropertyDeclarations)
{
    NanotraceHR::Tracer tracer{"reset removed alias properties to null", category()};

    PropertyDeclarationIds propertyDeclarationIds;
    propertyDeclarationIds.reserve(types.size());

    for (auto &&type : types)
        resetRemovedAliasPropertyDeclarationsToNull(type, propertyDeclarationIds);

    removeRelinkableEntries(relinkableAliasPropertyDeclarations,
                            propertyDeclarationIds,
                            &AliasPropertyDeclaration::propertyDeclarationId);
}

void ProjectStorage::handleBasesWithSourceId(SourceId sourceId, Bases &relinkableBases)
{
    NanotraceHR::Tracer tracer{"handle bases with source id",
                               category(),
                               keyValue("source id", sourceId)};

    auto callback =
        [&](TypeId typeId, ImportedTypeNameId prototypeNameId, ImportedTypeNameId extensionNameId) {
            s->deleteAllBasesStatement.write(typeId);

            if (prototypeNameId or extensionNameId) {
                tracer.tick("add relinkable base", keyValue("type id", typeId));

                relinkableBases.emplace_back(typeId, prototypeNameId, extensionNameId);
            }
        };

    s->selectTypeBySourceIdStatement.readCallback(callback, sourceId);
}

ImportId ProjectStorage::insertDocumentImport(const Storage::Import &import,
                                              Storage::Synchronization::ImportKind importKind,
                                              ModuleId sourceModuleId,
                                              ImportId parentImportId,
                                              Relink relink,
                                              Bases &relinkableBases)
{
    if (relink == Relink::Yes)
        handleBasesWithSourceId(import.sourceId, relinkableBases);

    return s->insertDocumentImportWithVersionStatement.value<ImportId>(import.sourceId,
                                                                       import.moduleId,
                                                                       sourceModuleId,
                                                                       importKind,
                                                                       import.version.major.value,
                                                                       import.version.minor.value,
                                                                       parentImportId,
                                                                       import.contextSourceId,
                                                                       import.alias);
}

void ProjectStorage::synchronizeDocumentImports(Storage::Imports &imports,
                                                const SourceIds &updatedSourceIds,
                                                Storage::Synchronization::ImportKind importKind,
                                                Relink relink,
                                                Bases &relinkableBases)
{
    std::ranges::sort(imports, [](auto &&first, auto &&second) {
        return std::tie(first.sourceId, first.moduleId, first.alias, first.version)
               < std::tie(second.sourceId, second.moduleId, second.alias, second.version);
    });

    auto range = s->selectDocumentImportForContextSourceIdStatement
                     .range<Storage::Synchronization::ImportView>(Sqlite::toIntegers(updatedSourceIds),
                                                                  importKind);

    auto compareKey = [](const Storage::Synchronization::ImportView &view,
                         const Storage::Import &import) {
        return std::tie(view.sourceId,
                        view.moduleId,
                        view.alias,
                        view.version.major.value,
                        view.version.minor.value)
               <=> std::tie(import.sourceId,
                            import.moduleId,
                            import.alias,
                            import.version.major.value,
                            import.version.minor.value);
    };

    auto insert = [&](const Storage::Import &import) {
        NanotraceHR::Tracer tracer{"insert import",
                                   category(),
                                   keyValue("import", import),
                                   keyValue("import kind", importKind),
                                   keyValue("source id", import.sourceId),
                                   keyValue("module id", import.moduleId)};

        auto importId = insertDocumentImport(
            import, importKind, import.moduleId, ImportId{}, relink, relinkableBases);
        auto callback = [&](ModuleId exportedModuleId,
                            unsigned int majorVersion,
                            unsigned int minorVersion) {
            Storage::Import additionalImport{exportedModuleId,
                                             Storage::Version{majorVersion, minorVersion},
                                             import.sourceId,
                                             import.contextSourceId};

            auto exportedImportKind = importKind == Storage::Synchronization::ImportKind::Import
                                          ? Storage::Synchronization::ImportKind::ModuleExportedImport
                                          : Storage::Synchronization::ImportKind::ModuleExportedModuleDependency;

            NanotraceHR::Tracer tracer{"insert indirect import",
                                       category(),
                                       keyValue("import", additionalImport),
                                       keyValue("import kind", exportedImportKind),
                                       keyValue("source id", additionalImport.sourceId),
                                       keyValue("module id", additionalImport.moduleId)};

            auto indirectImportId = insertDocumentImport(additionalImport,
                                                         exportedImportKind,
                                                         import.moduleId,
                                                         importId,
                                                         relink,
                                                         relinkableBases);

            tracer.end(keyValue("import id", indirectImportId));
        };

        s->selectModuleExportedImportsForModuleIdStatement.readCallback(callback,
                                                                        import.moduleId,
                                                                        import.version.major.value,
                                                                        import.version.minor.value);
        tracer.end(keyValue("import id", importId));
    };

    auto update = [](const Storage::Synchronization::ImportView &, const Storage::Import &) {
        return Sqlite::UpdateChange::No;
    };

    auto remove = [&](const Storage::Synchronization::ImportView &view) {
        NanotraceHR::Tracer tracer{"remove import",
                                   category(),
                                   keyValue("import", view),
                                   keyValue("import id", view.importId),
                                   keyValue("source id", view.sourceId),
                                   keyValue("module id", view.moduleId)};

        s->deleteDocumentImportStatement.write(view.importId);
        s->deleteDocumentImportsWithParentImportIdStatement.write(view.sourceId, view.importId);
        if (relink == Relink::Yes) {
            handleBasesWithSourceId(view.sourceId, relinkableBases);
        }
    };

    Sqlite::insertUpdateDelete(range, imports, compareKey, insert, update, remove);
}

Utils::PathString ProjectStorage::createJson(const Storage::Synchronization::ParameterDeclarations &parameters)
{
    NanotraceHR::Tracer tracer{"create json from parameter declarations", category()};

    Utils::PathString json;
    json.append("[");

    Utils::SmallStringView comma{""};

    for (const auto &parameter : parameters) {
        json.append(comma);
        comma = ",";
        json.append(R"({"n":")");
        json.append(parameter.name);
        json.append(R"(","tn":")");
        json.append(parameter.typeName);
        if (parameter.traits == Storage::PropertyDeclarationTraits::None) {
            json.append("\"}");
        } else {
            json.append(R"(","tr":)");
            json.append(Utils::SmallString::number(Utils::to_underlying(parameter.traits)));
            json.append("}");
        }
    }

    json.append("]");

    return json;
}

TypeId ProjectStorage::fetchTypeIdByModuleIdAndExportedName(ModuleId moduleId,
                                                            Utils::SmallStringView name) const
{
    NanotraceHR::Tracer tracer{"fetch type id by module id and exported name",
                               category(),
                               keyValue("module id", moduleId),
                               keyValue("exported name", name)};

    return s->selectTypeIdByModuleIdAndExportedNameStatement.value<TypeId>(moduleId, name);
}

void ProjectStorage::addTypeIdToPropertyEditorQmlPaths(
    Storage::Synchronization::PropertyEditorQmlPaths &paths)
{
    NanotraceHR::Tracer tracer{"add type id to property editor qml paths", category()};

    for (auto &path : paths)
        path.typeId = fetchTypeIdByModuleIdAndExportedName(path.moduleId, path.typeName);
}

void ProjectStorage::synchronizePropertyEditorPaths(
    Storage::Synchronization::PropertyEditorQmlPaths &paths,
    DirectoryPathIds updatedPropertyEditorQmlPathsDirectoryPathIds)
{
    using Storage::Synchronization::PropertyEditorQmlPath;
    std::ranges::sort(paths, {}, &PropertyEditorQmlPath::typeId);

    auto range = s->selectPropertyEditorPathsForForSourceIdsStatement.range<PropertyEditorQmlPathView>(
        Sqlite::toIntegers(updatedPropertyEditorQmlPathsDirectoryPathIds));

    auto compareKey = [](const PropertyEditorQmlPathView &view, const PropertyEditorQmlPath &value) {
        return view.typeId <=> value.typeId;
    };

    auto insert = [&](const PropertyEditorQmlPath &path) {
        NanotraceHR::Tracer tracer{"insert property editor paths",
                                   category(),
                                   keyValue("property editor qml path", path)};

        if (path.typeId)
            s->insertPropertyEditorPathStatement.write(path.typeId, path.pathId, path.directoryId);
    };

    auto update = [&](const PropertyEditorQmlPathView &view, const PropertyEditorQmlPath &value) {
        NanotraceHR::Tracer tracer{"update property editor paths",
                                   category(),
                                   keyValue("property editor qml path", value),
                                   keyValue("property editor qml path view", view)};

        if (value.pathId != view.pathId || value.directoryId != view.directoryId) {
            s->updatePropertyEditorPathsStatement.write(value.typeId, value.pathId, value.directoryId);

            tracer.end(keyValue("updated", "yes"));

            return Sqlite::UpdateChange::Update;
        }
        return Sqlite::UpdateChange::No;
    };

    auto remove = [&](const PropertyEditorQmlPathView &view) {
        NanotraceHR::Tracer tracer{"remove property editor paths",
                                   category(),
                                   keyValue("property editor qml path view", view)};

        s->deletePropertyEditorPathStatement.write(view.typeId);
    };

    Sqlite::insertUpdateDelete(range, paths, compareKey, insert, update, remove);
}

void ProjectStorage::synchronizePropertyEditorQmlPaths(
    Storage::Synchronization::PropertyEditorQmlPaths &paths,
    DirectoryPathIds updatedPropertyEditorQmlPathsSourceIds)
{
    NanotraceHR::Tracer tracer{"synchronize property editor qml paths", category()};

    addTypeIdToPropertyEditorQmlPaths(paths);
    synchronizePropertyEditorPaths(paths, updatedPropertyEditorQmlPathsSourceIds);
}

void ProjectStorage::synchronizeFunctionDeclarations(
    TypeId typeId, Storage::Synchronization::FunctionDeclarations &functionsDeclarations)
{
    NanotraceHR::Tracer tracer{"synchronize function declaration", category()};

    std::ranges::sort(functionsDeclarations, [](auto &&first, auto &&second) {
        auto compare = Sqlite::compare(first.name, second.name);

        if (compare == 0) {
            Utils::PathString firstSignature{createJson(first.parameters)};
            Utils::PathString secondSignature{createJson(second.parameters)};

            return Sqlite::compare(firstSignature, secondSignature) < 0;
        }

        return compare < 0;
    });

    auto range = s->selectFunctionDeclarationsForTypeIdStatement
                     .range<Storage::Synchronization::FunctionDeclarationView>(typeId);

    auto compareKey = [](const Storage::Synchronization::FunctionDeclarationView &view,
                         const Storage::Synchronization::FunctionDeclaration &value) {
        auto nameKey = view.name <=> value.name;
        if (nameKey != std::strong_ordering::equal)
            return nameKey;

        Utils::PathString valueSignature{createJson(value.parameters)};

        return view.signature <=> valueSignature;
    };

    auto insert = [&](const Storage::Synchronization::FunctionDeclaration &value) {
        NanotraceHR::Tracer tracer{"insert function declaration",
                                   category(),
                                   keyValue("function declaration", value)};

        Utils::PathString signature{createJson(value.parameters)};

        s->insertFunctionDeclarationStatement.write(typeId, value.name, value.returnTypeName, signature);
    };

    auto update = [&](const Storage::Synchronization::FunctionDeclarationView &view,
                      const Storage::Synchronization::FunctionDeclaration &value) {
        NanotraceHR::Tracer tracer{"update function declaration",
                                   category(),
                                   keyValue("function declaration", value),
                                   keyValue("function declaration view", view)};

        Utils::PathString signature{createJson(value.parameters)};

        if (value.returnTypeName == view.returnTypeName && signature == view.signature)
            return Sqlite::UpdateChange::No;

        s->updateFunctionDeclarationStatement.write(view.id, value.returnTypeName, signature);

        tracer.end(keyValue("updated", "yes"));

        return Sqlite::UpdateChange::Update;
    };

    auto remove = [&](const Storage::Synchronization::FunctionDeclarationView &view) {
        NanotraceHR::Tracer tracer{"remove function declaration",
                                   category(),
                                   keyValue("function declaration view", view)};

        s->deleteFunctionDeclarationStatement.write(view.id);
    };

    Sqlite::insertUpdateDelete(range, functionsDeclarations, compareKey, insert, update, remove);
}

void ProjectStorage::synchronizeSignalDeclarations(
    TypeId typeId, Storage::Synchronization::SignalDeclarations &signalDeclarations)
{
    NanotraceHR::Tracer tracer{"synchronize signal declaration", category()};

    std::ranges::sort(signalDeclarations, [](auto &&first, auto &&second) {
        auto compare = Sqlite::compare(first.name, second.name);

        if (compare == 0) {
            Utils::PathString firstSignature{createJson(first.parameters)};
            Utils::PathString secondSignature{createJson(second.parameters)};

            return Sqlite::compare(firstSignature, secondSignature) < 0;
        }

        return compare < 0;
    });

    auto range = s->selectSignalDeclarationsForTypeIdStatement
                     .range<Storage::Synchronization::SignalDeclarationView>(typeId);

    auto compareKey = [](const Storage::Synchronization::SignalDeclarationView &view,
                         const Storage::Synchronization::SignalDeclaration &value) {
        auto nameKey = view.name <=> value.name;
        if (nameKey != std::strong_ordering::equal)
            return nameKey;

        Utils::PathString valueSignature{createJson(value.parameters)};

        return view.signature <=> valueSignature;
    };

    auto insert = [&](const Storage::Synchronization::SignalDeclaration &value) {
        NanotraceHR::Tracer tracer{"insert signal declaration",
                                   category(),
                                   keyValue("signal declaration", value)};

        Utils::PathString signature{createJson(value.parameters)};

        s->insertSignalDeclarationStatement.write(typeId, value.name, signature);
    };

    auto update = [&]([[maybe_unused]] const Storage::Synchronization::SignalDeclarationView &view,
                      [[maybe_unused]] const Storage::Synchronization::SignalDeclaration &value) {
        return Sqlite::UpdateChange::No;
    };

    auto remove = [&](const Storage::Synchronization::SignalDeclarationView &view) {
        NanotraceHR::Tracer tracer{"remove signal declaration",
                                   category(),
                                   keyValue("signal declaration view", view)};

        s->deleteSignalDeclarationStatement.write(view.id);
    };

    Sqlite::insertUpdateDelete(range, signalDeclarations, compareKey, insert, update, remove);
}

Utils::PathString ProjectStorage::createJson(
    const Storage::Synchronization::EnumeratorDeclarations &enumeratorDeclarations)
{
    NanotraceHR::Tracer tracer{"create json from enumerator declarations", category()};

    Utils::PathString json;
    json.append("{");

    Utils::SmallStringView comma{"\""};

    for (const auto &enumerator : enumeratorDeclarations) {
        json.append(comma);
        comma = ",\"";
        json.append(enumerator.name);
        if (enumerator.hasValue) {
            json.append("\":\"");
            json.append(Utils::SmallString::number(enumerator.value));
            json.append("\"");
        } else {
            json.append("\":null");
        }
    }

    json.append("}");

    return json;
}

void ProjectStorage::synchronizeEnumerationDeclarations(
    TypeId typeId, Storage::Synchronization::EnumerationDeclarations &enumerationDeclarations)
{
    NanotraceHR::Tracer tracer{"synchronize enumeration declaration", category()};

    std::ranges::sort(enumerationDeclarations, {}, &EnumerationDeclaration::name);

    auto range = s->selectEnumerationDeclarationsForTypeIdStatement
                     .range<Storage::Synchronization::EnumerationDeclarationView>(typeId);

    auto compareKey = [](const Storage::Synchronization::EnumerationDeclarationView &view,
                         const Storage::Synchronization::EnumerationDeclaration &value) {
        return view.name <=> value.name;
    };

    auto insert = [&](const Storage::Synchronization::EnumerationDeclaration &value) {
        NanotraceHR::Tracer tracer{"insert enumeration declaration",
                                   category(),
                                   keyValue("enumeration declaration", value)};

        Utils::PathString signature{createJson(value.enumeratorDeclarations)};

        s->insertEnumerationDeclarationStatement.write(typeId, value.name, signature);
    };

    auto update = [&](const Storage::Synchronization::EnumerationDeclarationView &view,
                      const Storage::Synchronization::EnumerationDeclaration &value) {
        NanotraceHR::Tracer tracer{"update enumeration declaration",
                                   category(),
                                   keyValue("enumeration declaration", value),
                                   keyValue("enumeration declaration view", view)};

        Utils::PathString enumeratorDeclarations{createJson(value.enumeratorDeclarations)};

        if (enumeratorDeclarations == view.enumeratorDeclarations)
            return Sqlite::UpdateChange::No;

        s->updateEnumerationDeclarationStatement.write(view.id, enumeratorDeclarations);

        tracer.end(keyValue("updated", "yes"));

        return Sqlite::UpdateChange::Update;
    };

    auto remove = [&](const Storage::Synchronization::EnumerationDeclarationView &view) {
        NanotraceHR::Tracer tracer{"remove enumeration declaration",
                                   category(),
                                   keyValue("enumeration declaration view", view)};

        s->deleteEnumerationDeclarationStatement.write(view.id);
    };

    Sqlite::insertUpdateDelete(range, enumerationDeclarations, compareKey, insert, update, remove);
}

TypeId ProjectStorage::declareType(std::string_view typeName, SourceId sourceId)
{
    NanotraceHR::Tracer tracer{"declare type",
                               category(),
                               keyValue("source id", sourceId),
                               keyValue("type name", typeName)};

    if (typeName.empty()) {
        auto typeId = s->selectTypeIdBySourceIdStatement.value<TypeId>(sourceId);

        tracer.end(keyValue("type id", typeId));

        return typeId;
    }

    auto typeId = s->selectTypeIdBySourceIdAndNameStatement.value<TypeId>(sourceId, typeName);

    if (!typeId) {
        NanotraceHR::Tracer _{"insert type", category()};
        typeId = s->insertTypeStatement.value<TypeId>(sourceId, typeName);
    }

    tracer.end(keyValue("type id", typeId));

    return typeId;
}

void ProjectStorage::syncDeclarations(Storage::Synchronization::Type &type,
                                      AliasPropertyDeclarations &aliasPropertyDeclarationsToLink,
                                      PropertyDeclarationIds &propertyDeclarationIds)
{
    NanotraceHR::Tracer tracer{"synchronize declaration per type", category()};

    synchronizePropertyDeclarations(type.typeId,
                                    type.propertyDeclarations,
                                    type.sourceId,
                                    aliasPropertyDeclarationsToLink,
                                    propertyDeclarationIds);
    synchronizeFunctionDeclarations(type.typeId, type.functionDeclarations);
    synchronizeSignalDeclarations(type.typeId, type.signalDeclarations);
    synchronizeEnumerationDeclarations(type.typeId, type.enumerationDeclarations);
}

void ProjectStorage::syncDeclarations(Storage::Synchronization::Types &types,
                                      AliasPropertyDeclarations &aliasPropertyDeclarationsToLink,
                                      PropertyDeclarations &relinkablePropertyDeclarations)
{
    NanotraceHR::Tracer tracer{"synchronize declaration", category()};

    PropertyDeclarationIds propertyDeclarationIds;
    propertyDeclarationIds.reserve(types.size() * 10);

    for (auto &&type : types)
        syncDeclarations(type, aliasPropertyDeclarationsToLink, propertyDeclarationIds);

    removeRelinkableEntries(relinkablePropertyDeclarations,
                            propertyDeclarationIds,
                            &PropertyDeclaration::propertyDeclarationId);
}

void ProjectStorage::syncDefaultProperties(Storage::Synchronization::Types &types)
{
    NanotraceHR::Tracer tracer{"synchronize default properties", category()};

    auto range = s->selectTypesWithDefaultPropertyStatement.range<TypeWithDefaultPropertyView>();

    auto compareKey = [](const TypeWithDefaultPropertyView &view,
                         const Storage::Synchronization::Type &value) {
        return view.typeId <=> value.typeId;
    };

    auto insert = [&](const Storage::Synchronization::Type &) {

    };

    auto update = [&](const TypeWithDefaultPropertyView &view,
                      const Storage::Synchronization::Type &value) {
        NanotraceHR::Tracer tracer{"synchronize default properties by update",
                                   category(),
                                   keyValue("type id", value.typeId),
                                   keyValue("value", value),
                                   keyValue("view", view)};

        PropertyDeclarationId valueDefaultPropertyId;
        if (value.defaultPropertyName.size()) {
            auto defaultPropertyDeclaration = fetchPropertyDeclarationByTypeIdAndNameUngarded(
                value.typeId, value.defaultPropertyName);

            if (defaultPropertyDeclaration) {
                valueDefaultPropertyId = defaultPropertyDeclaration->propertyDeclarationId;
            } else {
                errorNotifier->missingDefaultProperty(value.typeName,
                                                      value.defaultPropertyName,
                                                      value.sourceId);
            }
        }

        if (compareInvalidAreTrue(valueDefaultPropertyId, view.defaultPropertyId))
            return Sqlite::UpdateChange::No;

        s->updateDefaultPropertyIdStatement.write(value.typeId, valueDefaultPropertyId);

        tracer.end(keyValue("updated", "yes"),
                   keyValue("default property id", valueDefaultPropertyId));

        return Sqlite::UpdateChange::Update;
    };

    auto remove = [&](const TypeWithDefaultPropertyView &) {};

    Sqlite::insertUpdateDelete(range, types, compareKey, insert, update, remove);
}

void ProjectStorage::resetDefaultPropertiesIfChanged(Storage::Synchronization::Types &types)
{
    NanotraceHR::Tracer tracer{"reset changed default properties", category()};

    auto range = s->selectTypesWithDefaultPropertyStatement.range<TypeWithDefaultPropertyView>();

    auto compareKey = [](const TypeWithDefaultPropertyView &view,
                         const Storage::Synchronization::Type &value) {
        return view.typeId <=> value.typeId;
    };

    auto insert = [&](const Storage::Synchronization::Type &) {

    };

    auto update = [&](const TypeWithDefaultPropertyView &view,
                      const Storage::Synchronization::Type &value) {
        NanotraceHR::Tracer tracer{"reset changed default properties by update",
                                   category(),
                                   keyValue("type id", value.typeId),
                                   keyValue("value", value),
                                   keyValue("view", view)};

        PropertyDeclarationId valueDefaultPropertyId;
        if (value.defaultPropertyName.size()) {
            valueDefaultPropertyId = fetchPropertyDeclarationIdByTypeIdAndNameUngarded(
                value.typeId, value.defaultPropertyName);
        }

        if (compareInvalidAreTrue(valueDefaultPropertyId, view.defaultPropertyId))
            return Sqlite::UpdateChange::No;

        s->updateDefaultPropertyIdStatement.write(value.typeId, Sqlite::NullValue{});

        tracer.end(keyValue("updated", "yes"));

        return Sqlite::UpdateChange::Update;
    };

    auto remove = [&](const TypeWithDefaultPropertyView &) {};

    Sqlite::insertUpdateDelete(range, types, compareKey, insert, update, remove);
}

void ProjectStorage::checkForPrototypeChainCycle(TypeId typeId) const
{
    NanotraceHR::Tracer tracer{"check for prototype chain cycle",
                               category(),
                               keyValue("type id", typeId)};

    auto callback = [&](TypeId currentTypeId) {
        if (typeId == currentTypeId) {
            auto [name, sourceId] = s->selectTypeNameAndSourceIdByTypeIdStatement
                                        .value<std::tuple<Utils::SmallString, SourceId>>(typeId);
            errorNotifier->prototypeCycle(name, sourceId);
            throw PrototypeChainCycle{};
        }
    };

    s->selectPrototypeAndExtensionIdsStatement.readCallback(callback, typeId);
}

void ProjectStorage::checkForAliasChainCycle(PropertyDeclarationId propertyDeclarationId) const
{
    NanotraceHR::Tracer tracer{"check for alias chain cycle",
                               category(),
                               keyValue("property declaration id", propertyDeclarationId)};
    auto callback = [&](PropertyDeclarationId currentPropertyDeclarationId) {
        if (propertyDeclarationId == currentPropertyDeclarationId) {
            auto [propertyName, typeId] = s->selectPropertyDeclarationNameAndTypeIdForPropertyDeclarationIdStatement
                                              .value<std::tuple<Utils::SmallString, TypeId>>(
                                                  propertyDeclarationId);
            auto [typeName, sourceId] = s->selectTypeNameAndSourceIdByTypeIdStatement
                                            .value<std::tuple<Utils::SmallString, SourceId>>(typeId);
            errorNotifier->aliasCycle(typeName, propertyName, sourceId);

            throw AliasChainCycle{};
        }
    };

    s->selectPropertyDeclarationIdsForAliasChainStatement.readCallback(callback,
                                                                       propertyDeclarationId);
}

std::pair<TypeId, ImportedTypeNameId> ProjectStorage::fetchImportedTypeNameIdAndTypeId(
    const Storage::Synchronization::ImportedTypeName &importedTypeName, SourceId sourceId)
{
    NanotraceHR::Tracer tracer{"fetch imported type name id and type id",
                               category(),
                               keyValue("imported type name", importedTypeName),
                               keyValue("source id", sourceId)};

    TypeId typeId;
    ImportedTypeNameId typeNameId;
    Storage::Synchronization::TypeNameKind typeNameKind;

    auto typeName = std::visit([](auto &&importedTypeName) { return importedTypeName.name; },
                               importedTypeName);
    if (!typeName.empty()) {
        std::tie(typeNameId, typeNameKind) = fetchImportedTypeNameId(importedTypeName, sourceId);

        typeId = fetchTypeId(typeNameId, typeNameKind);

        tracer.end(keyValue("type id", typeId), keyValue("type name id", typeNameId));

        if (!typeId) {
            errorNotifier->typeNameCannotBeResolved(typeName, sourceId);
            return {unresolvedTypeId, typeNameId};
        }
    }

    return {typeId, typeNameId};
}

bool ProjectStorage::updateBases(TypeId typeId, TypeId prototypeId, TypeId extensionId)
{
    NanotraceHR::Tracer tracer{"update bases",
                               category(),
                               keyValue("type id", typeId),
                               keyValue("prototype id", prototypeId),
                               keyValue("extension id", extensionId)};

    QVarLengthArray<TypeId, 2> baseIds;

    if (not prototypeId.isNull())
        baseIds.push_back(prototypeId);

    if (not extensionId.isNull() and prototypeId.internalId() != extensionId.internalId())
        baseIds.push_back(extensionId);

    std::ranges::sort(baseIds);

    auto range = s->selectBaseIdsStatement.range<TypeId>(typeId);

    auto compareKey = [](TypeId first, TypeId second) { return first <=> second; };

    bool changedBaseId = false;

    auto insert = [&](TypeId baseId) {
        NanotraceHR::Tracer tracer{"insert base id", category(), keyValue("base id", baseId)};

        s->insertBasesStatement.write(typeId, baseId);

        changedBaseId = true;
    };

    auto update = [&](TypeId viewBaseId, TypeId baseId) {
        NanotraceHR::Tracer tracer{"update base id",
                                   category(),
                                   keyValue("view base id", viewBaseId),
                                   keyValue("base id", baseId)};

        if (viewBaseId != baseId) {
            s->updateBasesStatement.write(typeId, baseId, viewBaseId);

            tracer.end(keyValue("updated", "yes"));

            changedBaseId = true;

            return Sqlite::UpdateChange::Update;
        }
        return Sqlite::UpdateChange::No;
    };

    auto remove = [&](TypeId viewBaseId) {
        NanotraceHR::Tracer tracer{"remove base id", category(), keyValue("view base id", viewBaseId)};

        s->deleteBasesStatement.write(typeId, viewBaseId);

        changedBaseId = true;
    };

    Sqlite::insertUpdateDelete(range, baseIds, compareKey, insert, update, remove);

    return changedBaseId;
}

bool ProjectStorage::updatePrototypes(TypeId typeId, TypeId prototypeId)
{
    NanotraceHR::Tracer tracer{"update prototypes",
                               category(),
                               keyValue("type id", typeId),
                               keyValue("prototype id", prototypeId)};

    QVarLengthArray<TypeId, 2> baseIds;

    if (not prototypeId.isNull())
        s->upsertPrototypesStatement.write(typeId, prototypeId);
    else
        s->deletePrototypesStatement.write(typeId);

    return database.changesCount();
}

void ProjectStorage::syncPrototypeAndExtension(Storage::Synchronization::Type &type,
                                               TypeIds &typeIds,
                                               SmallTypeIds<256> &updatedPrototypeIds)
{
    NanotraceHR::Tracer tracer{"synchronize prototype and extension",
                               category(),
                               keyValue("prototype", type.prototype),
                               keyValue("extension", type.extension),
                               keyValue("type id", type.typeId),
                               keyValue("source id", type.sourceId)};

    auto [prototypeId, prototypeTypeNameId] = fetchImportedTypeNameIdAndTypeId(type.prototype,
                                                                               type.sourceId);
    auto [extensionId, extensionTypeNameId] = fetchImportedTypeNameIdAndTypeId(type.extension,
                                                                               type.sourceId);

    s->updatePrototypeAndExtensionNameStatement.write(type.typeId,
                                                      prototypeTypeNameId,
                                                      extensionTypeNameId);

    auto changedBaseIds = updateBases(type.typeId, prototypeId, extensionId);
    auto changedPrototypes = updatePrototypes(type.typeId, prototypeId);

    if (changedBaseIds)
        checkForPrototypeChainCycle(type.typeId);

    if (changedPrototypes) {
        tracer.tick("updated prototype id", keyValue("type id", type.typeId));
        updatedPrototypeIds.push_back(type.typeId);
    }

    typeIds.push_back(type.typeId);

    tracer.end(keyValue("prototype id", prototypeId),
               keyValue("prototype type name id", prototypeTypeNameId),
               keyValue("extension id", extensionId),
               keyValue("extension type name id", extensionTypeNameId));
}

void ProjectStorage::syncPrototypesAndExtensions(Storage::Synchronization::Types &types,
                                                 Bases &relinkableBases,
                                                 SmallTypeIds<256> &updatedPrototypeIds)
{
    NanotraceHR::Tracer tracer{"synchronize prototypes and extensions", category()};

    TypeIds typeIds;
    typeIds.reserve(types.size());

    for (auto &type : types)
        syncPrototypeAndExtension(type, typeIds, updatedPrototypeIds);

    removeRelinkableEntries(relinkableBases, typeIds, &Base::typeId);
}

ImportId ProjectStorage::fetchImportId(SourceId sourceId, const Storage::Import &import) const
{
    NanotraceHR::Tracer tracer{"fetch imported type name id",
                               category(),
                               keyValue("import", import),
                               keyValue("source id", sourceId)};

    ImportId importId = s->selectImportIdBySourceIdAndModuleIdAndVersionAndAliasStatement
                            .value<ImportId>(sourceId,
                                             import.moduleId,
                                             import.version.major.value,
                                             import.version.minor.value,
                                             import.alias);

    tracer.end(keyValue("import id", importId));

    return importId;
}

ImportId ProjectStorage::fetchImportId(SourceId sourceId, Utils::SmallStringView alias) const
{
    NanotraceHR::Tracer tracer{"fetch imported type name id",
                               category(),
                               keyValue("alias", alias),
                               keyValue("source id", sourceId)};

    ImportId importId = s->selectImportIdBySourceIdAndAliasStatement.value<ImportId>(sourceId, alias);

    tracer.end(keyValue("import id", importId));

    return importId;
}

std::tuple<ImportedTypeNameId, Storage::Synchronization::TypeNameKind> ProjectStorage::fetchImportedTypeNameId(
    const Storage::Synchronization::ImportedTypeName &name, SourceId sourceId)
{
    using Storage::Synchronization::TypeNameKind;

    struct Inspect
    {
        auto operator()(const Storage::Synchronization::ImportedType &importedType)
        {
            NanotraceHR::Tracer tracer{"fetch imported type name id",
                                       category(),
                                       keyValue("imported type name", importedType.name),
                                       keyValue("source id", sourceId),
                                       keyValue("type name kind", "exported"sv)};

            return std::tuple(storage.fetchImportedTypeNameId(TypeNameKind::Exported,
                                                              sourceId,
                                                              importedType.name),
                              TypeNameKind::Exported);
        }

        auto operator()(const Storage::Synchronization::QualifiedImportedType &importedType)
        {
            NanotraceHR::Tracer tracer{"fetch imported type name id",
                                       category(),
                                       keyValue("imported type name", importedType.name),
                                       keyValue("alias", importedType.alias),
                                       keyValue("type name kind", "qualified exported"sv)};

            ImportId importId = storage.fetchImportId(sourceId, importedType.alias);

            auto importedTypeNameId = storage.fetchImportedTypeNameId(TypeNameKind::QualifiedExported,
                                                                      importId,
                                                                      importedType.name);

            tracer.end(keyValue("import id", importId), keyValue("source id", sourceId));

            return std::tuple(importedTypeNameId, TypeNameKind::QualifiedExported);
        }

        ProjectStorage &storage;
        SourceId sourceId;
    };

    return std::visit(Inspect{*this, sourceId}, name);
}

TypeId ProjectStorage::fetchTypeId(ImportedTypeNameId typeNameId) const
{
    NanotraceHR::Tracer tracer{"fetch type id", category(), keyValue("type name id", typeNameId)};

    auto kind = s->selectKindFromImportedTypeNamesStatement.value<Storage::Synchronization::TypeNameKind>(
        typeNameId);

    auto typeId = fetchTypeId(typeNameId, kind);

    tracer.end(keyValue("type id", typeId), keyValue("type name kind", kind));

    return typeId;
}

Storage::Info::ExportedTypeName ProjectStorage::fetchExportedTypeName(
    ImportedTypeNameId typeNameId, Storage::Synchronization::TypeNameKind kind) const
{
    NanotraceHR::Tracer tracer{"fetch exported type name with type name kind",
                               category(),
                               keyValue("type name id", typeNameId),
                               keyValue("type name kind", kind)};

    if (kind == Storage::Synchronization::TypeNameKind::Exported) {
        return s->selectExportedTypeNameForImportedTypeNameStatement
            .value<Storage::Info::ExportedTypeName>(typeNameId);
    } else {
        return s->selectExportedTypeNameForQualifiedImportedTypeNameStatement
            .value<Storage::Info::ExportedTypeName>(typeNameId);
    }
}

Storage::Info::ExportedTypeName ProjectStorage::fetchExportedTypeName(ImportedTypeNameId typeNameId) const
{
    NanotraceHR::Tracer tracer{"fetch exported type name",
                               category(),
                               keyValue("type name id", typeNameId)};

    auto kind = s->selectKindFromImportedTypeNamesStatement.value<Storage::Synchronization::TypeNameKind>(
        typeNameId);

    return fetchExportedTypeName(typeNameId, kind);
}

Utils::SmallString ProjectStorage::fetchImportedTypeName(ImportedTypeNameId typeNameId) const
{
    return s->selectNameFromImportedTypeNamesStatement.value<Utils::SmallString>(typeNameId);
}

SourceId ProjectStorage::fetchTypeSourceId(TypeId typeId) const
{
    return s->selectSourceIdByTypeIdStatement.value<SourceId>(typeId);
}

TypeId ProjectStorage::fetchTypeId(ImportedTypeNameId typeNameId,
                                   Storage::Synchronization::TypeNameKind kind) const
{
    NanotraceHR::Tracer tracer{"fetch type id with type name kind",
                               category(),
                               keyValue("type name id", typeNameId),
                               keyValue("type name kind", kind)};

    TypeId typeId;
    if (kind == Storage::Synchronization::TypeNameKind::Exported) {
        typeId = s->selectTypeIdForImportedTypeNameStatement.value<UnresolvedTypeId>(typeNameId);
    } else {
        typeId = s->selectTypeIdForQualifiedImportedTypeNameStatement.value<UnresolvedTypeId>(
            typeNameId);
    }

    tracer.end(keyValue("type id", typeId));

    return typeId;
}

std::optional<ProjectStorage::FetchPropertyDeclarationResult>
ProjectStorage::fetchPropertyDeclarationByTypeIdAndNameUngarded(TypeId typeId,
                                                                Utils::SmallStringView name)
{
    NanotraceHR::Tracer tracer{"fetch optional property declaration by type id and name ungarded",
                               category(),
                               keyValue("type id", typeId),
                               keyValue("property name", name)};

    auto propertyDeclarationId = fetchPropertyDeclarationId(typeId, name);
    auto propertyDeclaration = s->selectPropertyDeclarationResultByPropertyDeclarationIdStatement
                                   .optionalValue<FetchPropertyDeclarationResult>(
                                       propertyDeclarationId);

    tracer.end(keyValue("property declaration", propertyDeclaration));

    return propertyDeclaration;
}

PropertyDeclarationId ProjectStorage::fetchPropertyDeclarationIdByTypeIdAndNameUngarded(
    TypeId typeId, Utils::SmallStringView name)
{
    NanotraceHR::Tracer tracer{"fetch property declaration id by type id and name ungarded",
                               category(),
                               keyValue("type id", typeId),
                               keyValue("property name", name)};

    auto propertyDeclarationId = fetchPropertyDeclarationId(typeId, name);

    tracer.end(keyValue("property declaration id", propertyDeclarationId));

    return propertyDeclarationId;
}

TypeId ProjectStorage::fetchPrototypeId(TypeId typeId)
{
    NanotraceHR::Tracer tracer{"fetch prototype id", category()};

    return s->selectPrototypeIdByTypeIdStatement.value<TypeId>(Sqlite::source_location::current(),
                                                               typeId);
}

TypeId ProjectStorage::fetchExtensionId(TypeId typeId)
{
    NanotraceHR::Tracer tracer{"fetch extension id", category()};

    return s->selectExtensionIdByTypeIdStatement.value<TypeId>(Sqlite::source_location::current(),
                                                               typeId);
}

Storage::Synchronization::PropertyDeclarations ProjectStorage::fetchPropertyDeclarations(TypeId typeId) const
{
    NanotraceHR::Tracer tracer{"fetch property declarations", category(), keyValue("type id", typeId)};

    auto propertyDeclarations = s->selectPropertyDeclarationsByTypeIdStatement
                                    .values<Storage::Synchronization::PropertyDeclaration, 24>(typeId);

    tracer.end(keyValue("property declarations", propertyDeclarations));

    return propertyDeclarations;
}

Storage::Synchronization::FunctionDeclarations ProjectStorage::fetchFunctionDeclarations(TypeId typeId) const
{
    NanotraceHR::Tracer tracer{"fetch signal declarations", category(), keyValue("type id", typeId)};

    Storage::Synchronization::FunctionDeclarations functionDeclarations;

    auto callback = [&](Utils::SmallStringView name,
                        Utils::SmallStringView returnType,
                        FunctionDeclarationId functionDeclarationId) {
        auto &functionDeclaration = functionDeclarations.emplace_back(name, returnType);
        functionDeclaration.parameters = s->selectFunctionParameterDeclarationsStatement
                                             .values<Storage::Synchronization::ParameterDeclaration, 8>(
                                                 functionDeclarationId);
    };

    s->selectFunctionDeclarationsForTypeIdWithoutSignatureStatement.readCallback(callback, typeId);

    tracer.end(keyValue("function declarations", functionDeclarations));

    return functionDeclarations;
}

Storage::Synchronization::SignalDeclarations ProjectStorage::fetchSignalDeclarations(TypeId typeId) const
{
    NanotraceHR::Tracer tracer{"fetch signal declarations", category(), keyValue("type id", typeId)};

    Storage::Synchronization::SignalDeclarations signalDeclarations;

    auto callback = [&](Utils::SmallStringView name, SignalDeclarationId signalDeclarationId) {
        auto &signalDeclaration = signalDeclarations.emplace_back(name);
        signalDeclaration.parameters = s->selectSignalParameterDeclarationsStatement
                                           .values<Storage::Synchronization::ParameterDeclaration, 8>(
                                               signalDeclarationId);
    };

    s->selectSignalDeclarationsForTypeIdWithoutSignatureStatement.readCallback(callback, typeId);

    tracer.end(keyValue("signal declarations", signalDeclarations));

    return signalDeclarations;
}

Storage::Synchronization::EnumerationDeclarations ProjectStorage::fetchEnumerationDeclarations(
    TypeId typeId) const
{
    NanotraceHR::Tracer tracer{"fetch enumeration declarations",
                               category(),
                               keyValue("type id", typeId)};

    Storage::Synchronization::EnumerationDeclarations enumerationDeclarations;

    auto callback = [&](Utils::SmallStringView name,
                        EnumerationDeclarationId enumerationDeclarationId) {
        enumerationDeclarations.emplace_back(
            name,
            s->selectEnumeratorDeclarationStatement
                .values<Storage::Synchronization::EnumeratorDeclaration, 8>(enumerationDeclarationId));
    };

    s->selectEnumerationDeclarationsForTypeIdWithoutEnumeratorDeclarationsStatement
        .readCallback(callback, typeId);

    tracer.end(keyValue("enumeration declarations", enumerationDeclarations));

    return enumerationDeclarations;
}

void ProjectStorage::resetBasesCache()
{
    NanotraceHR::Tracer tracer{"reset bases cache", category()};

    basesCache.clear();
    auto maxSize = s->selectMaxTypeIdStatement.value<long long>();
    basesCache.resize(static_cast<std::size_t>(maxSize));
}

namespace {
template<typename... TypeIds>
TypeId findTypeId(auto &&range, TypeIds... baseTypeIds)
{
    for (TypeId currentTypeId : range) {
        if (((currentTypeId == baseTypeIds) || ...))
            return currentTypeId;
    }

    return TypeId{};
}

} // namespace

template<typename... TypeIds>
TypeId ProjectStorage::basedOn_(TypeId typeId, TypeIds... baseTypeIds) const
{
    NanotraceHR::Tracer tracer{"is based on",
                               category(),
                               keyValue("type id", typeId),
                               keyValue("base type ids", NanotraceHR::array(baseTypeIds...))};

    static_assert(((std::is_same_v<TypeId, TypeIds>) && ...), "Parameter must be a TypeId!");

    if (((typeId == baseTypeIds) || ...)) {
        tracer.end(keyValue("is based on", true));
        return typeId;
    }

    auto &cache = basesCache[static_cast<std::size_t>(typeId.internalId() - 1)];
    if (!cache) {
        cache.emplace(
            s->selectPrototypeAndExtensionIdsStatement.valuesWithTransaction<SmallTypeIds<12>>(typeId));
    }

    auto foundTypeId = findTypeId(*cache, baseTypeIds...);

    tracer.end(keyValue("is based on", foundTypeId));

    return foundTypeId;
}

bool ProjectStorage::inheritsAll(Utils::span<const TypeId> typeIds, TypeId baseTypeId) const
{
    NanotraceHR::Tracer tracer{"is based on type ids", category()};

    return Sqlite::withImplicitTransaction(database, [&] {
        auto isHeir = [&](TypeId typeId) -> bool {
            if (not typeId)
                return false;

            if (typeId == baseTypeId)
                return true;

            auto &cache = basesCache[static_cast<std::size_t>(typeId.internalId() - 1)];
            if (not cache) {
                cache.emplace(
                    s->selectPrototypeAndExtensionIdsStatement.values<SmallTypeIds<12>>(typeId));
            }

            return bool(findTypeId(*cache, baseTypeId));
        };

        return std::ranges::all_of(typeIds, isHeir);
    });
}

template<typename Id>
ImportedTypeNameId ProjectStorage::fetchImportedTypeNameId(Storage::Synchronization::TypeNameKind kind,
                                                           Id id,
                                                           Utils::SmallStringView typeName)
{
    NanotraceHR::Tracer tracer{"fetch imported type name id",
                               category(),
                               keyValue("imported type name", typeName),
                               keyValue("kind", kind)};

    auto importedTypeNameId = s->selectImportedTypeNameIdStatement.value<ImportedTypeNameId>(kind,
                                                                                             id,
                                                                                             typeName);

    if (!importedTypeNameId)
        importedTypeNameId = s->insertImportedTypeNameIdStatement.value<ImportedTypeNameId>(kind,
                                                                                            id,
                                                                                            typeName);

    tracer.end(keyValue("imported type name id", importedTypeNameId));

    return importedTypeNameId;
}

template<typename Relinkable, typename Ids, typename Projection>
void ProjectStorage::removeRelinkableEntries(std::vector<Relinkable> &relinkables,
                                             Ids ids,
                                             Projection projection)
{
    NanotraceHR::Tracer tracer{"remove relinkable entries", category()};

    std::vector<Relinkable> newRelinkables;
    newRelinkables.reserve(relinkables.size());

    std::ranges::sort(ids);
    std::ranges::sort(relinkables, {}, projection);

    Utils::set_greedy_difference(
        relinkables,
        ids,
        [&](Relinkable &entry) {
            tracer.tick("relinkable id", keyValue("type id", entry.typeId));

            newRelinkables.push_back(std::move(entry));
        },
        {},
        projection);

    relinkables = std::move(newRelinkables);
}

} // namespace QmlDesigner
