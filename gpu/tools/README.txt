3bpp:
convert image.png -remap 332-8bit-color.png -dither FloydSteinberg -resize 160x120! -colors 7 out.png
6bpp:
convert image.png -remap 332-8bit-color.png -dither FloydSteinberg -resize 160x120! -colors 63 out.png
