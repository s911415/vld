@echo off
taskkill /im php.exe /f
rem copy /y %~1 %PHP_PATH%\ext
rem copy /y %~dpn1.pdb %PHP_PATH%\ext\
rem @del /f /q %PHP_PATH%\ext\%~n1.dll
rem @del /f /q %PHP_PATH%\ext\%~n1.pdb
rem mklink %PHP_PATH%\ext\%~n1.dll %~1
rem mklink %PHP_PATH%\ext\%~n1.pdb %~dpn1.pdb
rem %PHP_PATH%\php.exe -dzend_extension=sense_ext -dzend_extension=xdebug-2.8.0beta2-7.3-vc15-x86_64 -S %URI%
rem %PHP_PATH%\php.exe -dzend_extension=sense_ext "%2"
rem call vld-sense "%2"
rem %PHP_PATH%\php.exe -dzend_extension=sense_ext -dzend_extension=vld -dvld.active=1 -dvld.execute=0 handle_case\class.php
echo %1
%PHP_PATH%\php.exe -dextension=%1 -dvld.active=1 -dopcache.enable_cli=0 -dopcache.enable=0 -dvld.execute=0 %2

pause
