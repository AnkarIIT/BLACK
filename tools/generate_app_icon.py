from PIL import Image, ImageDraw, ImageFont

# Create multi-size icon from generated image
sizes = [256, 128, 64, 48, 32, 16]
images = []
for s in sizes:
    img = Image.new('RGBA', (s, s), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    # draw orange ring
    cx, cy = s//2, s//2
    r_outer = int(s*0.48)
    r_inner = int(s*0.28)
    draw.ellipse((cx-r_outer, cy-r_outer, cx+r_outer, cy+r_outer), fill=(0,0,0,0), outline=None)
    # gradient-like fill: simple solid for compatibility
    draw.ellipse((cx-r_outer, cy-r_outer, cx+r_outer, cy+r_outer), fill=(0,0,0,0))
    draw.ellipse((cx-r_inner, cy-r_inner, cx+r_inner, cy+r_inner), fill=(0,0,0,255))
    # draw a bright ring using smaller ellipse
    draw.ellipse((cx-r_inner- (s//12), cy-r_inner- (s//12), cx+r_inner+ (s//12), cy+r_inner+ (s//12)), outline=(255,200,0,255), width=max(1, s//16))
    # draw center black circle
    draw.ellipse((cx-(s//4), cy-(s//4), cx+(s//4), cy+(s//4)), fill=(0,0,0,255))
    # optional letter B
    try:
        font = ImageFont.truetype("arial.ttf", max(10, s//3))
    except Exception:
        font = ImageFont.load_default()
    text = "B"
    try:
        tw, th = font.getsize(text)
    except Exception:
        tw, th = draw.textbbox((0,0), text, font=font)[2:]
    draw.text((cx-tw//2, cy-th//2), text, font=font, fill=(255,200,0,255))
    images.append(img.convert('RGBA'))

# Save multi-size ICO
ico_path = 'c:/Codes/BLACK/app.ico'
images[0].save(ico_path, sizes=[(s,s) for s in sizes])
print('Wrote', ico_path)
