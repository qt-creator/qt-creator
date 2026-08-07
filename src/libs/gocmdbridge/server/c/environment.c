// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Environment command handler.
// Included by cmdbridge.c -- do not compile separately.

/* ================================================================== */
/*  Environment command                                               */
/* ================================================================== */

static void h_environment(value *cmd)
{
#ifdef _WIN32
    const char *ostype = "windows";
#else
    const char *ostype = "unix";
#endif

    /* Count environment variables */
    int env_count = 0;
    if (environ) {
        while (environ[env_count])
            env_count++;
    }

    /* Build Env array */
    value *env_arr = vnew_x();
    env_arr->type = V_ARRAY;
    if (env_count > 0) {
        env_arr->kids = (value **) xmalloc((size_t) env_count * sizeof(value *));
        for (int i = 0; i < env_count; i++)
            env_arr->kids[i] = vs(environ[i] ? environ[i] : "");
    }
    env_arr->nkids = (size_t) env_count;

    /* Build result map: Type, Id, OsType, Env[] */
    value *result = mk4(
        "Type", vs("environment"), "Id", vi(mkey(cmd, "Id")), "OsType", vs(ostype), "Env", env_arr);

    size_t l;
    uint8_t *c = encode(result, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    vfree(result);
}
