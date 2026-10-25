#!/bin/sh
# Real-time physical MTD flash dumper redirector for Cisco WAP610N
# Redirects legacy CGI requests to native C streaming handler /goform/dumpFirmware
echo "Status: 302 Found"
echo "Location: /goform/dumpFirmware?${QUERY_STRING}"
echo ""
