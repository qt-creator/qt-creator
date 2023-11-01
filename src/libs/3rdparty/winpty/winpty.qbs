import qbs
import qbs.TextFile

Project {
    name: "Winpty"
    condition: qbs.targetOS.contains("windows")

    Product {
        name: "winpty_genversion_header"
        type: "hpp"

        Group {
            files: "VERSION.txt"
            fileTags: "txt.in"
        }

        Rule {
            inputs: "txt.in"
            Artifact {
                filePath: "GenVersion.h"
                fileTags: "hpp"
            }
            prepare: {
                var cmd = new JavaScriptCommand();
                cmd.description = "generating GenVersion.h";
                cmd.highlight = "codegen";
                cmd.sourceCode = function() {
                    var inFile = new TextFile(input.filePath);
                    var versionTxt = inFile.readAll();
                    inFile.close();
                    // remove any line endings
                    versionTxt = versionTxt.replace(/[\r\n]/g, "");

                    var content = 'const char GenVersion_Version[] = "@VERSION@";\n'
                            + 'const char GenVersion_Commit[] = "@COMMIT_HASH@";\n';
                    content = content.replace(/@VERSION@/g, versionTxt);

                    var outFile = new TextFile(output.filePath, TextFile.WriteOnly);
                    outFile.truncate();
                    outFile.write(content);
                    outFile.close();
                }
                return cmd;
            }
        }

        Export {
            Depends { name: "cpp" }
            cpp.includePaths: exportingProduct.buildDirectory
        }
    }

    QtcTool {
        name: "winpty-agent"
        Depends { name: "winpty_genversion_header" }
        Depends { name: "cpp" }

        useQt: false

        cpp.includePaths: base.concat([sourceDirectory + "/include", buildDirectory])
        cpp.defines: base.concat(["WINPTY_AGENT_ASSERT",
                                  "NOMINMAX", "UNICODE", "_UNICODE"
                                 ])
        cpp.dynamicLibraries: ["user32", "shell32", "advapi32"]

        files: [
            "src/agent/Agent.h",
            "src/agent/Agent.cc",
            "src/agent/AgentCreateDesktop.h",
            "src/agent/AgentCreateDesktop.cc",
            "src/agent/ConsoleFont.cc",
            "src/agent/ConsoleFont.h",
            "src/agent/ConsoleInput.cc",
            "src/agent/ConsoleInput.h",
            "src/agent/ConsoleInputReencoding.cc",
            "src/agent/ConsoleInputReencoding.h",
            "src/agent/ConsoleLine.cc",
            "src/agent/ConsoleLine.h",
            "src/agent/Coord.h",
            "src/agent/DebugShowInput.h",
            "src/agent/DebugShowInput.cc",
            "src/agent/DefaultInputMap.h",
            "src/agent/DefaultInputMap.cc",
            "src/agent/DsrSender.h",
            "src/agent/EventLoop.h",
            "src/agent/EventLoop.cc",
            "src/agent/InputMap.h",
            "src/agent/InputMap.cc",
            "src/agent/LargeConsoleRead.h",
            "src/agent/LargeConsoleRead.cc",
            "src/agent/NamedPipe.h",
            "src/agent/NamedPipe.cc",
            "src/agent/Scraper.h",
            "src/agent/Scraper.cc",
            "src/agent/SimplePool.h",
            "src/agent/SmallRect.h",
            "src/agent/Terminal.h",
            "src/agent/Terminal.cc",
            "src/agent/UnicodeEncoding.h",
            "src/agent/Win32Console.cc",
            "src/agent/Win32Console.h",
            "src/agent/Win32ConsoleBuffer.cc",
            "src/agent/Win32ConsoleBuffer.h",
            "src/agent/main.cc",
        ]

        Group {
            name: "Shared sources"
            prefix: "src/shared/"
            files: [
                "AgentMsg.h",
                "BackgroundDesktop.h",
                "BackgroundDesktop.cc",
                "Buffer.h",
                "Buffer.cc",
                "DebugClient.h",
                "DebugClient.cc",
                "GenRandom.h",
                "GenRandom.cc",
                "OsModule.h",
                "OwnedHandle.h",
                "OwnedHandle.cc",
                "StringBuilder.h",
                "StringUtil.cc",
                "StringUtil.h",
                "UnixCtrlChars.h",
                "WindowsSecurity.cc",
                "WindowsSecurity.h",
                "WindowsVersion.h",
                "WindowsVersion.cc",
                "WinptyAssert.h",
                "WinptyAssert.cc",
                "WinptyException.h",
                "WinptyException.cc",
                "WinptyVersion.h",
                "WinptyVersion.cc",
                "winpty_snprintf.h",
            ]
        }
    }

    QtcLibrary {
        name: "winpty"
        type: "staticlibrary"

        Depends { name: "winpty_genversion_header" }
        Depends { name: "cpp" }

        useNonGuiPchFile: false
        useGuiPchFile: false

        cpp.defines: base.concat(["COMPILING_WINPTY_DLL",
                                  "NOMINMAX", "UNICODE", "_UNICODE"
                                 ])
        cpp.dynamicLibraries: ["user32", "shell32", "advapi32"]
        cpp.includePaths: base.concat(sourceDirectory + "/src/include")


        files: [
            "src/libwinpty/AgentLocation.cc",
            "src/libwinpty/AgentLocation.h",
            "src/libwinpty/winpty.cc",
        ]

        Group {
            name: "Shared sources" // FIXME duplication
            prefix: "src/shared/"
            files: [
                "AgentMsg.h",
                "BackgroundDesktop.h",
                "BackgroundDesktop.cc",
                "Buffer.h",
                "Buffer.cc",
                "DebugClient.h",
                "DebugClient.cc",
                "GenRandom.h",
                "GenRandom.cc",
                "OsModule.h",
                "OwnedHandle.h",
                "OwnedHandle.cc",
                "StringBuilder.h",
                "StringUtil.cc",
                "StringUtil.h",
                "UnixCtrlChars.h",
                "WindowsSecurity.cc",
                "WindowsSecurity.h",
                "WindowsVersion.h",
                "WindowsVersion.cc",
                "WinptyAssert.h",
                "WinptyAssert.cc",
                "WinptyException.h",
                "WinptyException.cc",
                "WinptyVersion.h",
                "WinptyVersion.cc",
                "winpty_snprintf.h",
            ]
        }

        Export {
            Depends { name: "cpp" }
            cpp.defines: "COMPILING_WINPTY_DLL"
            cpp.includePaths: exportingProduct.sourceDirectory + "/src/include"
        }
    }
}
