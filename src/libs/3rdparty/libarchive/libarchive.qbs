Project {
    StaticLibrary {
        name: "bundledLibArchive"
        builtByDefault: false

        Depends { name: "cpp" }
        Depends { name: "qtcZLib" }
        Depends { name: "qtcBZip2" }
        Depends { name: "qtcXz" }

        Group {
            name: "common"
            prefix: "libarchive/"
            files: [
                "archive.h",
                "archive_acl.c",
                "archive_acl_private.h",
                "archive_blake2s_ref.c",
                "archive_blake2sp_ref.c",
                "archive_check_magic.c",
                "archive_cmdline.c",
                "archive_cmdline_private.h",
                "archive_crc32.h",
                "archive_cryptor.c",
                "archive_cryptor_private.h",
                "archive_digest.c",
                "archive_digest_private.h",
                "archive_endian.h",
                "archive_entry.c",
                "archive_entry.h",
                "archive_entry_copy_stat.c",
                "archive_entry_link_resolver.c",
                "archive_entry_locale.h",
                "archive_entry_private.h",
                "archive_entry_sparse.c",
                "archive_entry_stat.c",
                "archive_entry_strmode.c",
                "archive_entry_xattr.c",
                "archive_getdate.c",
                "archive_getdate.h",
                "archive_hmac.c",
                "archive_hmac_private.h",
                "archive_match.c",
                "archive_openssl_evp_private.h",
                "archive_openssl_hmac_private.h",
                "archive_options.c",
                "archive_options_private.h",
                "archive_pack_dev.c",
                "archive_pack_dev.h",
                "archive_pathmatch.c",
                "archive_pathmatch.h",
                "archive_platform.h",
                "archive_platform_acl.h",
                "archive_platform_xattr.h",
                "archive_ppmd7.c",
                "archive_ppmd7_private.h",
                "archive_ppmd8.c",
                "archive_ppmd8_private.h",
                "archive_ppmd_private.h",
                "archive_private.h",
                "archive_random.c",
                "archive_random_private.h",
                "archive_rb.c",
                "archive_rb.h",
                "archive_read.c",
                "archive_read_add_passphrase.c",
                "archive_read_append_filter.c",
                "archive_read_data_into_fd.c",
                "archive_read_disk_entry_from_file.c",
                "archive_read_disk_posix.c",
                "archive_read_disk_private.h",
                "archive_read_disk_set_standard_lookup.c",
                "archive_read_extract.c",
                "archive_read_extract2.c",
                "archive_read_open_fd.c",
                "archive_read_open_file.c",
                "archive_read_open_filename.c",
                "archive_read_open_memory.c",
                "archive_read_private.h",
                "archive_read_set_format.c",
                "archive_read_set_options.c",
                "archive_read_support_filter_all.c",
                "archive_read_support_filter_by_code.c",
                "archive_read_support_filter_bzip2.c",
                "archive_read_support_filter_compress.c",
                "archive_read_support_filter_grzip.c",
                "archive_read_support_filter_gzip.c",
                "archive_read_support_filter_lrzip.c",
                "archive_read_support_filter_lz4.c",
                "archive_read_support_filter_lzop.c",
                "archive_read_support_filter_none.c",
                "archive_read_support_filter_program.c",
                "archive_read_support_filter_rpm.c",
                "archive_read_support_filter_uu.c",
                "archive_read_support_filter_xz.c",
                "archive_read_support_filter_zstd.c",
                "archive_read_support_format_7zip.c",
                "archive_read_support_format_all.c",
                "archive_read_support_format_ar.c",
                "archive_read_support_format_by_code.c",
                "archive_read_support_format_cab.c",
                "archive_read_support_format_cpio.c",
                "archive_read_support_format_empty.c",
                "archive_read_support_format_iso9660.c",
                "archive_read_support_format_lha.c",
                "archive_read_support_format_mtree.c",
                "archive_read_support_format_rar5.c",
                "archive_read_support_format_raw.c",
                "archive_read_support_format_tar.c",
                "archive_read_support_format_warc.c",
                "archive_read_support_format_zip.c",
                "archive_string.c",
                "archive_string.h",
                "archive_string_composition.h",
                "archive_string_sprintf.c",
                "archive_util.c",
                "archive_version_details.c",
                "archive_virtual.c",
                "archive_write.c",
                "archive_write_add_filter.c",
                "archive_write_add_filter_b64encode.c",
                "archive_write_add_filter_by_name.c",
                "archive_write_add_filter_bzip2.c",
                "archive_write_add_filter_compress.c",
                "archive_write_add_filter_grzip.c",
                "archive_write_add_filter_gzip.c",
                "archive_write_add_filter_lrzip.c",
                "archive_write_add_filter_lz4.c",
                "archive_write_add_filter_lzop.c",
                "archive_write_add_filter_none.c",
                "archive_write_add_filter_program.c",
                "archive_write_add_filter_uuencode.c",
                "archive_write_add_filter_xz.c",
                "archive_write_add_filter_zstd.c",
                "archive_write_disk_posix.c",
                "archive_write_disk_private.h",
                "archive_write_disk_set_standard_lookup.c",
                "archive_write_open_fd.c",
                "archive_write_open_file.c",
                "archive_write_open_filename.c",
                "archive_write_open_memory.c",
                "archive_write_private.h",
                "archive_write_set_format.c",
                "archive_write_set_format_7zip.c",
                "archive_write_set_format_ar.c",
                "archive_write_set_format_by_name.c",
                "archive_write_set_format_cpio.c",
                "archive_write_set_format_cpio_binary.c",
                "archive_write_set_format_cpio_newc.c",
                "archive_write_set_format_cpio_odc.c",
                "archive_write_set_format_filter_by_ext.c",
                "archive_write_set_format_gnutar.c",
                "archive_write_set_format_iso9660.c",
                "archive_write_set_format_mtree.c",
                "archive_write_set_format_pax.c",
                "archive_write_set_format_private.h",
                "archive_write_set_format_raw.c",
                "archive_write_set_format_shar.c",
                "archive_write_set_format_ustar.c",
                "archive_write_set_format_v7tar.c",
                "archive_write_set_format_warc.c",
                "archive_write_set_format_zip.c",
                "archive_write_set_options.c",
                "archive_write_set_passphrase.c",
                "archive_xxhash.h",
                "filter_fork.h",
                "filter_fork_posix.c",
                "xxhash.c",
            ]
        }

        Group {
            name: "Windows-specific"
            condition: qbs.targetOS.contains("windows")
            prefix: "libarchive/"
            files: [
                "archive_entry_copy_bhfi.c",
                "archive_read_disk_windows.c",
                "archive_windows.c",
                "archive_windows.h",
                "archive_write_disk_windows.c",
                "filter_fork_windows.c",
            ]
        }

        Group {
            name: "darwin-specific"
            condition: qbs.targetOS.contains("darwin")
            product.cpp.defines: ["ARCHIVE_ACL_DARWIN=1"]
            files: "libarchive/archive_disk_acl_darwin.c"
        }

        Group {
            name: "FreeBSD-specific"
            condition: qbs.targetOS.contains("freebsd")
            files: "libarchive/archive_disk_acl_freebsd.c"
        }

        Group {
            name: "Linux-specific"
            condition: qbs.targetOS.contains("linux")
            product.cpp.defines: ["ARCHIVE_ACL_LIBACL=1", "HAVE_ACL_LIBACL_H=1", "HAVE_SYS_ACL_H=1"]
            product.cpp.dynamicLibraries: "acl"
            files: "libarchive/archive_disk_acl_linux.c"
        }

        files: configFileName

        property string configFileName
        Properties {
            cpp.defines: [
                'PLATFORM_CONFIG_H="' + sourceDirectory + '/' + configFileName + '"',
                "HAVE_ZLIB_H=1",
                "HAVE_LIBZ=1",
                "HAVE_BZLIB_H=1",
                "HAVE_LIBBZ2=1",
                "USE_BZIP2_STATIC",
                "HAVE_LZMA_H=1",
                "HAVE_LIBLZMA=1",
                "LZMA_API_STATIC",
                "LIBARCHIVE_STATIC",
            ]
        }
        Properties {
            condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("darwin")
            configFileName: "config_unix.h"
        }
        Properties {
            condition: qbs.toolchain.contains("mingw")
            configFileName: "config_mingw.h"
        }
        Properties {
            condition: qbs.toolchain.contains("msvc")
            configFileName: "config_win.h"
            cpp.cFlags: "/wd4996"
        }
        Properties {
            condition: qbs.targetOS.contains("darwin")
            configFileName: "config_mac.h"
        }
        Properties {
            condition: !qbs.targetOS.contains("windows")
            cpp.dynamicLibraries: "iconv"
        }
        cpp.includePaths: "."

        Export {
            Depends { name: "cpp" }
            cpp.defines: "LIBARCHIVE_STATIC"
            cpp.includePaths: exportingProduct.sourceDirectory + "/libarchive"
        }
    }

    Product {
        name: "qtcLibArchive"

        property bool preferExternal: true
        property bool libArchiveFound: preferExternal && libarchive_static.present

        Depends { name: "libarchive_static"; condition: preferExternal; required: false }

        Export {
            Depends { name: "cpp" }
            Depends { name: "bundledLibArchive"; condition: !exportingProduct.libArchiveFound }
            Properties {
                condition: exportingProduct.libArchiveFound
                cpp.includePaths: exportingProduct.libarchive_static.libarchiveIncludeDir
                cpp.libraryPaths: exportingProduct.libarchive_static.libarchiveLibDir
                cpp.staticLibraries: exportingProduct.libarchive_static.libarchiveStatic
                                     ? exportingProduct.libarchive_static.libarchiveNames : []
                cpp.dynamicLibraries: {
                    var libs = [];
                    if (!exportingProduct.libarchive_static.libarchiveStatic)
                        libs = libs.concat(exportingProduct.libarchive_static.libarchiveNames);
                    if (qbs.toolchain.contains("mingw"))
                        libs.push("bcrypt");
                    return libs;
                }
            }
        }
    }
}
