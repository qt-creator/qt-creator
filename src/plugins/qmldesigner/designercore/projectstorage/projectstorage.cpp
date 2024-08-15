// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectstorage.h"

#include <sqlitedatabase.h>

namespace QmlDesigner {

enum class SpecialIdState { Unresolved = -1 };

constexpr TypeId unresolvedTypeId = TypeId::createSpecialState(SpecialIdState::Unresolved);

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

struct ProjectStorage::Statements
{
    Statements(Sqlite::Database &database)
        : database{database}
    {}

    Sqlite::Database &database;
    Sqlite::ReadWriteStatement<1, 2> insertTypeStatement{
        "INSERT OR IGNORE INTO types(sourceId, name) VALUES(?1, ?2) RETURNING typeId", database};
    Sqlite::WriteStatement<5> updatePrototypeAndExtensionStatement{
        "UPDATE types "
        "SET prototypeId=?2, prototypeNameId=?3, extensionId=?4, extensionNameId=?5 "
        "WHERE typeId=?1 AND ( "
        "  prototypeId IS NOT ?2 "
        "  OR extensionId IS NOT ?3 "
        "  OR prototypeId IS NOT ?4 "
        "  OR extensionNameId IS NOT ?5)",
        database};
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
    mutable Sqlite::ReadStatement<8, 1> selectTypeByTypeIdStatement{
        "SELECT sourceId, t.name, t.typeId, prototypeId, extensionId, traits, annotationTraits, "
        "pd.name "
        "FROM types AS t LEFT JOIN propertyDeclarations AS pd ON "
        "defaultPropertyId=propertyDeclarationId "
        "WHERE t.typeId=?",
        database};
    mutable Sqlite::ReadStatement<4, 1> selectExportedTypesByTypeIdStatement{
        "SELECT moduleId, name, ifnull(majorVersion, -1), ifnull(minorVersion, -1) FROM "
        "exportedTypeNames WHERE typeId=?",
        database};
    mutable Sqlite::ReadStatement<4, 2> selectExportedTypesByTypeIdAndSourceIdStatement{
        "SELECT etn.moduleId, name, ifnull(etn.majorVersion, -1), ifnull(etn.minorVersion, -1) "
        "FROM exportedTypeNames AS etn JOIN documentImports USING(moduleId) WHERE typeId=?1 AND "
        "sourceId=?2",
        database};
    mutable Sqlite::ReadStatement<8> selectTypesStatement{
        "SELECT sourceId, t.name, t.typeId, prototypeId, extensionId, traits, annotationTraits, "
        "pd.name "
        "FROM types AS t LEFT JOIN propertyDeclarations AS pd ON "
        "defaultPropertyId=propertyDeclarationId",
        database};
    Sqlite::WriteStatement<2> updateTypeTraitStatement{
        "UPDATE types SET traits = ?2 WHERE typeId=?1", database};
    Sqlite::WriteStatement<2> updateTypeAnnotationTraitStatement{
        "WITH RECURSIVE "
        "  typeSelection(typeId) AS ("
        "      VALUES(?1) "
        "    UNION ALL "
        "      SELECT t.typeId "
        "      FROM types AS t JOIN typeSelection AS ts "
        "      WHERE prototypeId=ts.typeId "
        "        AND t.typeId NOT IN (SELECT typeId FROM typeAnnotations)) "
        "UPDATE types AS t "
        "SET annotationTraits = ?2 "
        "FROM typeSelection ts "
        "WHERE t.typeId=ts.typeId",
        database};
    Sqlite::ReadStatement<1, 2> selectNotUpdatedTypesInSourcesStatement{
        "SELECT DISTINCT typeId FROM types WHERE (sourceId IN carray(?1) AND typeId NOT IN "
        "carray(?2))",
        database};
    Sqlite::WriteStatement<1> deleteTypeNamesByTypeIdStatement{
        "DELETE FROM exportedTypeNames WHERE typeId=?", database};
    Sqlite::WriteStatement<1> deleteEnumerationDeclarationByTypeIdStatement{
        "DELETE FROM enumerationDeclarations WHERE typeId=?", database};
    Sqlite::WriteStatement<1> deletePropertyDeclarationByTypeIdStatement{
        "DELETE FROM propertyDeclarations WHERE typeId=?", database};
    Sqlite::WriteStatement<1> deleteFunctionDeclarationByTypeIdStatement{
        "DELETE FROM functionDeclarations WHERE typeId=?", database};
    Sqlite::WriteStatement<1> deleteSignalDeclarationByTypeIdStatement{
        "DELETE FROM signalDeclarations WHERE typeId=?", database};
    Sqlite::WriteStatement<1> deleteTypeStatement{"DELETE FROM types  WHERE typeId=?", database};
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
        "INSERT INTO functionDeclarations(typeId, name, returnTypeName, signature) VALUES(?1, ?2, "
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
        "SELECT name, signature, signalDeclarationId FROM signalDeclarations WHERE typeId=? ORDER "
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
        "SELECT name, enumerationDeclarationId FROM enumerationDeclarations WHERE typeId=? ORDER "
        "BY name",
        database};
    mutable Sqlite::ReadStatement<3, 1> selectEnumeratorDeclarationStatement{
        "SELECT json_each.key, json_each.value, json_each.type!='null' FROM "
        "enumerationDeclarations, json_each(enumerationDeclarations.enumeratorDeclarations) WHERE "
        "enumerationDeclarationId=?",
        database};
    Sqlite::WriteStatement<3> insertEnumerationDeclarationStatement{
        "INSERT INTO enumerationDeclarations(typeId, name, enumeratorDeclarations) VALUES(?1, ?2, "
        "?3)",
        database};
    Sqlite::WriteStatement<2> updateEnumerationDeclarationStatement{
        "UPDATE enumerationDeclarations SET  enumeratorDeclarations=?2 WHERE "
        "enumerationDeclarationId=?1",
        database};
    Sqlite::WriteStatement<1> deleteEnumerationDeclarationStatement{
        "DELETE FROM enumerationDeclarations WHERE enumerationDeclarationId=?", database};
    mutable Sqlite::ReadStatement<1, 2> selectModuleIdByNameStatement{
        "SELECT moduleId FROM modules WHERE kind=?1 AND name=?2 LIMIT 1", database};
    mutable Sqlite::ReadWriteStatement<1, 2> insertModuleNameStatement{
        "INSERT INTO modules(kind, name) VALUES(?1, ?2) RETURNING moduleId", database};
    mutable Sqlite::ReadStatement<2, 1> selectModuleStatement{
        "SELECT name, kind FROM modules WHERE moduleId =?1", database};
    mutable Sqlite::ReadStatement<3> selectAllModulesStatement{
        "SELECT name, kind, moduleId FROM modules", database};
    mutable Sqlite::ReadStatement<1, 2> selectTypeIdBySourceIdAndNameStatement{
        "SELECT typeId FROM types WHERE sourceId=?1 and name=?2", database};
    mutable Sqlite::ReadStatement<1, 3> selectTypeIdByModuleIdsAndExportedNameStatement{
        "SELECT typeId FROM exportedTypeNames WHERE moduleId IN carray(?1, ?2, 'int32') AND "
        "name=?3",
        database};
    mutable Sqlite::ReadStatement<4> selectAllDocumentImportForSourceIdStatement{
        "SELECT moduleId, majorVersion, minorVersion, sourceId "
        "FROM documentImports ",
        database};
    mutable Sqlite::ReadStatement<5, 2> selectDocumentImportForSourceIdStatement{
        "SELECT importId, sourceId, moduleId, majorVersion, minorVersion "
        "FROM documentImports WHERE sourceId IN carray(?1) AND kind=?2 ORDER BY sourceId, "
        "moduleId, majorVersion, minorVersion",
        database};
    Sqlite::ReadWriteStatement<1, 5> insertDocumentImportWithoutVersionStatement{
        "INSERT INTO documentImports(sourceId, moduleId, sourceModuleId, kind, "
        "parentImportId) VALUES (?1, ?2, ?3, ?4, ?5) RETURNING importId",
        database};
    Sqlite::ReadWriteStatement<1, 6> insertDocumentImportWithMajorVersionStatement{
        "INSERT INTO documentImports(sourceId, moduleId, sourceModuleId, kind, majorVersion, "
        "parentImportId) VALUES (?1, ?2, ?3, ?4, ?5, ?6) RETURNING importId",
        database};
    Sqlite::ReadWriteStatement<1, 7> insertDocumentImportWithVersionStatement{
        "INSERT INTO documentImports(sourceId, moduleId, sourceModuleId, kind, majorVersion, "
        "minorVersion, parentImportId) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7) RETURNING "
        "importId",
        database};
    Sqlite::WriteStatement<1> deleteDocumentImportStatement{
        "DELETE FROM documentImports WHERE importId=?1", database};
    Sqlite::WriteStatement<2> deleteDocumentImportsWithParentImportIdStatement{
        "DELETE FROM documentImports WHERE sourceId=?1 AND parentImportId=?2", database};
    Sqlite::WriteStatement<1> deleteDocumentImportsWithSourceIdsStatement{
        "DELETE FROM documentImports WHERE sourceId IN carray(?1)", database};
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
        "propertyTypeId, propertyTraits FROM propertyDeclarations WHERE propertyDeclarationId=?1) "
        "AS new WHERE aliasPropertyDeclarationId=?2",
        database};
    Sqlite::WriteStatement<1> updateAliasPropertyDeclarationToNullStatement{
        "UPDATE propertyDeclarations SET aliasPropertyDeclarationId=NULL, propertyTypeId=NULL, "
        "propertyTraits=NULL WHERE propertyDeclarationId=? AND (aliasPropertyDeclarationId IS NOT "
        "NULL OR propertyTypeId IS NOT NULL OR propertyTraits IS NOT NULL)",
        database};
    Sqlite::ReadStatement<5, 1> selectAliasPropertiesDeclarationForPropertiesWithTypeIdStatement{
        "SELECT alias.typeId, alias.propertyDeclarationId, alias.aliasPropertyImportedTypeNameId, "
        "  alias.aliasPropertyDeclarationId, alias.aliasPropertyDeclarationTailId "
        "FROM propertyDeclarations AS alias JOIN propertyDeclarations AS target "
        "  ON alias.aliasPropertyDeclarationId=target.propertyDeclarationId OR "
        "    alias.aliasPropertyDeclarationTailId=target.propertyDeclarationId "
        "WHERE alias.propertyTypeId=?1 "
        "UNION ALL "
        "SELECT alias.typeId, alias.propertyDeclarationId, alias.aliasPropertyImportedTypeNameId, "
        "  alias.aliasPropertyDeclarationId, alias.aliasPropertyDeclarationTailId "
        "FROM propertyDeclarations AS alias JOIN propertyDeclarations AS target "
        "  ON alias.aliasPropertyDeclarationId=target.propertyDeclarationId OR "
        "    alias.aliasPropertyDeclarationTailId=target.propertyDeclarationId "
        "WHERE target.typeId=?1 "
        "UNION ALL "
        "SELECT alias.typeId, alias.propertyDeclarationId, alias.aliasPropertyImportedTypeNameId, "
        "  alias.aliasPropertyDeclarationId, alias.aliasPropertyDeclarationTailId "
        "FROM propertyDeclarations AS alias JOIN propertyDeclarations AS target "
        "  ON alias.aliasPropertyDeclarationId=target.propertyDeclarationId OR "
        "    alias.aliasPropertyDeclarationTailId=target.propertyDeclarationId "
        "WHERE  alias.aliasPropertyImportedTypeNameId IN "
        "  (SELECT importedTypeNameId FROM exportedTypeNames JOIN importedTypeNames USING(name) "
        "   WHERE typeId=?1)",
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
        "        pd.aliasPropertyDeclarationId FROM propertyDeclarations AS pd JOIN properties AS "
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
    Sqlite::ReadWriteStatement<2, 2> updatePrototypeIdToTypeIdStatement{
        "UPDATE types "
        "SET prototypeId=?2 "
        "WHERE prototypeId=?1 "
        "RETURNING typeId, prototypeNameId",
        database};
    Sqlite::ReadWriteStatement<2, 2> updateExtensionIdToTypeIdStatement{
        "UPDATE types "
        "SET extensionId=?2 "
        "WHERE extensionId=?1 "
        "RETURNING typeId, extensionNameId",
        database};
    Sqlite::ReadStatement<2, 2> selectTypeIdAndPrototypeNameIdForPrototypeIdAndTypeNameStatement{
        "SELECT typeId, prototypeNameId "
        "FROM types "
        "WHERE prototypeNameId IN ( "
        "    SELECT importedTypeNameId "
        "    FROM "
        "    importedTypeNames WHERE name=?1) "
        "  AND prototypeId=?2",
        database};
    Sqlite::ReadStatement<2, 2> selectTypeIdAndPrototypeNameIdForPrototypeIdAndSourceIdStatement{
        "SELECT typeId , prototypeNameId "
        "FROM types "
        "WHERE prototypeId=?1 AND sourceId=?2",
        database};
    Sqlite::ReadStatement<2, 2> selectTypeIdAndExtensionNameIdForExtensionIdAndSourceIdStatement{
        "SELECT typeId, extensionNameId "
        "FROM types "
        "WHERE extensionId=?1 AND sourceId=?2",
        database};
    Sqlite::ReadWriteStatement<3, 3> updatePrototypeIdAndExtensionIdToTypeIdForSourceIdStatement{
        "UPDATE types "
        "SET prototypeId=?2, extensionId=?3 "
        "WHERE sourceId=?1 "
        "RETURNING typeId, prototypeNameId, extensionNameId",
        database};
    Sqlite::ReadStatement<2, 2> selectTypeIdForExtensionIdAndTypeNameStatement{
        "SELECT typeId , extensionNameId "
        "FROM types "
        "WHERE extensionNameId IN (  "
        "    SELECT importedTypeNameId "
        "    FROM importedTypeNames "
        "    WHERE name=?1) "
        "  AND extensionId=?2",
        database};
    Sqlite::WriteStatement<2> updateTypePrototypeStatement{
        "UPDATE types SET prototypeId=?2 WHERE typeId=?1", database};
    Sqlite::WriteStatement<2> updateTypeExtensionStatement{
        "UPDATE types SET extensionId=?2 WHERE typeId=?1", database};
    mutable Sqlite::ReadStatement<1, 1> selectPrototypeAndExtensionIdsStatement{
        "WITH RECURSIVE "
        "  prototypes(typeId) AS (  "
        "      SELECT prototypeId FROM types WHERE typeId=?1 "
        "    UNION ALL "
        "      SELECT extensionId FROM types WHERE typeId=?1 "
        "    UNION ALL "
        "      SELECT prototypeId FROM types JOIN prototypes USING(typeId) "
        "    UNION ALL "
        "      SELECT extensionId FROM types JOIN prototypes USING(typeId)) "
        "SELECT typeId FROM prototypes WHERE typeId IS NOT NULL",
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
        "SELECT sourceId, size, lastModified FROM fileStatuses WHERE sourceId IN carray(?1) ORDER "
        "BY sourceId",
        database};
    mutable Sqlite::ReadStatement<3, 1> selectFileStatusesForSourceIdStatement{
        "SELECT sourceId, size, lastModified FROM fileStatuses WHERE sourceId=?1 ORDER BY sourceId",
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
        "SELECT importedTypeNameId FROM importedTypeNames WHERE kind=?1 AND importOrSourceId=?2 "
        "AND name=?3 LIMIT 1",
        database};
    mutable Sqlite::ReadWriteStatement<1, 3> insertImportedTypeNameIdStatement{
        "INSERT INTO importedTypeNames(kind, importOrSourceId, name) VALUES (?1, ?2, ?3) "
        "RETURNING importedTypeNameId",
        database};
    mutable Sqlite::ReadStatement<1, 2> selectImportIdBySourceIdAndModuleIdStatement{
        "SELECT importId FROM documentImports WHERE sourceId=?1 AND moduleId=?2 AND majorVersion "
        "IS NULL AND minorVersion IS NULL LIMIT 1",
        database};
    mutable Sqlite::ReadStatement<1, 3> selectImportIdBySourceIdAndModuleIdAndMajorVersionStatement{
        "SELECT importId FROM documentImports WHERE sourceId=?1 AND moduleId=?2 AND "
        "majorVersion=?3 AND minorVersion IS NULL LIMIT 1",
        database};
    mutable Sqlite::ReadStatement<1, 4> selectImportIdBySourceIdAndModuleIdAndVersionStatement{
        "SELECT importId FROM documentImports WHERE sourceId=?1 AND moduleId=?2 AND "
        "majorVersion=?3 AND minorVersion=?4 LIMIT 1",
        database};
    mutable Sqlite::ReadStatement<1, 1> selectKindFromImportedTypeNamesStatement{
        "SELECT kind FROM importedTypeNames WHERE importedTypeNameId=?1", database};
    mutable Sqlite::ReadStatement<1, 1> selectNameFromImportedTypeNamesStatement{
        "SELECT name FROM importedTypeNames WHERE importedTypeNameId=?1", database};
    mutable Sqlite::ReadStatement<1, 1> selectTypeIdForQualifiedImportedTypeNameNamesStatement{
        "SELECT typeId FROM importedTypeNames AS itn JOIN documentImports AS di ON "
        "importOrSourceId=di.importId JOIN documentImports AS di2 ON di.sourceId=di2.sourceId AND "
        "di.moduleId=di2.sourceModuleId "
        "JOIN exportedTypeNames AS etn ON di2.moduleId=etn.moduleId WHERE "
        "itn.kind=2 AND importedTypeNameId=?1 AND itn.name=etn.name AND "
        "(di.majorVersion IS NULL OR (di.majorVersion=etn.majorVersion AND (di.minorVersion IS "
        "NULL OR di.minorVersion>=etn.minorVersion))) ORDER BY etn.majorVersion DESC NULLS FIRST, "
        "etn.minorVersion DESC NULLS FIRST LIMIT 1",
        database};
    mutable Sqlite::ReadStatement<1, 1> selectTypeIdForImportedTypeNameNamesStatement{
        "WITH "
        "  importTypeNames(moduleId, name, kind, majorVersion, minorVersion) AS ( "
        "    SELECT moduleId, name, di.kind, majorVersion, minorVersion "
        "    FROM importedTypeNames AS itn JOIN documentImports AS di ON "
        "      importOrSourceId=sourceId "
        "    WHERE "
        "      importedTypeNameId=?1 AND itn.kind=1) "
        "SELECT typeId FROM importTypeNames AS itn "
        "  JOIN exportedTypeNames AS etn USING(moduleId, name) "
        "WHERE (itn.majorVersion IS NULL OR (itn.majorVersion=etn.majorVersion "
        "  AND (itn.minorVersion IS NULL OR itn.minorVersion>=etn.minorVersion))) "
        "ORDER BY itn.kind, etn.majorVersion DESC NULLS FIRST, etn.minorVersion DESC NULLS FIRST "
        "LIMIT 1",
        database};
    mutable Sqlite::ReadStatement<6, 1> selectExportedTypesForSourceIdsStatement{
        "SELECT moduleId, name, ifnull(majorVersion, -1), ifnull(minorVersion, -1), typeId, "
        "exportedTypeNameId FROM exportedTypeNames WHERE typeId in carray(?1) ORDER BY moduleId, "
        "name, majorVersion, minorVersion",
        database};
    Sqlite::WriteStatement<5> insertExportedTypeNamesWithVersionStatement{
        "INSERT INTO exportedTypeNames(moduleId, name, majorVersion, minorVersion, typeId) "
        "VALUES(?1, ?2, ?3, ?4, ?5)",
        database};
    Sqlite::WriteStatement<4> insertExportedTypeNamesWithMajorVersionStatement{
        "INSERT INTO exportedTypeNames(moduleId, name, majorVersion, typeId) "
        "VALUES(?1, ?2, ?3, ?4)",
        database};
    Sqlite::WriteStatement<3> insertExportedTypeNamesWithoutVersionStatement{
        "INSERT INTO exportedTypeNames(moduleId, name, typeId) VALUES(?1, ?2, ?3)", database};
    Sqlite::WriteStatement<1> deleteExportedTypeNameStatement{
        "DELETE FROM exportedTypeNames WHERE exportedTypeNameId=?", database};
    Sqlite::WriteStatement<2> updateExportedTypeNameTypeIdStatement{
        "UPDATE exportedTypeNames SET typeId=?2 WHERE exportedTypeNameId=?1", database};
    mutable Sqlite::ReadStatement<4, 1> selectDirectoryInfosForSourceIdsStatement{
        "SELECT directorySourceId, sourceId, moduleId, fileType FROM directoryInfos WHERE "
        "directorySourceId IN carray(?1) ORDER BY directorySourceId, sourceId",
        database};
    Sqlite::WriteStatement<4> insertDirectoryInfoStatement{
        "INSERT INTO directoryInfos(directorySourceId, sourceId, "
        "moduleId, fileType) VALUES(?1, ?2, ?3, ?4)",
        database};
    Sqlite::WriteStatement<2> deleteDirectoryInfoStatement{
        "DELETE FROM directoryInfos WHERE directorySourceId=?1 AND sourceId=?2", database};
    Sqlite::WriteStatement<4> updateDirectoryInfoStatement{
        "UPDATE directoryInfos SET moduleId=?3, fileType=?4 WHERE directorySourceId=?1 AND sourceId=?2",
        database};
    mutable Sqlite::ReadStatement<4, 1> selectDirectoryInfosForSourceIdStatement{
        "SELECT directorySourceId, sourceId, moduleId, fileType FROM directoryInfos WHERE "
        "directorySourceId=?1",
        database};
    mutable Sqlite::ReadStatement<4, 2> selectDirectoryInfosForSourceIdAndFileTypeStatement{
        "SELECT directorySourceId, sourceId, moduleId, fileType FROM directoryInfos WHERE "
        "directorySourceId=?1 AND fileType=?2",
        database};
    mutable Sqlite::ReadStatement<1, 2> selectDirectoryInfosSourceIdsForSourceIdAndFileTypeStatement{
        "SELECT sourceId FROM directoryInfos WHERE directorySourceId=?1 AND fileType=?2", database};
    mutable Sqlite::ReadStatement<4, 1> selectDirectoryInfoForSourceIdStatement{
        "SELECT directorySourceId, sourceId, moduleId, fileType FROM directoryInfos WHERE "
        "sourceId=?1 LIMIT 1",
        database};
    mutable Sqlite::ReadStatement<1, 1> selectTypeIdsForSourceIdsStatement{
        "SELECT typeId FROM types WHERE sourceId IN carray(?1)", database};
    mutable Sqlite::ReadStatement<6, 1> selectModuleExportedImportsForSourceIdStatement{
        "SELECT moduleExportedImportId, moduleId, exportedModuleId, ifnull(majorVersion, -1), "
        "ifnull(minorVersion, -1), isAutoVersion FROM moduleExportedImports WHERE moduleId IN "
        "carray(?1) ORDER BY moduleId, exportedModuleId",
        database};
    Sqlite::WriteStatement<3> insertModuleExportedImportWithoutVersionStatement{
        "INSERT INTO moduleExportedImports(moduleId, exportedModuleId, isAutoVersion) "
        "VALUES (?1, ?2, ?3)",
        database};
    Sqlite::WriteStatement<4> insertModuleExportedImportWithMajorVersionStatement{
        "INSERT INTO moduleExportedImports(moduleId, exportedModuleId, isAutoVersion, "
        "majorVersion) VALUES (?1, ?2, ?3, ?4)",
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
        "SELECT DISTINCT moduleId, ifnull(majorVersion, -1), ifnull(minorVersion, -1) "
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
    mutable Sqlite::ReadStatement<1, 1> selectSignalDeclarationNamesForTypeStatement{
        "WITH RECURSIVE "
        "  all_prototype_and_extension(typeId, prototypeId) AS ("
        "       SELECT typeId, prototypeId FROM types WHERE prototypeId IS NOT NULL"
        "    UNION ALL "
        "       SELECT typeId, extensionId FROM types WHERE extensionId IS NOT NULL),"
        "  typeChain(typeId) AS ("
        "      VALUES(?1)"
        "    UNION ALL "
        "      SELECT prototypeId FROM all_prototype_and_extension JOIN typeChain "
        "        USING(typeId)) "
        "SELECT name FROM typeChain JOIN signalDeclarations "
        "  USING(typeId) ORDER BY name",
        database};
    mutable Sqlite::ReadStatement<1, 1> selectFuncionDeclarationNamesForTypeStatement{
        "WITH RECURSIVE "
        "  all_prototype_and_extension(typeId, prototypeId) AS ("
        "       SELECT typeId, prototypeId FROM types WHERE prototypeId IS NOT NULL"
        "    UNION ALL "
        "       SELECT typeId, extensionId FROM types WHERE extensionId IS NOT NULL),"
        "  typeChain(typeId) AS ("
        "      VALUES(?1)"
        "    UNION ALL "
        "      SELECT prototypeId FROM all_prototype_and_extension JOIN typeChain "
        "        USING(typeId))"
        "SELECT name FROM typeChain JOIN functionDeclarations "
        "  USING(typeId) ORDER BY name",
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
        "SELECT  annotationTraits "
        "FROM types "
        "WHERE typeId=(SELECT prototypeId FROM types WHERE typeId=?)",
        database};
    mutable Sqlite::ReadStatement<1, 1> selectDefaultPropertyDeclarationIdStatement{
        "SELECT defaultPropertyId FROM types WHERE typeId=?", database};
    mutable Sqlite::ReadStatement<1, 1> selectPrototypeIdsForTypeIdInOrderStatement{
        "WITH RECURSIVE "
        "  all_prototype_and_extension(typeId, prototypeId) AS ("
        "       SELECT typeId, prototypeId FROM types WHERE prototypeId IS NOT NULL"
        "    UNION ALL "
        "       SELECT typeId, extensionId FROM types WHERE extensionId IS NOT NULL),"
        "  prototypes(typeId, level) AS ("
        "       SELECT prototypeId, 0 FROM all_prototype_and_extension WHERE typeId=?"
        "    UNION ALL "
        "      SELECT prototypeId, p.level+1 FROM all_prototype_and_extension JOIN "
        "        prototypes AS p USING(typeId)) "
        "SELECT typeId FROM prototypes ORDER BY level",
        database};
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
        "INSERT INTO propertyEditorPaths(typeId, pathSourceId, directoryId) VALUES (?1, ?2, ?3)",
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
        "  typeAnnotations(typeId, sourceId, directorySourceId, typeName, iconPath, itemLibrary, "
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
        "SELECT sourceId FROM typeAnnotations WHERE directorySourceId=?1 ORDER BY sourceId", database};
    mutable Sqlite::ReadStatement<1, 0> selectTypeAnnotationDirectorySourceIdsStatement{
        "SELECT DISTINCT directorySourceId FROM typeAnnotations ORDER BY directorySourceId", database};
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
    mutable Sqlite::ReadStatement<3, 1> selectItemLibraryPropertiesStatement{
        "SELECT p.value->>0, p.value->>1, p.value->>2 FROM json_each(?1) AS p", database};
    mutable Sqlite::ReadStatement<1, 1> selectItemLibraryExtraFilePathsStatement{
        "SELECT p.value FROM json_each(?1) AS p", database};
    mutable Sqlite::ReadStatement<1, 1> selectTypeIdsByModuleIdStatement{
        "SELECT DISTINCT typeId FROM exportedTypeNames WHERE moduleId=?", database};
    mutable Sqlite::ReadStatement<1, 1> selectHeirTypeIdsStatement{
        "WITH RECURSIVE "
        "  typeSelection(typeId) AS ("
        "      SELECT typeId FROM types WHERE prototypeId=?1 OR extensionId=?1"
        "    UNION ALL "
        "      SELECT t.typeId "
        "      FROM types AS t JOIN typeSelection AS ts "
        "      WHERE prototypeId=ts.typeId OR extensionId=ts.typeId)"
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
};

class ProjectStorage::Initializer
{
public:
    Initializer(Database &database, bool isInitialized)
    {
        if (!isInitialized) {
            auto moduleIdColumn = createModulesTable(database);

            createTypesAndePropertyDeclarationsTables(database, moduleIdColumn);
            createExportedTypeNamesTable(database, moduleIdColumn);
            createImportedTypeNamesTable(database);
            createEnumerationsTable(database);
            createFunctionsTable(database);
            createSignalsTable(database);
            createModuleExportedImportsTable(database, moduleIdColumn);
            createDocumentImportsTable(database, moduleIdColumn);
            createFileStatusesTable(database);
            createDirectoryInfosTable(database);
            createPropertyEditorPathsTable(database);
            createTypeAnnotionsTable(database);
        }
        database.setIsInitialized(true);
    }

    void createTypesAndePropertyDeclarationsTables(
        Database &database, [[maybe_unused]] const Sqlite::StrictColumn &foreignModuleIdColumn)
    {
        Sqlite::StrictTable typesTable;
        typesTable.setUseIfNotExists(true);
        typesTable.setName("types");
        typesTable.addColumn("typeId", Sqlite::StrictColumnType::Integer, {Sqlite::PrimaryKey{}});
        auto &sourceIdColumn = typesTable.addColumn("sourceId", Sqlite::StrictColumnType::Integer);
        auto &typesNameColumn = typesTable.addColumn("name", Sqlite::StrictColumnType::Text);
        typesTable.addColumn("traits", Sqlite::StrictColumnType::Integer);
        auto &prototypeIdColumn = typesTable.addColumn("prototypeId",
                                                       Sqlite::StrictColumnType::Integer);
        auto &prototypeNameIdColumn = typesTable.addColumn("prototypeNameId",
                                                           Sqlite::StrictColumnType::Integer);
        auto &extensionIdColumn = typesTable.addColumn("extensionId",
                                                       Sqlite::StrictColumnType::Integer);
        auto &extensionNameIdColumn = typesTable.addColumn("extensionNameId",
                                                           Sqlite::StrictColumnType::Integer);
        auto &defaultPropertyIdColumn = typesTable.addColumn("defaultPropertyId",
                                                             Sqlite::StrictColumnType::Integer);
        typesTable.addColumn("annotationTraits", Sqlite::StrictColumnType::Integer);
        typesTable.addUniqueIndex({sourceIdColumn, typesNameColumn});
        typesTable.addIndex({defaultPropertyIdColumn});
        typesTable.addIndex({prototypeIdColumn, sourceIdColumn});
        typesTable.addIndex({extensionIdColumn, sourceIdColumn});
        typesTable.addIndex({prototypeNameIdColumn});
        typesTable.addIndex({extensionNameIdColumn});

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

    void createExportedTypeNamesTable(Database &database,
                                      const Sqlite::StrictColumn &foreignModuleIdColumn)
    {
        Sqlite::StrictTable table;
        table.setUseIfNotExists(true);
        table.setName("exportedTypeNames");
        table.addColumn("exportedTypeNameId",
                        Sqlite::StrictColumnType::Integer,
                        {Sqlite::PrimaryKey{}});
        auto &moduleIdColumn = table.addForeignKeyColumn("moduleId",
                                                         foreignModuleIdColumn,
                                                         Sqlite::ForeignKeyAction::NoAction,
                                                         Sqlite::ForeignKeyAction::NoAction);
        auto &nameColumn = table.addColumn("name", Sqlite::StrictColumnType::Text);
        auto &typeIdColumn = table.addColumn("typeId", Sqlite::StrictColumnType::Integer);
        auto &majorVersionColumn = table.addColumn("majorVersion", Sqlite::StrictColumnType::Integer);
        auto &minorVersionColumn = table.addColumn("minorVersion", Sqlite::StrictColumnType::Integer);

        table.addUniqueIndex({moduleIdColumn, nameColumn},
                             "majorVersion IS NULL AND minorVersion IS NULL");
        table.addUniqueIndex({moduleIdColumn, nameColumn, majorVersionColumn},
                             "majorVersion IS NOT NULL AND minorVersion IS NULL");
        table.addUniqueIndex({moduleIdColumn, nameColumn, majorVersionColumn, minorVersionColumn},
                             "majorVersion IS NOT NULL AND minorVersion IS NOT NULL");

        table.addIndex({typeIdColumn});
        table.addIndex({moduleIdColumn, nameColumn});

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

    Sqlite::StrictColumn createModulesTable(Database &database)
    {
        Sqlite::StrictTable table;
        table.setUseIfNotExists(true);
        table.setName("modules");
        auto &modelIdColumn = table.addColumn("moduleId",
                                              Sqlite::StrictColumnType::Integer,
                                              {Sqlite::PrimaryKey{}});
        auto &kindColumn = table.addColumn("kind", Sqlite::StrictColumnType::Integer);
        auto &nameColumn = table.addColumn("name", Sqlite::StrictColumnType::Text);

        table.addUniqueIndex({kindColumn, nameColumn});

        table.initialize(database);

        return std::move(modelIdColumn);
    }

    void createModuleExportedImportsTable(Database &database,
                                          const Sqlite::StrictColumn &foreignModuleIdColumn)
    {
        Sqlite::StrictTable table;
        table.setUseIfNotExists(true);
        table.setName("moduleExportedImports");
        table.addColumn("moduleExportedImportId",
                        Sqlite::StrictColumnType::Integer,
                        {Sqlite::PrimaryKey{}});
        auto &moduleIdColumn = table.addForeignKeyColumn("moduleId",
                                                         foreignModuleIdColumn,
                                                         Sqlite::ForeignKeyAction::NoAction,
                                                         Sqlite::ForeignKeyAction::Cascade,
                                                         Sqlite::Enforment::Immediate);
        auto &sourceIdColumn = table.addColumn("exportedModuleId", Sqlite::StrictColumnType::Integer);
        table.addColumn("isAutoVersion", Sqlite::StrictColumnType::Integer);
        table.addColumn("majorVersion", Sqlite::StrictColumnType::Integer);
        table.addColumn("minorVersion", Sqlite::StrictColumnType::Integer);

        table.addUniqueIndex({sourceIdColumn, moduleIdColumn});

        table.initialize(database);
    }

    void createDocumentImportsTable(Database &database,
                                    const Sqlite::StrictColumn &foreignModuleIdColumn)
    {
        Sqlite::StrictTable table;
        table.setUseIfNotExists(true);
        table.setName("documentImports");
        table.addColumn("importId", Sqlite::StrictColumnType::Integer, {Sqlite::PrimaryKey{}});
        auto &sourceIdColumn = table.addColumn("sourceId", Sqlite::StrictColumnType::Integer);
        auto &moduleIdColumn = table.addForeignKeyColumn("moduleId",
                                                         foreignModuleIdColumn,
                                                         Sqlite::ForeignKeyAction::NoAction,
                                                         Sqlite::ForeignKeyAction::Cascade,
                                                         Sqlite::Enforment::Immediate);
        auto &sourceModuleIdColumn = table.addForeignKeyColumn("sourceModuleId",
                                                               foreignModuleIdColumn,
                                                               Sqlite::ForeignKeyAction::NoAction,
                                                               Sqlite::ForeignKeyAction::Cascade,
                                                               Sqlite::Enforment::Immediate);
        auto &kindColumn = table.addColumn("kind", Sqlite::StrictColumnType::Integer);
        auto &majorVersionColumn = table.addColumn("majorVersion", Sqlite::StrictColumnType::Integer);
        auto &minorVersionColumn = table.addColumn("minorVersion", Sqlite::StrictColumnType::Integer);
        auto &parentImportIdColumn = table.addColumn("parentImportId",
                                                     Sqlite::StrictColumnType::Integer);

        table.addUniqueIndex(
            {sourceIdColumn, moduleIdColumn, kindColumn, sourceModuleIdColumn, parentImportIdColumn},
            "majorVersion IS NULL AND minorVersion IS NULL");
        table.addUniqueIndex({sourceIdColumn,
                              moduleIdColumn,
                              kindColumn,
                              sourceModuleIdColumn,
                              majorVersionColumn,
                              parentImportIdColumn},
                             "majorVersion IS NOT NULL AND minorVersion IS NULL");
        table.addUniqueIndex({sourceIdColumn,
                              moduleIdColumn,
                              kindColumn,
                              sourceModuleIdColumn,
                              majorVersionColumn,
                              minorVersionColumn,
                              parentImportIdColumn},
                             "majorVersion IS NOT NULL AND minorVersion IS NOT NULL");

        table.addIndex({sourceIdColumn, kindColumn});

        table.initialize(database);
    }

    void createFileStatusesTable(Database &database)
    {
        Sqlite::StrictTable table;
        table.setUseIfNotExists(true);
        table.setName("fileStatuses");
        table.addColumn("sourceId", Sqlite::StrictColumnType::Integer, {Sqlite::PrimaryKey{}});
        table.addColumn("size", Sqlite::StrictColumnType::Integer);
        table.addColumn("lastModified", Sqlite::StrictColumnType::Integer);

        table.initialize(database);
    }

    void createDirectoryInfosTable(Database &database)
    {
        Sqlite::StrictTable table;
        table.setUseIfNotExists(true);
        table.setUseWithoutRowId(true);
        table.setName("directoryInfos");
        auto &directorySourceIdColumn = table.addColumn("directorySourceId",
                                                      Sqlite::StrictColumnType::Integer);
        auto &sourceIdColumn = table.addColumn("sourceId", Sqlite::StrictColumnType::Integer);
        table.addColumn("moduleId", Sqlite::StrictColumnType::Integer);
        auto &fileTypeColumn = table.addColumn("fileType", Sqlite::StrictColumnType::Integer);

        table.addPrimaryKeyContraint({directorySourceIdColumn, sourceIdColumn});
        table.addUniqueIndex({sourceIdColumn});
        table.addIndex({directorySourceIdColumn, fileTypeColumn});

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
        auto &directorySourceIdColumn = table.addColumn("directorySourceId",
                                                        Sqlite::StrictColumnType::Integer);
        table.addColumn("typeName", Sqlite::StrictColumnType::Text);
        table.addColumn("iconPath", Sqlite::StrictColumnType::Text);
        table.addColumn("itemLibrary", Sqlite::StrictColumnType::Text);
        table.addColumn("hints", Sqlite::StrictColumnType::Text);

        table.addUniqueIndex({sourceIdColumn, typeIdColumn});
        table.addIndex({directorySourceIdColumn});

        table.initialize(database);
    }
};

ProjectStorage::ProjectStorage(Database &database,
                               ProjectStorageErrorNotifierInterface &errorNotifier,
                               bool isInitialized)
    : database{database}
    , errorNotifier{&errorNotifier}
    , exclusiveTransaction{database}
    , initializer{std::make_unique<ProjectStorage::Initializer>(database, isInitialized)}
    , moduleCache{ModuleStorageAdapter{*this}}
    , s{std::make_unique<ProjectStorage::Statements>(database)}
{
    NanotraceHR::Tracer tracer{"initialize"_t, projectStorageCategory()};

    exclusiveTransaction.commit();

    database.walCheckpointFull();

    moduleCache.populate();
}

ProjectStorage::~ProjectStorage() = default;

void ProjectStorage::synchronize(Storage::Synchronization::SynchronizationPackage package)
{
    NanotraceHR::Tracer tracer{"synchronize"_t, projectStorageCategory()};

    TypeIds deletedTypeIds;
    ExportedTypesChanged exportedTypesChanged = ExportedTypesChanged::No;

    Sqlite::withImmediateTransaction(database, [&] {
        AliasPropertyDeclarations aliasPropertyDeclarationsToLink;

        AliasPropertyDeclarations relinkableAliasPropertyDeclarations;
        PropertyDeclarations relinkablePropertyDeclarations;
        Prototypes relinkablePrototypes;
        Prototypes relinkableExtensions;

        TypeIds updatedTypeIds;
        updatedTypeIds.reserve(package.types.size());

        TypeIds typeIdsToBeDeleted;

        std::sort(package.updatedSourceIds.begin(), package.updatedSourceIds.end());

        synchronizeFileStatuses(package.fileStatuses, package.updatedFileStatusSourceIds);
        synchronizeImports(package.imports,
                           package.updatedSourceIds,
                           package.moduleDependencies,
                           package.updatedModuleDependencySourceIds,
                           package.moduleExportedImports,
                           package.updatedModuleIds,
                           relinkablePrototypes,
                           relinkableExtensions);
        synchronizeTypes(package.types,
                         updatedTypeIds,
                         aliasPropertyDeclarationsToLink,
                         relinkableAliasPropertyDeclarations,
                         relinkablePropertyDeclarations,
                         relinkablePrototypes,
                         relinkableExtensions,
                         exportedTypesChanged,
                         package.updatedSourceIds);
        synchronizeTypeAnnotations(package.typeAnnotations, package.updatedTypeAnnotationSourceIds);
        synchronizePropertyEditorQmlPaths(package.propertyEditorQmlPaths,
                                          package.updatedPropertyEditorQmlPathSourceIds);

        deleteNotUpdatedTypes(updatedTypeIds,
                              package.updatedSourceIds,
                              typeIdsToBeDeleted,
                              relinkableAliasPropertyDeclarations,
                              relinkablePropertyDeclarations,
                              relinkablePrototypes,
                              relinkableExtensions,
                              deletedTypeIds);

        relink(relinkableAliasPropertyDeclarations,
               relinkablePropertyDeclarations,
               relinkablePrototypes,
               relinkableExtensions,
               deletedTypeIds);

        repairBrokenAliasPropertyDeclarations();

        linkAliases(aliasPropertyDeclarationsToLink, RaiseError::Yes);

        synchronizeDirectoryInfos(package.directoryInfos, package.updatedDirectoryInfoSourceIds);

        commonTypeCache_.resetTypeIds();
    });

    callRefreshMetaInfoCallback(deletedTypeIds, exportedTypesChanged);
}

void ProjectStorage::synchronizeDocumentImports(Storage::Imports imports, SourceId sourceId)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"synchronize document imports"_t,
                               projectStorageCategory(),
                               keyValue("imports", imports),
                               keyValue("source id", sourceId)};

    Sqlite::withImmediateTransaction(database, [&] {
        AliasPropertyDeclarations relinkableAliasPropertyDeclarations;
        PropertyDeclarations relinkablePropertyDeclarations;
        Prototypes relinkablePrototypes;
        Prototypes relinkableExtensions;
        TypeIds deletedTypeIds;

        synchronizeDocumentImports(imports,
                                   {sourceId},
                                   Storage::Synchronization::ImportKind::Import,
                                   Relink::Yes,
                                   relinkablePrototypes,
                                   relinkableExtensions);

        relink(relinkableAliasPropertyDeclarations,
               relinkablePropertyDeclarations,
               relinkablePrototypes,
               relinkableExtensions,
               deletedTypeIds);
    });
}

void ProjectStorage::addObserver(ProjectStorageObserver *observer)
{
    NanotraceHR::Tracer tracer{"add observer"_t, projectStorageCategory()};
    observers.push_back(observer);
}

void ProjectStorage::removeObserver(ProjectStorageObserver *observer)
{
    NanotraceHR::Tracer tracer{"remove observer"_t, projectStorageCategory()};
    observers.removeOne(observer);
}

ModuleId ProjectStorage::moduleId(Utils::SmallStringView moduleName, Storage::ModuleKind kind) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get module id"_t,
                               projectStorageCategory(),
                               keyValue("module name", moduleName)};

    auto moduleId = moduleCache.id({moduleName, kind});

    tracer.end(keyValue("module id", moduleId));

    return moduleId;
}

