#!/bin/bash

CDN=$1

if [ ! -n "$CDN" ]
then
   echo "Missing CDN ID"
   exit 0
fi

if [ ! -e "/var/lock/apache2/$CDN" ]
then
  mkdir -p /var/lock/apache2/$CDN
fi

export APACHE_LOCK_DIR=/var/lock/apache2/$CDN
export APACHE_RUN_USER=www-data
export APACHE_RUN_GROUP=www-data

if [ ! -e "/var/log/apache2/$CDN" ]
then
  mkdir -p /var/log/apache2/$CDN
fi

export APACHE_LOG_DIR=/var/log/apache2/$CDN

if [ ! -e "/var/run/apache2/$CDN" ]
then
  mkdir -p /var/run/apache2/$CDN
fi

export APACHE_PID_FILE=/var/run/apache2/$CDN/apache2.pid
export APACHE_RUN_DIR=/var/run/apache2/$CDN
apache2 -k stop
apache2 -k start