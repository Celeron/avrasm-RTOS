@echo off 
@rem ���������� � ���������� ������ ���������	(� ���������, �������� ������ �������� �� ���� ������� set �� ��������. ������� ����� ��������������� ��������� ����������� � ����.)
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