Storage::Module ProjectStorage::module(ModuleId moduleId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get module name"_t,
                               projectStorageCategory(),
                               keyValue("module id", moduleId)};

    if (!moduleId)
        throw ModuleDoesNotExists{};

    auto module = moduleCache.value(moduleId);

    tracer.end(keyValue("module name", module.name));
    tracer.end(keyValue("module kind", module.kind));

    return {module.name, module.kind};
}

TypeId ProjectStorage::typeId(ModuleId moduleId,
                              Utils::SmallStringView exportedTypeName,
                              Storage::Version version) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get type id by exported name"_t,
                               projectStorageCategory(),
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

TypeId ProjectStorage::typeId(ImportedTypeNameId typeNameId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get type id by imported type name"_t,
                               projectStorageCategory(),
                               keyValue("imported type name id", typeNameId)};

    auto typeId = Sqlite::withDeferredTransaction(database, [&] { return fetchTypeId(typeNameId); });

    tracer.end(keyValue("type id", typeId));

    return typeId;
}

QVarLengthArray<TypeId, 256> ProjectStorage::typeIds(ModuleId moduleId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get type ids by module id"_t,
                               projectStorageCategory(),
                               keyValue("module id", moduleId)};

    auto typeIds = s->selectTypeIdsByModuleIdStatement
                       .valuesWithTransaction<QVarLengthArray<TypeId, 256>>(moduleId);

    tracer.end(keyValue("type ids", typeIds));

    return typeIds;
}

