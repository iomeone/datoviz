# Simple test with Python

import numpy as np
import numpy.random as nr

# Import the library.
from datoviz import canvas, run, colormap

# Create a new canvas and scene. There's only 1 subplot (panel) by default.
c = canvas(show_fps=False)

# Get a panel (by default, the one spanning the entire canvas) # We specify the type of controller
# we want. Here, we want 2D axes.
panel = c.panel(controller='axes')

# We create a new "marker" visual.
visual = panel.visual('marker')

# We set the visual data via "visual properties" (or props). Each visual # comes with a set of
# predefined props. Here, we set the marker positions, colors (RGBA bytes), and size (in pixels)
N = 100_000
pos = nr.randn(N, 3)
ms = nr.uniform(low=2, high=40, size=N)
color = colormap(nr.rand(N), vmin=0, vmax=1, alpha=.75 * np.ones(N))

visual.data('pos', pos)
visual.data('color', color)
visual.data('ms', ms)

# We run the main event loop, which will display the canvas until Escape is # pressed or the
# window is closed.
run()
