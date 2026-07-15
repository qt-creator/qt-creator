import { S3Client, PutObjectCommand } from "@aws-sdk/client-s3";
import { mkdirSync, writeFileSync, readFileSync } from "fs";
import { join } from "path";
import Ajv, { type ValidateFunction } from "ajv";
import addFormats from "ajv-formats";

// Types are generated from the JSON Schema files by scripts/gen-types.mjs
// (`npm run gen:types`). The schemas are the single source of truth: ajv
// validates against them at runtime, these types describe the same shapes at
// compile time.
import type {
  ServerDetail,
  Package as RegistryPackage,
  Argument as RegistryArgument,
  Icon as RegistryIcon,
  KeyValueInput as RegistryKeyValueInput,
} from "./generated/registry";
import type {
  McpRegistry,
  Server as CondensedServer,
  Package as CondensedPackage,
  Argument as CondensedArgument,
  Icon as CondensedIcon,
  KeyValueInput as CondensedKeyValueInput,
  Remote as CondensedRemote,
} from "./generated/condensed";

// ---------------------------------------------------------------------------
// Types not covered by the generated schema types
// ---------------------------------------------------------------------------

// A registry list entry: the server payload plus registry-managed metadata.
// registry.schema.json describes only the `server` object, so the envelope
// _meta (which carries the lifecycle status) is declared here.
interface ServerResponse {
  server: ServerDetail;
  _meta?: {
    "io.modelcontextprotocol.registry/official"?: {
      status?: "active" | "deprecated" | "deleted";
    };
    [key: string]: unknown;
  };
}

// One page of the paginated /servers response. The pagination envelope isn't
// part of either schema, so it's typed and lightly checked here; each entry is
// validated against registry.schema.json separately.
interface ServerListEnvelope {
  servers: unknown[];
  metadata: { nextCursor?: string | null; count: number };
}

// ---------------------------------------------------------------------------
// Fetch helpers
// ---------------------------------------------------------------------------

const REGISTRY_BASE = "https://registry.modelcontextprotocol.io";
const PAGE_LIMIT = 100;

async function fetchPage(cursor?: string): Promise<ServerListEnvelope> {
  const params = new URLSearchParams({ limit: String(PAGE_LIMIT), version: "latest" });
  if (cursor) params.set("cursor", cursor);

  const url = `${REGISTRY_BASE}/v0.1/servers?${params}`;
  const res = await fetch(url);
  if (!res.ok) {
    throw new Error(`Registry fetch failed: ${res.status} ${res.statusText} — ${url}`);
  }

  const json = (await res.json()) as ServerListEnvelope;
  if (!Array.isArray(json?.servers) || typeof json?.metadata?.count !== "number") {
    throw new Error(`Unexpected registry response shape — ${url}`);
  }
  return json;
}

// Publishing tooling sometimes emits an optional object empty (e.g.
// `repository: {}`) instead of omitting it. Such an empty object fails schema
// validation for its now-missing required fields, so treat it as not provided.
function normalizeServer(server: unknown): void {
  if (!server || typeof server !== "object") return;
  const s = server as Record<string, unknown>;
  if (
    s.repository &&
    typeof s.repository === "object" &&
    Object.keys(s.repository as object).length === 0
  ) {
    delete s.repository;
  }
}

async function fetchAllServers(
  validateRegistryServer: ValidateFunction,
  outDir?: string,
): Promise<ServerResponse[]> {
  const all: ServerResponse[] = [];
  let cursor: string | undefined;
  let pageNum = 0;
  let dropped = 0;

  if (outDir) mkdirSync(outDir, { recursive: true });

  do {
    const page = await fetchPage(cursor);

    // Validate each entry's server object against registry.schema.json; drop
    // any that don't conform.
    for (const entry of page.servers) {
      const server = (entry as { server?: unknown })?.server;
      normalizeServer(server);
      if (validateRegistryServer(server)) {
        all.push(entry as ServerResponse);
      } else {
        dropped++;
        const name = (server as { name?: unknown })?.name ?? "<unknown>";
        console.warn(
          `  Dropped invalid entry "${String(name)}": ${formatAjvErrors(validateRegistryServer.errors)}`,
        );
      }
    }

    cursor = page.metadata.nextCursor ?? undefined;
    pageNum++;
    console.log(`Fetched ${all.length} valid servers so far (page count: ${page.metadata.count})`);

    if (outDir) {
      const pagePath = join(outDir, `page-${String(pageNum).padStart(3, "0")}.json`);
      writeFileSync(pagePath, JSON.stringify(page, null, 2), "utf-8");
      console.log(`  Saved ${pagePath}`);
    }
  } while (cursor);

  if (dropped > 0) {
    console.log(`Dropped ${dropped} entr${dropped === 1 ? "y" : "ies"} that failed schema validation`);
  }

  return all;
}

// ---------------------------------------------------------------------------
// Condensing logic
// ---------------------------------------------------------------------------