Storage::Info::ExportedTypeNames ProjectStorage::exportedTypeNames(TypeId typeId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get exported type names by type id"_t,
                               projectStorageCategory(),
                               keyValue("type id", typeId)};

    auto exportedTypenames = s->selectExportedTypesByTypeIdStatement
                                 .valuesWithTransaction<Storage::Info::ExportedTypeName, 4>(typeId);

    tracer.end(keyValue("exported type names", exportedTypenames));

    return exportedTypenames;
}

Storage::Info::ExportedTypeNames ProjectStorage::exportedTypeNames(TypeId typeId, SourceId sourceId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get exported type names by source id"_t,
                               projectStorageCategory(),
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
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get import id by import"_t,
                               projectStorageCategory(),
                               keyValue("import", import)};

    auto importId = Sqlite::withDeferredTransaction(database, [&] {
        return fetchImportId(import.sourceId, import);
    });

    tracer.end(keyValue("import id", importId));

    return importId;
}

ImportedTypeNameId ProjectStorage::importedTypeNameId(ImportId importId,
                                                      Utils::SmallStringView typeName)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get imported type name id by import id"_t,
                               projectStorageCategory(),
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
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get imported type name id by source id"_t,
                               projectStorageCategory(),
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
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get property declaration ids"_t,
                               projectStorageCategory(),
                               keyValue("type id", typeId)};

    auto propertyDeclarationIds = Sqlite::withDeferredTransaction(database, [&] {
        return fetchPropertyDeclarationIds(typeId);
    });

    std::sort(propertyDeclarationIds.begin(), propertyDeclarationIds.end());

    tracer.end(keyValue("property declaration ids", propertyDeclarationIds));

    return propertyDeclarationIds;
}

QVarLengthArray<PropertyDeclarationId, 128> ProjectStorage::localPropertyDeclarationIds(TypeId typeId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get local property declaration ids"_t,
                               projectStorageCategory(),
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
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get property declaration id"_t,
                               projectStorageCategory(),
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
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get local property declaration id"_t,
                               projectStorageCategory(),
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
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get default property declaration id"_t,
                               projectStorageCategory(),
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
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get property declaration"_t,
                               projectStorageCategory(),
                               keyValue("property declaration id", propertyDeclarationId)};

    auto propertyDeclaration = s->selectPropertyDeclarationForPropertyDeclarationIdStatement
                                   .optionalValueWithTransaction<Storage::Info::PropertyDeclaration>(
                                       propertyDeclarationId);

    tracer.end(keyValue("property declaration", propertyDeclaration));

    return propertyDeclaration;
}

std::optional<Storage::Info::Type> ProjectStorage::type(TypeId typeId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get type"_t, projectStorageCategory(), keyValue("type id", typeId)};

    auto type = s->selectInfoTypeByTypeIdStatement.optionalValueWithTransaction<Storage::Info::Type>(
        typeId);

    tracer.end(keyValue("type", type));

    return type;
}

Utils::PathString ProjectStorage::typeIconPath(TypeId typeId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get type icon path"_t,
                               projectStorageCategory(),
                               keyValue("type id", typeId)};

    auto typeIconPath = s->selectTypeIconPathStatement.valueWithTransaction<Utils::PathString>(typeId);

    tracer.end(keyValue("type icon path", typeIconPath));

    return typeIconPath;
}

Storage::Info::TypeHints ProjectStorage::typeHints(TypeId typeId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get type hints"_t,
                               projectStorageCategory(),
                               keyValue("type id", typeId)};

    auto typeHints = s->selectTypeHintsStatement.valuesWithTransaction<Storage::Info::TypeHints, 4>(
        typeId);

    tracer.end(keyValue("type hints", typeHints));

    return typeHints;
}

SmallSourceIds<4> ProjectStorage::typeAnnotationSourceIds(SourceId directoryId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get type annotaion source ids"_t,
                               projectStorageCategory(),
                               keyValue("source id", directoryId)};

    auto sourceIds = s->selectTypeAnnotationSourceIdsStatement.valuesWithTransaction<SmallSourceIds<4>>(
        directoryId);

    tracer.end(keyValue("source ids", sourceIds));

    return sourceIds;
}

