"""Execute diagnostic adapters inside the paired original-68000 fixture."""
from system_cpu import check_system
def check_debug(project,hz):
    return check_system(project,hz,diagnostics=True)
