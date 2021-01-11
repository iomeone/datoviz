import time
import numpy as np
from visky import canvas, run

c = canvas()
panel = c.panel()
visual = panel.visual('marker')

N = 1 * 1024 * 1024
visual.data('pos', (.25 * np.random.randn(N, 3)).astype(np.float32))
visual.data('color', np.random.randint(
    low=50, high=255, size=(N, 4)).astype(np.uint8))
visual.data('ms', (2 * np.ones(1)).astype(np.float32))

run()