#!/bin/bash

# Main parameters
DIR="/opt/vr/"
NETCONFDIR="${DIR}/netconf6/"
MININETDIR="${DIR}/mininet/util/"
TRACEDIR="${DIR}/viewport_traces_roberto/per_user_roberto/"
TRACEUSER="user_1"
VIDEO="video_2"
NUMBERSEGMENTS=60
TIMEOUT=100
TILE_V=4
TILE_H=12
SERVER_IP="10.0.0.251"
SOURCE="u001"

for CONFIG in `ls -1 ${NETCONFDIR}` ; do

	# Kill mininet (if running)
	for PID in $(ps -ef | grep "Gent_" | awk '{print $2}'); do kill -9 $PID; done

	# Flush mininet configuration
	/usr/bin/mn -c

	# Run mininet topology
	${NETCONFDIR}$CONFIG &
	
	# Wait for mininet to start
	sleep 10s

	# Kill apache 
	/usr/bin/killall -2 apache2

	# HTTP up on CDN node
	${MININETDIR}.m cdn1 bash /home/rtcosta/start_apache.sh cdn1
	sleep 3s

	for ERROR in `seq 0 25 100` ; do
		# Run player
		${MININETDIR}/m ${SOURCE} VREXP_player ${SERVER_IP} ${VIDEO} ${NUMBERSEGMENTS} ${ERROR}_${CONFIG} ${TRACEDIR}/${TRACEUSER}/${VIDEO} ${TIMEOUT} ${TILE_V} ${TILE_H} ${ERROR}
	done
done

