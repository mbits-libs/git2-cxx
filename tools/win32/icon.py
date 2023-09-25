from math import floor
import os
from typing import List, Union

number = Union[int, float]


def drop(n: number) -> number:
    i = floor(n)
    i100000 = floor(n * 10000)
    if i * 10000 == i100000:
        return i
    return i100000 / 10000


class pt:
    def __init__(self, x: number, y: number):
        self.x = drop(x)
        self.y = drop(y)

    def __str__(self):
        return f"{self.x},{self.y}"

    def __add__(self, other):
        return pt(self.x + other.x, self.y + other.y)

    def __iadd__(self, other):
        self.x += other.x
        self.y += other.y
        return self

    def __sub__(self, other):
        return pt(self.x - other.x, self.y - other.y)

    def __isub__(self, other):
        self.x -= other.x
        self.y -= other.y
        return self


class style:
    def __init__(self, type: str, value: str, **kwargs):
        self.type = type
        self.value = value
        self.kwargs = kwargs

    def __str__(self):
        result = [
            f'{self.type}="{self.value}"',
            *(f'{self.type}-{kwarg}="{self.kwargs[kwarg]}"' for kwarg in self.kwargs),
        ]
        return " ".join(result)


def glyph_stroke(color: str):
    return style("stroke", color, width="2.5")


class path:
    def __init__(self):
        self.pos = pt(0, 0)
        self.code = []

    def __str__(self):
        return " ".join(self.code)

    def M(self, p: pt):
        self.code.append(f"M{p}")
        self.pos = p

    def m(self, p: pt):
        self.code.append(f"M{self.pos + p}")
        self.pos += p

    def l(self, p: pt):
        self.code.append(f"L{self.pos + p}")
        self.pos += p

    def a(
        self,
        p: pt,
        r: Union[pt, number],
        x_rot: number = 0,
        large_arc: bool = False,
        sweep: bool = False,
    ):
        radius = r if isinstance(r, pt) else pt(r, r)
        large_arc_s = "1" if large_arc else "0"
        sweep_s = "1" if sweep else "0"
        self.code.append(f"A{radius} {x_rot} {large_arc_s} {sweep_s} {self.pos + p}")
        self.pos += p

    def c(self, dst: pt, c1: pt, c2: pt):
        self.code.append(f"C{self.pos + c1} {dst - c2} {dst}")
        self.pos = dst

    def v(self, l):
        self.pos.y += l
        self.code.append(f"V{self.pos.y}")

    def h(self, l):
        self.pos.x += l
        self.code.append(f"H{self.pos.x}")

    def z(self):
        self.code.append("z")


class svg_path:
    def __init__(self, d: path, *args: List[style]):
        self.d = d
        self.args = args

    def __str__(self):
        args = "".join(f" {arg}" for arg in self.args)
        return f'<path{args} d="{self.d}"/>'


class svg_group:
    def __init__(self, translate: pt):
        self.translate = translate
        self.paths: List[svg_path] = []

    def __str__(self):
        paths = "".join(f"{p}\n" for p in self.paths)
        return f'<g transform="translate({self.translate})">\n{paths}</g>'


class svg_mask:
    def __init__(self, id, *args: Union[svg_path, svg_group, str]):
        self.children = args
        self.id = id

    def __str__(self):
        # <rect width="{self.size}" height="{self.size}" stroke="blue" />
        children = "".join(f"{node}\n" for node in self.children)
        return f"""<mask id="{self.id}">
{children}</mask>"""


class svg_root:
    def __init__(
        self, size, *args: Union[svg_path, svg_group, svg_mask, str], **kwargs
    ):
        self.children = args
        self.size = size
        self.px_size = kwargs.get("px_size", 256)

    def __str__(self):
        # <rect width="{self.size}" height="{self.size}" stroke="blue" />
        children = "".join(f"{node}\n" for node in self.children)
        return f"""<svg xmlns='http://www.w3.org/2000/svg'
    width='{self.px_size}px' height='{self.px_size}px' viewBox='0 0 {self.size} {self.size}' version='1.1'
    fill="none">
{children}</svg>"""


ICON_WIDTH = 18
ICON_HEIGHT = 24
ICON_SIZE = drop(1.5 * ICON_HEIGHT)

TOP = 3
BOTTOM = 14

