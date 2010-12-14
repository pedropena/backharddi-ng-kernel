#!/usr/bin/python

import Image, ImageFont, ImageDraw

version = open('debian/changelog').readline().partition('(')[2].partition(')')[0]
font = ImageFont.truetype("/usr/share/fonts/truetype/freefont/FreeSans.ttf", 12)
fontcolor = '#000000'
im = Image.open("udebs/d-i/rootskel-gtk/source/src/usr/share/graphics/logo_debian.tpl.png")
draw = ImageDraw.Draw(im)
draw.text((11, 40), "Version: " + version, font=font, fill=fontcolor)
del draw
im.save('udebs/d-i/rootskel-gtk/source/src/usr/share/graphics/logo_debian.png')
