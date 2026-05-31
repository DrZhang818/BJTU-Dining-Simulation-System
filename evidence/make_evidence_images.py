from pathlib import Path
import textwrap
import pandas as pd
import matplotlib.pyplot as plt
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
E = ROOT / 'evidence'

def text_image(text, out, width=1600, padding=28, font_size=24):
    font_paths = [
        '/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf',
        '/usr/share/fonts/truetype/liberation2/LiberationMono-Regular.ttf',
    ]
    for fp in font_paths:
        try:
            font = ImageFont.truetype(fp, font_size)
            break
        except Exception:
            font = ImageFont.load_default()
    lines = []
    for raw in text.splitlines():
        if len(raw) > 110:
            lines.extend(textwrap.wrap(raw, width=110, replace_whitespace=False, drop_whitespace=False))
        else:
            lines.append(raw)
    line_h = int(font_size * 1.45)
    height = padding * 2 + line_h * max(1, len(lines))
    img = Image.new('RGB', (width, height), 'white')
    draw = ImageDraw.Draw(img)
    y = padding
    for line in lines:
        draw.text((padding, y), line, fill=(20, 20, 20), font=font)
        y += line_h
    img.save(out)

# Terminal evidence images
build_text = (E/'build_configure.log').read_text() + '\n' + (E/'build.log').read_text()
# Keep build image compact
build_lines = [ln for ln in build_text.splitlines() if ('-- ' in ln or '[' in ln or 'Built target' in ln)]
text_image('\n'.join(build_lines[-22:]), E/'build_evidence.png')
text_image((E/'test_core.log').read_text(), E/'test_evidence.png')
text_image((E/'run_headless.log').read_text(), E/'run_evidence.png')

# Charts
csv_path = E/'simulation_log.csv'
df = pd.read_csv(csv_path)
for col, title, ylabel, name in [
    ('total_queue_length', 'Total queue length over time', 'Students', 'queue_length_chart.png'),
    ('waiting_for_seat_count', 'Waiting-for-seat count over time', 'Students', 'seat_wait_chart.png'),
    ('seat_utilization', 'Seat utilization over time', 'Percent', 'seat_utilization_chart.png'),
    ('finished_students', 'Finished students over time', 'Students', 'finished_students_chart.png'),
]:
    plt.figure(figsize=(9, 4.8), dpi=160)
    plt.plot(df['time'], df[col])
    plt.title(title)
    plt.xlabel('Simulation time (seconds)')
    plt.ylabel(ylabel)
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig(E/name)
    plt.close()