SmallSourceIds<64> ProjectStorage::typeAnnotationDirectorySourceIds() const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get type annotaion source ids"_t, projectStorageCategory()};

    auto sourceIds = s->selectTypeAnnotationDirectorySourceIdsStatement
                         .valuesWithTransaction<SmallSourceIds<64>>();

    tracer.end(keyValue("source ids", sourceIds));

    return sourceIds;
}

Storage::Info::ItemLibraryEntries ProjectStorage::itemLibraryEntries(TypeId typeId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get item library entries  by type id"_t,
                               projectStorageCategory(),
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
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get item library entries  by import id"_t,
                               projectStorageCategory(),
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

Storage::Info::ItemLibraryEntries ProjectStorage::itemLibraryEntries(SourceId sourceId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get item library entries by source id"_t,
                               projectStorageCategory(),
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
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get all item library entries"_t, projectStorageCategory()};

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

std::vector<Utils::SmallString> ProjectStorage::signalDeclarationNames(TypeId typeId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get signal names"_t,
                               projectStorageCategory(),
                               keyValue("type id", typeId)};

    auto signalDeclarationNames = s->selectSignalDeclarationNamesForTypeStatement
                                      .valuesWithTransaction<Utils::SmallString, 32>(typeId);

    tracer.end(keyValue("signal names", signalDeclarationNames));

    return signalDeclarationNames;
}

std::vector<Utils::SmallString> ProjectStorage::functionDeclarationNames(TypeId typeId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get function names"_t,
                               projectStorageCategory(),
                               keyValue("type id", typeId)};

    auto functionDeclarationNames = s->selectFuncionDeclarationNamesForTypeStatement
                                        .valuesWithTransaction<Utils::SmallString, 32>(typeId);

    tracer.end(keyValue("function names", functionDeclarationNames));

    return functionDeclarationNames;
}

std::optional<Utils::SmallString> ProjectStorage::propertyName(
    PropertyDeclarationId propertyDeclarationId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get property name"_t,
                               projectStorageCategory(),
                               keyValue("property declaration id", propertyDeclarationId)};

    auto propertyName = s->selectPropertyNameStatement.optionalValueWithTransaction<Utils::SmallString>(
        propertyDeclarationId);

    tracer.end(keyValue("property name", propertyName));

    return propertyName;
}

SmallTypeIds<16> ProjectStorage::prototypeIds(TypeId type) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get prototypes"_t, projectStorageCategory(), keyValue("type id", type)};

    auto prototypeIds = s->selectPrototypeAndExtensionIdsStatement
                            .valuesWithTransaction<SmallTypeIds<16>>(type);

    tracer.end(keyValue("type ids", prototypeIds));

    return prototypeIds;
}

SmallTypeIds<16> ProjectStorage::prototypeAndSelfIds(TypeId typeId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get prototypes and self"_t, projectStorageCategory()};

    SmallTypeIds<16> prototypeAndSelfIds;
    prototypeAndSelfIds.push_back(typeId);

    s->selectPrototypeAndExtensionIdsStatement.readToWithTransaction(prototypeAndSelfIds, typeId);

    tracer.end(keyValue("type ids", prototypeAndSelfIds));

    return prototypeAndSelfIds;
}

SmallTypeIds<64> ProjectStorage::heirIds(TypeId typeId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"get heirs"_t, projectStorageCategory()};

    auto heirIds = s->selectHeirTypeIdsStatement.valuesWithTransaction<SmallTypeIds<64>>(typeId);

    tracer.end(keyValue("type ids", heirIds));

    return heirIds;
}

bool ProjectStorage::isBasedOn(TypeId) const
{
    return false;
}

bool ProjectStorage::isBasedOn(TypeId typeId, TypeId id1) const
{
    return isBasedOn_(typeId, id1);
}

bool ProjectStorage::isBasedOn(TypeId typeId, TypeId id1, TypeId id2) const
{
    return isBasedOn_(typeId, id1, id2);
}

bool ProjectStorage::isBasedOn(TypeId typeId, TypeId id1, TypeId id2, TypeId id3) const
{
    return isBasedOn_(typeId, id1, id2, id3);
}

bool ProjectStorage::isBasedOn(TypeId typeId, TypeId id1, TypeId id2, TypeId id3, TypeId id4) const
{
    return isBasedOn_(typeId, id1, id2, id3, id4);
}

bool ProjectStorage::isBasedOn(TypeId typeId, TypeId id1, TypeId id2, TypeId id3, TypeId id4, TypeId id5) const
{
    return isBasedOn_(typeId, id1, id2, id3, id4, id5);
}

bool ProjectStorage::isBasedOn(
    TypeId typeId, TypeId id1, TypeId id2, TypeId id3, TypeId id4, TypeId id5, TypeId id6) const
{
    return isBasedOn_(typeId, id1, id2, id3, id4, id5, id6);
}

bool ProjectStorage::isBasedOn(
    TypeId typeId, TypeId id1, TypeId id2, TypeId id3, TypeId id4, TypeId id5, TypeId id6, TypeId id7) const
{
    return isBasedOn_(typeId, id1, id2, id3, id4, id5, id6, id7);
}

TypeId ProjectStorage::fetchTypeIdByExportedName(Utils::SmallStringView name) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"is based on"_t,
                               projectStorageCategory(),
                               keyValue("exported type name", name)};

    auto typeId = s->selectTypeIdByExportedNameStatement.valueWithTransaction<TypeId>(name);

    tracer.end(keyValue("type id", typeId));

    return typeId;
}

TypeId ProjectStorage::fetchTypeIdByModuleIdsAndExportedName(ModuleIds moduleIds,
                                                             Utils::SmallStringView name) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch type id by module ids and exported name"_t,
                               projectStorageCategory(),
                               keyValue("module ids", NanotraceHR::array(moduleIds)),
                               keyValue("exported type name", name)};
    auto typeId = s->selectTypeIdByModuleIdsAndExportedNameStatement.valueWithTransaction<TypeId>(
        static_cast<void *>(moduleIds.data()), static_cast<long long>(moduleIds.size()), name);

    tracer.end(keyValue("type id", typeId));

    return typeId;
}

TypeId ProjectStorage::fetchTypeIdByName(SourceId sourceId, Utils::SmallStringView name)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch type id by name"_t,
                               projectStorageCategory(),
                               keyValue("source id", sourceId),
                               keyValue("internal type name", name)};

    auto typeId = s->selectTypeIdBySourceIdAndNameStatement.valueWithTransaction<TypeId>(sourceId,
                                                                                         name);

    tracer.end(keyValue("type id", typeId));

    return typeId;
}

Storage::Synchronization::Type ProjectStorage::fetchTypeByTypeId(TypeId typeId)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch type by type id"_t,
                               projectStorageCategory(),
                               keyValue("type id", typeId)};

    auto type = Sqlite::withDeferredTransaction(database, [&] {
        auto type = s->selectTypeByTypeIdStatement.value<Storage::Synchronization::Type>(typeId);

        type.exportedTypes = fetchExportedTypes(typeId);
        type.propertyDeclarations = fetchPropertyDeclarations(type.typeId);
        type.functionDeclarations = fetchFunctionDeclarations(type.typeId);
        type.signalDeclarations = fetchSignalDeclarations(type.typeId);
        type.enumerationDeclarations = fetchEnumerationDeclarations(type.typeId);
        type.typeId = typeId;

        return type;
    });

    tracer.end(keyValue("type", type));

    return type;
}

Storage::Synchronization::Types ProjectStorage::fetchTypes()
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch types"_t, projectStorageCategory()};

    auto types = Sqlite::withDeferredTransaction(database, [&] {
        auto types = s->selectTypesStatement.values<Storage::Synchronization::Type, 64>();

        for (Storage::Synchronization::Type &type : types) {
            type.exportedTypes = fetchExportedTypes(type.typeId);
            type.propertyDeclarations = fetchPropertyDeclarations(type.typeId);
            type.functionDeclarations = fetchFunctionDeclarations(type.typeId);
            type.signalDeclarations = fetchSignalDeclarations(type.typeId);
            type.enumerationDeclarations = fetchEnumerationDeclarations(type.typeId);
        }

        return types;
    });

    tracer.end(keyValue("type", types));

    return types;
}

FileStatuses ProjectStorage::fetchAllFileStatuses() const
{
    NanotraceHR::Tracer tracer{"fetch all file statuses"_t, projectStorageCategory()};

    return s->selectAllFileStatusesStatement.valuesWithTransaction<FileStatus>();
}

FileStatus ProjectStorage::fetchFileStatus(SourceId sourceId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch file status"_t,
                               projectStorageCategory(),
                               keyValue("source id", sourceId)};

    auto fileStatus = s->selectFileStatusesForSourceIdStatement.valueWithTransaction<FileStatus>(
        sourceId);

    tracer.end(keyValue("file status", fileStatus));

    return fileStatus;
}

std::optional<Storage::Synchronization::DirectoryInfo> ProjectStorage::fetchDirectoryInfo(SourceId sourceId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch directory info"_t,
                               projectStorageCategory(),
                               keyValue("source id", sourceId)};

    auto directoryInfo = s->selectDirectoryInfoForSourceIdStatement
                           .optionalValueWithTransaction<Storage::Synchronization::DirectoryInfo>(
                               sourceId);

    tracer.end(keyValue("directory info", directoryInfo));

    return directoryInfo;
}

Storage::Synchronization::DirectoryInfos ProjectStorage::fetchDirectoryInfos(SourceId directorySourceId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch directory infos by source id"_t,
                               projectStorageCategory(),
                               keyValue("source id", directorySourceId)};

    auto directoryInfos = s->selectDirectoryInfosForSourceIdStatement
                            .valuesWithTransaction<Storage::Synchronization::DirectoryInfo, 1024>(
                                directorySourceId);

    tracer.end(keyValue("directory infos", directoryInfos));

    return directoryInfos;
}

Storage::Synchronization::DirectoryInfos ProjectStorage::fetchDirectoryInfos(
    SourceId directorySourceId, Storage::Synchronization::FileType fileType) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch directory infos by source id and file type"_t,
                               projectStorageCategory(),
                               keyValue("source id", directorySourceId),
                               keyValue("file type", fileType)};

    auto directoryInfos = s->selectDirectoryInfosForSourceIdAndFileTypeStatement
                              .valuesWithTransaction<Storage::Synchronization::DirectoryInfo, 16>(
                                  directorySourceId, fileType);

    tracer.end(keyValue("directory infos", directoryInfos));

    return directoryInfos;
}

Storage::Synchronization::DirectoryInfos ProjectStorage::fetchDirectoryInfos(
    const SourceIds &directorySourceIds) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch directory infos by source ids"_t,
                               projectStorageCategory(),
                               keyValue("source ids", directorySourceIds)};

    auto directoryInfos = s->selectDirectoryInfosForSourceIdsStatement
                            .valuesWithTransaction<Storage::Synchronization::DirectoryInfo, 64>(
                                toIntegers(directorySourceIds));

    tracer.end(keyValue("directory infos", directoryInfos));

    return directoryInfos;
}

SmallSourceIds<32> ProjectStorage::fetchSubdirectorySourceIds(SourceId directorySourceId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch subdirectory source ids"_t,
                               projectStorageCategory(),
                               keyValue("source id", directorySourceId)};

    auto sourceIds = s->selectDirectoryInfosSourceIdsForSourceIdAndFileTypeStatement
                         .valuesWithTransaction<SmallSourceIds<32>>(
                             directorySourceId, Storage::Synchronization::FileType::Directory);

    tracer.end(keyValue("source ids", sourceIds));

    return sourceIds;
}

void ProjectStorage::setPropertyEditorPathId(TypeId typeId, SourceId pathId)
{
    Sqlite::ImmediateSessionTransaction transaction{database};

    s->upsertPropertyEditorPathIdStatement.write(typeId, pathId);

    transaction.commit();
}

SourceId ProjectStorage::propertyEditorPathId(TypeId typeId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"property editor path id"_t,
                               projectStorageCategory(),
                               keyValue("type id", typeId)};

    auto sourceId = s->selectPropertyEditorPathIdStatement.valueWithTransaction<SourceId>(typeId);

    tracer.end(keyValue("source id", sourceId));

    return sourceId;
}

Storage::Imports ProjectStorage::fetchDocumentImports() const
{
    NanotraceHR::Tracer tracer{"fetch document imports"_t, projectStorageCategory()};

    return s->selectAllDocumentImportForSourceIdStatement.valuesWithTransaction<Storage::Imports>();
}

void ProjectStorage::resetForTestsOnly()
{
    database.clearAllTablesForTestsOnly();
    commonTypeCache_.clearForTestsOnly();
    moduleCache.clearForTestOnly();
    observers.clear();
}

ModuleId ProjectStorage::fetchModuleId(Utils::SmallStringView moduleName,
                                       Storage::ModuleKind moduleKind)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch module id"_t,
                               projectStorageCategory(),
                               keyValue("module name", moduleName),
                               keyValue("module kind", moduleKind)};

    auto moduleId = Sqlite::withDeferredTransaction(database, [&] {
        return fetchModuleIdUnguarded(moduleName, moduleKind);
    });

    tracer.end(keyValue("module id", moduleId));

    return moduleId;
}

Storage::Module ProjectStorage::fetchModule(ModuleId id)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch module name"_t,
                               projectStorageCategory(),
                               keyValue("module id", id)};

    auto module = Sqlite::withDeferredTransaction(database, [&] { return fetchModuleUnguarded(id); });

    tracer.end(keyValue("module name", module.name));
    tracer.end(keyValue("module name", module.kind));

    return module;
}

ProjectStorage::ModuleCacheEntries ProjectStorage::fetchAllModules() const
{
    NanotraceHR::Tracer tracer{"fetch all modules"_t, projectStorageCategory()};

    return s->selectAllModulesStatement.valuesWithTransaction<ModuleCacheEntry, 128>();
}

void ProjectStorage::callRefreshMetaInfoCallback(TypeIds &deletedTypeIds,
                                                 ExportedTypesChanged &exportedTypesChanged)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"call refresh meta info callback"_t,
                               projectStorageCategory(),
                               keyValue("type ids", deletedTypeIds)};

    if (deletedTypeIds.size()) {
        std::ranges::sort(deletedTypeIds);

        for (ProjectStorageObserver *observer : observers)
            observer->removedTypeIds(deletedTypeIds);
    }

    if (exportedTypesChanged == ExportedTypesChanged::Yes) {
        for (ProjectStorageObserver *observer : observers)
            observer->exportedTypesChanged();
    }
}

SourceIds ProjectStorage::filterSourceIdsWithoutType(const SourceIds &updatedSourceIds,
                                                     SourceIds &sourceIdsOfTypes)
{
    std::sort(sourceIdsOfTypes.begin(), sourceIdsOfTypes.end());

    SourceIds sourceIdsWithoutTypeSourceIds;
    sourceIdsWithoutTypeSourceIds.reserve(updatedSourceIds.size());
    std::set_difference(updatedSourceIds.begin(),
                        updatedSourceIds.end(),
                        sourceIdsOfTypes.begin(),
                        sourceIdsOfTypes.end(),
                        std::back_inserter(sourceIdsWithoutTypeSourceIds));

    return sourceIdsWithoutTypeSourceIds;
}

TypeIds ProjectStorage::fetchTypeIds(const SourceIds &sourceIds)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch type ids"_t,
                               projectStorageCategory(),
                               keyValue("source ids", sourceIds)};

    return s->selectTypeIdsForSourceIdsStatement.values<TypeId, 128>(toIntegers(sourceIds));
}

void ProjectStorage::unique(SourceIds &sourceIds)
{
    std::sort(sourceIds.begin(), sourceIds.end());
    auto newEnd = std::unique(sourceIds.begin(), sourceIds.end());
    sourceIds.erase(newEnd, sourceIds.end());
}

void ProjectStorage::synchronizeTypeTraits(TypeId typeId, Storage::TypeTraits traits)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"synchronize type traits"_t,
                               projectStorageCategory(),
                               keyValue("type id", typeId),
                               keyValue("type traits", traits)};

    s->updateTypeAnnotationTraitStatement.write(typeId, traits.annotation);
}

void ProjectStorage::updateTypeIdInTypeAnnotations(Storage::Synchronization::TypeAnnotations &typeAnnotations)
{
    NanotraceHR::Tracer tracer{"update type id in type annotations"_t, projectStorageCategory()};

    for (auto &annotation : typeAnnotations) {
        annotation.typeId = fetchTypeIdByModuleIdAndExportedName(annotation.moduleId,
                                                                 annotation.typeName);
    }

    typeAnnotations.erase(std::remove_if(typeAnnotations.begin(),
                                         typeAnnotations.end(),
                                         [](const auto &annotation) { return !annotation.typeId; }),
                          typeAnnotations.end());
}

