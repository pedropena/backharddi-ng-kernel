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

import fileinput

for line in fileinput.FileInput("build/config/common", inplace=1):
	if "LSB_DISTRIB_RELEASE=" in line:
		line = 'LSB_DISTRIB_RELEASE="%s (build $(BUILD_DATE))' % version
	print line[:-1]
