#!/bin/bash 

# open browser
URL=http://localhost:8001
[[ -x $BROWSER ]] && exec "$BROWSER" "$URL"
path=$(which xdg-open || which gnome-open) && exec "$path" "$URL" &
 
# launch backend
node visualization/app.js 
