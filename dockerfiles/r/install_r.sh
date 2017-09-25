#!/bin/bash
pushd /opt/r
mv R /usr/lib64
popd

pushd /opt/ds
rpm2cpio DataScienceR-*.x86_64.rpm | cpio -di
mv temp/ext/DataScienceR/* ./
popd

source /opt/r_env.sh
R --version
