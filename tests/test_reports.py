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

    def test_asset_reports_require_disc_art_and_executed_decoder(self):
        case={'disc':{'files':{'IPX.MMD':'hash','GARDEN.MDA':'garden','CAVERN.MDA':'cavern'}},
              'boot_payloads':{'sp.bin':'hash'},'startup':{'main':{'probe_execution':True},'sub':{},'room':{}},'sub_callback':True}
        a={'passed':True,'skill':'megadev-assets','platform':'linux/arm64','cases':{k:copy.deepcopy(case) for k in CASES|{'us-tour'}}}
        b=copy.deepcopy(a);b['platform']='linux/amd64'
        with tempfile.TemporaryDirectory() as folder:
            paths=[Path(folder)/'a.json',Path(folder)/'b.json']
            def run(right):
                for p,r in zip(paths,[a,right]):p.write_text(json.dumps(r))
                return compare(paths)
            self.assertTrue(run(b))
            for mutate in [lambda q:q['cases'].pop('us-tour'),
                           lambda q:q['cases']['us-debug']['startup']['main'].pop('probe_execution'),
                           lambda q:q['cases']['us-release']['disc']['files'].pop('CAVERN.MDA')]:
                bad=copy.deepcopy(b);mutate(bad)
                with self.assertRaises(ValueError):run(bad)

    def test_graphics_reports_require_executed_publisher_and_both_layers(self):
        case={'disc':{'files':{'IPX.MMD':'hash','SKY.MDA':'sky','QUAY.MDA':'quay'}},
              'boot_payloads':{'sp.bin':'hash'},'startup':{'main':{'probe_execution':True},'sub':{},'room':{}},'sub_callback':True}
        a={'passed':True,'skill':'megadev-graphics','platform':'linux/arm64','cases':{k:copy.deepcopy(case) for k in CASES|{'us-tour'}}}
        b=copy.deepcopy(a);b['platform']='linux/amd64'
        with tempfile.TemporaryDirectory() as folder:
            paths=[Path(folder)/'a.json',Path(folder)/'b.json']
            def run(right):
                for p,r in zip(paths,[a,right]):p.write_text(json.dumps(r))
                return compare(paths)
            self.assertTrue(run(b))
            for mutate in [lambda q:q['cases'].pop('us-tour'),
                           lambda q:q['cases']['eu-release']['startup']['main'].pop('probe_execution'),
                           lambda q:q['cases']['us-release']['disc']['files'].pop('QUAY.MDA')]:
                bad=copy.deepcopy(b);mutate(bad)
                with self.assertRaises(ValueError):run(bad)

    def test_input_reports_require_ea_tour_and_executed_protocols(self):
        case={'disc':{'files':{'IPX.MMD':'hash'}},'boot_payloads':{'sp.bin':'hash'},
              'startup':{'main':{'probe_execution':True},'sub':{},'room':{}},'sub_callback':True}
        a={'passed':True,'skill':'megadev-input','platform':'linux/arm64',
           'cases':{k:copy.deepcopy(case) for k in CASES|{'us-tour','us-ea'}}}
        b=copy.deepcopy(a);b['platform']='linux/amd64'
        with tempfile.TemporaryDirectory() as folder:
            paths=[Path(folder)/'a.json',Path(folder)/'b.json']
            def run(right):
                for p,r in zip(paths,[a,right]):p.write_text(json.dumps(r))
                return compare(paths)
            self.assertTrue(run(b))
            for mutate in [lambda q:q['cases'].pop('us-ea'),
                           lambda q:q['cases'].pop('us-tour'),
                           lambda q:q['cases']['us-debug']['startup']['main'].pop('probe_execution')]:
                bad=copy.deepcopy(b);mutate(bad)
                with self.assertRaises(ValueError):run(bad)

    def test_system_reports_require_tour_and_both_executed_cpus(self):
        case={'disc':{'files':{'IPX.MMD':'hash'}},'boot_payloads':{'sp.bin':'hash'},
              'startup':{'main':{'probe_execution':True},'sub':{'probe_execution':True},'room':{}},'sub_callback':True}
        a={'passed':True,'skill':'megadev-system','platform':'linux/arm64',
           'cases':{k:copy.deepcopy(case) for k in CASES|{'us-tour'}}}
        b=copy.deepcopy(a);b['platform']='linux/amd64'
        with tempfile.TemporaryDirectory() as folder:
            paths=[Path(folder)/'a.json',Path(folder)/'b.json']
            def run(right):
                for p,r in zip(paths,[a,right]):p.write_text(json.dumps(r))
                return compare(paths)
            self.assertTrue(run(b))
            for mutate in [lambda q:q['cases'].pop('us-tour'),
                           lambda q:q['cases']['us-debug']['startup']['main'].pop('probe_execution'),
                           lambda q:q['cases']['eu-release']['startup']['sub'].pop('probe_execution')]:
                bad=copy.deepcopy(b);mutate(bad)
                with self.assertRaises(ValueError):run(bad)

    def test_memory_reports_require_both_cpus_layout_and_tour(self):
        layout={'pools':[{},{}],'word_ram':{'bank_1m_bytes':131072}}
        case={'disc':{'files':{'IPX.MMD':'hash'}},'boot_payloads':{'sp.bin':'hash'},
              'startup':{'main':{'probe_execution':True,'layout':layout},'sub':{'probe_execution':True},'room':{}},'sub_callback':True}
        a={'passed':True,'skill':'megadev-memory','platform':'linux/arm64',
           'cases':{k:copy.deepcopy(case) for k in CASES|{'us-tour'}}}
        b=copy.deepcopy(a);b['platform']='linux/amd64'
        with tempfile.TemporaryDirectory() as folder:
            paths=[Path(folder)/'a.json',Path(folder)/'b.json']
            def run(right):
                for p,r in zip(paths,[a,right]):p.write_text(json.dumps(r))
                return compare(paths)
            self.assertTrue(run(b))
            for mutate in [lambda q:q['cases'].pop('us-tour'),
                           lambda q:q['cases']['us-debug']['startup']['sub'].pop('probe_execution'),
                           lambda q:q['cases']['eu-release']['startup']['main'].pop('layout')]:
                bad=copy.deepcopy(b);mutate(bad)
                with self.assertRaises(ValueError):run(bad)

    def test_streaming_reports_require_both_adapters_disc_areas_and_tour(self):
        case={'disc':{'files':{'IPX.MMD':'hash','GARDEN.MDA':'garden','CAVERN.MDA':'cavern'}},
              'boot_payloads':{'sp.bin':'hash'},'startup':{'main':{'probe_execution':True},'sub':{'probe_execution':True},'room':{}},'sub_callback':True}
        a={'passed':True,'skill':'megadev-streaming','platform':'linux/arm64',
           'cases':{k:copy.deepcopy(case) for k in CASES|{'us-tour'}}}
        b=copy.deepcopy(a);b['platform']='linux/amd64'
        with tempfile.TemporaryDirectory() as folder:
            paths=[Path(folder)/'a.json',Path(folder)/'b.json']
            def run(right):
                for p,r in zip(paths,[a,right]):p.write_text(json.dumps(r))
                return compare(paths)
            self.assertTrue(run(b))
            for mutate in [lambda q:q['cases'].pop('us-tour'),
                           lambda q:q['cases']['us-debug']['startup']['sub'].pop('probe_execution'),
                           lambda q:q['cases']['eu-release']['disc']['files'].pop('CAVERN.MDA')]:
                bad=copy.deepcopy(b);mutate(bad)
                with self.assertRaises(ValueError):run(bad)

    def test_audio_reports_require_both_adapters_external_tracks_and_tour(self):
        case={'disc':{'files':{'IPX.MMD':'hash','GARDEN.MDA':'garden','CAVERN.MDA':'cavern'}},
              'audio_tracks':{'2':{'sha256':'two'},'3':{'sha256':'three'}},
              'boot_payloads':{'sp.bin':'hash'},'startup':{'main':{'probe_execution':True},'sub':{'probe_execution':True},'room':{}},'sub_callback':True}
        a={'passed':True,'skill':'megadev-audio','platform':'linux/arm64',
           'cases':{k:copy.deepcopy(case) for k in CASES|{'us-tour'}}}
        b=copy.deepcopy(a);b['platform']='linux/amd64'
        with tempfile.TemporaryDirectory() as folder:
            paths=[Path(folder)/'a.json',Path(folder)/'b.json']
            def run(right):
                for p,r in zip(paths,[a,right]):p.write_text(json.dumps(r))
                return compare(paths)
            self.assertTrue(run(b))
            for mutate in [lambda q:q['cases'].pop('us-tour'),
                           lambda q:q['cases']['us-debug']['startup']['sub'].pop('probe_execution'),
                           lambda q:q['cases']['eu-release']['audio_tracks'].pop('3'),
                           lambda q:q['cases']['us-release']['audio_tracks']['2'].update(sha256='changed')]:
                bad=copy.deepcopy(b);mutate(bad)
                with self.assertRaises(ValueError):run(bad)

    def test_scene_reports_require_scene_execution_in_addition_to_audio_and_reader(self):
        case={'disc':{'files':{'IPX.MMD':'ipx','GARDEN.MDA':'garden','CAVERN.MDA':'cavern'}},
              'audio_tracks':{'2':'two','3':'three'},'boot_payloads':{'sp.bin':'sp'},
              'startup':{'main':{'probe_execution':True,'scene_execution':True},'sub':{'probe_execution':True},'room':{}},'sub_callback':True}
        a={'passed':True,'skill':'megadev-scenes','platform':'linux/arm64',
           'cases':{k:copy.deepcopy(case) for k in CASES|{'us-tour'}}}
        b=copy.deepcopy(a);b['platform']='linux/amd64'
        with tempfile.TemporaryDirectory() as folder:
            paths=[Path(folder)/'a.json',Path(folder)/'b.json']
            def run(right):
                for p,r in zip(paths,[a,right]):p.write_text(json.dumps(r))
                return compare(paths)
            self.assertTrue(run(b))
            for mutate in [lambda q:q['cases'].pop('us-tour'),
                           lambda q:q['cases']['us-debug']['startup']['main'].pop('scene_execution'),
                           lambda q:q['cases']['eu-release']['audio_tracks'].pop('3'),
                           lambda q:q['cases']['us-release']['startup']['sub'].pop('probe_execution')]:
                bad=copy.deepcopy(b);mutate(bad)
                with self.assertRaises(ValueError):run(bad)

    def test_save_reports_require_both_executed_adapters_and_readonly_tour(self):
        case={'disc':{'files':{'IPX.MMD':'ipx'}},'boot_payloads':{'sp.bin':'sp'},
              'startup':{'main':{'probe_execution':True,'cart_execution':True},'sub':{'probe_execution':True},'room':{}},'sub_callback':True}
        a={'passed':True,'skill':'megadev-save','platform':'linux/arm64',
           'cases':{k:copy.deepcopy(case) for k in CASES|{'us-tour'}}}
        b=copy.deepcopy(a);b['platform']='linux/amd64'
        with tempfile.TemporaryDirectory() as folder:
            paths=[Path(folder)/'a.json',Path(folder)/'b.json']
            def run(right):
                for p,r in zip(paths,[a,right]):p.write_text(json.dumps(r))
                return compare(paths)
            self.assertTrue(run(b))
            for mutate in [lambda q:q['cases'].pop('us-tour'),
                           lambda q:q['cases']['us-release']['startup']['main'].pop('cart_execution'),
                           lambda q:q['cases']['us-debug']['startup']['main'].pop('probe_execution'),
                           lambda q:q['cases']['eu-release']['startup']['sub'].pop('probe_execution')]:
                bad=copy.deepcopy(b);mutate(bad)
                with self.assertRaises(ValueError):run(bad)

    def test_scaling_reports_require_both_cpus_asic_and_tour(self):
        case={'disc':{'files':{'IPX.MMD':'ipx'}},'boot_payloads':{'sp.bin':'sp'},
              'startup':{'main':{'probe_execution':True},'sub':{'probe_execution':True,'asic_execution':True},'room':{}},'sub_callback':True}
        a={'passed':True,'skill':'megadev-scaling','platform':'linux/arm64',
           'cases':{k:copy.deepcopy(case) for k in CASES|{'us-tour'}}}
        b=copy.deepcopy(a);b['platform']='linux/amd64'
        with tempfile.TemporaryDirectory() as folder:
            paths=[Path(folder)/'a.json',Path(folder)/'b.json']
            def run(right):
                for p,r in zip(paths,[a,right]):p.write_text(json.dumps(r))
                return compare(paths)
            self.assertTrue(run(b))
            for mutate in [lambda q:q['cases'].pop('us-tour'),
                           lambda q:q['cases']['us-release']['startup']['sub'].pop('asic_execution'),
                           lambda q:q['cases']['us-debug']['startup']['main'].pop('probe_execution')]:
                bad=copy.deepcopy(b);mutate(bad)
                with self.assertRaises(ValueError):run(bad)

    def test_debug_requires_paired_diagnostic_evidence_and_tour(self):
        case={'disc':{'files':{'IPX.MMD':'ipx'}},'boot_payloads':{'sp.bin':'sp'},
              'startup':{'main':{'probe_execution':True,'debug_execution':True},'sub':{'probe_execution':True},'room':{}},'sub_callback':True}
        a={'passed':True,'skill':'megadev-debug','platform':'linux/arm64',
           'cases':{k:copy.deepcopy(case) for k in CASES|{'us-tour'}}}
        b=copy.deepcopy(a);b['platform']='linux/amd64'
        with tempfile.TemporaryDirectory() as folder:
            paths=[Path(folder)/'a.json',Path(folder)/'b.json']
            def run(right):
                for p,r in zip(paths,[a,right]):p.write_text(json.dumps(r))
                return compare(paths)
            self.assertTrue(run(b))
            for mutate in [lambda q:q['cases'].pop('us-tour'),
                           lambda q:q['cases']['us-release']['startup']['main'].pop('debug_execution'),
                           lambda q:q['cases']['eu-release']['startup']['sub'].pop('probe_execution')]:
                bad=copy.deepcopy(b);mutate(bad)
                with self.assertRaises(ValueError):run(bad)

    def test_testing_reports_require_replay_combined_io_audio_and_tour(self):
        case={'disc':{'files':{'IPX.MMD':'ipx','GARDEN.MDA':'garden','CAVERN.MDA':'cavern'}},'boot_payloads':{'sp.bin':'sp'},
              'audio_tracks':{'2':'two','3':'three'},
              'startup':{'main':{'probe_execution':True,'replay_execution':True,'integration_execution':True},'sub':{'probe_execution':True},'room':{}},'sub_callback':True}
        a={'passed':True,'skill':'megadev-testing','platform':'linux/arm64',
           'cases':{k:copy.deepcopy(case) for k in CASES|{'us-tour'}}}
        b=copy.deepcopy(a);b['platform']='linux/amd64'
        with tempfile.TemporaryDirectory() as folder:
            paths=[Path(folder)/'a.json',Path(folder)/'b.json']
            def run(right):
                for p,r in zip(paths,[a,right]):p.write_text(json.dumps(r))
                return compare(paths)
            self.assertTrue(run(b))
            for mutate in [lambda q:q['cases'].pop('us-tour'),
                           lambda q:q['cases']['us-release']['startup']['main'].pop('replay_execution'),
                           lambda q:q['cases']['eu-release']['startup']['main'].pop('integration_execution'),
                           lambda q:q['cases']['us-debug']['audio_tracks'].pop('3'),
                           lambda q:q['cases']['us-tour']['startup']['sub'].pop('probe_execution')]:
                bad=copy.deepcopy(b);mutate(bad)
                with self.assertRaises(ValueError):run(bad)

    def test_performance_reports_require_observer_and_asic_evidence(self):
        case={'disc':{'files':{'IPX.MMD':'ipx'}},'boot_payloads':{'sp.bin':'sp'},
              'startup':{'main':{'probe_execution':True,'measurement_execution':True},'sub':{'probe_execution':True,'asic_execution':True},'room':{}},'sub_callback':True}
        a={'passed':True,'skill':'megadev-performance','platform':'linux/arm64',
           'cases':{k:copy.deepcopy(case) for k in CASES|{'us-tour'}}}
        b=copy.deepcopy(a);b['platform']='linux/amd64'
        with tempfile.TemporaryDirectory() as folder:
            paths=[Path(folder)/'a.json',Path(folder)/'b.json']
            def run(right):
                for p,r in zip(paths,[a,right]):p.write_text(json.dumps(r))
                return compare(paths)
            self.assertTrue(run(b))
            for mutate in [lambda q:q['cases'].pop('us-tour'),
                           lambda q:q['cases']['us-release']['startup']['main'].pop('measurement_execution'),
                           lambda q:q['cases']['eu-release']['startup']['sub'].pop('asic_execution')]:
                bad=copy.deepcopy(b);mutate(bad)
                with self.assertRaises(ValueError):run(bad)
