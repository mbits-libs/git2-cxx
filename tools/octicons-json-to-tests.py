import json
import os

__root__ = os.path.dirname(os.path.dirname(__file__))
with open(os.path.join(__root__, "data/html/res/octicons.json"), encoding='utf-8') as f:
    data = json.load(f)

with open(os.path.join(__root__, "extras/libs/web/tests/octicons.inc"), "w", encoding='utf-8') as f:
    for key in sorted(data.keys()):
        icon = data[key]
        print(f"{{ {json.dumps(key)}sv, {icon['width']}u, {json.dumps(icon['path'])}sv }},", file=f)
