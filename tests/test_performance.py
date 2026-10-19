import copy
import json
import os
from pathlib import Path
import shutil
import tempfile
import unittest
from test_project import ROOT,load
SKILL=ROOT/'skills/megadev-performance';LAB=SKILL/'assets/performance-lab'
create=load(SKILL/'scripts/create_project.py','perf_create').create
verify=load(LAB/'tools/check_work.py','perf_work').verify
compare=load(SKILL/'scripts/compare_runs.py','perf_compare').compare
class PerformanceTests(unittest.TestCase):
    def test_work_equivalence_wrapping_and_measurement_windows(self):
        if not any(shutil.which(n) for n in ['cc','clang','gcc']):
            if os.environ.get('MEGADEV_REQUIRE_HOST_CC')=='1':self.fail('C compiler required')
            self.skipTest('No host compiler')
        for mode in ['-O0','-O3']:
            result=json.loads(verify(LAB,mode));self.assertTrue(result['passed']);self.assertEqual(result['equivalent_cases'],1200)
    def test_comparison_normalizes_region_and_rejects_confounded_or_failed_runs(self):
        a=dict(schema=1,case=0,level=1,method=0,hud=True,timer=True,hz=60,elapsed=240,loops=240,late=0,blocks=240,upload_kib=0,images=0,check_ok=True)
        b={**a,'hz':50,'elapsed':200,'loops':200,'blocks':200}
        c=compare(a,b,'region');self.assertEqual(c['metrics']['blocks']['b_per_second'],50)
        self.assertIsNone(c['metrics']['images']['b_over_a'])
        for mutate in [lambda x:x.update(level=2),lambda x:x.update(check_ok=False),lambda x:x.update(elapsed=0),lambda x:x.update(loops=True)]:
            bad=copy.deepcopy(b);mutate(bad)
            with self.assertRaises(ValueError):compare(a,bad,'region')
    def test_standalone_resource_fidelity_and_existing_asic_owner(self):
        with tempfile.TemporaryDirectory() as folder:
            game=create(Path(folder)/'perf game')
            with self.assertRaises(ValueError):create(game)
            self.assertEqual((game/'tools/compare_runs.py').read_bytes(),(SKILL/'scripts/compare_runs.py').read_bytes())
            for name in ['scale.c','scale_main.c','scale_sub.c']:
                self.assertEqual((game/'src'/name).read_bytes(),(ROOT/'skills/megadev-scaling/assets/scaling-lab/src'/name).read_bytes())
