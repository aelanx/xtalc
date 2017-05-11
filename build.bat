@echo off

cd "%~dp0"

msbuild /p:OutDir=bin\ /p:IntDir=bin\ xtalc.vcxproj

rem bin\xtalc -o other_option.xtal other_option.xt
