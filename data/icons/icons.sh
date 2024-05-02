#!/bin/sh

DIR=$(dirname $0)
DST="$(dirname $DIR)/html/img"

mkdir -p "$DIR/png"
mkdir -p "$DST/dark"
mkdir -p "$DST/light"

for MODE in light dark
do
    echo $DST/$MODE/favicon.png
    convert -background none "$DIR/$MODE/favicon.svg" "$DIR/favicon-mask.svg" -alpha Off -compose CopyOpacity -composite "$DST/$MODE/favicon.png"
    convert -background none "$DIR/$MODE/favicon.svg" "$DIR/favicon-mask-with-outline.svg" -alpha Off -compose CopyOpacity -composite "$DIR/png/$MODE-favicon-with-outline.png"

    for SHAPE in passing incomplete failing
    do
        echo $DST/$MODE/favicon-$SHAPE.png
        convert -background none "$DIR/png/$MODE-favicon-with-outline.png" "$DIR/$MODE/favicon-$SHAPE.svg" -composite "$DST/$MODE/favicon-$SHAPE.png"
    done
done
