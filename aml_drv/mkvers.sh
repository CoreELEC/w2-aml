#!/bin/bash
#
# Outputs svn revision of directory $VERDIR into $TARGET if it changes $TARGET
# example output omitting enclosing quotes:
#
# '#define AML_VERS_REV "svn1676M"'
# '#define AML_VERS_MOD "vX.X.X.X"'
# '#define AML_VERS_OTH "build: username date hour"'

set -e

export PATH=$PATH:/usr/bin

VERDIR=$(dirname $(readlink -f $0))
TARGET=$1

DATE_FORMAT1="%b %d %Y %T"
DATE_FORMAT="W2.%Y.W%02W.%02w"
AML_VERS_MOD=$(grep AML_VERS_NUM $VERDIR/Makefile | cut -f2 -d= | sed 's/\s\+$//')
tmpout=$TARGET.tmp
cd $VERDIR

if [[ ! -d ../firmware && -f $TARGET ]];then
	exit 0
fi

if (svn info . >& /dev/null)
then
    svnrev=svn$(svnversion -c | sed 's/.*://')
elif (git status -uno >& /dev/null)
then
    # If git-svn find the corresponding svn revision
    if (git svn info >& /dev/null)
    then
	idx=0
	while [ ! "$svnrev" ]
	do
	    # loop on all commit to find the first one that match a svn revision
	    svnrev=$(git svn find-rev HEAD~$idx)
	    if [ $? -ne 0 ]
	    then
		break;
	    elif [ ! "$svnrev" ]
	    then
		idx=$((idx + 1))
	    elif [ $idx -gt 0 ] || (git status -uno --porcelain | grep -q '^ M')
	    then
		# If this is not the HEAD, or working copy is not clean then we're
		# not at a commited svn revision so add 'M'
		svnrev=$svnrev"M"
	    fi
	done
    fi

    # append git info (sha1 and branch name)
    git_sha1=$(git rev-parse --short HEAD)
    #if (git status -uno --porcelain | grep -q '^ M')
    #then
    #    git_sha1+="M"
    #fi
    git_branch=$(git symbolic-ref --short HEAD 2>/dev/null || echo "detached")
    if [ "$svnrev" ]
    then
    	svnrev="svn$svnrev ($git_sha1)"
    else
    	svnrev="$git_sha1"
    fi
    drv_hash=$(git log -1 | awk 'NR==1{print}' | awk -F" " '{print $2}')
    fw_hash=$(git log -1 ../firmware | awk 'NR==1{print}' | awk -F" " '{print $2}')
    common_hash=$(git log -1 ../common | awk 'NR==1{print}' | awk -F" " '{print $2}')

    if [ -f "../firmware/wifi_w2_fw_pcie.bin" ]
    then
        fw_date=$(stat ../firmware/wifi_w2_fw_pcie.bin | grep -w "Modify" | awk '{print $2 " " $3}' | cut -d . -f 1)
        fw_size=$(stat ../firmware/wifi_w2_fw_pcie.bin | grep -w "Size" | awk '{print $2}')
    elif [ -f "../firmware/wifi_w2_fw_usb.bin" ]
    then
        fw_date=$(stat ../firmware/wifi_w2_fw_usb.bin | grep -w "Modify" | awk '{print $2 " " $3}' | cut -d . -f 1)
        fw_size=$(stat ../firmware/wifi_w2_fw_usb.bin | grep -w "Size" | awk '{print $2}')
    elif [ -f "../firmware/wifi_w2_fw_sdio.bin" ]
    then
        fw_date=$(stat ../firmware/wifi_w2_fw_sdio.bin | grep -w "Modify" | awk '{print $2 " " $3}' | cut -d . -f 1)
        fw_size=$(stat ../firmware/wifi_w2_fw_sdio.bin | grep -w "Size" | awk '{print $2}')
    else
        echo "cannot stat wifi_w2_fw bin : No such file or directory"
    fi

    common_date_year=$(git log -1 ../common | awk 'NR==3{print}' | awk -F" " '{print $6}')
    common_date_mon=$(git log -1 ../common | awk 'NR==3{print}' | awk -F" " '{print $3}')
    common_date_day=$(git log -1 ../common | awk 'NR==3{print}' | awk -F" " '{print $4}')
    common_date_time=$(git log -1 ../common | awk 'NR==3{print}' | awk -F" " '{print $5}')
else
    svnrev="Unknown Revision"
fi

date=$(LC_TIME=C date +"$DATE_FORMAT")
date1=$(LC_TIME=C date +"$DATE_FORMAT1")

AML_VERS_REV="$svnrev"
#      "lmac vX.X.X.X - build:"
banner="$date ($date1 - driver:$AML_VERS_REV)"

define() { echo "#define $1 \"$2\""; }
{
	define "AML_VERS_REV"    "$AML_VERS_REV"
	define "AML_VERS_MOD"    "$AML_VERS_MOD"
	define "AML_VERS_BANNER" "$banner"
        define "AML_DRIVER_COMPILE_INFO" "driver compile date: $date,driver hash: $drv_hash"
        define "FIRMWARE_INFO" "fw compile date: $fw_date,fw hash: $fw_hash,fw size: $fw_size"
        define "COMMON_INFO" "common: last commit: $common_date_year/$common_date_mon/$common_date_day $common_date_time  hash: $common_hash"
} > $tmpout




cmp -s $TARGET $tmpout && rm -f $tmpout || mv $tmpout $TARGET
