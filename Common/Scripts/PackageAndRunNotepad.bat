set oldCd=%cd%

pushd C:\WM_Studio\VS2015\Build\x86\bin
../../../Common/Scripts/PackageAndRun.bat

pushd %oldCd%