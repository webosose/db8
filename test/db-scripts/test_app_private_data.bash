#!/bin/bash

#-----------------------------------------------------------------------------------
# This script will test application private data add/remove to db using luna-send
#-----------------------------------------------------------------------------------

if [ $# -eq 0 ]; then
  echo "No arguments supplied. Please use: test_app_private_data db_name [sender_name]"
  echo "db_name : com.palm.db | com.palm.testdb | com.palm.mediadb"
  echo "sender_name : by default is com.palm.configurator"
  exit
fi

# init variables
TARGET_DB="com.palm.db"
USE_SENDER_NAME="com.palm.configurator"

if [ $# -gt 0 ]; then
  TARGET_DB=$1
fi

if [ $# -gt 1 ]; then
  USE_SENDER_NAME=$2
fi

#print used vars
echo "Target db:" $TARGET_DB
echo "used sender name:" $USE_SENDER_NAME

LUNA_REPLY_TEXT=""
GREP_OK_TEXT=""

echo "- add new kind"
LUNA_REPLY_TEXT=`luna-send -n 1 -a ${USE_SENDER_NAME} luna://${TARGET_DB}/putKind '{"id":"com.palm.test:1","owner":"'"$USE_SENDER_NAME"'","private":true,"indexes":[{"name":"foo", "props":[{"name":"foo"}]}]}'`
GREP_OK_TEXT=`echo "'$LUNA_REPLY_TEXT'" | grep "errorCode"`
if [ ${#GREP_OK_TEXT} -ne 0 ]; then
    echo $LUNA_REPLY_TEXT
    return 1
fi

echo "- put test data"
LUNA_REPLY_TEXT=`luna-send -n 1 -a ${USE_SENDER_NAME} luna://${TARGET_DB}/put '{"objects":[{"_kind":"com.palm.test:1","foo":10}]}'`
GREP_OK_TEXT=`echo "'$LUNA_REPLY_TEXT'" | grep "errorCode"`
if [ ${#GREP_OK_TEXT} -ne 0 ]; then
    echo $LUNA_REPLY_TEXT
    return 1
fi

echo "- add extended kind"
LUNA_REPLY_TEXT=`luna-send -n 1 -a ${USE_SENDER_NAME} luna://${TARGET_DB}/putKind '{"id":"com.palm.child:1","owner":"'$USE_SENDER_NAME'","extends":["com.palm.test:1"]}'`
GREP_OK_TEXT=`echo "'$LUNA_REPLY_TEXT'" | grep "errorCode"`
if [ ${#GREP_OK_TEXT} -ne 0 ]; then
    echo $LUNA_REPLY_TEXT
    return 1
fi

echo "- delete data by owner"
LUNA_REPLY_TEXT=`luna-send -n 1 -a ${USE_SENDER_NAME} luna://${TARGET_DB}/removeAppData '{"owners":["'$USE_SENDER_NAME'"]}'`
GREP_OK_TEXT=`echo "'$LUNA_REPLY_TEXT'" | grep "errorCode"`
if [ ${#GREP_OK_TEXT} -ne 0 ]; then
    echo $LUNA_REPLY_TEXT
    return 1
fi

echo "- verification: get data"
LUNA_REPLY_TEXT=`luna-send -n 1 -a ${USE_SENDER_NAME} luna://${TARGET_DB}/search '{"query":{"from":"com.palm.test:1"}}'`
GREP_OK_TEXT=`echo "'$LUNA_REPLY_TEXT'" | grep "errorCode"`
if [ ${#GREP_OK_TEXT} -eq 0 ]; then
    echo $LUNA_REPLY_TEXT
    echo "Test failed"
    return 1
fi

echo "Test completed successfully"
return 0

