// Generates TypeScript types from the JSON Schema files into src/generated/.
// The schemas are the single source of truth for both runtime validation
// (ajv, in src/index.ts) and these compile-time types. Regenerate with
// `npm run gen:types` whenever a schema changes.
import { readFileSync, writeFileSync, mkdirSync } from "fs";
import { fileURLToPath } from "url";
import { dirname, join } from "path";
import { compile } from "json-schema-to-typescript";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");
const outDir = join(root, "src", "generated");
mkdirSync(outDir, { recursive: true });

// Remove validation-only anyOf/oneOf branches (members carrying nothing but a
// `required` list). They express "at least one of these must be present" — a
// runtime constraint ajv still enforces from the original schema — but add
// nothing to the type, and their presence forces json2ts to fall back to a
// `{ [k: string]: unknown }` index signature that hides sibling properties
// (e.g. PositionalArgument's `type`/`valueHint`).
function stripValidationOnlyCombinators(node) {
  if (Array.isArray(node)) {
    node.forEach(stripValidationOnlyCombinators);
    return;
  }
  if (!node || typeof node !== "object") return;
  for (const key of ["anyOf", "oneOf"]) {
    const branches = node[key];
    if (
      Array.isArray(branches) &&
      branches.every(
        (m) => m && typeof m === "object" && Object.keys(m).every((k) => k === "required"),
      )
    ) {
      delete node[key];
    }
  }
  for (const value of Object.values(node)) stripValidationOnlyCombinators(value);
}

const banner = "/* eslint-disable */\n// Auto-generated from the JSON Schema files. Do not edit by hand.\n// Run `npm run gen:types` to regenerate.";

const options = {
  bannerComment: banner,
  additionalProperties: false,
  style: { singleQuote: false },
};

async function gen(schemaFile, name, { stripRootRef = false } = {}) {
  const schema = JSON.parse(readFileSync(join(root, schemaFile), "utf-8"));
  // A schema whose root is itself a $ref (registry.schema.json) can't be
  // compiled directly; drop the root ref and emit every definition instead.
  if (stripRootRef) delete schema.$ref;
  stripValidationOnlyCombinators(schema);
  const ts = await compile(schema, name, {
    ...options,
    unreachableDefinitions: stripRootRef,
  });
  const outFile = join(outDir, `${name}.ts`);
  writeFileSync(outFile, ts, "utf-8");
  console.log(`Generated ${outFile}`);
}

await gen("registry.schema.json", "registry", { stripRootRef: true });
await gen("condensed.schema.json", "condensed");
