#!/bin/bash

if [[ -n "${VERBOSE}" && "${VERBOSE}" -gt 0 ]]; then
    set -x
fi
set -eo pipefail

if [ "_${1}" != "_" ]; then
	UUID_IN="$1"
	if [ "_${2}" != "_" ]; then
		UUID_OUT_EXPECTED="$2"
	else
		UUID_OUT_EXPECTED="$1"
	fi
else
	UUID_OUT_EXPECTED='4badc0de-00ab-cdef-0123-456789abcdef'
	UUID_MIXED_CASE='{ 4badc0de-00AB-CDEF-0123-456789abcdef }'
	UUID_IN="$UUID_MIXED_CASE"
fi

# export CFLAGS="-Wall -Wextra $CFLAGS"
make clean && make

dd if=/dev/zero of=blockfile bs=1M count=128
DEV_LOOPX=$( sudo losetup -f )
echo $DEV_LOOPX
sudo losetup $DEV_LOOPX blockfile

function cleanup() {
	sudo umount -v $DEV_LOOPX || true
	sudo losetup -v -d $DEV_LOOPX || true
	sudo rm -vfr \
		blockfile \
		test-fs-files \
		mnt-test-fs-foo \
		|| true
	echo "cleanup complete"
}
trap cleanup EXIT

if [[ -n "${VERBOSE}" && "${VERBOSE}" -gt 0 ]]; then
	losetup --json --list $DEV_LOOPX
fi

mkdir -pv test-fs-files
echo "foo" > test-fs-files/foo.txt

mkdir -pv test-out

FS_EPOCH=1
sudo ./make_ext4fs -v \
	-T $FS_EPOCH \
	-L test-fs-foo \
	-u "$UUID_IN" \
	$DEV_LOOPX \
	test-fs-files \
	| tee test-out/test-fs-foo.make_ext4fs.out
grep "Label: test-fs-foo" test-out/test-fs-foo.make_ext4fs.out
grep "UUID: $UUID_OUT_EXPECTED" test-out/test-fs-foo.make_ext4fs.out

mkdir -pv mnt-test-fs-foo
sudo mount $DEV_LOOPX mnt-test-fs-foo

sudo ls -l mnt-test-fs-foo

ERRORS=0

sudo diff -u \
	test-fs-files/foo.txt \
	mnt-test-fs-foo/foo.txt \
	|| ERRORS=$(( 1 + $ERRORS ))

sudo blkid $DEV_LOOPX | tee test-out/test-fs-foo.blkid.out
grep --only-matching "LABEL=\"test-fs-foo\"" \
	test-out/test-fs-foo.blkid.out \
	|| ERRORS=$(( 1 + $ERRORS ))

grep --only-matching "UUID=\"${UUID_OUT_EXPECTED}\"" \
	test-out/test-fs-foo.blkid.out \
	|| ERRORS=$(( 1 + $ERRORS ))

if [ $ERRORS -gt 255 ]; then ERRORS=255; fi
exit $ERRORS