void ProjectStorage::synchronizeTypeAnnotations(Storage::Synchronization::TypeAnnotations &typeAnnotations,
                                                const SourceIds &updatedTypeAnnotationSourceIds)
{
    NanotraceHR::Tracer tracer{"synchronize type annotations"_t, projectStorageCategory()};

    using Storage::Synchronization::TypeAnnotation;

    updateTypeIdInTypeAnnotations(typeAnnotations);

    auto compareKey = [](auto &&first, auto &&second) { return first.typeId - second.typeId; };

    std::sort(typeAnnotations.begin(), typeAnnotations.end(), [&](auto &&first, auto &&second) {
        return first.typeId < second.typeId;
    });

    auto range = s->selectTypeAnnotationsForSourceIdsStatement.range<TypeAnnotationView>(
        toIntegers(updatedTypeAnnotationSourceIds));

    auto insert = [&](const TypeAnnotation &annotation) {
        if (!annotation.sourceId)
            throw TypeAnnotationHasInvalidSourceId{};


        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"insert type annotations"_t,
                                   projectStorageCategory(),
                                   keyValue("type annotation", annotation)};

        s->insertTypeAnnotationStatement.write(annotation.typeId,
                                               annotation.sourceId,
                                               annotation.directorySourceId,
                                               annotation.typeName,
                                               annotation.iconPath,
                                               createEmptyAsNull(annotation.itemLibraryJson),
                                               createEmptyAsNull(annotation.hintsJson));

        synchronizeTypeTraits(annotation.typeId, annotation.traits);
    };

    auto update = [&](const TypeAnnotationView &annotationFromDatabase,
                      const TypeAnnotation &annotation) {

        if (annotationFromDatabase.typeName != annotation.typeName
            || annotationFromDatabase.iconPath != annotation.iconPath
            || annotationFromDatabase.itemLibraryJson != annotation.itemLibraryJson
            || annotationFromDatabase.hintsJson != annotation.hintsJson) {
            using NanotraceHR::keyValue;
            NanotraceHR::Tracer tracer{"update type annotations"_t,
                                       projectStorageCategory(),
                                       keyValue("type annotation from database",
                                                annotationFromDatabase),
                                       keyValue("type annotation", annotation)};

            s->updateTypeAnnotationStatement.write(annotation.typeId,
                                                   annotation.typeName,
                                                   annotation.iconPath,
                                                   createEmptyAsNull(annotation.itemLibraryJson),
                                                   createEmptyAsNull(annotation.hintsJson));

            synchronizeTypeTraits(annotation.typeId, annotation.traits);

            return Sqlite::UpdateChange::Update;
        }

        synchronizeTypeTraits(annotation.typeId, annotation.traits);

        return Sqlite::UpdateChange::No;
    };

    auto remove = [&](const TypeAnnotationView &annotationFromDatabase) {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"remove type annotations"_t,
                                   projectStorageCategory(),
                                   keyValue("type annotation", annotationFromDatabase)};

        auto prototypeAnnotationTraits = s->selectPrototypeAnnotationTraitsByTypeIdStatement
                                             .value<long long>(annotationFromDatabase.typeId);
        s->deleteTypeAnnotationStatement.write(annotationFromDatabase.typeId);

        s->updateTypeAnnotationTraitStatement.write(annotationFromDatabase.typeId,
                                                    prototypeAnnotationTraits);
    };

    Sqlite::insertUpdateDelete(range, typeAnnotations, compareKey, insert, update, remove);
}

void ProjectStorage::synchronizeTypeTrait(const Storage::Synchronization::Type &type)
{
    if (type.changeLevel == Storage::Synchronization::ChangeLevel::Minimal)
        return;

    s->updateTypeTraitStatement.write(type.typeId, type.traits.type);
}

void ProjectStorage::synchronizeTypes(Storage::Synchronization::Types &types,
                                      TypeIds &updatedTypeIds,
                                      AliasPropertyDeclarations &aliasPropertyDeclarationsToLink,
                                      AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                                      PropertyDeclarations &relinkablePropertyDeclarations,
                                      Prototypes &relinkablePrototypes,
                                      Prototypes &relinkableExtensions,
                                      ExportedTypesChanged &exportedTypesChanged,
                                      const SourceIds &updatedSourceIds)
{
    NanotraceHR::Tracer tracer{"synchronize types"_t, projectStorageCategory()};

    Storage::Synchronization::ExportedTypes exportedTypes;
    exportedTypes.reserve(types.size() * 3);
    SourceIds sourceIdsOfTypes;
    sourceIdsOfTypes.reserve(updatedSourceIds.size());
    SourceIds notUpdatedExportedSourceIds;
    notUpdatedExportedSourceIds.reserve(updatedSourceIds.size());
    SourceIds exportedSourceIds;
    exportedSourceIds.reserve(types.size());

    for (auto &type : types) {
        if (!type.sourceId)
            throw TypeHasInvalidSourceId{};

        TypeId typeId = declareType(type);
        synchronizeTypeTrait(type);
        sourceIdsOfTypes.push_back(type.sourceId);
        updatedTypeIds.push_back(typeId);
        if (type.changeLevel != Storage::Synchronization::ChangeLevel::ExcludeExportedTypes) {
            exportedSourceIds.push_back(type.sourceId);
            extractExportedTypes(typeId, type, exportedTypes);
        }
    }

    std::sort(types.begin(), types.end(), [](const auto &first, const auto &second) {
        return first.typeId < second.typeId;
    });

    unique(exportedSourceIds);

    SourceIds sourceIdsWithoutType = filterSourceIdsWithoutType(updatedSourceIds, sourceIdsOfTypes);
    exportedSourceIds.insert(exportedSourceIds.end(),
                             sourceIdsWithoutType.begin(),
                             sourceIdsWithoutType.end());
    TypeIds exportedTypeIds = fetchTypeIds(exportedSourceIds);
    synchronizeExportedTypes(exportedTypeIds,
                             exportedTypes,
                             relinkableAliasPropertyDeclarations,
                             relinkablePropertyDeclarations,
                             relinkablePrototypes,
                             relinkableExtensions,
                             exportedTypesChanged);

    syncPrototypesAndExtensions(types, relinkablePrototypes, relinkableExtensions);
    resetDefaultPropertiesIfChanged(types);
    resetRemovedAliasPropertyDeclarationsToNull(types, relinkableAliasPropertyDeclarations);
    syncDeclarations(types, aliasPropertyDeclarationsToLink, relinkablePropertyDeclarations);
    syncDefaultProperties(types);
}

void ProjectStorage::synchronizeDirectoryInfos(Storage::Synchronization::DirectoryInfos &directoryInfos,
                                             const SourceIds &updatedDirectoryInfoSourceIds)
{
    NanotraceHR::Tracer tracer{"synchronize directory infos"_t, projectStorageCategory()};

    auto compareKey = [](auto &&first, auto &&second) {
        auto directorySourceIdDifference = first.directorySourceId - second.directorySourceId;
        if (directorySourceIdDifference != 0)
            return directorySourceIdDifference;

        return first.sourceId - second.sourceId;
    };

    std::sort(directoryInfos.begin(), directoryInfos.end(), [&](auto &&first, auto &&second) {
        return std::tie(first.directorySourceId, first.sourceId)
               < std::tie(second.directorySourceId, second.sourceId);
    });

    auto range = s->selectDirectoryInfosForSourceIdsStatement.range<Storage::Synchronization::DirectoryInfo>(
        toIntegers(updatedDirectoryInfoSourceIds));

    auto insert = [&](const Storage::Synchronization::DirectoryInfo &directoryInfo) {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"insert directory info"_t,
                                   projectStorageCategory(),
                                   keyValue("directory info", directoryInfo)};

        if (!directoryInfo.directorySourceId)
            throw DirectoryInfoHasInvalidProjectSourceId{};
        if (!directoryInfo.sourceId)
            throw DirectoryInfoHasInvalidSourceId{};

        s->insertDirectoryInfoStatement.write(directoryInfo.directorySourceId,
                                            directoryInfo.sourceId,
                                            directoryInfo.moduleId,
                                            directoryInfo.fileType);
    };

    auto update = [&](const Storage::Synchronization::DirectoryInfo &directoryInfoFromDatabase,
                      const Storage::Synchronization::DirectoryInfo &directoryInfo) {
        if (directoryInfoFromDatabase.fileType != directoryInfo.fileType
            || !compareInvalidAreTrue(directoryInfoFromDatabase.moduleId, directoryInfo.moduleId)) {
            using NanotraceHR::keyValue;
            NanotraceHR::Tracer tracer{"update directory info"_t,
                                       projectStorageCategory(),
                                       keyValue("directory info", directoryInfo),
                                       keyValue("directory info from database",
                                                directoryInfoFromDatabase)};

            s->updateDirectoryInfoStatement.write(directoryInfo.directorySourceId,
                                                directoryInfo.sourceId,
                                                directoryInfo.moduleId,
                                                directoryInfo.fileType);
            return Sqlite::UpdateChange::Update;
        }

        return Sqlite::UpdateChange::No;
    };

    auto remove = [&](const Storage::Synchronization::DirectoryInfo &directoryInfo) {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"remove directory info"_t,
                                   projectStorageCategory(),
                                   keyValue("directory info", directoryInfo)};

        s->deleteDirectoryInfoStatement.write(directoryInfo.directorySourceId, directoryInfo.sourceId);
    };

    Sqlite::insertUpdateDelete(range, directoryInfos, compareKey, insert, update, remove);
}

void ProjectStorage::synchronizeFileStatuses(FileStatuses &fileStatuses,
                                             const SourceIds &updatedSourceIds)
{
    NanotraceHR::Tracer tracer{"synchronize file statuses"_t, projectStorageCategory()};

    auto compareKey = [](auto &&first, auto &&second) { return first.sourceId - second.sourceId; };

    std::sort(fileStatuses.begin(), fileStatuses.end(), [&](auto &&first, auto &&second) {
        return first.sourceId < second.sourceId;
    });

    auto range = s->selectFileStatusesForSourceIdsStatement.range<FileStatus>(
        toIntegers(updatedSourceIds));

    auto insert = [&](const FileStatus &fileStatus) {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"insert file status"_t,
                                   projectStorageCategory(),
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
            using NanotraceHR::keyValue;
            NanotraceHR::Tracer tracer{"update file status"_t,
                                       projectStorageCategory(),
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
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"remove file status"_t,
                                   projectStorageCategory(),
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
                                        Prototypes &relinkablePrototypes,
                                        Prototypes &relinkableExtensions)
{
    NanotraceHR::Tracer tracer{"synchronize imports"_t, projectStorageCategory()};

    synchromizeModuleExportedImports(moduleExportedImports, updatedModuleIds);
    NanotraceHR::Tracer importTracer{"synchronize qml document imports"_t, projectStorageCategory()};
    synchronizeDocumentImports(imports,
                               updatedSourceIds,
                               Storage::Synchronization::ImportKind::Import,
                               Relink::No,
                               relinkablePrototypes,
                               relinkableExtensions);
    importTracer.end();
    NanotraceHR::Tracer moduleDependenciesTracer{"synchronize module depdencies"_t,
                                                 projectStorageCategory()};
    synchronizeDocumentImports(moduleDependencies,
                               updatedModuleDependencySourceIds,
                               Storage::Synchronization::ImportKind::ModuleDependency,
                               Relink::Yes,
                               relinkablePrototypes,
                               relinkableExtensions);
    moduleDependenciesTracer.end();
}

void ProjectStorage::synchromizeModuleExportedImports(
    Storage::Synchronization::ModuleExportedImports &moduleExportedImports,
    const ModuleIds &updatedModuleIds)
{
    NanotraceHR::Tracer tracer{"synchronize module exported imports"_t, projectStorageCategory()};
    std::sort(moduleExportedImports.begin(),
              moduleExportedImports.end(),
              [](auto &&first, auto &&second) {
                  return std::tie(first.moduleId, first.exportedModuleId)
                         < std::tie(second.moduleId, second.exportedModuleId);
              });

    auto range = s->selectModuleExportedImportsForSourceIdStatement
                     .range<Storage::Synchronization::ModuleExportedImportView>(
                         toIntegers(updatedModuleIds));

    auto compareKey = [](const Storage::Synchronization::ModuleExportedImportView &view,
                         const Storage::Synchronization::ModuleExportedImport &import) -> long long {
        auto moduleIdDifference = view.moduleId - import.moduleId;
        if (moduleIdDifference != 0)
            return moduleIdDifference;

        return view.exportedModuleId - import.exportedModuleId;
    };

    auto insert = [&](const Storage::Synchronization::ModuleExportedImport &import) {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"insert module exported import"_t,
                                   projectStorageCategory(),
                                   keyValue("module exported import", import),
                                   keyValue("module id", import.moduleId)};
        tracer.tick("exported module"_t, keyValue("module id", import.exportedModuleId));

        if (import.version.minor) {
            s->insertModuleExportedImportWithVersionStatement.write(import.moduleId,
                                                                    import.exportedModuleId,
                                                                    import.isAutoVersion,
                                                                    import.version.major.value,
                                                                    import.version.minor.value);
        } else if (import.version.major) {
            s->insertModuleExportedImportWithMajorVersionStatement.write(import.moduleId,
                                                                         import.exportedModuleId,
                                                                         import.isAutoVersion,
                                                                         import.version.major.value);
        } else {
            s->insertModuleExportedImportWithoutVersionStatement.write(import.moduleId,
                                                                       import.exportedModuleId,
                                                                       import.isAutoVersion);
        }
    };

    auto update = [](const Storage::Synchronization::ModuleExportedImportView &,
                     const Storage::Synchronization::ModuleExportedImport &) {
        return Sqlite::UpdateChange::No;
    };

    auto remove = [&](const Storage::Synchronization::ModuleExportedImportView &view) {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"remove module exported import"_t,
                                   projectStorageCategory(),
                                   keyValue("module exported import view", view),
                                   keyValue("module id", view.moduleId)};
        tracer.tick("exported module"_t, keyValue("module id", view.exportedModuleId));

        s->deleteModuleExportedImportStatement.write(view.moduleExportedImportId);
    };

    Sqlite::insertUpdateDelete(range, moduleExportedImports, compareKey, insert, update, remove);
}

ModuleId ProjectStorage::fetchModuleIdUnguarded(Utils::SmallStringView name,
                                                Storage::ModuleKind kind) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch module id ungarded"_t,
                               projectStorageCategory(),
                               keyValue("module name", name),
                               keyValue("module kind", kind)};

    auto moduleId = s->selectModuleIdByNameStatement.value<ModuleId>(kind, name);

    if (!moduleId)
        moduleId = s->insertModuleNameStatement.value<ModuleId>(kind, name);

    tracer.end(keyValue("module id", moduleId));

    return moduleId;
}

Storage::Module ProjectStorage::fetchModuleUnguarded(ModuleId id) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch module ungarded"_t,
                               projectStorageCategory(),
                               keyValue("module id", id)};

    auto module = s->selectModuleStatement.value<Storage::Module>(id);

    if (!module)
        throw ModuleDoesNotExists{};

    tracer.end(keyValue("module name", module.name));
    tracer.end(keyValue("module name", module.kind));

    return module;
}

void ProjectStorage::handleAliasPropertyDeclarationsWithPropertyType(
    TypeId typeId, AliasPropertyDeclarations &relinkableAliasPropertyDeclarations)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"handle alias property declarations with property type"_t,
                               projectStorageCategory(),
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
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"handle property declarations with property type"_t,
                               projectStorageCategory(),
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
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"handle property declarations with exported type name and type id"_t,
                               projectStorageCategory(),
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
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"handle alias property declarations with exported type name and type id"_t,
                               projectStorageCategory(),
                               keyValue("type name", exportedTypeName),
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
    };

    s->selectAliasPropertyDeclarationForPrototypeIdAndTypeNameStatement.readCallback(callback,
                                                                                     exportedTypeName,
                                                                                     typeId);
}

void ProjectStorage::handlePrototypes(TypeId prototypeId, Prototypes &relinkablePrototypes)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"handle prototypes"_t,
                               projectStorageCategory(),
                               keyValue("type id", prototypeId),
                               keyValue("relinkable prototypes", relinkablePrototypes)};

    auto callback = [&](TypeId typeId, ImportedTypeNameId prototypeNameId) {
        if (prototypeNameId)
            relinkablePrototypes.emplace_back(typeId, prototypeNameId);
    };

    s->updatePrototypeIdToTypeIdStatement.readCallback(callback, prototypeId, unresolvedTypeId);
}

