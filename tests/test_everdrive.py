"""Protocol transcripts and native serial integration; no console is contacted."""
from collections import deque
import contextlib
import importlib.util
import io
import json
import os
from pathlib import Path
import select
import shutil
import struct
import subprocess
import sys
import tempfile
import threading
import unittest
from unittest.mock import patch
from types import SimpleNamespace


ROOT = Path(__file__).resolve().parents[1]
SKILL = ROOT / "skills/megadev-project"
sys.path.insert(0, str(SKILL / "scripts"))
SPEC = importlib.util.spec_from_file_location("everdrive", SKILL / "scripts/everdrive.py")
usb = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(usb)


def frame(command, payload=b""):
    return bytes((0x2B, 0xD4, command, command ^ 0xFF)) + payload


def memory(address, data):
    return frame(0x1A, struct.pack(">IIB", address, len(data), 0) + data)


def identity(protocol=5, device=0x18):
    return [(bytes(66), b""), (frame(0x40), bytes((0x5A, protocol, device, 0))),
            (frame(0x10), b"\xA5\x00")]


def legacy_identity(device=0x18):
    info = bytearray(64)
    info[44:50] = bytes.fromhex("011200010101")
    info[50] = device
    return [(bytes(66), b""), (frame(0x40), b""),
            (frame(0x10), b"\xA5\x00"), (frame(0x26), bytes(info))]


class TranscriptPort:
    """A peer that releases responses only after the expected request arrives."""

    def __init__(self, steps, read_fragment=65536, write_fragment=65536):
        self.steps = deque(steps)
        self.read_fragment = read_fragment
        self.write_fragment = write_fragment
        self.expected_offset = 0
        self.received = bytearray()
        self.responses = bytearray()
        self.write_sizes = []

    def reset_input_buffer(self):
        self.responses.clear()

    def write(self, data):
        size = min(len(data), self.write_fragment)
        self.write_sizes.append(len(data))
        data = data[:size]
        self.received.extend(data)
        while data:
            if not self.steps:
                raise AssertionError(f"unexpected request {data.hex()}")
            expected, response = self.steps[0]
            length = min(len(data), len(expected) - self.expected_offset)
            actual = data[:length]
            want = expected[self.expected_offset:self.expected_offset + length]
            if actual != want:
                raise AssertionError(f"wire mismatch: got {actual.hex()}, expected {want.hex()}")
            self.expected_offset += length
            data = data[length:]
            if self.expected_offset == len(expected):
                self.responses.extend(response)
                self.steps.popleft()
                self.expected_offset = 0
        return size

    def read(self, size):
        length = min(size, self.read_fragment, len(self.responses))
        result = bytes(self.responses[:length])
        del self.responses[:length]
        return result


def bundle_steps(payloads, reset=2, install=0):
    metadata = {name: {'bytes': len(data), 'sha256': usb.hashlib.sha256(data).hexdigest()}
                for name, data in payloads.items()}
    tag = usb.hashlib.sha256(json.dumps(metadata, sort_keys=True).encode()).hexdigest()[:12]
    folder = 'usb-games/game-' + tag
    steps = identity() + [(frame(0x29, bytes((reset,))), b''),
                          (memory(0x1800000, bytes(256)), b''), (frame(0x29, b'\0'), b'r')]
    for name in ('usb-games', folder):
        steps += [(frame(0xd2, usb._encode_path(name)), b''), (frame(0x10), b'\xa5\x08')]
    for name, data in payloads.items():
        steps += [(frame(0xc9, b'\x0a'+usb._encode_path(folder+'/'+name)), b''),
                  (frame(0x10), b'\xa5\0'), (frame(0xcc, struct.pack('>I', len(data))), b'\0')]
        for offset in range(0, len(data), 1024):
            steps.append((data[offset:offset+1024], b'\0' if offset+1024 < len(data) else b''))
        steps += [(frame(0x10), b'\xa5\0'), (frame(0xce), b''), (frame(0x10), b'\xa5\0')]
    path = usb._encode_path(folder+'/game.cue')
    steps += [(memory(0x1810000, b'*i'), b''), (memory(0x1810000, path[:2]), b''),
              (memory(0x1810000, path[2:]), bytes((install,)))]
    if install == 0:
        steps.append((memory(0x1810000, b'*s'), b''))
    return steps, metadata


