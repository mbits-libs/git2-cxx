# Copyright (c) 2022 Marcin Zdun
# This code is licensed under MIT license (see LICENSE for details)

$Icons = Resolve-Path "$PSScriptRoot\..\..\data\icons"
$sizes = @("16", "24", "32", "48", "256")

$null = New-Item -Type Directory -Force "$Icons\png"

convert -background none `
    $Icons\appicon-win32.svg `
    $Icons\appicon-win32-mask.svg `
    -alpha Off -compose CopyOpacity `
    -composite $Icons\png\win32.png

$paths = @()
foreach($size in $sizes) {
    $paths += "$Icons\png\win32-$size.png"
    convert -background none `
        $Icons\png\win32.png `
        -resize "${size}x${size}" -depth 32 `
        "$Icons\png\win32-$size.png"
}

convert $paths "$Icons\win32.ico"

convert -background none `
    $Icons\plugin-win32.svg `
    $Icons\appicon-win32-mask.svg `
    -alpha Off -compose CopyOpacity `
    -composite $Icons\png\plugin.png

$paths = @()
foreach($size in $sizes) {
    $paths += "$Icons\png\plugin-$size.png"
    convert -background none `
        $Icons\png\plugin.png `
        -resize "${size}x${size}" -depth 32 `
        "$Icons\png\plugin-$size.png"
}

convert $paths "$Icons\plugin.ico"
