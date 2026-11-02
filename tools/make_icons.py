"""生成实验报告用的工具栏图标（22x22 PNG）。

做法是在 4 倍尺寸上绘制再缩小，获得抗锯齿效果；所有图标使用同一套
配色，保证工具栏看起来是成套的。

用法：python tools/make_icons.py
"""

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

SCALE = 4
SIZE = 22 * SCALE
ROOT = Path(__file__).resolve().parent.parent
ICON_DIR = ROOT / "icons"

BLUE = (54, 116, 180, 255)
BLUE_LIGHT = (150, 196, 235, 255)
BLUE_DARK = (36, 82, 132, 255)
GREY = (110, 118, 128, 255)
GREY_LIGHT = (196, 202, 210, 255)
AMBER = (240, 178, 58, 255)
AMBER_DARK = (198, 138, 30, 255)
GREEN = (62, 150, 92, 255)
RED = (192, 82, 78, 255)
WHITE = (253, 253, 253, 255)
INK = (44, 52, 64, 255)
ORANGE = (222, 128, 52, 255)

FONT_BOLD = "C:/Windows/Fonts/arialbd.ttf"
FONT_MONO = "C:/Windows/Fonts/consola.ttf"


def canvas():
    image = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    return image, ImageDraw.Draw(image)


def font(path, size):
    return ImageFont.truetype(path, size)


def save(image, name):
    image = image.resize((22, 22), Image.LANCZOS)
    image.save(ICON_DIR / f"{name}.png")


def draw_page(draw, left, top, right, bottom, fill=WHITE, outline=BLUE, cut=14, width=5):
    """画一张右上角折角的纸。"""
    points = [
        (left, top),
        (right - cut, top),
        (right, top + cut),
        (right, bottom),
        (left, bottom),
    ]
    draw.polygon(points, fill=fill)
    draw.line(points + [points[0]], fill=outline, width=width, joint="curve")
    draw.line([(right - cut, top), (right - cut, top + cut), (right, top + cut)],
              fill=outline, width=width, joint="curve")


def icon_new():
    image, draw = canvas()
    draw_page(draw, 20, 12, 68, 78)
    for y in (40, 50, 60, 70):
        draw.line([(30, y), (56, y)], fill=GREY_LIGHT, width=4)
    save(image, "new")


def icon_open():
    image, draw = canvas()
    draw.polygon([(10, 30), (40, 30), (48, 20), (82, 20), (82, 32), (10, 32)],
                 fill=AMBER_DARK)
    draw.polygon([(8, 32), (80, 32), (72, 76), (16, 76)], fill=AMBER)
    draw.line([(8, 32), (80, 32)], fill=AMBER_DARK, width=4)
    save(image, "open")


def icon_save():
    image, draw = canvas()
    draw.rounded_rectangle([12, 10, 76, 78], radius=6, fill=BLUE, outline=BLUE_DARK, width=4)
    draw.rectangle([28, 12, 62, 34], fill=WHITE)
    draw.rounded_rectangle([24, 46, 64, 76], radius=3, fill=WHITE)
    for y in (54, 62, 70):
        draw.line([(30, y), (58, y)], fill=BLUE_LIGHT, width=3)
    save(image, "save")


def icon_saveas():
    image, draw = canvas()
    draw.rounded_rectangle([8, 12, 60, 70], radius=6, fill=BLUE, outline=BLUE_DARK, width=4)
    draw.rectangle([20, 14, 48, 30], fill=WHITE)
    draw.rounded_rectangle([16, 40, 52, 66], radius=3, fill=WHITE)
    # 铅笔
    draw.polygon([(46, 78), (58, 44), (70, 48), (58, 82)], fill=AMBER)
    draw.polygon([(46, 78), (52, 80), (49, 86)], fill=INK)
    draw.line([(58, 44), (70, 48)], fill=AMBER_DARK, width=3)
    save(image, "saveas")


def icon_undo():
    image, draw = canvas()
    draw.arc([20, 18, 76, 74], start=10, end=200, fill=GREEN, width=9)
    draw.polygon([(20, 46), (40, 36), (40, 56)], fill=GREEN)
    save(image, "undo")


def icon_redo():
    image, draw = canvas()
    draw.arc([12, 18, 68, 74], start=340, end=170, fill=GREEN, width=9)
    draw.polygon([(68, 46), (48, 36), (48, 56)], fill=GREEN)
    save(image, "redo")


def icon_cut():
    image, draw = canvas()
    draw.line([(28, 16), (62, 58)], fill=GREY, width=6)
    draw.line([(60, 16), (26, 58)], fill=GREY, width=6)
    draw.ellipse([10, 56, 30, 76], outline=RED, width=6)
    draw.ellipse([58, 56, 78, 76], outline=RED, width=6)
    save(image, "cut")


def icon_copy():
    image, draw = canvas()
    draw.rounded_rectangle([10, 10, 54, 62], radius=4, fill=WHITE, outline=GREY_LIGHT, width=5)
    draw.rounded_rectangle([28, 26, 76, 80], radius=4, fill=WHITE, outline=BLUE, width=5)
    for y in (44, 54, 64):
        draw.line([(38, y), (66, y)], fill=BLUE_LIGHT, width=4)
    save(image, "copy")