def fake_files(payloads):
    return {name: SimpleNamespace(open=lambda mode, data=data: io.BytesIO(data)) for name, data in payloads.items()}


def iso_bytes():
    data = bytearray(17*2048)
    data[:16] = b'SEGADISCSYSTEM  '
    data[32768:32775] = b'\x01CD001\x01'
    return bytes(data)


class DiscDeploymentTests(unittest.TestCase):
    def test_all_tracks_then_cue_then_install_with_fragmented_io(self):
        payloads = {'game.iso': bytes(range(256))*10+b'tail', 'music.wav': b'a'*512, 'game.cue': b'cue'}
        steps, metadata = bundle_steps(payloads)
        port = TranscriptPort(steps, read_fragment=1, write_fragment=7)
        client = usb.MegaEverDrivePro(port)
        with patch.object(usb.time, 'sleep'):
            client.identify()
            result = client.deploy_disc('game.cue', fake_files(payloads), metadata)
        self.assertFalse(port.steps);self.assertFalse(port.responses)
        self.assertTrue(result['installed']);self.assertTrue(result['launch_sent'])
        self.assertFalse(result['runtime_verified']);self.assertEqual(result['reset'], 'hard')
        self.assertEqual(result['files'], metadata)

    def test_explicit_soft_does_not_change_subsequent_hard_default(self):
        data = {'game.iso': b'x'*512, 'game.cue': b'cue'}
        first, metadata = bundle_steps(data, reset=1); second, _ = bundle_steps(data)
        port = TranscriptPort(first+second[3:]);client=usb.MegaEverDrivePro(port)
        with patch.object(usb.time,'sleep'):
            client.identify();client.deploy_disc('game.cue',fake_files(data),metadata,'soft')
            self.assertEqual(client.deploy_disc('game.cue',fake_files(data),metadata)['reset'],'hard')
        self.assertFalse(port.steps);self.assertNotIn(512,port.write_sizes)

    def test_wrong_device_rejected_before_reset_and_legacy_pro_supported(self):
        for steps in (identity(device=0x25),legacy_identity(device=0x25),identity(protocol=6)):
            port=TranscriptPort(steps)
            with self.assertRaisesRegex(usb.ProtocolError,'expected Mega EverDrive Pro'):
                usb.MegaEverDrivePro(port).identify()
            self.assertNotIn(frame(0x29),port.received)
        port=TranscriptPort(legacy_identity(),read_fragment=1)
        result=usb.MegaEverDrivePro(port).identify()
        self.assertEqual(result['mcu_version'],'0x0112');self.assertFalse(port.steps)

    def test_install_failure_never_launches(self):
        data={'game.iso':b'iso','game.cue':b'cue'};steps,metadata=bundle_steps(data,install=9)
        port=TranscriptPort(steps);client=usb.MegaEverDrivePro(port)
        with patch.object(usb.time,'sleep'):
            client.identify()
            with self.assertRaisesRegex(usb.ProtocolError,'installation failed.*09'):
                client.deploy_disc('game.cue',fake_files(data),metadata)
        self.assertFalse(port.steps);self.assertNotIn(memory(0x1810000,b'*s'),port.received)

    def test_reset_release_even_after_interruption_or_expired_deadline(self):
        for error in (OSError('disconnected'),KeyboardInterrupt()):
            port=TranscriptPort(identity()+[(frame(0x29,b'\x02'),b''),(frame(0x29,b'\0'),b'')])
            client=usb.MegaEverDrivePro(port);client.identify()
            with patch.object(usb.time,'sleep'),patch.object(client,'_memory_write',side_effect=error):
                with self.assertRaises(type(error)):client._reset_to_menu('hard')
            self.assertFalse(client.reset_asserted);self.assertFalse(port.steps)

    def test_expired_overall_deadline_still_releases_asserted_reset(self):
        port=TranscriptPort(identity()+[(frame(0x29,b'\x02'),b''),(frame(0x29,b'\0'),b'')])
        client=usb.MegaEverDrivePro(port);client.identify()
        def expire(seconds):client.deadline=0
        with patch.object(usb.time,'sleep',side_effect=expire),self.assertRaisesRegex(usb.ProtocolError,'timeout'):
            client._reset_to_menu('hard')
        self.assertFalse(client.reset_asserted);self.assertFalse(port.steps)

    def test_rejected_track_write_does_not_install_or_launch(self):
        data={'game.iso':b'iso','game.cue':b'cue'};steps,metadata=bundle_steps(data)
        index=next(i for i,(request,_) in enumerate(steps) if request.startswith(frame(0xcc)))
        steps[index]=(steps[index][0],b'\x07')
        port=TranscriptPort(steps[:index+1]);client=usb.MegaEverDrivePro(port)
        with patch.object(usb.time,'sleep'):
            client.identify()
            with self.assertRaisesRegex(usb.ProtocolError,'write rejected'):
                client.deploy_disc('game.cue',fake_files(data),metadata)
        self.assertFalse(port.steps);self.assertNotIn(memory(0x1810000,b'*i'),port.received)

    def test_missing_menu_does_not_fall_back_to_soft(self):
        port=TranscriptPort(identity()+[(frame(0x29,b'\x02'),b''),(memory(0x1800000,bytes(256)),b''),(frame(0x29,b'\0'),b'')])
        client=usb.MegaEverDrivePro(port);client.identify()
        with patch.object(usb.time,'sleep'),self.assertRaisesRegex(usb.ProtocolError,'menu ready'):
            client._reset_to_menu('hard')
        self.assertFalse(port.steps);self.assertNotIn(frame(0x29,b'\x01'),port.received)

    def test_failed_build_never_opens_usb_and_preserves_exit_code(self):
        with tempfile.TemporaryDirectory() as folder:
            (Path(folder)/'Makefile').write_text('all:')
            with patch.object(usb.subprocess,'run',return_value=SimpleNamespace(returncode=7)) as run,patch.object(usb,'open_serial') as opening:
                self.assertEqual(usb.main(['--port','unused','build-deploy',folder,'game.iso']),7)
                opening.assert_not_called()
                self.assertIn('--cpus=2',run.call_args.args[0]);self.assertIn('ewrogers/megadev:latest',run.call_args.args[0])


