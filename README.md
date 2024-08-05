# make_ext4fs

<!-- SPDX-License-Identifier: Apache-2.0 -->

The make_ext4fs utility can be used to create reproducible file-system images.

If a UUID is not specified on the command-line, then a UUIDv5 will be created,
based on the label, and thus will be the same every time.

An initial set of files can be included at creation time.

## Origins

The source code originated from the Android Open Source Project.
Later OpenWRT imported the sources and adapted them.

The OpenWRT sources can be found at: https://github.com/openwrt/make_ext4fs

This version here is maintained by Open Electronics Lab for reproducible builds.

## Changes in this fork

Noteworthy changes from the OpenWRT version are:

 * added a command-line `-u` option for specifying the UUID
 * default UUID generation now follows rfc9562 version 5 UUIDs
 * no longer requires a loopback device, can now target a file
 * some minor fixes, e.g.: fixed a memory leak that has been there since import
 * added a [ChangeLog](ChangeLog) (includes the excellent work from OpenWRT)
 * moved the sources into the [`src/`](src/) directory
 * added an acceptance test
 * added this README

## Building

Simply invoke `make`.

See: [`Makefile`](Makefile).

If set, the `BUILD_DIR` variable should be honoured.

## Testing

The script [`tests/build-and-test.sh`](tests/build-and-test.sh)
contains a basic acceptance test.

Because this test mounts a loopback device, `sudo` is invoked a few times.

The uses of `sudo` are intended to be clear and easy to review.

## Roadmap

Some ideas for future work include:

* add unit tests (refactor as needed to support this)
* write some documentation
* create separate build targets, e.g.: static, debug, coverage
* add code coverage reports
* consider rename more descriptive name (maybe `make-reproducible-ext4fs`)
* remove dead code in unused code-paths

## License and Copyright

The code is licensed as Apache License Version 2.0.

Most of the code is Copyright (c) 2010, The Android Open Source Project.

Considerable changes and contributions have come from additional individuals
including people who contribute to OpenWRT and the Open Electronics Lab.
The copyrights of these contributions belong to the contributors.

Information about significant contributions can be found in the ChangeLog.
