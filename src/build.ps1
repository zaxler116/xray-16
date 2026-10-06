Stop-Process -Name cl, MSBuild -Force -ErrorAction SilentlyContinue
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'C:\DEV\code\personal\xray-16\src\engine.sln' -t:xrGame -p:Configuration=Release -p:Platform=x64 -m -nologo -v:minimal -clp:ErrorsOnly
exit $LASTEXITCODE
