#  Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
#  SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import sys
import subprocess
import os

def main():
    if len(sys.argv) < 3:
        print("Usage: run_app.py <stamp_file> <executable> [args...]")
        return 1

    stamp_file = sys.argv[1]
    executable = sys.argv[2]
    app_args = sys.argv[3:]

    print(f"Executing: {executable} {' '.join(app_args)}")
    sys.stdout.flush()

    try:
        subprocess.check_call([executable] + app_args)
    except subprocess.CalledProcessError as e:
        print(f"Error running app: {e}")
        return e.returncode
    except Exception as e:
        print(f"Unexpected error: {e}")
        return 1

    # Touch the stamp file
    with open(stamp_file, 'w') as f:
        f.write("")

    return 0

if __name__ == '__main__':
    sys.exit(main())