class DiscPreflightTests(unittest.TestCase):
    def test_iso_wrapped_in_cue_and_snapshot_is_immutable(self):
        with tempfile.TemporaryDirectory(prefix='disc path ') as folder:
            image=Path(folder)/'game.iso';image.write_bytes(iso_bytes())
            with usb.prepare_disc(image) as (entry,files,metadata):
                image.write_bytes(b'rebuilt')
                self.assertEqual(entry,'game.cue');self.assertEqual(list(files),['game.iso','game.cue'])
                self.assertEqual(files['game.iso'].read_bytes(),iso_bytes())
                self.assertIn(b'TRACK 01 MODE1/2048',files[entry].read_bytes())
                snapshot=files[entry]
            self.assertFalse(snapshot.exists())

    def test_bad_disc_and_missing_or_unsafe_dependencies_never_open_usb(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);(root/'game.iso').write_bytes(iso_bytes())
            for text in ('FILE "../game.iso" BINARY\nTRACK 01 MODE1/2048\nINDEX 01 00:00:00',
                         'FILE "missing.iso" BINARY\nTRACK 01 MODE1/2048\nINDEX 01 00:00:00',
                         'FILE "game.iso" BINARY\nTRACK 01 MODE1/2352\nINDEX 01 00:00:00',
                         'FILE "game.iso" BINARY\nTRACK 01 MODE1/2048\nINDEX 01 00:00:00\nFILE "missing.wav" WAVE\nTRACK 02 AUDIO\nINDEX 01 00:00:00'):
                cue=root/'game.cue';cue.write_text(text)
                with patch.object(usb,'open_serial') as opening,self.assertRaises(ValueError):
                    usb.deploy_disc(cue,'unused')
                opening.assert_not_called()
            (root/'game.iso').write_bytes(b'bad')
            with patch.object(usb,'open_serial') as opening,self.assertRaises(ValueError):usb.deploy_disc(root/'game.iso','unused')
            opening.assert_not_called()

    def test_authored_wav_track_and_symlink_rejection(self):
        import wave
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);(root/'game.iso').write_bytes(iso_bytes())
            with wave.open(str(root/'song.wav'),'wb') as audio:
                audio.setparams((2,2,44100,0,'NONE','not compressed'));audio.writeframes(bytes(44100*4*4))
            cue=root/'game.cue';cue.write_text('FILE "game.iso" BINARY\nTRACK 01 MODE1/2048\nINDEX 01 00:00:00\nFILE "song.wav" WAVE\nTRACK 02 AUDIO\nINDEX 01 00:00:00\n')
            with usb.prepare_disc(cue) as (_,files,_):self.assertEqual(list(files),['game.iso','song.wav','game.cue'])
            (root/'song.wav').write_bytes(b'bad wave')
            with patch.object(usb,'open_serial') as opening,self.assertRaises(ValueError):
                usb.deploy_disc(cue,'unused')
            opening.assert_not_called()
            (root/'song.wav').unlink()
            try:(root/'song.wav').symlink_to(root/'game.iso')
            except (OSError,NotImplementedError):return  # Some Windows hosts prohibit symlinks.
            with self.assertRaisesRegex(ValueError,'unsafe track'):
                with usb.prepare_disc(cue):self.fail('must reject symlink')