void ProjectStorage::handlePrototypesWithExportedTypeNameAndTypeId(
    Utils::SmallStringView exportedTypeName, TypeId typeId, Prototypes &relinkablePrototypes)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"handle invalid prototypes"_t,
                               projectStorageCategory(),
                               keyValue("type id", exportedTypeName),
                               keyValue("relinkable prototypes", relinkablePrototypes)};

    auto callback = [&](TypeId typeId, ImportedTypeNameId prototypeNameId) {
        relinkablePrototypes.emplace_back(typeId, prototypeNameId);
    };

    s->selectTypeIdAndPrototypeNameIdForPrototypeIdAndTypeNameStatement.readCallback(callback,
                                                                                     exportedTypeName,
                                                                                     typeId);
}

void ProjectStorage::handleExtensions(TypeId extensionId, Prototypes &relinkableExtensions)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"handle extension"_t,
                               projectStorageCategory(),
                               keyValue("type id", extensionId),
                               keyValue("relinkable extensions", relinkableExtensions)};

    auto callback = [&](TypeId typeId, ImportedTypeNameId extensionNameId) {
        if (extensionNameId)
            relinkableExtensions.emplace_back(typeId, extensionNameId);
    };

    s->updateExtensionIdToTypeIdStatement.readCallback(callback, extensionId, unresolvedTypeId);
}

void ProjectStorage::handleExtensionsWithExportedTypeNameAndTypeId(
    Utils::SmallStringView exportedTypeName, TypeId typeId, Prototypes &relinkableExtensions)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"handle invalid extensions"_t,
                               projectStorageCategory(),
                               keyValue("type id", exportedTypeName),
                               keyValue("relinkable extensions", relinkableExtensions)};

    auto callback = [&](TypeId typeId, ImportedTypeNameId extensionNameId) {
        relinkableExtensions.emplace_back(typeId, extensionNameId);
    };

    s->selectTypeIdForExtensionIdAndTypeNameStatement.readCallback(callback, exportedTypeName, typeId);
}

void ProjectStorage::deleteType(TypeId typeId,
                                AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                                PropertyDeclarations &relinkablePropertyDeclarations,
                                Prototypes &relinkablePrototypes,
                                Prototypes &relinkableExtensions)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"delete type"_t, projectStorageCategory(), keyValue("type id", typeId)};

    handlePropertyDeclarationWithPropertyType(typeId, relinkablePropertyDeclarations);
    handleAliasPropertyDeclarationsWithPropertyType(typeId, relinkableAliasPropertyDeclarations);
    handlePrototypes(typeId, relinkablePrototypes);
    handleExtensions(typeId, relinkableExtensions);
    s->deleteTypeNamesByTypeIdStatement.write(typeId);
    s->deleteEnumerationDeclarationByTypeIdStatement.write(typeId);
    s->deletePropertyDeclarationByTypeIdStatement.write(typeId);
    s->deleteFunctionDeclarationByTypeIdStatement.write(typeId);
    s->deleteSignalDeclarationByTypeIdStatement.write(typeId);
    s->deleteTypeStatement.write(typeId);
}

void ProjectStorage::relinkAliasPropertyDeclarations(AliasPropertyDeclarations &aliasPropertyDeclarations,
                                                     const TypeIds &deletedTypeIds)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"relink alias properties"_t,
                               projectStorageCategory(),
                               keyValue("alias property declarations", aliasPropertyDeclarations),
                               keyValue("deleted type ids", deletedTypeIds)};

    std::sort(aliasPropertyDeclarations.begin(), aliasPropertyDeclarations.end());
    // todo remove duplicates

    Utils::set_greedy_difference(
        aliasPropertyDeclarations.cbegin(),
        aliasPropertyDeclarations.cend(),
        deletedTypeIds.begin(),
        deletedTypeIds.end(),
        [&](const AliasPropertyDeclaration &alias) {
            auto typeId = fetchTypeId(alias.aliasImportedTypeNameId);

            if (typeId) {
                auto propertyDeclaration = fetchPropertyDeclarationByTypeIdAndNameUngarded(
                    typeId, alias.aliasPropertyName);
                if (propertyDeclaration) {
                    auto [propertyImportedTypeNameId, propertyTypeId, aliasId, propertyTraits] = *propertyDeclaration;

                    s->updatePropertyDeclarationWithAliasAndTypeStatement
                        .write(alias.propertyDeclarationId,
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
        },
        TypeCompare<AliasPropertyDeclaration>{});
}

void ProjectStorage::relinkPropertyDeclarations(PropertyDeclarations &relinkablePropertyDeclaration,
                                                const TypeIds &deletedTypeIds)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"relink property declarations"_t,
                               projectStorageCategory(),
                               keyValue("relinkable property declarations",
                                        relinkablePropertyDeclaration),
                               keyValue("deleted type ids", deletedTypeIds)};

    std::sort(relinkablePropertyDeclaration.begin(), relinkablePropertyDeclaration.end());
    relinkablePropertyDeclaration.erase(std::unique(relinkablePropertyDeclaration.begin(),
                                                    relinkablePropertyDeclaration.end()),
                                        relinkablePropertyDeclaration.end());

    Utils::set_greedy_difference(
        relinkablePropertyDeclaration.cbegin(),
        relinkablePropertyDeclaration.cend(),
        deletedTypeIds.begin(),
        deletedTypeIds.end(),
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
        TypeCompare<PropertyDeclaration>{});
}

template<typename Callable>
void ProjectStorage::relinkPrototypes(Prototypes &relinkablePrototypes,
                                      const TypeIds &deletedTypeIds,
                                      Callable updateStatement)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"relink prototypes"_t,
                               projectStorageCategory(),
                               keyValue("relinkable prototypes", relinkablePrototypes),
                               keyValue("deleted type ids", deletedTypeIds)};

    std::sort(relinkablePrototypes.begin(), relinkablePrototypes.end());
    relinkablePrototypes.erase(std::unique(relinkablePrototypes.begin(), relinkablePrototypes.end()),
                               relinkablePrototypes.end());

    Utils::set_greedy_difference(
        relinkablePrototypes.cbegin(),
        relinkablePrototypes.cend(),
        deletedTypeIds.begin(),
        deletedTypeIds.end(),
        [&](const Prototype &prototype) {
            TypeId prototypeId = fetchTypeId(prototype.prototypeNameId);

            if (!prototypeId)
                errorNotifier->typeNameCannotBeResolved(fetchImportedTypeName(prototype.prototypeNameId),
                                                        fetchTypeSourceId(prototype.typeId));

            updateStatement(prototype.typeId, prototypeId);
            checkForPrototypeChainCycle(prototype.typeId);
        },
        TypeCompare<Prototype>{});
}

void ProjectStorage::deleteNotUpdatedTypes(const TypeIds &updatedTypeIds,
                                           const SourceIds &updatedSourceIds,
                                           const TypeIds &typeIdsToBeDeleted,
                                           AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                                           PropertyDeclarations &relinkablePropertyDeclarations,
                                           Prototypes &relinkablePrototypes,
                                           Prototypes &relinkableExtensions,
                                           TypeIds &deletedTypeIds)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"delete not updated types"_t,
                               projectStorageCategory(),
                               keyValue("updated type ids", updatedTypeIds),
                               keyValue("updated source ids", updatedSourceIds),
                               keyValue("type ids to be deleted", typeIdsToBeDeleted)};

    auto callback = [&](TypeId typeId) {
        deletedTypeIds.push_back(typeId);
        deleteType(typeId,
                   relinkableAliasPropertyDeclarations,
                   relinkablePropertyDeclarations,
                   relinkablePrototypes,
                   relinkableExtensions);
    };

    s->selectNotUpdatedTypesInSourcesStatement.readCallback(callback,
                                                            toIntegers(updatedSourceIds),
                                                            toIntegers(updatedTypeIds));
    for (TypeId typeIdToBeDeleted : typeIdsToBeDeleted)
        callback(typeIdToBeDeleted);
}

void ProjectStorage::relink(AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                            PropertyDeclarations &relinkablePropertyDeclarations,
                            Prototypes &relinkablePrototypes,
                            Prototypes &relinkableExtensions,
                            TypeIds &deletedTypeIds)
{
    NanotraceHR::Tracer tracer{"relink"_t, projectStorageCategory()};

    std::sort(deletedTypeIds.begin(), deletedTypeIds.end());

    relinkPrototypes(relinkablePrototypes, deletedTypeIds, [&](TypeId typeId, TypeId prototypeId) {
        s->updateTypePrototypeStatement.write(typeId, prototypeId);
    });
    relinkPrototypes(relinkableExtensions, deletedTypeIds, [&](TypeId typeId, TypeId prototypeId) {
        s->updateTypeExtensionStatement.write(typeId, prototypeId);
    });
    relinkPropertyDeclarations(relinkablePropertyDeclarations, deletedTypeIds);
    relinkAliasPropertyDeclarations(relinkableAliasPropertyDeclarations, deletedTypeIds);
}

PropertyDeclarationId ProjectStorage::fetchAliasId(TypeId aliasTypeId,
                                                   Utils::SmallStringView aliasPropertyName,
                                                   Utils::SmallStringView aliasPropertyNameTail)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch alias id"_t,
                               projectStorageCategory(),
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
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"link alias property declarations alias ids"_t,
                               projectStorageCategory(),
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

                errorNotifier->propertyNameDoesNotExists(aliasDeclaration.composedProperyName(),
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
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"update alias property declarations"_t,
                               projectStorageCategory(),
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
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"check alias property declarations cycles"_t,
                               projectStorageCategory(),
                               keyValue("alias property declarations", aliasDeclarations)};
    for (const auto &aliasDeclaration : aliasDeclarations)
        checkForAliasChainCycle(aliasDeclaration.propertyDeclarationId);
}

void ProjectStorage::linkAliases(const AliasPropertyDeclarations &aliasPropertyDeclarationsToLink,
                                 RaiseError raiseError)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"link aliases"_t, projectStorageCategory()};

    linkAliasPropertyDeclarationAliasIds(aliasPropertyDeclarationsToLink, raiseError);

    checkAliasPropertyDeclarationCycles(aliasPropertyDeclarationsToLink);

    updateAliasPropertyDeclarationValues(aliasPropertyDeclarationsToLink);
}

void ProjectStorage::repairBrokenAliasPropertyDeclarations()
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"repair broken alias property declarations"_t,
                               projectStorageCategory()};

    auto brokenAliasPropertyDeclarations = s->selectBrokenAliasPropertyDeclarationsStatement
                                               .values<AliasPropertyDeclaration>();

    linkAliases(brokenAliasPropertyDeclarations, RaiseError::No);
}

void ProjectStorage::synchronizeExportedTypes(const TypeIds &updatedTypeIds,
                                              Storage::Synchronization::ExportedTypes &exportedTypes,
                                              AliasPropertyDeclarations &relinkableAliasPropertyDeclarations,
                                              PropertyDeclarations &relinkablePropertyDeclarations,
                                              Prototypes &relinkablePrototypes,
                                              Prototypes &relinkableExtensions,
                                              ExportedTypesChanged &exportedTypesChanged)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"synchronize exported types"_t, projectStorageCategory()};

    std::sort(exportedTypes.begin(), exportedTypes.end(), [](auto &&first, auto &&second) {
        if (first.moduleId < second.moduleId)
            return true;
        else if (first.moduleId > second.moduleId)
            return false;

        auto nameCompare = Sqlite::compare(first.name, second.name);

        if (nameCompare < 0)
            return true;
        else if (nameCompare > 0)
            return false;

        return first.version < second.version;
    });

    auto range = s->selectExportedTypesForSourceIdsStatement
                     .range<Storage::Synchronization::ExportedTypeView>(toIntegers(updatedTypeIds));

    auto compareKey = [](const Storage::Synchronization::ExportedTypeView &view,
                         const Storage::Synchronization::ExportedType &type) -> long long {
        auto moduleIdDifference = view.moduleId - type.moduleId;
        if (moduleIdDifference != 0)
            return moduleIdDifference;

        auto nameDifference = Sqlite::compare(view.name, type.name);
        if (nameDifference != 0)
            return nameDifference;

        auto versionDifference = view.version.major.value - type.version.major.value;
        if (versionDifference != 0)
            return versionDifference;

        return view.version.minor.value - type.version.minor.value;
    };

    auto insert = [&](const Storage::Synchronization::ExportedType &type) {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"insert exported type"_t,
                                   projectStorageCategory(),
                                   keyValue("exported type", type),
                                   keyValue("type id", type.typeId),
                                   keyValue("module id", type.moduleId)};
        if (!type.moduleId)
            throw QmlDesigner::ModuleDoesNotExists{};

        try {
            if (type.version) {
                s->insertExportedTypeNamesWithVersionStatement.write(type.moduleId,
                                                                     type.name,
                                                                     type.version.major.value,
                                                                     type.version.minor.value,
                                                                     type.typeId);

            } else if (type.version.major) {
                s->insertExportedTypeNamesWithMajorVersionStatement.write(type.moduleId,
                                                                          type.name,
                                                                          type.version.major.value,
                                                                          type.typeId);
            } else {
                s->insertExportedTypeNamesWithoutVersionStatement.write(type.moduleId,
                                                                        type.name,
                                                                        type.typeId);
            }
        } catch (const Sqlite::ConstraintPreventsModification &) {
            throw QmlDesigner::ExportedTypeCannotBeInserted{type.name};
        }

        handlePropertyDeclarationsWithExportedTypeNameAndTypeId(type.name,
                                                                TypeId{},
                                                                relinkablePropertyDeclarations);
        handleAliasPropertyDeclarationsWithExportedTypeNameAndTypeId(type.name,
                                                                     TypeId{},
                                                                     relinkableAliasPropertyDeclarations);
        handlePrototypesWithExportedTypeNameAndTypeId(type.name, unresolvedTypeId, relinkablePrototypes);
        handleExtensionsWithExportedTypeNameAndTypeId(type.name, unresolvedTypeId, relinkableExtensions);

        exportedTypesChanged = ExportedTypesChanged::Yes;
    };

    auto update = [&](const Storage::Synchronization::ExportedTypeView &view,
                      const Storage::Synchronization::ExportedType &type) {
        if (view.typeId != type.typeId) {
            NanotraceHR::Tracer tracer{"update exported type"_t,
                                       projectStorageCategory(),
                                       keyValue("exported type", type),
                                       keyValue("exported type view", view),
                                       keyValue("type id", type.typeId),
                                       keyValue("module id", type.typeId)};

            handlePropertyDeclarationWithPropertyType(view.typeId, relinkablePropertyDeclarations);
            handleAliasPropertyDeclarationsWithPropertyType(view.typeId,
                                                            relinkableAliasPropertyDeclarations);
            handlePrototypes(view.typeId, relinkablePrototypes);
            handleExtensions(view.typeId, relinkableExtensions);
            s->updateExportedTypeNameTypeIdStatement.write(view.exportedTypeNameId, type.typeId);
            exportedTypesChanged = ExportedTypesChanged::Yes;

            return Sqlite::UpdateChange::Update;
        }
        return Sqlite::UpdateChange::No;
    };

    auto remove = [&](const Storage::Synchronization::ExportedTypeView &view) {
        NanotraceHR::Tracer tracer{"remove exported type"_t,
                                   projectStorageCategory(),
                                   keyValue("exported type", view),
                                   keyValue("type id", view.typeId),
                                   keyValue("module id", view.moduleId)};

        handlePropertyDeclarationWithPropertyType(view.typeId, relinkablePropertyDeclarations);
        handleAliasPropertyDeclarationsWithPropertyType(view.typeId,
                                                        relinkableAliasPropertyDeclarations);
        handlePrototypes(view.typeId, relinkablePrototypes);
        handleExtensions(view.typeId, relinkableExtensions);

        s->deleteExportedTypeNameStatement.write(view.exportedTypeNameId);

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
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"insert property declaration to alias"_t,
                               projectStorageCategory(),
                               keyValue("property declaration", value)};

    auto propertyImportedTypeNameId = fetchImportedTypeNameId(value.typeName, sourceId);

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
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"insert property declaration"_t,
                               projectStorageCategory(),
                               keyValue("property declaration", value)};

    auto propertyImportedTypeNameId = fetchImportedTypeNameId(value.typeName, sourceId);
    auto propertyTypeId = fetchTypeId(propertyImportedTypeNameId);

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
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"update property declaration to alias"_t,
                               projectStorageCategory(),
                               keyValue("property declaration", value),
                               keyValue("property declaration view", view)};

    aliasPropertyDeclarationsToLink.emplace_back(view.propertyTypeId,
                                                 view.id,
                                                 fetchImportedTypeNameId(value.typeName, sourceId),
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
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"update property declaration"_t,
                               projectStorageCategory(),
                               keyValue("property declaration", value),
                               keyValue("property declaration view", view)};

    auto propertyImportedTypeNameId = fetchImportedTypeNameId(value.typeName, sourceId);

    auto propertyTypeId = fetchTypeId(propertyImportedTypeNameId);

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
    NanotraceHR::Tracer tracer{"synchronize property declaration"_t, projectStorageCategory()};

    std::sort(propertyDeclarations.begin(), propertyDeclarations.end(), [](auto &&first, auto &&second) {
        return Sqlite::compare(first.name, second.name) < 0;
    });

    auto range = s->selectPropertyDeclarationsForTypeIdStatement
                     .range<Storage::Synchronization::PropertyDeclarationView>(typeId);

    auto compareKey = [](const Storage::Synchronization::PropertyDeclarationView &view,
                         const Storage::Synchronization::PropertyDeclaration &value) {
        return Sqlite::compare(view.name, value.name);
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
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"remove property declaration"_t,
                                   projectStorageCategory(),
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
    NanotraceHR::Tracer tracer{"reset removed alias property declaration to null"_t,
                               projectStorageCategory()};

    if (type.changeLevel == Storage::Synchronization::ChangeLevel::Minimal)
        return;

    Storage::Synchronization::PropertyDeclarations &aliasDeclarations = type.propertyDeclarations;

    std::sort(aliasDeclarations.begin(), aliasDeclarations.end(), [](auto &&first, auto &&second) {
        return Sqlite::compare(first.name, second.name) < 0;
    });

    auto range = s->selectPropertyDeclarationsWithAliasForTypeIdStatement
                     .range<AliasPropertyDeclarationView>(type.typeId);

    auto compareKey = [](const AliasPropertyDeclarationView &view,
                         const Storage::Synchronization::PropertyDeclaration &value) {
        return Sqlite::compare(view.name, value.name);
    };

    auto insert = [&](const Storage::Synchronization::PropertyDeclaration &) {};

    auto update = [&](const AliasPropertyDeclarationView &,
                      const Storage::Synchronization::PropertyDeclaration &) {
        return Sqlite::UpdateChange::No;
    };

    auto remove = [&](const AliasPropertyDeclarationView &view) {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"reset removed alias property declaration to null"_t,
                                   projectStorageCategory(),
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
    NanotraceHR::Tracer tracer{"reset removed alias properties to null"_t, projectStorageCategory()};

    PropertyDeclarationIds propertyDeclarationIds;
    propertyDeclarationIds.reserve(types.size());

    for (auto &&type : types)
        resetRemovedAliasPropertyDeclarationsToNull(type, propertyDeclarationIds);

    removeRelinkableEntries(relinkableAliasPropertyDeclarations,
                            propertyDeclarationIds,
                            PropertyCompare<AliasPropertyDeclaration>{});
}

void ProjectStorage::handlePrototypesWithSourceIdAndPrototypeId(SourceId sourceId,
                                                                TypeId prototypeId,
                                                                Prototypes &relinkablePrototypes)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"handle prototypes with source id and prototype id"_t,
                               projectStorageCategory(),
                               keyValue("source id", sourceId),
                               keyValue("type id", prototypeId)};

    auto callback = [&](TypeId typeId, ImportedTypeNameId prototypeNameId) {
        if (prototypeNameId)
            relinkablePrototypes.emplace_back(typeId, prototypeNameId);
    };

    s->selectTypeIdAndPrototypeNameIdForPrototypeIdAndSourceIdStatement.readCallback(callback,
                                                                                     prototypeId,
                                                                                     sourceId);
}

