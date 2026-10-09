#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

"""Helpers for testing the "Qt Creator C++ Plugin" wizard template on CI.

expand: Instantiates the template into a directory, like the wizard would.
run-steps: Runs steps of the generated GitHub workflow outside of GitHub Actions.
"""

import argparse
import json
import os
import platform
import re
import subprocess
import sys
import tempfile
from pathlib import Path

TEMPLATE_DIR = Path(__file__).resolve().parent.parent / "share/qtcreator/templates/wizards/qtcreatorplugin"
PLUGIN_NAME = "TestPlugin"


def js_values(plugin_name, qt_version, qtc_version):
    # Evaluations of the JS expressions the template uses. An unknown expression is an error,
    # so that a new one in the template makes this test fail until it is handled here.
    lower = plugin_name.lower()
    return {
        "Util.qtVersion()": qt_version,
        "Util.qtCreatorVersion()": qtc_version,
        "value('PluginName').toLowerCase()": lower,
        "value('VendorName').toLowerCase().replace(/ /g, '')": "testvendor",
        "encodeURIComponent(value('VendorName').toLowerCase())": "test%20vendor",
        "Cpp.className(value('PluginName') + 'Plugin')": plugin_name + "Plugin",
        "Cpp.headerGuard(value('PluginName')) + '_EXPORT'": plugin_name.upper() + "_EXPORT",
        "Cpp.headerGuard(value('PluginName')) + '_LIBRARY'": plugin_name.upper() + "_LIBRARY",
        "Util.fileName(value('PluginName'), 'json.in')": plugin_name + ".json.in",
        "Util.fileName(value('PluginNameLower') + 'constants', Util.preferredSuffix('text/x-c++hdr'))":
            lower + "constants.h",
        "Util.fileName(value('PluginNameLower') + 'tr', Util.preferredSuffix('text/x-c++hdr'))":
            lower + "tr.h",
        "Util.fileName(value('PluginNameLower'), 'moc')": lower + ".moc",
        "Util.fileName(value('PluginNameLower'), Util.preferredSuffix('text/x-c++src'))":
            lower + ".cpp",
        "value('ProjectName').charAt(0).toUpperCase() + value('ProjectName').slice(1)": plugin_name,
        "value('TsFileName') !== ''": "false",
        "!value('IsSubproject') && value('VersionControl') === 'G.Git'": "true",
        "Util.isDirectory('%{QtCreatorBuild}/Qt Creator.app/Contents/Resources/lib/cmake/QtCreator')":
            "false",
        "Util.isDirectory('%{QtCreatorBuild}/Contents/Resources/lib/cmake/QtCreator')": "false",
    }


def template_variables(plugin_name):
    lower = plugin_name.lower()
    return {
        "PluginName": plugin_name,
        "PluginNameLower": lower,
        "CN": plugin_name + "Plugin",
        "VendorName": "Test Vendor",
        "Copyright": "(C) 2026 Test Vendor",
        "License": "Put short license information here",
        "Description": "Put a short description of your plugin here",
        "Url": "https://www.example.com",
        "QtCreatorBuild": "",
        "ProjectDirectory": ".",
        "SrcFileName": lower + ".cpp",
        "MocFileName": lower + ".moc",
        "ConstantsHdrFileName": lower + "constants.h",
        "TrHdrFileName": lower + "tr.h",
        "PluginJsonFile": plugin_name + ".json.in",
        "TsFileName": "",
        "HasTranslation": "false",
        "Cpp:LicenseTemplate": "// Test license\n",
    }


def process_text(text, variables, js):
    def expand_vars(s):
        # JS expressions are matched literally, as nested variables are part of them
        def replace_js(m):
            expr = m.group(1).strip()
            if expr not in js:
                sys.exit(f"Unhandled JS expression in template: {expr}")
            return js[expr]

        s = re.sub(r"%\{JS:((?:[^{}]|%\{[^{}]*\})*)\}", replace_js, s)

        def replace_var(m):
            if m.group(1) not in variables:
                sys.exit(f"Unknown variable in template: {m.group(1)}")
            return variables[m.group(1)]

        return re.sub(r"%\{([^{}]+)\}", replace_var, s)

    result = []
    active = [True]
    for line in text.splitlines(keepends=True):
        stripped = line.strip()
        if stripped.startswith("@if "):
            cond = expand_vars(stripped[4:]).strip()
            active.append(active[-1] and cond == "true")
            continue
        if stripped == "@else":
            parent = active[-2]
            was_active = active[-1]
            active[-1] = parent and not was_active
            continue
        if stripped == "@endif":
            active.pop()
            continue
        if active[-1]:
            result.append(expand_vars(line))
    out = "".join(result)
    # The template engine treats a trailing backslash as line continuation and "\x" as escape
    out = out.replace("\\\n", "")
    return re.sub(r"\\(.)", r"\1", out)


