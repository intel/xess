param(
[string]$version
)
$file = 'Meshconvert\meshconvert.rc'
$versionComma = $version.Replace(".", ",")
(Get-Content $file).replace('1,0,0,0', $versionComma).replace('1.0.0.0', $version) | Set-Content $file
(Get-Content $file)