def icon_paste():
    image, draw = canvas()
    draw.rounded_rectangle([18, 14, 70, 80], radius=6, fill=AMBER, outline=AMBER_DARK, width=4)
    draw.rounded_rectangle([32, 8, 56, 24], radius=4, fill=GREY_LIGHT, outline=GREY, width=3)
    draw.rounded_rectangle([26, 30, 62, 72], radius=3, fill=WHITE)
    for y in (40, 50, 60):
        draw.line([(32, y), (56, y)], fill=GREY_LIGHT, width=4)
    save(image, "paste")


def draw_magnifier(draw, left, top, size, handle_end):
    draw.ellipse([left, top, left + size, top + size], fill=BLUE_LIGHT, outline=BLUE, width=7)
    draw.line([(left + size * 0.72, top + size * 0.72), handle_end],
              fill=BLUE_DARK, width=9)
    draw.line([(left + size * 0.72, top + size * 0.72), handle_end],
              fill=BLUE, width=6)


def icon_find():
    image, draw = canvas()
    draw_magnifier(draw, 14, 10, 44, (74, 70))
    save(image, "find")


def icon_replace():
    image, draw = canvas()
    draw_magnifier(draw, 8, 8, 36, (54, 54))
    draw.line([(40, 74), (74, 74)], fill=GREEN, width=7)
    draw.polygon([(74, 74), (60, 64), (60, 84)], fill=GREEN)
    save(image, "replace")


def icon_font():
    image, draw = canvas()
    draw.text((24, 4), "A", font=font(FONT_BOLD, 68), fill=INK)
    draw.line([(24, 76), (64, 76)], fill=GREY, width=5)
    save(image, "font")


def icon_textcolor():
    image, draw = canvas()
    draw.text((26, 0), "A", font=font(FONT_BOLD, 54), fill=INK)
    draw.rounded_rectangle([22, 66, 66, 78], radius=3, fill=BLUE)
    save(image, "textcolor")


def icon_textbackground():
    image, draw = canvas()
    draw.rectangle([16, 44, 72, 72], fill=(255, 232, 128, 255))
    draw.text((26, 4), "A", font=font(FONT_BOLD, 54), fill=INK)
    draw.rounded_rectangle([16, 44, 72, 72], radius=2, outline=(226, 190, 60, 255), width=3)
    save(image, "textbackground")


def icon_editorbackground():
    image, draw = canvas()
    draw.arc([26, 8, 62, 44], start=180, end=360, fill=GREY, width=5)
    draw.polygon([(16, 32), (56, 22), (70, 56), (30, 68)], fill=ORANGE)
    draw.polygon([(22, 46), (60, 36), (64, 50), (26, 62)], fill=(255, 236, 170, 255))
    draw.ellipse([62, 66, 76, 80], fill=BLUE)
    save(image, "editorbackground")


def icon_linenumbers():
    image, draw = canvas()
    for x in (32, 54):
        draw.line([(x, 18), (x, 70)], fill=GREY, width=6)
    for y in (34, 54):
        draw.line([(20, y), (66, y)], fill=GREY, width=6)
    save(image, "linenumbers")


def icon_wrap():
    image, draw = canvas()
    draw.line([(16, 22), (60, 22)], fill=BLUE, width=7)
    draw.line([(60, 22), (60, 50), (30, 50)], fill=BLUE, width=7, joint="curve")
    draw.polygon([(18, 50), (34, 40), (34, 60)], fill=BLUE)
    save(image, "wrap")


def icon_app():
    big = 512
    image = Image.new("RGBA", (big, big), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle([56, 40, 456, 472], radius=28, fill=WHITE, outline=BLUE, width=16)
    draw.rounded_rectangle([56, 40, 456, 128], radius=28, fill=BLUE)
    draw.rectangle([56, 96, 456, 128], fill=BLUE)
    for x in (128, 200, 272, 344, 416):
        draw.ellipse([x - 12, 56, x + 12, 80], fill=WHITE)
    for y in (176, 232, 288, 344):
        draw.rounded_rectangle([112, y, 400, y + 14], radius=7, fill=GREY_LIGHT)
    draw.rounded_rectangle([112, 176, 300, 190], radius=7, fill=BLUE_LIGHT)
    image = image.resize((128, 128), Image.LANCZOS)
    image.save(ICON_DIR / "app.png")


def main():
    ICON_DIR.mkdir(parents=True, exist_ok=True)
    icon_new()
    icon_open()
    icon_save()
    icon_saveas()
    icon_undo()
    icon_redo()
    icon_cut()
    icon_copy()
    icon_paste()
    icon_find()
    icon_replace()
    icon_font()
    icon_textcolor()
    icon_textbackground()
    icon_editorbackground()
    icon_linenumbers()
    icon_wrap()
    icon_app()
    print(f"icons written to {ICON_DIR}")


if __name__ == "__main__":
    main()