CONTROL_H = 5
CONTROL_V = BOTTOM * 8 / 14


def shield_calc():
    width = ICON_WIDTH
    height = ICON_HEIGHT
    mid = 24 - (TOP + BOTTOM)
    return (width, height, ICON_HEIGHT * TOP / 24, ICON_HEIGHT * mid / 24)


def trace_right_side(p: path):
    width, height, top, mid = shield_calc()

    p.c(pt(width / 2, top), pt(0, 0), pt(CONTROL_H, 0))
    p.v(mid)
    p.c(pt(0, height), pt(0, CONTROL_V), pt(0, 0))


def trace_shadow() -> path:
    p = path()
    p.M(pt(0, 0))
    trace_right_side(p)
    p.z()
    return p


def trace_shield() -> path:
    width, _, top, mid = shield_calc()

    p = path()
    p.M(pt(-width / 2, top))
    p.c(pt(0, 0), pt(CONTROL_H, 0), pt(0, 0))
    trace_right_side(p)
    p.c(pt(-width / 2, top + mid), pt(0, 0), pt(0, -CONTROL_V))
    p.z()
    return p


def trace_fail() -> path:
    height = shield_calc()[1]
    p = path()
    p.pos = pt(0, height * 21 / 48)

    length = 8

    p.m(pt(-length / 2, -length / 2))
    p.l(pt(length, length))
    p.m(pt(0, -length))
    p.l(pt(-length, length))
    return p


def trace_mark() -> path:
    height = shield_calc()[1]
    p = path()
    p.pos = pt(0, height * 21 / 48)

    left = 3
    right = 6

    height = max(left, right)

    p.m(pt((left + right) / -2, height / 2 - left))
    p.l(pt(left, left))
    p.l(pt(right, -right))
    return p


class puzzle_side:
    def __init__(self, width: number):
        self.width = width

        init_stub = 1
        init_edge = 4
        init_radius = 3

        scale = width / (2 * (init_edge + init_radius))

        self.stub = init_stub * scale
        self.edge = init_edge * scale
        self.radius = init_radius * scale

    def draw(self, p: path, horiz: bool, innie: bool, returning: bool, last=False):
        sign = -1 if returning else 1
        stub_sign = -sign if (innie ^ horiz) else sign
        main = p.h if horiz else p.v
        across = p.v if horiz else p.h

        reach = sign * 2 * self.radius
        endpoint = pt(reach, 0) if horiz else pt(0, reach)

        main(sign * self.edge)
        across(stub_sign * self.stub)
        # main(reach)
        p.a(endpoint, self.radius, sweep=not innie)
        across(-stub_sign * self.stub)
        if not last:
            main(sign * self.edge)


def trace_puzzle() -> path:
    width, _, top, mid = shield_calc()
    puzzle_width = width / 2

    side = puzzle_side(puzzle_width)

    p = path()
    p.M(pt(-puzzle_width / 2, top + mid - puzzle_width / 3))

    side.draw(p, horiz=True, innie=False, returning=False)
    side.draw(p, horiz=False, innie=True, returning=False)
    side.draw(p, horiz=True, innie=False, returning=True)
    side.draw(p, horiz=False, innie=True, returning=True, last=True)

    p.z()
    return p


def trace_glyphs():
    return (trace_shield(), trace_shadow(), trace_fail(), trace_mark(), trace_puzzle())


