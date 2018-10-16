import qbs

Product {
    name: "Botan"
    condition: !qtc.useSystemBotan
    type: ["staticlibrary", "hpp"]
    Depends { name: "cpp" }
    Depends { name: "qtc" }
    Depends { name: "xcode"; condition: qbs.toolchain.contains("xcode") }
    files: "update-botan.sh"
    Group {
        name: "Botan sources"
        prefix: "../3rdparty/botan/"
        Group {
            name: "Botan configure script"
            files: "configure.py"
            fileTags: "botan.configure"
        }
        Group {
            name: "Other Botan files"
            files: "**/*"
            excludeFiles: "configure.py"
            fileTags: "botan.sources"
        }
    }

    Rule {
        inputs: "botan.configure"
        Artifact {
            filePath: "Makefile"
            fileTags: "botan.Makefile"
        }
        Artifact {
            filePath: "build/include/botan/build.h"
            fileTags: "hpp"
        }
        prepare: {
            var args = [input.filePath, "--amalgamation", "--minimized-build", "--disable-shared",
                        "--without-documentation"];
            var modules = "aes,aes_ssse3,auto_rng,bigint,block,cbc,ctr,des,dh,dsa,ec_group,ecdh,"
                    + "ecdsa,entropy,filters,hmac,mode_pad,pubkey,rsa,sha1,sha1_sse2,sha1_x86,"
                    + "sha2_32,sha2_32_x86,sha2_64,simd,system_rng,emsa_pkcs1,pbes2,pbkdf2";
            args.push("--enable-modules=" + modules);
            var cxxFlags = [];
            var tc = product.qbs.toolchain;
            if (tc.contains("msvc")) {
                cxxFlags.push("/wd4100", "/wd4800", "/wd4127", "/wd4244", "/wd4250", "/wd4267",
                              "/wd4334", "/wd4702", "/wd4996", "/D_ENABLE_EXTENDED_ALIGNED_STORAGE");
            }
            else if (product.qbs.toolchain.contains("gcc"))
                cxxFlags.push("-Wno-unused-parameter");
            if (product.qbs.targetOS.contains("macos")) {
                cxxFlags.push("-mmacosx-version-min=" + project.minimumMacosVersion);
                if (product.qbs.toolchain.contains("xcode"))
                    cxxFlags.push("-isysroot", product.xcode.sdkPath);
            }
            if (product.qbs.targetOS.contains("unix"))
                cxxFlags.push("-fPIC");
            if (product.qbs.buildVariant === "release")
                cxxFlags.push(tc.contains("msvc") ? "/O2" : "-O3");
            if (cxxFlags.length > 0)
                args.push("--cxxflags=" + cxxFlags.join(" "));
            var ccOption = "--cc=";
            if (tc.contains("msvc"))
                ccOption += "msvc";
            else if (tc.contains("clang"))
                ccOption += "clang";
            else
                ccOption += "gcc";
            args.push(ccOption);
            if (!tc.contains("msvc"))
                args.push("--cc-bin=" + product.cpp.compilerPath);
            if (tc.contains("mingw")) {
                args.push("--os=mingw", "--without-stack-protector");
                if (product.qbs.targetOS.contains("windows"))
                    args.push("--link-method=hardlink");
            }
            var arch = product.qbs.architecture;
            if (arch == "x86" || arch == "x86_64")
                args.push("--cpu=" + arch);
            if (product.qbs.debugInformation)
                args.push("--debug-mode");
            var cmd = new Command("python", args);
            cmd.workingDirectory = product.buildDirectory;
            cmd.description = "Configuring Botan";
            return cmd;
        }
    }

    Rule {
        multiplex: true
        inputs: ["botan.Makefile", "botan.sources"]
        Artifact {
            filePath: (product.qbs.toolchain.contains("msvc") ? "botan" : "libbotan-2")
                      + product.cpp.staticLibrarySuffix;
            fileTags: "staticlibrary"
        }

        prepare: {
            var tc = product.moduleProperty("qbs", "toolchain");
            var make = "make";
            if (tc.contains("msvc")) {
                make = "nmake";
            } else if (tc.contains("mingw")
                     && product.moduleProperty("qbs", "hostOS").contains("windows")) {
                make = "mingw32-make";
            }
            var cmd = new Command(make, ["libs"]);
            cmd.workingDirectory = product.buildDirectory;
            cmd.description = "Building Botan";
            return cmd;
        }
    }

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: product.buildDirectory + "/build/include"
    }
}
