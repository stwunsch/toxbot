Toxbot
======

This chatbot accepts friend requests and invites friends to a groupchat. It logs the groupchat and sends the log on command to a friend.

Inbuild commands:  
'help' : display commands  
'invite' : invites to groupchat  
'log' : sends log  
'log n' : sends log with n lines

**Install guide**  
Change to any folder in your home directory and enter following commands in your terminal.

`git clone https://github.com/stwunsch/toxbot.git` // clone this repository  
`cd toxbot/`  
`mkdir build` // make build folder  
`cd build/`  
`cmake ../` // build makefiles  
`make` // build toxbot  
`./Toxbot` // run toxbot

