$Icons = "$PSScriptRoot\..\data\icons"
$sizes = @("16", "24", "32", "48", "256")
$paths = @()

convert -background none $Icons\appicon-win32.svg $Icons\appicon-win32-mask.svg -alpha Off -compose CopyOpacity -composite $Icons\png\win32.png
foreach($size in $sizes) {
    $paths += "$Icons\png\win32-$size.png"
    convert -background none $Icons\png\win32.png -resize "${size}x${size}" -depth 32 "$Icons\png\win32-$size.png"
}

convert $paths "$Icons\win32.ico"