class HardwareLabLinkTests(unittest.TestCase):
    @staticmethod
    def packet(kind, nonce):
        import binascii
        body = b"HLV1" + struct.pack(">HI", kind, nonce)
        return body + struct.pack(">H", binascii.crc_hqx(body, 0xffff))

    def exchange(self, nonce, first_reply=None):
        reply = self.packet(2, nonce ^ 0xA55A5AA5) if first_reply is None else first_reply
        return [(memory(0x1810000, self.packet(1, nonce)), reply),
                (memory(0x1810000, self.packet(3, nonce)), self.packet(4, nonce))]

    def test_fragmented_exchange_waits_for_matching_response_and_guest_ack(self):
        response = b"noise" + self.packet(2, 0x777) + self.packet(2, 0x12345678 ^ 0xA55A5AA5)
        port = TranscriptPort(identity() + self.exchange(0x12345678,response), read_fragment=1, write_fragment=3)
        client = usb.MegaEverDrivePro(port);client.identify()
        with patch.object(usb.secrets, "randbelow", return_value=0x12345677):
            result = client.verify_lab()
        self.assertTrue(result["lab_link_verified"])
        self.assertEqual((result["handshakes"], result["lease_seconds"]), (1, 5))
        self.assertEqual(result["console_model"], "unknown")
        self.assertFalse(port.steps);self.assertFalse(port.responses)

    def test_stale_or_corrupt_response_does_not_send_ack(self):
        bad = bytearray(self.packet(2, 0x123 ^ 0xA55A5AA5));bad[-1] ^= 1
        for reply in [self.packet(4,0x123),bytes(bad),self.packet(2,0x124 ^ 0xA55A5AA5)]:
            port = TranscriptPort(identity()+self.exchange(0x123,reply)[:1])
            client = usb.MegaEverDrivePro(port);client.identify()
            with patch.object(usb.secrets, "randbelow", return_value=0x122):
                with self.assertRaisesRegex(usb.ProtocolError, "handshake"):client.verify_lab()
            self.assertFalse(port.steps)

    def test_missing_final_guest_ack_is_a_failure(self):
        steps = self.exchange(0x123);steps[-1] = (steps[-1][0],b"")
        port = TranscriptPort(identity()+steps)
        client = usb.MegaEverDrivePro(port);client.identify()
        with patch.object(usb.secrets, "randbelow", return_value=0x122):
            with self.assertRaises(usb.ProtocolError):client.verify_lab()
        self.assertFalse(port.steps)

    def test_absent_guest_times_out_without_ack_reset_or_sd_writes(self):
        port = TranscriptPort(identity()+self.exchange(0x123,b"")[:1])
        client = usb.MegaEverDrivePro(port);client.identify()
        with patch.object(usb.secrets, "randbelow", return_value=0x122):
            with self.assertRaises(usb.ProtocolError):client.verify_lab()
        self.assertFalse(port.steps)

    def test_duration_renews_with_a_fresh_nonce(self):
        port = TranscriptPort(identity()+self.exchange(0x123)+self.exchange(0x124))
        now = [10.0]
        with patch.object(usb.time, "monotonic", side_effect=lambda: now[0]), patch.object(usb.time, "sleep", side_effect=lambda seconds: now.__setitem__(0, now[0]+seconds)), patch.object(usb.secrets, "randbelow", return_value=0x122):
            client = usb.MegaEverDrivePro(port);client.identify()
            result = client.verify_lab(duration=1)
        self.assertEqual(result["handshakes"],2);self.assertFalse(port.steps)

    def test_unidentified_or_expired_transport_cannot_verify(self):
        port = TranscriptPort([]);client = usb.MegaEverDrivePro(port)
        with self.assertRaisesRegex(usb.ProtocolError,"identify"):client.verify_lab()
        client.identified=True;client.deadline=0
        with self.assertRaisesRegex(usb.ProtocolError,"timeout"):client.verify_lab()
        self.assertFalse(port.received)

    def test_invalid_duration_never_opens_the_port(self):
        for value in ["nan","inf","-1","301"]:
            with patch.object(usb,"open_serial") as opening, contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit):usb.main(["--port","unused","verify-lab","--duration",value])
                opening.assert_not_called()




