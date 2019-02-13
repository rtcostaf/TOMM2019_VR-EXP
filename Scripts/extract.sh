#!/bin/bash

OUTFILE="error1.txt"
DIR="/opt/vr/"
DATADIR="${DIR}/LOG/R20/error1/"
NETCONF="netconf5.txt"
TILE="12x4"
VIDEO="V2"
USER="U4"
REQMETHOD=1

for LOG in `ls -1 $DATADIR` ; do
        tiles720z1=`tail -17 ${DATADIR}/$LOG | head -1 |cut -d " " -f 2`
        tiles1080z1=`tail -16 ${DATADIR}/$LOG | head -1 |cut -d " " -f 2`
        tiles4kz1=`tail -15 ${DATADIR}/$LOG | head -1 |cut -d " " -f 2`
        tiles720z2=`tail -14 ${DATADIR}/$LOG | head -1 |cut -d " " -f 2`
        tiles1080z2=`tail -13 ${DATADIR}/$LOG | head -1 |cut -d " " -f 2`
        tiles4kz2=`tail -12 ${DATADIR}/$LOG | head -1 |cut -d " " -f 2`
        tiles720z3=`tail -11 ${DATADIR}/$LOG | head -1 |cut -d " " -f 2`
        tiles1080z3=`tail -10 ${DATADIR}/$LOG | head -1 |cut -d " " -f 2`
        tiles4kz3=`tail -9 ${DATADIR}/$LOG | head -1 |cut -d " " -f 2`
        qualityswitch1=`tail -5 ${DATADIR}/$LOG | head -1 |cut -d " " -f 2`
        qualityswitch2=`tail -4 ${DATADIR}/$LOG | head -1 |cut -d " " -f 2`
        qualityswitch3=`tail -3 ${DATADIR}/$LOG | head -1 |cut -d " " -f 2`
        stall=`tail -2 ${DATADIR}/$LOG | head -1 |cut -d " " -f 2`
        startuptime=`tail -1 ${DATADIR}/$LOG | head -1 |cut -d " " -f 2`
	ERR=$(echo $LOG |cut -d "_" -f 1)
        AUX=$(echo $LOG |cut -d "." -f 1)
        CONFa=$(echo $AUX | cut -d "_" -f 4)
	CONF=${CONFa:0:2}

        AUX2="@"
        AUX2+=$CONF
        AUX2+="@"
        bandwidth=`cat $NETCONF | grep $AUX2 | cut -d "_" -f 2`
        delay=`cat $NETCONF | grep $AUX2 | cut -d "_" -f 3`
        loss=`cat $NETCONF | grep $AUX2 | cut -d "_" -f 4`

        echo  ${VIDEO}_${USER}_${TILE}_${CONF}_${ERR}_${bandwidth}_${delay}_${loss}_${tiles720z1}_${tiles1080z1}_${tiles4kz1}_${tiles720z2}_${tiles1080z2}_${tiles4kz2}_${tiles720z3}_${tiles1080z3}_${tiles4kz3}_${qualityswitch1}_${qualityswitch2}_${qualityswitch3}_${stall}_${startuptime}_${REQMETHOD} >> $OUTFILE

done
