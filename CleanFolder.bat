@echo off 
@rem Сохранение в переменной вывода программы	(К сожаление, передача вывода программ на вход команды set не работает. Поэтому можно воспользоваться временным сохранением в файл.)
dir /b *.aps >%temp%\0.tmp
set /p FILENAME="" <%temp%\0.tmp
del %temp%\0.tmp
set FILENAME=%FILENAME:~0,-4%
rem echo %FILENAME%
@echo on

del AvrBuild.bat
del labels.tmp
del %FILENAME%.aws
del %FILENAME%.hex
del %FILENAME%.map
del %FILENAME%.obj