@unittest.skipUnless(os.name == "posix" and importlib.util.find_spec("serial"),
                     "native pseudo-terminal integration needs POSIX and pySerial")
class NativeSerialTests(unittest.TestCase):
    def test_installed_cli_deploys_over_real_serial_api_to_simulated_cartridge(self):
        import pty
        master, slave = pty.openpty()
        port = os.ttyname(slave)
        data = iso_bytes()
        errors = []
        cue = b'FILE "game.iso" BINARY\n  TRACK 01 MODE1/2048\n    INDEX 01 00:00:00\n'
        steps, _ = bundle_steps({'game.iso': data, 'game.cue': cue})

        def peer():
            try:
                for expected, response in steps:
                    received = bytearray()
                    while len(received) < len(expected):
                        if not select.select([master], [], [], 8)[0]:
                            raise AssertionError("host did not send the next protocol request")
                        received.extend(os.read(master, len(expected) - len(received)))
                    if received != expected:
                        raise AssertionError(f"request mismatch: {bytes(received).hex()}")
                    if response:
                        os.write(master, response)
            except BaseException as error:
                errors.append(error)

        thread = threading.Thread(target=peer, daemon=True)
        try:
            with tempfile.TemporaryDirectory(prefix="installed USB skill ") as temporary:
                installed = Path(temporary) / "megadev-project"
                shutil.copytree(SKILL, installed, ignore=shutil.ignore_patterns("__pycache__"))
                rom = Path(temporary) / "game.iso"
                rom.write_bytes(data)
                thread.start()
                command = [sys.executable, str(installed / "scripts/everdrive.py")]
                if sys.platform == "darwin":
                    # macOS PTYs reject the custom baud ioctl used for 921600.
                    # Change only the PTY line speed; retain real pySerial I/O
                    # and run the copied CLI in its own process.
                    bootstrap = ("import runpy,sys; from pathlib import Path; sys.path.insert(0,str(Path(sys.argv[1]).parent)); ns=runpy.run_path(sys.argv[1]); "
                                 "ns['main'].__globals__['BAUDRATE']=115200; "
                                 "raise SystemExit(ns['main'](sys.argv[2:]))")
                    command = [sys.executable, "-c", bootstrap, command[1]]
                result = subprocess.run(command + [
                                         "--port", port, "--timeout", "10", "deploy", str(rom)],
                                        cwd=temporary, capture_output=True, text=True, timeout=15)
                self.assertEqual(result.returncode, 0, result.stderr)
                status = json.loads(result.stdout)
                self.assertTrue(status["installed"])
                self.assertTrue(status["launch_sent"])
                self.assertFalse(status["runtime_verified"])
                self.assertEqual(status["reset"], "hard")
                thread.join(timeout=1)
                self.assertFalse(thread.is_alive(), "simulated cartridge did not finish")
                self.assertEqual(errors, [])
        finally:
            os.close(slave)
            os.close(master)


if __name__ == "__main__":
    unittest.main()