class Shield:
    def __init__(self):
        shield_glyph, shadow, fail, mark, puzzle = trace_glyphs()
        self.shield_glyph = shield_glyph
        self.shadow = shadow
        self.fail_glyph = fail
        self.mark_glyph = mark
        self.puzzle = puzzle

    def _favicon(self, symbol: path, shield_color: str, symbol_color: str):
        width, height, _, _ = shield_calc()
        result = svg_group(pt(ICON_SIZE - width / 2, ICON_SIZE - height))
        result.paths = [
            svg_path(self.shield_glyph, style("fill", shield_color)),
            svg_path(symbol, glyph_stroke(symbol_color)),
            svg_path(
                self.shadow,
                style("fill", "#000"),
                style("opacity", ".2"),
            ),
        ]
        return svg_root(ICON_SIZE, result)

    def _appicon(
        self, symbol: path, shield_color: str, symbol_color: str, add_mask: bool
    ):
        height = shield_calc()[1]
        result = svg_group(pt(ICON_HEIGHT / 2, ICON_HEIGHT - height))
        mask_style = []
        if add_mask:
            mask_style = [style("mask", "url(#mask)")]
        result.paths = [
            svg_path(self.shield_glyph, style("fill", shield_color)),
            svg_path(symbol, glyph_stroke(symbol_color)),
            svg_path(
                self.shadow,
                style("fill", "#000"),
                style("opacity", ".2"),
            ),
            svg_path(
                self.shield_glyph, style("stroke", symbol_color, width="2"), *mask_style
            ),
        ]
        if add_mask:
            mask = svg_mask(
                "mask",
                f'<rect width="{ICON_HEIGHT}" height="{ICON_HEIGHT}" fill="#000"/>',
                svg_path(self.shield_glyph, style("fill", "#fff")),
            )
            result.paths.insert(0, mask)
        return svg_root(ICON_HEIGHT, result, px_size=256 if add_mask else 1024)

    def appicon_cov(self):
        return self._appicon(self.mark_glyph, "#b9e7fc", "#1f88e5", True)

    def appicon_win32(self):
        return self._appicon(self.mark_glyph, "#b9e7fc", "#1f88e5", False)

    def plugin_win32(self):
        shield_color = "#b9e7fc"
        symbol_color = "#1f88e5"
        height = shield_calc()[1]
        result = svg_group(pt(ICON_HEIGHT / 2, ICON_HEIGHT - height))
        result.paths = [
            svg_path(self.shield_glyph, style("fill", shield_color)),
            svg_path(
                self.puzzle,
                style("stroke", symbol_color),
                style("fill", symbol_color, opacity=".6"),
            ),
            svg_path(
                self.shadow,
                style("fill", "#000"),
                style("opacity", ".2"),
            ),
            svg_path(self.shield_glyph, style("stroke", symbol_color, width="2")),
        ]
        return svg_root(ICON_HEIGHT, result, px_size=1024)

    def appicon_win32_mask(self):
        height = shield_calc()[1]
        result = svg_group(pt(ICON_HEIGHT / 2, ICON_HEIGHT - height))
        result.paths = [
            f'<rect width="{ICON_HEIGHT}" height="{ICON_HEIGHT}" x="{-ICON_HEIGHT/2}" fill="#000"/>',
            svg_path(self.shield_glyph, style("fill", "#fff")),
        ]
        return svg_root(ICON_HEIGHT, result, px_size=1024)

    def favicon_bad(self):
        return self._favicon(self.fail_glyph, "#cf222e", "#fff")

    def favicon_good(self):
        return self._favicon(self.mark_glyph, "#1a7f37", "#fff")

    def favicon_passing(self):
        return self._favicon(self.mark_glyph, "#ffc108", "#222")

    def favicon_outline(self):
        width, height, _, _ = shield_calc()
        result = svg_group(pt(ICON_SIZE - width / 2, ICON_SIZE - height))
        result.paths = [
            svg_path(
                self.shield_glyph,
                style("fill", "#000"),
                style("stroke", "#000", width="6"),
            ),
        ]
        return svg_root(ICON_SIZE, result)


shield = Shield()

__dir_name__ = os.path.dirname(__file__)
__root_dir__ = os.path.dirname(os.path.dirname(__dir_name__))
__icons__ = os.path.join(__root_dir__, "data", "icons")

for icon in ["good", "bad", "passing", "outline"]:
    pathname = f"favicon-{icon}.svg"
    with open(os.path.join(__icons__, pathname), "wb") as f:
        svg = str(getattr(shield, f"favicon_{icon}")())
        f.write(svg.encode("UTF-8"))
        f.write(b"\n")

for icon in ["cov", "win32", "win32_mask"]:
    try:
        pathname = {"cov": "appicon.svg"}[icon]
    except KeyError:
        pathname = f"appicon-{icon.replace('_', '-')}.svg"
    with open(os.path.join(__icons__, pathname), "wb") as f:
        svg = str(getattr(shield, f"appicon_{icon}")())
        f.write(svg.encode("UTF-8"))
        f.write(b"\n")

with open(os.path.join(__icons__, "plugin-win32.svg"), "wb") as f:
    svg = str(shield.plugin_win32())
    f.write(svg.encode("UTF-8"))
    f.write(b"\n")
