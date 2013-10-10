#!/bin/bash

if [ $# -ge 1 ];
then
    echo "** testing PGAS apps"
    suffix=pgas
fi;

lo_cor=3;
hi_cor=4;

echo "./run $lo_cor bmarks/bank$suffix -a32 -c50 -d1 | grep  \")))\" "
./run $lo_cor bmarks/bank$suffix -a32 -c50 -d1 | grep  ")))"
echo "./run $hi_cor bmarks/bank$suffix -a32 -c50 -d1 | grep  \")))\" "
./run $hi_cor bmarks/bank$suffix -a32 -c50 -d1 | grep  ")))"
echo "./run $lo_cor bmarks/bank$suffix -d1 | grep  \")))\" "
./run $lo_cor bmarks/bank$suffix -d1 | grep  ")))"
echo "./run $hi_cor bmarks/bank$suffix -d1 | grep  \")))\" "
./run $hi_cor bmarks/bank$suffix -d1 | grep  ")))"

echo "./run $lo_cor bmarks/mbll$suffix -u10 -i32 -r64 -d1 | grep  \")))\" "
./run $lo_cor bmarks/mbll$suffix -u10 -i32 -r64 -d1 | grep  ")))"
echo "./run $hi_cor bmarks/mbll$suffix -u10 -i32 -r64 -d1 | grep  \")))\" "
./run $hi_cor bmarks/mbll$suffix -u10 -i32 -r64 -d1 | grep  ")))"
echo "./run $lo_cor bmarks/mbll$suffix -u10 -i1024 -r2048 -d1 | grep  \")))\" "
./run $lo_cor bmarks/mbll$suffix -u10 -i1024 -r2048 -d1 | grep  ")))"
echo "./run $hi_cor bmarks/mbll$suffix -u10 -i1024 -r2048 -d1 | grep  \")))\" "
./run $hi_cor bmarks/mbll$suffix -u10 -i1024 -r2048 -d1 | grep  ")))"

echo "./run $lo_cor bmarks/mbht$suffix -u10 -i32 -r64 -d1 | grep  \")))\" "
./run $lo_cor bmarks/mbht$suffix -u10 -i32 -r64 -d1 | grep  ")))"
echo "./run $hi_cor bmarks/mbht$suffix -u10 -i32 -r64 -d1 | grep  \")))\" "
./run $hi_cor bmarks/mbht$suffix -u10 -i32 -r64 -d1 | grep  ")))"
echo "./run $lo_cor bmarks/mbht$suffix -u10 -i1024 -r2048 -d1 | grep  \")))\" "
./run $lo_cor bmarks/mbht$suffix -u10 -i1024 -r2048 -d1 | grep  ")))"
echo "./run $hi_cor bmarks/mbht$suffix -u10 -i1024 -r2048 -d1 | grep  \")))\" "
./run $hi_cor bmarks/mbht$suffix -u10 -i1024 -r2048 -d1 | grep  ")))"
echo "./run $lo_cor bmarks/mbht$suffix -u10 -i32 -r64 -l2 -d1 | grep  \")))\" "
./run $lo_cor bmarks/mbht$suffix -u10 -i32 -r64 -l2 -d1 | grep  ")))"
echo "./run $hi_cor bmarks/mbht$suffix -u10 -i32 -r64 -l2 -d1 | grep  \")))\" "
./run $hi_cor bmarks/mbht$suffix -u10 -i32 -r64 -l2 -d1 | grep  ")))"
echo "./run $lo_cor bmarks/mbht$suffix -u10 -i1024 -r2048 -l2  -d1 | grep  \")))\" "
./run $lo_cor bmarks/mbht$suffix -u10 -i1024 -r2048 -l2  -d1 | grep  ")))"
echo "./run $hi_cor bmarks/mbht$suffix -u10  -i1024 -r2048 -l2 -d1 | grep  \")))\" "
./run $hi_cor bmarks/mbht$suffix -u10  -i1024 -r2048 -l2 -d1 | grep  ")))"
