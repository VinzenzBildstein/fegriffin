#!/bin/bash

# keep starting the frontend if it crashes until it is stopped
iteration=0
until ./fedescant; do
	echo "$iteration: frontend crashed/was killed: $?"
	((iteration++))
# avoid constant trying to start it if it crashes during startup
	sleep 1
done
echo "$iteration restart(s) happened!"

