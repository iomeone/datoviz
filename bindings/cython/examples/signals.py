"""
# Digital signals

"""

import numpy as np
import numpy.random as nr

from datoviz import canvas, run, colormap

c = canvas(show_fps=True)

panel = c.panel(controller='panzoom')
visual = panel.visual('line_strip')

n_signals = 200
n_points = 2000
n_vert = n_signals * n_points

t = np.linspace(0, 5, n_points)
x = np.tile(t, (n_signals,))
assert x.ndim == 1

y = .2 * nr.randn(n_signals, n_points)
offsets = np.tile(np.arange(n_signals)[:, np.newaxis], (1, n_points))
y += offsets
pos = np.c_[x, y.ravel(), np.zeros(n_vert)]

color = np.repeat(colormap(np.linspace(0, 1, n_signals), cmap='glasbey'), n_points, axis=0)
length = np.repeat(np.array([n_points]), n_signals)

assert pos.shape == (n_vert, 3)
assert color.shape == (n_vert, 4)
assert length.shape == (n_signals,)

visual.data('pos', pos)
visual.data('color', color)
visual.data('length', length)

i = 0
k = 50
def f():
    global i
    yk = .2 * nr.randn(n_signals, k)
    offsets = np.tile(np.arange(n_signals)[:, np.newaxis], (1, k))
    yk += offsets
    y[:, i * k:(i + 1) * k] = yk
    pos[:, 1] = y.ravel()
    visual.data('pos', pos)
    i += 1
    i = i % (n_points // k)

c._connect('timer', f, .05)

run()
