set filesCount=0 & for %%f in (*) do @(set /a filesCount+=1 > nul)
echo %filesCount% 
if %filesCount%==8 (exit /b)  
del fw.bin
set filename="" & for /r %%i in (*.bin) do @(set "filename=%%i")
echo %filename%
ren "%filename%" fw.bin
set filename="" & for /r %%i in (*.txt) do @(set "filename=%%i")
echo %filename%
Type Nul > bin_version.temp
for /f "delims=" %%x in (bin_version.txt) do set Build=%%x
set /a Build= %Build%+1
echo %Build% >> bin_version.temp
set filename="" & for /r %%i in (*.temp) do @(set "filename=%%i")
echo %filename% 
del bin_version.txt
ren "%filename%" bin_version.txt

call git add *
call git commit -m "version nueva numero:%Build%"
call git push 

