echo off
nestcmd -s -w *.il? echo $$ ; del $
nestcmd -s -w *.tds echo $$ ; del $
nestcmd -s -w *.db echo $$ ; del $
nestcmd -s -w *.dsk echo $$ ; del $
nestcmd -s -w *.~* echo $$ ; del $
nestcmd -s -w *.obj echo $$ ; del $
nestcmd -s -w *.$$$ echo $$ ; del $
nestcmd -s -w *.pch echo $$ ; del $
nestcmd -s -w *.pdb echo $$ ; del $
nestcmd -s -w *.ncb echo $$ ; del $
nestcmd -s -w *.iobj echo $$ ; del $
nestcmd -s -w *.idb echo $$ ; del $
nestcmd -s -w *.ipch echo $$ ; del $
nestcmd -s -w *.ipdb echo $$ ; del $
nestcmd -s -w *.log echo $$ ; del $
nestcmd -s -w *.tlog echo $$ ; del $
nestcmd -s -w *.lastbuildstate echo $$ ; del $
nestcmd -s -w *.sdf echo $$ ; del $
nestcmd -s -H -w *.suo echo $$ ; attrib -H $ ; del $
nestcmd -s -S -H -w Thumbs.db echo $$ ; attrib -H -S $ ; del $
nestcmd -s -w *.aps echo $$ ; del $
nestcmd -s -w *.user echo $$ ; del $
nestcmd -s -w desktop.ini echo $$ ; del $
rem nestcmd -s -w *.exe echo $$ ; del $
rem nestcmd -s -w *.gie echo $$ ; del $
