import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SKILL = ROOT / 'skills/megadev-planning'
SCRIPT = SKILL / 'scripts/budget_check.py'
spec = importlib.util.spec_from_file_location('planning_budget', SCRIPT)
budget = importlib.util.module_from_spec(spec)
spec.loader.exec_module(budget)


class PlanningTests(unittest.TestCase):
    def setUp(self):
        self.plan = json.loads((SKILL / 'assets/budget-example.json').read_text())

    def allocation(self, key):
        return next(a for a in self.plan['allocations'] if a['name'] == key)

    def test_phase_lifetimes_and_known_totals(self):
        r = budget.check(self.plan)
        self.assertTrue(r['ok'], r['errors'])
        self.assertEqual(r['phases']['play']['used_bytes']['work'], 47360)
        self.assertEqual(r['phases']['play']['used_bytes']['vram'], 53248)
        self.assertEqual(r['phases']['play']['used_bytes']['word'], 98304)
        self.assertEqual(r['phases']['load']['used_bytes']['word'], 196608)
        self.assertEqual(r['phases']['stream']['word_bytes'], {'main': 98304, 'sub': 114688})
        self.assertEqual(r['peak_bytes']['word'], 212992)
        self.assertTrue(any('unmeasured allocations' in w for w in r['warnings']))
        # Current and next-room buffers can coexist in a streaming phase;
        # keeping a second full loading buffer too is a real peak overflow.
        self.allocation('Loading package and decode workspace')['phases'].append('stream')
        r = budget.check(self.plan)
        self.assertFalse(r['ok'])
        self.assertTrue(any('word exceeds' in e for e in r['errors']))

    def test_each_physical_pool_exact_fit_and_one_byte_overflow(self):
        for pool, capacity in budget.CAPACITY.items():
            with self.subTest(pool=pool):
                p = copy.deepcopy(self.plan)
                p['phases'] = [p['phases'][0]]
                p['allocations'] = [{'name': 'exact', 'pool': pool, 'bytes': capacity,
                                     'owner': sorted(budget.OWNERS[pool])[0],
                                     'phases': ['play'], 'evidence': 'reservation'}]
                self.assertTrue(budget.check(p)['ok'])
                p['allocations'][0]['bytes'] += 1
                self.assertFalse(budget.check(p)['ok'])

    def test_word_bank_capacity_cannot_borrow_from_other_cpu(self):
        a = self.allocation('Current streaming bank')
        a['bytes'] = 131072
        self.assertTrue(budget.check(self.plan)['ok'])
        a['bytes'] += 1
        r = budget.check(self.plan)
        self.assertLess(r['phases']['stream']['used_bytes']['word'], 262144)
        self.assertFalse(r['ok'])
        self.assertTrue(any('main Word RAM bank' in e for e in r['errors']))

    def test_two_megabit_ownership_changes_are_explicit(self):
        self.plan['phases'][0]['word_owner'] = 'sub'
        r = budget.check(self.plan)
        self.assertFalse(r['ok'])
        self.assertTrue(any('needs main access while sub owns' in e for e in r['errors']))
        self.allocation('Current room module and data')['owner'] = 'sub'
        self.assertTrue(budget.check(self.plan)['ok'])

    def test_scanline_limits_differ_from_frame_count_and_mode(self):
        g = self.plan['phases'][0]['graphics']
        for metric, value in [('palette_lines', 5), ('sprites', 81),
                              ('peak_line_sprites', 21), ('peak_line_pixels', 321)]:
            p = copy.deepcopy(self.plan)
            p['phases'][0]['graphics'][metric] = value
            self.assertFalse(budget.check(p)['ok'], metric)
        g.update(sprites=65, peak_line_sprites=17, peak_line_pixels=257)
        self.assertTrue(budget.check(self.plan)['ok'])
        self.plan['display']['mode'] = 'H32'
        r = budget.check(self.plan)
        self.assertEqual(len(r['errors']), 3)

    def test_pal_ntsc_and_parallel_cpu_budgets(self):
        self.plan['phases'][0]['cpu_us'] = {'main': 15000, 'sub': 15000}
        self.assertTrue(budget.check(self.plan)['ok'])
        self.plan['phases'][0]['cpu_us']['main'] = 19000
        self.assertFalse(budget.check(self.plan)['ok'])
        self.plan['display']['refresh_hz'] = 50
        self.assertTrue(budget.check(self.plan)['ok'])
        self.plan['phases'][0]['cpu_us']['sub'] = 20001
        self.assertFalse(budget.check(self.plan)['ok'])

    def test_preload_model_labels_assumptions_and_warns_about_lateness(self):
        r = budget.check(self.plan)
        self.assertAlmostEqual(r['loads'][0]['modeled_ms'], 1063 + 1/3)
        self.assertAlmostEqual(r['loads'][0]['margin_ms'], 436 + 2/3)
        self.assertTrue(any('not a drive measurement' in w for w in r['warnings']))
        self.plan['loads'][0]['available_ms'] = 500
        r = budget.check(self.plan)
        self.assertTrue(r['ok'])  # Late content requires a fallback, not a fabricated hardware failure.
        self.assertTrue(any('misses the preload window' in w for w in r['warnings']))
        self.plan['display']['refresh_hz'] = 50
        self.assertAlmostEqual(budget.check(self.plan)['loads'][0]['modeled_ms'], 1090)

    def test_invalid_or_ambiguous_declarations_fail_closed(self):
        bad = []
        def mutate(fn):
            p = copy.deepcopy(self.plan)
            fn(p)
            bad.append(p)
        mutate(lambda p: p.update(version=True))
        mutate(lambda p: p.update(allocations={}))
        mutate(lambda p: p.update(phases=[]))
        mutate(lambda p: p.update(unknown_pool_size=1000000))
        mutate(lambda p: p['allocations'][0].update(bytes=True))
        mutate(lambda p: p['allocations'][0].update(bytes=-1))
        mutate(lambda p: p['allocations'][0].update(count=0))
        mutate(lambda p: p['allocations'][0].update(phases=['play', 'play']))
        mutate(lambda p: p['allocations'][0].update(phases=['missing']))
        mutate(lambda p: p['allocations'][0].update(pool='heap'))
        mutate(lambda p: p['allocations'][0].update(pool=[]))
        mutate(lambda p: p['allocations'][0].update(owner='sub'))
        mutate(lambda p: p['allocations'][0].update(evidence='guessed-fact'))
        mutate(lambda p: p['allocations'].append(copy.deepcopy(p['allocations'][0])))
        mutate(lambda p: p['phases'].append(copy.deepcopy(p['phases'][0])))
        mutate(lambda p: p['phases'][2].update(word_owner='main'))
        mutate(lambda p: p['phases'][0].pop('word_owner'))
        mutate(lambda p: p['loads'][0].update(assumed_bytes_per_second=0))
        mutate(lambda p: p['loads'][0].update(assumed_decode_ms=float('nan')))
        mutate(lambda p: p['loads'][0].update(assumed_seek_ms=float('inf')))
        mutate(lambda p: p['loads'][0].update(packed_bytes=10**400))
        mutate(lambda p: p['loads'][0].update(assumed_bytes_per_second=1e-320))
        for p in bad:
            with self.subTest(plan=p), self.assertRaises(budget.InvalidPlan):
                budget.check(p)

    def test_cli_exit_codes_and_structured_output(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'budget.json'
            def run(p):
                path.write_text(json.dumps(p), encoding='utf-8')
                return subprocess.run([sys.executable, str(SCRIPT), str(path), '--json'],
                                      text=True, capture_output=True)
            r = run(self.plan)
            self.assertEqual(r.returncode, 0, r.stderr)
            self.assertTrue(json.loads(r.stdout)['ok'])
            self.allocation('Resident code and data')['bytes'] = 65536
            r = run(self.plan)
            self.assertEqual(r.returncode, 1)
            self.assertFalse(json.loads(r.stdout)['ok'])
            r = run({'version': 2})
            self.assertEqual(r.returncode, 2)
            self.assertIn('Invalid plan:', r.stderr)
