"""Thunder Heights heightmap for a UE5 Landscape.

Source: docs/level/vysi_layout_v0.1.md (metres; X = north/uphill, Y = east, Z = up; origin = village well).
Output: 16-bit grayscale PNG, 1 px = 1 m, width = X (north), height = Y.

UE import settings (Landscape > Import from file):
  - Location: X = -10000, Y = -50400, Z = 0 (cm)   -> pixel (0,0) is world (-100 m, -504 m)
  - Scale:    X = 100, Y = 100, Z = 100            -> 1 m per pixel, height range +-256 m
  - Resolution 1009 x 1009 (63 quads/section, 1 section, 16x16 components)
Height encoding at Z scale 100: value = 32768 + metres * 128.
"""

from pathlib import Path

import numpy as np
from PIL import Image
from scipy.ndimage import gaussian_filter

SIZE = 1009
ORIGIN_X, ORIGIN_Y = -100.0, -504.0  # metres
OUT = Path(__file__).resolve().parents[2] / "game" / "FadedNav" / "Heightmaps"

rng = np.random.default_rng(1409)

# Path spine: (x, y_centre, z, half_width) — walkable corridor, from the layout zones and ramps.
SPINE = np.array([
    (-100, 0, 0, 45), (-45, 0, 0, 45), (45, 10, 0, 40),       # 1 Sukhorechye
    (60, 20, 3, 12), (140, 38, 18, 12),                       #   Okolitsa corridor
    (145, 40, 18, 21), (180, 40, 18, 21),                     # 2 Oath Stone plateau
    (280, -20, 25, 20),                                       #   climb
    (300, -20, 29, 45), (420, -20, 55, 45),                   # 3 Strelokopni slope (90 m wide)
    (435, 0, 55, 15), (445, 0, 55, 12),                       #   exit / tree choice 1
    (520, 20, 65, 85), (560, 20, 65, 85),                     # 4 Bucket Row shelf (Y -65..105)
    (575, 18, 67, 8), (640, 10, 78, 7), (652, 10, 78, 7),     #   climb + treba
    (698, -8, 90, 4),                                         #   serpentine to the hill
    (742, 0, 90, 4), (850, 0, 118, 3), (908, 5, 118, 12),     # 6 Thunder Ridge + oak/portal platform
], dtype=float)


def spine_at(x):
    """Interpolate centre-line y, path height z and half-width for an array of x."""
    xs = SPINE[:, 0]
    return (np.interp(x, xs, SPINE[:, 1]), np.interp(x, xs, SPINE[:, 2]), np.interp(x, xs, SPINE[:, 3]))


def smooth_noise(scale_px, amp):
    n = rng.standard_normal((SIZE, SIZE))
    n = gaussian_filter(n, scale_px)
    return n / (np.abs(n).max() + 1e-6) * amp


def build():
    xs = ORIGIN_X + np.arange(SIZE)            # columns -> world X
    ys = ORIGIN_Y + np.arange(SIZE)            # rows    -> world Y
    X, Y = np.meshgrid(xs, ys)                 # shape (rows=Y, cols=X)

    yc, zp, hw = spine_at(X)
    d = np.maximum(0.0, np.abs(Y - yc) - hw)   # metres outside the walkable corridor

    # Broad mountain massif: gentle flanks below, steeper toward the hill, knife-edge along the ridge.
    steep = np.where(X > 760, 1.6, np.where(X > 600, 0.5, 0.33))
    H = zp - steep * np.power(d, 1.1)

    # Valley floor with low hills, and a raised rim far from the path so the chapter feels enclosed.
    valley = -5.0 + smooth_noise(40, 6.0)
    rim = np.clip((np.abs(Y) - 260.0) / 240.0, 0, 1) * 70.0 + smooth_noise(25, 10.0) * np.clip((np.abs(Y) - 240) / 100, 0, 1)
    H = np.maximum(H, valley + rim)

    # Soften the flanks (not the path) so terraces blend into one slope, then add surface breakup off the path.
    off_path = np.clip(d / 6.0, 0, 1)
    H = H * (1 - off_path) + gaussian_filter(H, 10) * off_path
    H += (smooth_noise(6, 1.5) + smooth_noise(18, 3.0)) * off_path

    # Dry riverbed south-west of the village (Y = -40, 8 m wide, 2 m deep), running south out of the map.
    river = (np.abs(Y + 40) < 4) & (X < 45)
    H = np.where(river, np.minimum(H, -2.0), H)

    # Strelokopni pits: 12 craters 3-6 m wide, 1.5-3 m deep, on the slope.
    for i in range(12):
        cx = 300 + (i % 4) * 30 + rng.uniform(-5, 5)
        cy = -55 + (i // 4) * 30 + rng.uniform(-6, 6)
        r = rng.uniform(1.5, 3.0)
        depth = rng.uniform(1.5, 3.0)
        dist = np.hypot(X - cx, Y - cy)
        H -= np.where(dist < r * 1.4, depth * np.clip(1 - (dist / (r * 1.4)) ** 2, 0, 1), 0)

    # Burial mound at the Oath Stone (half-sphere 14 m, 4 m high).
    dm = np.hypot(X - 175, Y - 70)
    H = np.maximum(H, np.where(dm < 7, 18 + 4 * np.sqrt(np.clip(1 - (dm / 7) ** 2, 0, 1)), -1e9))

    # Exam arena: flat bowl R 22 at Z 90 (edge 0.5 m higher), then a 10 m bank before the drop.
    da = np.hypot(X - 720, Y)
    arena = 90 + 0.5 * (da / 22) ** 2
    bank = 90.5 - np.clip(da - 22, 0, None) * 0.15
    H = np.where(da <= 22, arena, np.where(da <= 32, np.maximum(H, bank), H))
    # Idol pit in the centre (5 m wide, 1 m deep).
    H -= np.where(da < 2.5, 1.0 * (1 - (da / 2.5) ** 2), 0)

    return H


def main():
    H = build()
    OUT.mkdir(parents=True, exist_ok=True)
    raw = np.clip(np.round(32768 + H * 128), 0, 65535).astype(np.uint16)
    Image.fromarray(raw).save(OUT / "vysi_heightmap_1009.png")

    # Human-friendly preview (not for import): north up.
    prev = (H - H.min()) / (H.max() - H.min())
    img = Image.fromarray((np.flipud(prev.T) * 255).astype(np.uint8))
    img.save(OUT / "vysi_heightmap_preview.png")
    print(f"height range: {H.min():.1f} .. {H.max():.1f} m -> {OUT}")


if __name__ == "__main__":
    main()
