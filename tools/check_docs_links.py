"""Check local Markdown file links in repository documentation."""
from pathlib import Path
from urllib.parse import unquote, urlsplit
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
files = list((ROOT / 'docs').rglob('*.md')) + list((ROOT / 'references').glob('*.md'))
files += list((ROOT / 'memories').glob('*.md')) + list((ROOT / '.claude').rglob('*.md'))
files += [ROOT / 'README.md', ROOT / 'CLAUDE.md', ROOT / 'tools/README.md']
checked, errors = 0, []
for file in files:
    if not file.is_file():
        continue
    source = file.read_text(encoding='utf-8-sig')
    source = re.sub(r'```[^\n]*\n[\s\S]*?```', lambda m: '\n' * m.group(0).count('\n'), source)
    for match in re.finditer(r'\[[^\]\n]*\]\((<[^>]+>|[^)\n]+)\)', source):
        target = match.group(1).strip().strip('<>')
        if not target or target.startswith('#') or urlsplit(target).scheme:
            continue
        target = unquote(target.split('#', 1)[0].split('?', 1)[0])
        destination = ROOT / target.lstrip('/') if target.startswith('/') else file.parent / target
        checked += 1
        if not destination.exists():
            line = source[:match.start()].count('\n') + 1
            errors.append(f'{file.relative_to(ROOT)}:{line}: {target}')
print(f'Checked {checked} local links in {len(files)} Markdown files; missing: {len(errors)}')
for error in errors:
    print(error)
sys.exit(bool(errors))
