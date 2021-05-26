#!/bin/bash
kikit panelize \
    --layout 'grid; rows: 5; cols: 2; space: 2mm; alternation: cols' \
    --tabs 'fixed; width: 3mm' \
    --source 'tolerance: 15mm' \
    --cuts 'mousebites; drill: 0.5mm; spacing: 1mm; offset: 0.2mm; prolong: 0.5mm' \
    --framing 'frame; width: 5mm; space: 2mm; cuts: true' \
    --tooling '3hole; hoffset: 2.5mm; voffset: 2.5mm; size: 1.5mm' \
    --fiducials '3fid; hoffset: 5mm; voffset: 2.5mm; coppersize: 2mm; opening: 1mm;' \
    --post 'millradius: 1mm' \
stm32_ds2480_emu.kicad_pcb \
stm32_ds2480_emu_panel.kicad_pcb

