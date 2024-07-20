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

if [ -z "$BUILD_DIR" ]; then
	export BUILD_DIR=.
fi
mkdir -pv $BUILD_DIR

TEST_DIR=$( mktemp --tmpdir=$BUILD_DIR --directory test-XXXX )

export CFLAGS="-Wall -Wextra $CFLAGS"
make BUILD_DIR=$TEST_DIR

dd if=/dev/zero of=$TEST_DIR/blockfile bs=1M count=128

function setup-loopback() {
	DEV_LOOPX=$( sudo losetup -f )
	echo $DEV_LOOPX
	sudo losetup $DEV_LOOPX $TEST_DIR/blockfile
	if [[ -n "${VERBOSE}" && "${VERBOSE}" -gt 0 ]]; then
		losetup --json --list $DEV_LOOPX
	fi
}

setup-loopback
FS_TARGET=$DEV_LOOPX

function cleanup() {
	sudo umount -v $DEV_LOOPX || true
	sudo losetup -v -d $DEV_LOOPX || true
	sudo rm -vfr $TEST_DIR || true
	echo "cleanup complete"
}
trap cleanup EXIT


mkdir -pv $TEST_DIR/test-fs-files
echo "foo" > $TEST_DIR/test-fs-files/foo.txt

mkdir -pv $TEST_DIR/test-out

FS_EPOCH=1
sudo $TEST_DIR/make_ext4fs -v \
	-T $FS_EPOCH \
	-L test-fs-foo \
	-u "$UUID_IN" \
	$FS_TARGET \
	$TEST_DIR/test-fs-files \
	| tee $TEST_DIR/test-out/test-fs-foo.make_ext4fs.out
grep "Label: test-fs-foo" $TEST_DIR/test-out/test-fs-foo.make_ext4fs.out
grep "UUID: $UUID_OUT_EXPECTED" $TEST_DIR/test-out/test-fs-foo.make_ext4fs.out

mkdir -pv $TEST_DIR/mnt-test-fs-foo
sudo mount $DEV_LOOPX $TEST_DIR/mnt-test-fs-foo

sudo ls -l $TEST_DIR/mnt-test-fs-foo

ERRORS=0

sudo diff -u \
	$TEST_DIR/test-fs-files/foo.txt \
	$TEST_DIR/mnt-test-fs-foo/foo.txt \
	|| ERRORS=$(( 1 + $ERRORS ))

sudo blkid $DEV_LOOPX | tee $TEST_DIR/test-out/test-fs-foo.blkid.out
grep --only-matching "LABEL=\"test-fs-foo\"" \
	$TEST_DIR/test-out/test-fs-foo.blkid.out \
	|| ERRORS=$(( 1 + $ERRORS ))

grep --only-matching "UUID=\"${UUID_OUT_EXPECTED}\"" \
	$TEST_DIR/test-out/test-fs-foo.blkid.out \
	|| ERRORS=$(( 1 + $ERRORS ))

if [ $ERRORS -gt 255 ]; then ERRORS=255; fi
exit $ERRORS