function condensedStatus(srv: ServerResponse): "active" | "deprecated" | "deleted" {
  return srv._meta?.["io.modelcontextprotocol.registry/official"]?.status ?? "active";
}

function condenseKeyValueInputs(inputs: RegistryKeyValueInput[]): CondensedKeyValueInput[] {
  return inputs.map((kv) => ({
    name: kv.name,
    description: kv.description,
    required: kv.isRequired ?? false,
    secret: kv.isSecret ?? false,
    default: kv.default,
  }));
}

function condensedPackage(pkg: RegistryPackage): CondensedPackage {
  const transport = pkg.transport;

  const condenseArgs = (args: RegistryArgument[]): CondensedArgument[] => args
    .map((arg) => ({
      type: arg.type,
      name: arg.type === "named" ? arg.name : undefined,
      value_hint: arg.type === "positional" ? arg.valueHint : undefined,
      description: arg.description,
      required: arg.isRequired ?? false,
      repeated: arg.isRepeated ?? false,
      default: arg.default,
      value: arg.value,
    }));

  return {
    // The output schema constrains registry_type to a fixed enum; ajv drops any
    // condensed server that violates it, so the cast is safe here.
    registry_type: pkg.registryType as CondensedPackage["registry_type"],
    identifier: pkg.identifier,
    version: pkg.version,
    transport_type: transport.type,
    headers: transport.type !== "stdio" ? condenseKeyValueInputs(transport.headers ?? []) : [],
    runtime_hint: pkg.runtimeHint,
    runtime_arguments: condenseArgs(pkg.runtimeArguments ?? []),
    package_arguments: condenseArgs(pkg.packageArguments ?? []),
    env_vars: condenseKeyValueInputs(pkg.environmentVariables ?? []),
  };
}

async function fetchIconData(icon: RegistryIcon): Promise<CondensedIcon> {
  const base: CondensedIcon = {
    url: icon.src,
    data: undefined,
    mime_type: icon.mimeType,
    sizes: icon.sizes,
    theme: icon.theme,
  };

  if (icon.mimeType !== "image/svg+xml") return base;

  try {
    const res = await fetch(icon.src);
    if (!res.ok) {
      console.warn(`Icon fetch failed (${res.status}): ${icon.src}`);
      return base;
    }
    const buffer = await res.arrayBuffer();
    return { ...base, data: Buffer.from(buffer).toString("base64") };
  } catch (err) {
    console.warn(`Icon fetch error: ${icon.src} — ${(err as Error).message}`);
    return base;
  }
}

async function condenseServer(srv: ServerResponse): Promise<CondensedServer | null> {
  const { server } = srv;

  // Only keep packages with stdio transport
  const stdioPackages = (server.packages ?? []).filter((pkg) => pkg.transport.type === "stdio");
  const remotes = (server.remotes ?? []).map((r) => ({
    type: r.type,
    url: r.url,
    headers: condenseKeyValueInputs(r.headers ?? []),
  }));

  // Skip servers with no stdio packages and no remotes
  if (stdioPackages.length === 0 && remotes.length === 0) return null;

  const icons = await Promise.all((server.icons ?? []).map(fetchIconData));

  return {
    name: server.name,
    title: server.title,
    description: server.description,
    version: server.version,
    status: condensedStatus(srv),
    repository_url: server.repository?.url,
    website_url: server.websiteUrl,
    icons,
    packages: stdioPackages.map(condensedPackage),
    remotes,
  };
}

// ---------------------------------------------------------------------------
// Schema validation (JSON Schema)
// ---------------------------------------------------------------------------

// Matches the serialized shape: nulls and empty arrays (except the top-level
// "servers") are omitted from the written JSON. Used both to clean objects
// before validation and to serialize the final document, so the two agree.
function stripEmpty(key: string, value: unknown): unknown {
  if (value === null) return undefined;
  if (Array.isArray(value) && value.length === 0 && key !== "servers") return undefined;
  return value;
}

interface SchemaValidators {
  // Input: a registry entry's `server` object (registry.schema.json).
  registryServer: ValidateFunction;
  // Output: a single condensed server (condensed.schema.json).
  condensedServer: ValidateFunction;
  // Output: the whole condensed document (condensed.schema.json root).
  condensedRegistry: ValidateFunction;
}

function loadSchemaValidators(): SchemaValidators {
  const load = (name: string) =>
    JSON.parse(readFileSync(new URL(`../${name}`, import.meta.url), "utf-8"));

  const ajv = new Ajv({ allErrors: true, strict: false, verbose: true });
  addFormats(ajv);
  ajv.addSchema(load("registry.schema.json"), "registry");
  ajv.addSchema(load("condensed.schema.json"), "condensed");

  const registryServer = ajv.getSchema("registry");
  const condensedServer = ajv.getSchema("condensed#/definitions/Server");
  const condensedRegistry = ajv.getSchema("condensed");
  if (!registryServer || !condensedServer || !condensedRegistry) {
    throw new Error("Failed to compile schema validators");
  }
  return { registryServer, condensedServer, condensedRegistry };
}

