#!/usr/bin/env bash

## Open script-dir-homed subshell
(
ABS_SCRIPT_DIR=$(cd $(dirname $(which "$0")) && pwd)
cd "${ABS_SCRIPT_DIR}" || exit 1


## Internal config
KEEP_COUNT=7

## External config
DRY=0
case "$1" in
-n)
    DRY=1
    ;;
--just-print)
    DRY=1
    ;;
--dry-run)
    DRY=1
    ;;
esac

if [[ "$DRY" -eq 1 ]]; then
    echo 'NOTE: Running in simulation mode'
    echo
fi


function cleanup_snapshots {
    ## Harvest days
    LIST_OF_DAYS=
    for i in ????-??-?? ; do
        LIST_OF_DAYS="$LIST_OF_DAYS $i";
    done
    SORTED_DAYS=`echo ${LIST_OF_DAYS} | sed -r 's/ /\n/g' | sort | uniq`
    DAY_COUNT=`echo $SORTED_DAYS | wc -w`
    if [[ $DAY_COUNT < 1 ]]; then
        echo "No files deleted"
        exit 0
    fi

    ## Select days to delete
    DELETE_COUNT=$((DAY_COUNT - KEEP_COUNT))
    if [[ "${DELETE_COUNT}" -lt 1 ]]; then
        echo "No files deleted"
        return
    fi
    if [[ "${DELETE_COUNT}" -gt "${DAY_COUNT}" ]]; then
        DELETE_COUNT="${DAY_COUNT}"
    fi
    DELETE_DAYS=`echo ${SORTED_DAYS} | sed -r 's/ /\n/g' | head -n $DELETE_COUNT`
    echo "Deleting ${DELETE_COUNT} of ${DAY_COUNT} days"

    ## Delete days
    COUNTER=1
    for PRETTY_DAY in ${DELETE_DAYS} ; do
        echo "Day ${PRETTY_DAY} [${COUNTER}/${DELETE_COUNT}]"
        for j in ${PRETTY_DAY}/qtcreator-*-????????????-* ; do
            if [[ ! -f ${j} ]]; then
                continue
            fi

            echo "  ${j}"
            if [[ "$DRY" -eq 0 ]]; then
                ## Note: prefix for extra safety
                rm "../snapshots/${j}"
            fi
        done
        if [[ "$DRY" -eq 0 ]]; then
            ## Note: prefix for extra safety
            rmdir "../snapshots/${PRETTY_DAY}"
        fi
        COUNTER=$((COUNTER + 1))
    done
}


cleanup_snapshots
exit 0


## Properly close subshell
) 
exit $?
