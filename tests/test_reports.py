import copy
import json
from pathlib import Path
import tempfile
import unittest
from compare_reports import compare,CASES
class ReportTests(unittest.TestCase):
    def test_complete_matching_matrix_and_fail_closed_controls(self):
        case={'disc':{'files':{'IPX.MMD':'hash'}},'boot_payloads':{'sp.bin':'hash'},
              'startup':{'main':{'stack':1},'sub':{'stack':2},'room':{'stack':1}},'sub_callback':True}
        a={'passed':True,'platform':'linux/arm64','cases':{k:copy.deepcopy(case) for k in CASES}}
        b=copy.deepcopy(a);b['platform']='linux/amd64'
        with tempfile.TemporaryDirectory() as folder:
            paths=[Path(folder)/'a.json',Path(folder)/'b.json']
            def run(x,y):
                for p,r in zip(paths,[x,y]):p.write_text(json.dumps(r))
                return compare(paths)
            self.assertTrue(run(a,b))
            failures=[]
            q=copy.deepcopy(b);q['passed']=False;failures.append(q)
            q=copy.deepcopy(b);q['cases'].pop('eu-release');failures.append(q)
            q=copy.deepcopy(b);q['platform']='linux/arm64';failures.append(q)
            q=copy.deepcopy(b);q['cases']['us-release']['disc']['files']['IPX.MMD']='different';failures.append(q)
            q=copy.deepcopy(b);q['cases']['us-release']['sub_callback']=False;failures.append(q)
            for bad in failures:
                with self.assertRaises(ValueError):run(a,bad)

    def test_hardware_reports_require_tour_and_executed_probes(self):
        case={'disc':{'files':{'IPX.MMD':'hash'}},'boot_payloads':{'sp.bin':'hash'},
              'startup':{'main':{'probe_execution':True},'sub':{'probe_execution':True},'room':{}},'sub_callback':True}
        a={'passed':True,'skill':'megadev-hardware','platform':'linux/arm64','cases':{k:copy.deepcopy(case) for k in CASES|{'us-tour'}}}
        b=copy.deepcopy(a);b['platform']='linux/amd64'
        with tempfile.TemporaryDirectory() as folder:
            paths=[Path(folder)/'a.json',Path(folder)/'b.json']
            def run(right):
                for p,r in zip(paths,[a,right]):p.write_text(json.dumps(r))
                return compare(paths)
            self.assertTrue(run(b))
            for mutate in [lambda q:q['cases'].pop('us-tour'),
                           lambda q:q['cases']['us-tour']['startup']['sub'].pop('probe_execution'),
                           lambda q:q.update(skill='megadev-project')]:
                bad=copy.deepcopy(b);mutate(bad)
                with self.assertRaises(ValueError):run(bad)