// Render a value for an error message, truncating anything large so a
// root-level failure doesn't dump an entire server object into the log.
function describeValue(value: unknown): string {
  const s = JSON.stringify(value) ?? String(value);
  return s.length > 80 ? `${s.slice(0, 77)}…` : s;
}

function formatAjvErrors(errors: ValidateFunction["errors"]): string {
  if (!errors || errors.length === 0) return "unknown error";
  return errors
    .map((e) => {
      // For additionalProperties the offending key lives in params, not data.
      if (e.keyword === "additionalProperties") {
        return `${e.instancePath || "<root>"} ${e.message} (got "${e.params.additionalProperty}")`;
      }
      const got = e.data !== undefined ? ` (got ${describeValue(e.data)})` : "";
      return `${e.instancePath || "<root>"} ${e.message}${got}`;
    })
    .join("; ");
}

// ---------------------------------------------------------------------------
// S3 upload
// ---------------------------------------------------------------------------

async function uploadToS3(payload: string): Promise<void> {
  const bucket = process.env.S3_BUCKET;
  const region = process.env.AWS_REGION ?? "us-east-1";
  const key = process.env.S3_KEY ?? "mcp/registry.json";

  if (!bucket) throw new Error("S3_BUCKET environment variable is required");

  const client = new S3Client({ region });
  await client.send(
    new PutObjectCommand({
      Bucket: bucket,
      Key: key,
      Body: payload,
      ContentType: "application/json",
    })
  );
  console.log(`Uploaded ${key} to s3://${bucket}/${key}`);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

async function main(): Promise<void> {
  const outputFlag = process.argv.indexOf("--output");
  const outputFile = outputFlag !== -1 ? process.argv[outputFlag + 1] : undefined;
  if (outputFlag !== -1 && !outputFile) throw new Error("--output requires a file path argument");

  // Dry run: fetch, condense and validate, but don't write anywhere.
  const dry = process.argv.includes("--dry");

  // Without --output or --dry the result is uploaded to S3, so fail fast on a
  // missing bucket before doing all the fetching work.
  if (!outputFile && !dry && !process.env.S3_BUCKET) {
    throw new Error("S3_BUCKET environment variable is required (or pass --output <file> or --dry)");
  }

  // When writing to a file, store raw pages alongside it (skipped on a dry run)
  const outDir = outputFile && !dry ? join(outputFile, "..", "pages") : undefined;

  const { registryServer, condensedServer, condensedRegistry } = loadSchemaValidators();

  console.log("Fetching MCP registry…");
  const raw = await fetchAllServers(registryServer, outDir);

  // Drop deleted servers; keep active + deprecated
  const filtered = raw.filter((s) => condensedStatus(s) !== "deleted");
  console.log(`Total: ${raw.length}, after filtering deleted: ${filtered.length}`);

  // Condense servers with bounded concurrency to avoid exhausting sockets during icon fetches
  const CONCURRENCY = 20;
  const condensed: CondensedServer[] = [];
  for (let i = 0; i < filtered.length; i += CONCURRENCY) {
    const batch = filtered.slice(i, i + CONCURRENCY);
    const results = await Promise.all(batch.map(condenseServer));
    condensed.push(...results.filter((s): s is CondensedServer => s !== null));
    if ((i + CONCURRENCY) % 200 === 0) {
      console.log(`Condensed ${condensed.length} / ${filtered.length}`);
    }
  }
  // Validate each condensed server against condensed.schema.json (the output
  // contract), dropping any that don't conform. Validate the serialized shape,
  // since the writer omits nulls and empty arrays.
  const valid: CondensedServer[] = [];
  let invalid = 0;
  for (const srv of condensed) {
    const serialized = JSON.parse(JSON.stringify(srv, stripEmpty));
    if (condensedServer(serialized)) {
      valid.push(srv);
    } else {
      invalid++;
      console.warn(`  Dropped "${srv.name}": ${formatAjvErrors(condensedServer.errors)}`);
    }
  }
  if (invalid > 0) {
    console.log(`Dropped ${invalid} server${invalid === 1 ? "" : "s"} that failed output schema validation`);
  }

  const output: McpRegistry = {
    generated_at: new Date().toISOString(),
    count: valid.length,
    servers: valid,
  };

  const json = JSON.stringify(output, stripEmpty, 2);

  // Final guard: the assembled document must satisfy condensed.schema.json.
  if (!condensedRegistry(JSON.parse(json))) {
    throw new Error(`Output failed schema validation: ${formatAjvErrors(condensedRegistry.errors)}`);
  }

  if (dry) {
    console.log(`Dry run: ${valid.length} servers validated, not saved (${json.length} bytes).`);
  } else if (outputFile) {
    writeFileSync(outputFile, json, "utf-8");
    console.log(`Written to ${outputFile}`);
  } else {
    await uploadToS3(json);
  }

  console.log("Done.");
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