void ProjectStorage::handlePrototypesAndExtensionsWithSourceId(SourceId sourceId,
                                                               TypeId prototypeId,
                                                               TypeId extensionId,
                                                               Prototypes &relinkablePrototypes,
                                                               Prototypes &relinkableExtensions)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"handle prototypes with source id"_t,
                               projectStorageCategory(),
                               keyValue("source id", sourceId),
                               keyValue("prototype id", prototypeId),
                               keyValue("extension id", extensionId)};

    auto callback =
        [&](TypeId typeId, ImportedTypeNameId prototypeNameId, ImportedTypeNameId extensionNameId) {
            if (prototypeNameId)
                relinkablePrototypes.emplace_back(typeId, prototypeNameId);
            if (extensionNameId)
                relinkableExtensions.emplace_back(typeId, extensionNameId);
        };

    s->updatePrototypeIdAndExtensionIdToTypeIdForSourceIdStatement.readCallback(callback,
                                                                                sourceId,
                                                                                prototypeId,
                                                                                extensionId);
}

void ProjectStorage::handleExtensionsWithSourceIdAndExtensionId(SourceId sourceId,
                                                                TypeId extensionId,
                                                                Prototypes &relinkableExtensions)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"handle prototypes with source id and prototype id"_t,
                               projectStorageCategory(),
                               keyValue("source id", sourceId),
                               keyValue("type id", extensionId)};

    auto callback = [&](TypeId typeId, ImportedTypeNameId extensionNameId) {
        if (extensionNameId)
            relinkableExtensions.emplace_back(typeId, extensionNameId);
    };

    s->selectTypeIdAndExtensionNameIdForExtensionIdAndSourceIdStatement.readCallback(callback,
                                                                                     extensionId,
                                                                                     sourceId);
}

ImportId ProjectStorage::insertDocumentImport(const Storage::Import &import,
                                              Storage::Synchronization::ImportKind importKind,
                                              ModuleId sourceModuleId,
                                              ImportId parentImportId,
                                              Relink relink,
                                              Prototypes &relinkablePrototypes,
                                              Prototypes &relinkableExtensions)
{
    if (relink == Relink::Yes) {
        handlePrototypesWithSourceIdAndPrototypeId(import.sourceId,
                                                   unresolvedTypeId,
                                                   relinkablePrototypes);
        handleExtensionsWithSourceIdAndExtensionId(import.sourceId,
                                                   unresolvedTypeId,
                                                   relinkableExtensions);
    }

    if (import.version.minor) {
        return s->insertDocumentImportWithVersionStatement.value<ImportId>(import.sourceId,
                                                                           import.moduleId,
                                                                           sourceModuleId,
                                                                           importKind,
                                                                           import.version.major.value,
                                                                           import.version.minor.value,
                                                                           parentImportId);
    } else if (import.version.major) {
        return s->insertDocumentImportWithMajorVersionStatement.value<ImportId>(import.sourceId,
                                                                                import.moduleId,
                                                                                sourceModuleId,
                                                                                importKind,
                                                                                import.version.major.value,
                                                                                parentImportId);
    } else {
        return s->insertDocumentImportWithoutVersionStatement.value<ImportId>(import.sourceId,
                                                                              import.moduleId,
                                                                              sourceModuleId,
                                                                              importKind,
                                                                              parentImportId);
    }
}

void ProjectStorage::synchronizeDocumentImports(Storage::Imports &imports,
                                                const SourceIds &updatedSourceIds,
                                                Storage::Synchronization::ImportKind importKind,
                                                Relink relink,
                                                Prototypes &relinkablePrototypes,
                                                Prototypes &relinkableExtensions)
{
    std::sort(imports.begin(), imports.end(), [](auto &&first, auto &&second) {
        return std::tie(first.sourceId, first.moduleId, first.version)
               < std::tie(second.sourceId, second.moduleId, second.version);
    });

    auto range = s->selectDocumentImportForSourceIdStatement
                     .range<Storage::Synchronization::ImportView>(toIntegers(updatedSourceIds),
                                                                  importKind);

    auto compareKey = [](const Storage::Synchronization::ImportView &view,
                         const Storage::Import &import) -> long long {
        auto sourceIdDifference = view.sourceId - import.sourceId;
        if (sourceIdDifference != 0)
            return sourceIdDifference;

        auto moduleIdDifference = view.moduleId - import.moduleId;
        if (moduleIdDifference != 0)
            return moduleIdDifference;

        auto versionDifference = view.version.major.value - import.version.major.value;
        if (versionDifference != 0)
            return versionDifference;

        return view.version.minor.value - import.version.minor.value;
    };

    auto insert = [&](const Storage::Import &import) {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"insert import"_t,
                                   projectStorageCategory(),
                                   keyValue("import", import),
                                   keyValue("import kind", importKind),
                                   keyValue("source id", import.sourceId),
                                   keyValue("module id", import.moduleId)};

        auto importId = insertDocumentImport(import,
                                             importKind,
                                             import.moduleId,
                                             ImportId{},
                                             relink,
                                             relinkablePrototypes,
                                             relinkableExtensions);
        auto callback = [&](ModuleId exportedModuleId, int majorVersion, int minorVersion) {
            Storage::Import additionImport{exportedModuleId,
                                           Storage::Version{majorVersion, minorVersion},
                                           import.sourceId};

            auto exportedImportKind = importKind == Storage::Synchronization::ImportKind::Import
                                          ? Storage::Synchronization::ImportKind::ModuleExportedImport
                                          : Storage::Synchronization::ImportKind::ModuleExportedModuleDependency;

            NanotraceHR::Tracer tracer{"insert indirect import"_t,
                                       projectStorageCategory(),
                                       keyValue("import", import),
                                       keyValue("import kind", exportedImportKind),
                                       keyValue("source id", import.sourceId),
                                       keyValue("module id", import.moduleId)};

            auto indirectImportId = insertDocumentImport(additionImport,
                                                         exportedImportKind,
                                                         import.moduleId,
                                                         importId,
                                                         relink,
                                                         relinkablePrototypes,
                                                         relinkableExtensions);

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
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"remove import"_t,
                                   projectStorageCategory(),
                                   keyValue("import", view),
                                   keyValue("import id", view.importId),
                                   keyValue("source id", view.sourceId),
                                   keyValue("module id", view.moduleId)};

        s->deleteDocumentImportStatement.write(view.importId);
        s->deleteDocumentImportsWithParentImportIdStatement.write(view.sourceId, view.importId);
        if (relink == Relink::Yes) {
            handlePrototypesAndExtensionsWithSourceId(view.sourceId,
                                                      unresolvedTypeId,
                                                      unresolvedTypeId,
                                                      relinkablePrototypes,
                                                      relinkableExtensions);
        }
    };

    Sqlite::insertUpdateDelete(range, imports, compareKey, insert, update, remove);
}

Utils::PathString ProjectStorage::createJson(const Storage::Synchronization::ParameterDeclarations &parameters)
{
    NanotraceHR::Tracer tracer{"create json from parameter declarations"_t, projectStorageCategory()};

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
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch type id by module id and exported name"_t,
                               projectStorageCategory(),
                               keyValue("module id", moduleId),
                               keyValue("exported name", name)};

    return s->selectTypeIdByModuleIdAndExportedNameStatement.value<TypeId>(moduleId, name);
}

void ProjectStorage::addTypeIdToPropertyEditorQmlPaths(
    Storage::Synchronization::PropertyEditorQmlPaths &paths)
{
    NanotraceHR::Tracer tracer{"add type id to property editor qml paths"_t, projectStorageCategory()};

    for (auto &path : paths)
        path.typeId = fetchTypeIdByModuleIdAndExportedName(path.moduleId, path.typeName);
}

void ProjectStorage::synchronizePropertyEditorPaths(Storage::Synchronization::PropertyEditorQmlPaths &paths,
                                                    SourceIds updatedPropertyEditorQmlPathsSourceIds)
{
    using Storage::Synchronization::PropertyEditorQmlPath;
    std::sort(paths.begin(), paths.end(), [](auto &&first, auto &&second) {
        return first.typeId < second.typeId;
    });

    auto range = s->selectPropertyEditorPathsForForSourceIdsStatement.range<PropertyEditorQmlPathView>(
        toIntegers(updatedPropertyEditorQmlPathsSourceIds));

    auto compareKey = [](const PropertyEditorQmlPathView &view,
                         const PropertyEditorQmlPath &value) -> long long {
        return view.typeId - value.typeId;
    };

    auto insert = [&](const PropertyEditorQmlPath &path) {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"insert property editor paths"_t,
                                   projectStorageCategory(),
                                   keyValue("property editor qml path", path)};

        if (path.typeId)
            s->insertPropertyEditorPathStatement.write(path.typeId, path.pathId, path.directoryId);
    };

    auto update = [&](const PropertyEditorQmlPathView &view, const PropertyEditorQmlPath &value) {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"update property editor paths"_t,
                                   projectStorageCategory(),
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
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"remove property editor paths"_t,
                                   projectStorageCategory(),
                                   keyValue("property editor qml path view", view)};

        s->deletePropertyEditorPathStatement.write(view.typeId);
    };

    Sqlite::insertUpdateDelete(range, paths, compareKey, insert, update, remove);
}

void ProjectStorage::synchronizePropertyEditorQmlPaths(
    Storage::Synchronization::PropertyEditorQmlPaths &paths,
    SourceIds updatedPropertyEditorQmlPathsSourceIds)
{
    NanotraceHR::Tracer tracer{"synchronize property editor qml paths"_t, projectStorageCategory()};

    addTypeIdToPropertyEditorQmlPaths(paths);
    synchronizePropertyEditorPaths(paths, updatedPropertyEditorQmlPathsSourceIds);
}

void ProjectStorage::synchronizeFunctionDeclarations(
    TypeId typeId, Storage::Synchronization::FunctionDeclarations &functionsDeclarations)
{
    NanotraceHR::Tracer tracer{"synchronize function declaration"_t, projectStorageCategory()};

    std::sort(functionsDeclarations.begin(),
              functionsDeclarations.end(),
              [](auto &&first, auto &&second) {
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
        auto nameKey = Sqlite::compare(view.name, value.name);
        if (nameKey != 0)
            return nameKey;

        Utils::PathString valueSignature{createJson(value.parameters)};

        return Sqlite::compare(view.signature, valueSignature);
    };

    auto insert = [&](const Storage::Synchronization::FunctionDeclaration &value) {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"insert function declaration"_t,
                                   projectStorageCategory(),
                                   keyValue("function declaration", value)};

        Utils::PathString signature{createJson(value.parameters)};

        s->insertFunctionDeclarationStatement.write(typeId, value.name, value.returnTypeName, signature);
    };

    auto update = [&](const Storage::Synchronization::FunctionDeclarationView &view,
                      const Storage::Synchronization::FunctionDeclaration &value) {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"update function declaration"_t,
                                   projectStorageCategory(),
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
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"remove function declaration"_t,
                                   projectStorageCategory(),
                                   keyValue("function declaration view", view)};

        s->deleteFunctionDeclarationStatement.write(view.id);
    };

    Sqlite::insertUpdateDelete(range, functionsDeclarations, compareKey, insert, update, remove);
}

void ProjectStorage::synchronizeSignalDeclarations(
    TypeId typeId, Storage::Synchronization::SignalDeclarations &signalDeclarations)
{
    NanotraceHR::Tracer tracer{"synchronize signal declaration"_t, projectStorageCategory()};

    std::sort(signalDeclarations.begin(), signalDeclarations.end(), [](auto &&first, auto &&second) {
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
        auto nameKey = Sqlite::compare(view.name, value.name);
        if (nameKey != 0)
            return nameKey;

        Utils::PathString valueSignature{createJson(value.parameters)};

        return Sqlite::compare(view.signature, valueSignature);
    };

    auto insert = [&](const Storage::Synchronization::SignalDeclaration &value) {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"insert signal declaration"_t,
                                   projectStorageCategory(),
                                   keyValue("signal declaration", value)};

        Utils::PathString signature{createJson(value.parameters)};

        s->insertSignalDeclarationStatement.write(typeId, value.name, signature);
    };

    auto update = [&]([[maybe_unused]] const Storage::Synchronization::SignalDeclarationView &view,
                      [[maybe_unused]] const Storage::Synchronization::SignalDeclaration &value) {
        return Sqlite::UpdateChange::No;
    };

    auto remove = [&](const Storage::Synchronization::SignalDeclarationView &view) {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"remove signal declaration"_t,
                                   projectStorageCategory(),
                                   keyValue("signal declaration view", view)};

        s->deleteSignalDeclarationStatement.write(view.id);
    };

    Sqlite::insertUpdateDelete(range, signalDeclarations, compareKey, insert, update, remove);
}

