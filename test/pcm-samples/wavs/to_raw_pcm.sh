#!/bin/bash

for q in *.wav; do
    ffmpeg -y -i "$q" -acodec copy -f u8 "../${q::-4}.raw"
done