def expand(args):
    out_dir = Path(args.output)
    out_dir.mkdir(parents=True, exist_ok=True)
    variables = template_variables(PLUGIN_NAME)
    js = js_values(PLUGIN_NAME, args.qt_version, args.qtc_version)
    mapping = {
        "CMakeLists.txt": "CMakeLists.txt",
        "README.md": "README.md",
        "github_workflows_build_cmake.yml": ".github/workflows/build_cmake.yml",
        "github_workflows_README.md": ".github/workflows/README.md",
        "myplugin.cpp": variables["SrcFileName"],
        "mypluginconstants.h": variables["ConstantsHdrFileName"],
        "myplugintr.h": variables["TrHdrFileName"],
        "MyPlugin.json.in": variables["PluginJsonFile"],
    }
    for source, target in mapping.items():
        text = (TEMPLATE_DIR / source).read_text(encoding="utf-8")
        target_path = out_dir / target
        target_path.parent.mkdir(parents=True, exist_ok=True)
        target_path.write_text(process_text(text, variables, js), encoding="utf-8", newline="\n")
        print(f"Generated {target_path}")


def run_steps(args):
    import yaml

    workflow = yaml.safe_load(Path(args.workflow).read_text(encoding="utf-8"))
    job = workflow["jobs"]["build"]
    configs = job["strategy"]["matrix"]["config"]
    config = next((c for c in configs if c.get("platform") == args.platform), None)
    if config is None:
        sys.exit(f"No matrix entry for platform {args.platform}")

    runner_os = {"Windows": "Windows", "Linux": "Linux", "Darwin": "macOS"}[platform.system()]
    workspace = os.getcwd()
    context = {"runner.os": runner_os, "github.ref": "refs/heads/test", "github.run_id": "0"}
    for key, value in {**workflow.get("env", {}), **job.get("env", {})}.items():
        context[f"env.{key}"] = str(value)
    for key, value in config.items():
        context[f"matrix.config.{key}"] = str(value)
    # Step outputs persist between invocations, which stand in for consecutive steps
    context_file = Path(workspace) / ".plugin_template_ci_context.json"
    if context_file.exists():
        context.update(json.loads(context_file.read_text(encoding="utf-8")))
    for item in args.output_value or []:
        key, value = item.split("=", 1)
        context[key] = value

    steps_by_name = {step.get("name"): step for step in job["steps"]}
    for name in args.step:
        step = steps_by_name.get(name)
        if step is None:
            sys.exit(f'Step "{name}" not found in {args.workflow}')
        if "run" not in step:
            sys.exit(f'Step "{name}" is not a run step')
        if step.get("shell") != "cmake -P {0}":
            sys.exit(f'Step "{name}" does not use the cmake shell')

        def substitute(m):
            key = m.group(1).strip()
            if key.startswith("matrix.config.") and key not in context:
                return ""  # Unset matrix values evaluate to an empty string
            if key not in context:
                sys.exit(f'Unknown expression "${{{{ {key} }}}}" in step "{name}"')
            return context[key]

        script = re.sub(r"\$\{\{(.*?)\}\}", substitute, step["run"])
        with tempfile.TemporaryDirectory() as tmp:
            script_file = Path(tmp) / "step.cmake"
            output_file = Path(tmp) / "output"
            output_file.touch()
            script_file.write_text(script, encoding="utf-8")
            env = dict(os.environ)
            env.update({k[4:]: v for k, v in context.items() if k.startswith("env.")})
            env["GITHUB_WORKSPACE"] = workspace
            env["GITHUB_OUTPUT"] = str(output_file)
            print(f"::group::{name}", flush=True)
            result = subprocess.run(["cmake", "-P", str(script_file)], env=env)
            print("::endgroup::", flush=True)
            if result.returncode != 0:
                sys.exit(f'Step "{name}" failed with exit code {result.returncode}')
            step_id = step.get("id")
            if step_id:
                for line in output_file.read_text(encoding="utf-8").splitlines():
                    if "=" in line:
                        key, value = line.split("=", 1)
                        context[f"steps.{step_id}.outputs.{key}"] = value
    outputs = {k: v for k, v in context.items() if k.startswith("steps.")}
    context_file.write_text(json.dumps(outputs), encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(required=True)

    p = sub.add_parser("expand", help="Instantiate the template")
    p.add_argument("--output", required=True)
    p.add_argument("--qt-version", required=True)
    p.add_argument("--qtc-version", required=True)
    p.set_defaults(func=expand)

    p = sub.add_parser("run-steps", help="Run steps of the generated workflow")
    p.add_argument("--workflow", required=True)
    p.add_argument("--platform", required=True)
    p.add_argument("--step", action="append", required=True)
    p.add_argument("--output-value", action="append",
                   help="Context value, e.g. steps.qt_creator.outputs.path=qtcreator")
    p.set_defaults(func=run_steps)

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