Utils::PathString ProjectStorage::createJson(
    const Storage::Synchronization::EnumeratorDeclarations &enumeratorDeclarations)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"create json from enumerator declarations"_t, projectStorageCategory()};

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
    NanotraceHR::Tracer tracer{"synchronize enumeration declaration"_t, projectStorageCategory()};

    std::sort(enumerationDeclarations.begin(),
              enumerationDeclarations.end(),
              [](auto &&first, auto &&second) {
                  return Sqlite::compare(first.name, second.name) < 0;
              });

    auto range = s->selectEnumerationDeclarationsForTypeIdStatement
                     .range<Storage::Synchronization::EnumerationDeclarationView>(typeId);

    auto compareKey = [](const Storage::Synchronization::EnumerationDeclarationView &view,
                         const Storage::Synchronization::EnumerationDeclaration &value) {
        return Sqlite::compare(view.name, value.name);
    };

    auto insert = [&](const Storage::Synchronization::EnumerationDeclaration &value) {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"insert enumeration declaration"_t,
                                   projectStorageCategory(),
                                   keyValue("enumeration declaration", value)};

        Utils::PathString signature{createJson(value.enumeratorDeclarations)};

        s->insertEnumerationDeclarationStatement.write(typeId, value.name, signature);
    };

    auto update = [&](const Storage::Synchronization::EnumerationDeclarationView &view,
                      const Storage::Synchronization::EnumerationDeclaration &value) {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"update enumeration declaration"_t,
                                   projectStorageCategory(),
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
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"remove enumeration declaration"_t,
                                   projectStorageCategory(),
                                   keyValue("enumeration declaration view", view)};

        s->deleteEnumerationDeclarationStatement.write(view.id);
    };

    Sqlite::insertUpdateDelete(range, enumerationDeclarations, compareKey, insert, update, remove);
}

void ProjectStorage::extractExportedTypes(TypeId typeId,
                                          const Storage::Synchronization::Type &type,
                                          Storage::Synchronization::ExportedTypes &exportedTypes)
{
    for (const auto &exportedType : type.exportedTypes)
        exportedTypes.emplace_back(exportedType.name, exportedType.version, typeId, exportedType.moduleId);
}

TypeId ProjectStorage::declareType(Storage::Synchronization::Type &type)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"declare type"_t,
                               projectStorageCategory(),
                               keyValue("source id", type.sourceId),
                               keyValue("type name", type.typeName)};

    if (type.typeName.isEmpty()) {
        type.typeId = s->selectTypeIdBySourceIdStatement.value<TypeId>(type.sourceId);

        tracer.end(keyValue("type id", type.typeId));

        return type.typeId;
    }

    type.typeId = s->insertTypeStatement.value<TypeId>(type.sourceId, type.typeName);

    if (!type.typeId)
        type.typeId = s->selectTypeIdBySourceIdAndNameStatement.value<TypeId>(type.sourceId,
                                                                              type.typeName);

    tracer.end(keyValue("type id", type.typeId));

    return type.typeId;
}

void ProjectStorage::syncDeclarations(Storage::Synchronization::Type &type,
                                      AliasPropertyDeclarations &aliasPropertyDeclarationsToLink,
                                      PropertyDeclarationIds &propertyDeclarationIds)
{
    NanotraceHR::Tracer tracer{"synchronize declaration per type"_t, projectStorageCategory()};

    if (type.changeLevel == Storage::Synchronization::ChangeLevel::Minimal)
        return;

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
    NanotraceHR::Tracer tracer{"synchronize declaration"_t, projectStorageCategory()};

    PropertyDeclarationIds propertyDeclarationIds;
    propertyDeclarationIds.reserve(types.size() * 10);

    for (auto &&type : types)
        syncDeclarations(type, aliasPropertyDeclarationsToLink, propertyDeclarationIds);

    removeRelinkableEntries(relinkablePropertyDeclarations,
                            propertyDeclarationIds,
                            PropertyCompare<PropertyDeclaration>{});
}

void ProjectStorage::syncDefaultProperties(Storage::Synchronization::Types &types)
{
    NanotraceHR::Tracer tracer{"synchronize default properties"_t, projectStorageCategory()};

    auto range = s->selectTypesWithDefaultPropertyStatement.range<TypeWithDefaultPropertyView>();

    auto compareKey = [](const TypeWithDefaultPropertyView &view,
                         const Storage::Synchronization::Type &value) {
        return view.typeId - value.typeId;
    };

    auto insert = [&](const Storage::Synchronization::Type &) {

    };

    auto update = [&](const TypeWithDefaultPropertyView &view,
                      const Storage::Synchronization::Type &value) {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"synchronize default properties by update"_t,
                                   projectStorageCategory(),
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
    NanotraceHR::Tracer tracer{"reset changed default properties"_t, projectStorageCategory()};

    auto range = s->selectTypesWithDefaultPropertyStatement.range<TypeWithDefaultPropertyView>();

    auto compareKey = [](const TypeWithDefaultPropertyView &view,
                         const Storage::Synchronization::Type &value) {
        return view.typeId - value.typeId;
    };

    auto insert = [&](const Storage::Synchronization::Type &) {

    };

    auto update = [&](const TypeWithDefaultPropertyView &view,
                      const Storage::Synchronization::Type &value) {
        using NanotraceHR::keyValue;
        NanotraceHR::Tracer tracer{"reset changed default properties by update"_t,
                                   projectStorageCategory(),
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
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"check for prototype chain cycle"_t,
                               projectStorageCategory(),
                               keyValue("type id", typeId)};

    auto callback = [=](TypeId currentTypeId) {
        if (typeId == currentTypeId)
            throw PrototypeChainCycle{};
    };

    s->selectPrototypeAndExtensionIdsStatement.readCallback(callback, typeId);
}

void ProjectStorage::checkForAliasChainCycle(PropertyDeclarationId propertyDeclarationId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"check for alias chain cycle"_t,
                               projectStorageCategory(),
                               keyValue("property declaration id", propertyDeclarationId)};
    auto callback = [=](PropertyDeclarationId currentPropertyDeclarationId) {
        if (propertyDeclarationId == currentPropertyDeclarationId)
            throw AliasChainCycle{};
    };

    s->selectPropertyDeclarationIdsForAliasChainStatement.readCallback(callback,
                                                                       propertyDeclarationId);
}

std::pair<TypeId, ImportedTypeNameId> ProjectStorage::fetchImportedTypeNameIdAndTypeId(
    const Storage::Synchronization::ImportedTypeName &importedTypeName, SourceId sourceId)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch imported type name id and type id"_t,
                               projectStorageCategory(),
                               keyValue("imported type name", importedTypeName),
                               keyValue("source id", sourceId)};

    TypeId typeId;
    ImportedTypeNameId typeNameId;
    auto typeName = std::visit([](auto &&importedTypeName) { return importedTypeName.name; },
                               importedTypeName);
    if (!typeName.empty()) {
        typeNameId = fetchImportedTypeNameId(importedTypeName, sourceId);

        typeId = fetchTypeId(typeNameId);

        tracer.end(keyValue("type id", typeId), keyValue("type name id", typeNameId));

        if (!typeId) {
            errorNotifier->typeNameCannotBeResolved(typeName, sourceId);
            return {unresolvedTypeId, typeNameId};
        }
    }

    return {typeId, typeNameId};
}

void ProjectStorage::syncPrototypeAndExtension(Storage::Synchronization::Type &type, TypeIds &typeIds)
{
    if (type.changeLevel == Storage::Synchronization::ChangeLevel::Minimal)
        return;

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"synchronize prototype and extension"_t,
                               projectStorageCategory(),
                               keyValue("prototype", type.prototype),
                               keyValue("extension", type.extension),
                               keyValue("type id", type.typeId),
                               keyValue("source id", type.sourceId)};

    auto [prototypeId, prototypeTypeNameId] = fetchImportedTypeNameIdAndTypeId(type.prototype,
                                                                               type.sourceId);
    auto [extensionId, extensionTypeNameId] = fetchImportedTypeNameIdAndTypeId(type.extension,
                                                                               type.sourceId);

    s->updatePrototypeAndExtensionStatement.write(type.typeId,
                                                  prototypeId,
                                                  prototypeTypeNameId,
                                                  extensionId,
                                                  extensionTypeNameId);

    if (prototypeId || extensionId)
        checkForPrototypeChainCycle(type.typeId);

    typeIds.push_back(type.typeId);

    tracer.end(keyValue("prototype id", prototypeId),
               keyValue("prototype type name id", prototypeTypeNameId),
               keyValue("extension id", extensionId),
               keyValue("extension type name id", extensionTypeNameId));
}

void ProjectStorage::syncPrototypesAndExtensions(Storage::Synchronization::Types &types,
                                                 Prototypes &relinkablePrototypes,
                                                 Prototypes &relinkableExtensions)
{
    NanotraceHR::Tracer tracer{"synchronize prototypes and extensions"_t, projectStorageCategory()};

    TypeIds typeIds;
    typeIds.reserve(types.size());

    for (auto &type : types)
        syncPrototypeAndExtension(type, typeIds);

    removeRelinkableEntries(relinkablePrototypes, typeIds, TypeCompare<Prototype>{});
    removeRelinkableEntries(relinkableExtensions, typeIds, TypeCompare<Prototype>{});
}

ImportId ProjectStorage::fetchImportId(SourceId sourceId, const Storage::Import &import) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch imported type name id"_t,
                               projectStorageCategory(),
                               keyValue("import", import),
                               keyValue("source id", sourceId)};

    ImportId importId;
    if (import.version) {
        importId = s->selectImportIdBySourceIdAndModuleIdAndVersionStatement.value<ImportId>(
            sourceId, import.moduleId, import.version.major.value, import.version.minor.value);
    } else if (import.version.major) {
        importId = s->selectImportIdBySourceIdAndModuleIdAndMajorVersionStatement
                       .value<ImportId>(sourceId, import.moduleId, import.version.major.value);
    } else {
        importId = s->selectImportIdBySourceIdAndModuleIdStatement.value<ImportId>(sourceId,
                                                                                   import.moduleId);
    }

    tracer.end(keyValue("import id", importId));

    return importId;
}

ImportedTypeNameId ProjectStorage::fetchImportedTypeNameId(
    const Storage::Synchronization::ImportedTypeName &name, SourceId sourceId)
{
    struct Inspect
    {
        auto operator()(const Storage::Synchronization::ImportedType &importedType)
        {
            using NanotraceHR::keyValue;
            NanotraceHR::Tracer tracer{"fetch imported type name id"_t,
                                       projectStorageCategory(),
                                       keyValue("imported type name", importedType.name),
                                       keyValue("source id", sourceId),
                                       keyValue("type name kind", "exported"sv)};

            return storage.fetchImportedTypeNameId(Storage::Synchronization::TypeNameKind::Exported,
                                                   sourceId,
                                                   importedType.name);
        }

        auto operator()(const Storage::Synchronization::QualifiedImportedType &importedType)
        {
            using NanotraceHR::keyValue;
            NanotraceHR::Tracer tracer{"fetch imported type name id"_t,
                                       projectStorageCategory(),
                                       keyValue("imported type name", importedType.name),
                                       keyValue("import", importedType.import),
                                       keyValue("type name kind", "qualified exported"sv)};

            ImportId importId = storage.fetchImportId(sourceId, importedType.import);

            auto importedTypeNameId = storage.fetchImportedTypeNameId(
                Storage::Synchronization::TypeNameKind::QualifiedExported, importId, importedType.name);

            tracer.end(keyValue("import id", importId), keyValue("source id", sourceId));

            return importedTypeNameId;
        }

        ProjectStorage &storage;
        SourceId sourceId;
    };

    return std::visit(Inspect{*this, sourceId}, name);
}

TypeId ProjectStorage::fetchTypeId(ImportedTypeNameId typeNameId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch type id with type name kind"_t,
                               projectStorageCategory(),
                               keyValue("type name id", typeNameId)};

    auto kind = s->selectKindFromImportedTypeNamesStatement.value<Storage::Synchronization::TypeNameKind>(
        typeNameId);

    auto typeId = fetchTypeId(typeNameId, kind);

    tracer.end(keyValue("type id", typeId), keyValue("type name kind", kind));

    return typeId;
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
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch type id"_t,
                               projectStorageCategory(),
                               keyValue("type name id", typeNameId),
                               keyValue("type name kind", kind)};

    TypeId typeId;
    if (kind == Storage::Synchronization::TypeNameKind::Exported) {
        typeId = s->selectTypeIdForImportedTypeNameNamesStatement.value<UnresolvedTypeId>(typeNameId);
    } else {
        typeId = s->selectTypeIdForQualifiedImportedTypeNameNamesStatement.value<UnresolvedTypeId>(
            typeNameId);
    }

    tracer.end(keyValue("type id", typeId));

    return typeId;
}

std::optional<ProjectStorage::FetchPropertyDeclarationResult>
ProjectStorage::fetchPropertyDeclarationByTypeIdAndNameUngarded(TypeId typeId,
                                                                Utils::SmallStringView name)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch optional property declaration by type id and name ungarded"_t,
                               projectStorageCategory(),
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
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch property declaration id by type id and name ungarded"_t,
                               projectStorageCategory(),
                               keyValue("type id", typeId),
                               keyValue("property name", name)};

    auto propertyDeclarationId = fetchPropertyDeclarationId(typeId, name);

    tracer.end(keyValue("property declaration id", propertyDeclarationId));

    return propertyDeclarationId;
}

Storage::Synchronization::ExportedTypes ProjectStorage::fetchExportedTypes(TypeId typeId)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch exported type"_t,
                               projectStorageCategory(),
                               keyValue("type id", typeId)};

    auto exportedTypes = s->selectExportedTypesByTypeIdStatement
                             .values<Storage::Synchronization::ExportedType, 12>(typeId);

    tracer.end(keyValue("exported types", exportedTypes));

    return exportedTypes;
}

Storage::Synchronization::PropertyDeclarations ProjectStorage::fetchPropertyDeclarations(TypeId typeId)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch property declarations"_t,
                               projectStorageCategory(),
                               keyValue("type id", typeId)};

    auto propertyDeclarations = s->selectPropertyDeclarationsByTypeIdStatement
                                    .values<Storage::Synchronization::PropertyDeclaration, 24>(typeId);

    tracer.end(keyValue("property declarations", propertyDeclarations));

    return propertyDeclarations;
}

Storage::Synchronization::FunctionDeclarations ProjectStorage::fetchFunctionDeclarations(TypeId typeId)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch signal declarations"_t,
                               projectStorageCategory(),
                               keyValue("type id", typeId)};

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

Storage::Synchronization::SignalDeclarations ProjectStorage::fetchSignalDeclarations(TypeId typeId)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch signal declarations"_t,
                               projectStorageCategory(),
                               keyValue("type id", typeId)};

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

Storage::Synchronization::EnumerationDeclarations ProjectStorage::fetchEnumerationDeclarations(TypeId typeId)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch enumeration declarations"_t,
                               projectStorageCategory(),
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

template<typename... TypeIds>
bool ProjectStorage::isBasedOn_(TypeId typeId, TypeIds... baseTypeIds) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"is based on"_t,
                               projectStorageCategory(),
                               keyValue("type id", typeId),
                               keyValue("base type ids", NanotraceHR::array(baseTypeIds...))};

    static_assert(((std::is_same_v<TypeId, TypeIds>) &&...), "Parameter must be a TypeId!");

    if (((typeId == baseTypeIds) || ...)) {
        tracer.end(keyValue("is based on", true));
        return true;
    }

    auto range = s->selectPrototypeAndExtensionIdsStatement.rangeWithTransaction<TypeId>(typeId);

    auto isBasedOn = std::any_of(range.begin(), range.end(), [&](TypeId currentTypeId) {
        return ((currentTypeId == baseTypeIds) || ...);
    });

    tracer.end(keyValue("is based on", isBasedOn));

    return isBasedOn;
}

template<typename Id>
ImportedTypeNameId ProjectStorage::fetchImportedTypeNameId(Storage::Synchronization::TypeNameKind kind,
                                                           Id id,
                                                           Utils::SmallStringView typeName)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch imported type name id"_t,
                               projectStorageCategory(),
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

} // namespace QmlDesigner
