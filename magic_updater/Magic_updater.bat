echo off
echo Parsing Manalink.csv... >ERRORS.txt
echo. >>ERRORS.txt
csv2dat.exe >>ERRORS.txt
echo. >>ERRORS.txt
echo Parsing CT csv... >>ERRORS.txt
echo. >>ERRORS.txt
ct2exe ct_all.csv >>ERRORS.txt
copy *.dat updated_files
copy Magic.exe updated_files
del *.dat
