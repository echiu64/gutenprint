#!/bin/sh

set -e

DEBVERSION=`cat /etc/debian_version`

case $DEBVERSION in
       2.2)
            make ghost
            mkdir -p deb
            cd deb
            apt-get source gs libjpeg62
            mv libjpeg6b-6b libjpeg
            patch -p4 < ../Ghost/debian-patch
            cd gs-5.10
            cp ../../Ghost/debian-patch-stp debian/patches/stp
            mkdir -p contrib/stp
            cp ../../Ghost/README contrib/stp/README.stp
            cp ../../Ghost/gdevstp* contrib/stp
            cp ../../Ghost/devs.mak.addon-5.10 contrib/stp
            fakeroot dpkg-buildpackage
            ;;
       *)
            echo "Debian release $DEBVERSION not yet supported."
            exit 1
            ;;
esac

exit 0
