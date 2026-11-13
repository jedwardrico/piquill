import re, sys
MAX_NAME_LEN = 32
def sanitize_name(name):   # copied from src/main.py
    name = re.sub(r'[^A-Za-z0-9 _.-]', '_', name).strip().lstrip('.')
    for ext in ('.md', '.txt'):
        if name.lower().endswith(ext):
            name = name[:-len(ext)]
    return name.strip()[:MAX_NAME_LEN]
for line in sys.stdin:
    print(sanitize_name(bytes.fromhex(line.strip()).decode('latin-1')).encode('latin-1').hex())
