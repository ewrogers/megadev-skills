# EverDrive detection and optional guest link

The Lab reads the EDIO signature twice at Main CPU `$A130D4`. Both values must
match `$55A0` under mask `$FFF0`, and bit 3 must toggle. An observed signature
proves that interface responds, not the base console manufacturer, USB cable
presence, host connectivity or a complete physical Sega CD implementation.

Source: [Krikzz base_io.sv at 23dbd5e](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/fpga/mapper/lib_base/base_io.sv).
The [outer FPGA module](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/fpga/mapper/lib_base/everdrive.sv)
retains the shared base interface. Main-CPU signature detection was observed in
the running CD Lab. It does not identify a Mega Sg or standardize emulator detection.

## Firmware and host observation

The initial MEGAOS 4.20 setup lost its host USB port during CD/BIOS startup.
Replugging did not restore it; returning to the EverDrive menu did. Disabling
guest FIFO polling reproduced the loss before application entry. After the user
updated firmware, USB stayed available in CD mode and repeated Lab handshakes
completed. The captured screen showed VERIFIED alongside AUTO TOUR PASS. A
subsequent build-deploy hard-reset and replaced the running disc over USB.

The updated USB identity was VID:PID 38DF:0018 with extended status discovery;
the old setup used 0483:5740 and legacy system information. This is observed
configuration evidence, not a universal firmware capability table or a claim
that the status format alone proves CD-mode USB. Test the actual setup. The MCU
implementation is not published here and the exact first supporting firmware
release is not established. BIOS Start still requires controller input.

## Bounded guest link

`USB_LAB=1` (default) enables the cooperative parser and USB HOST CHECK row.
`USB_LAB=0` performs no FIFO access and displays USB UPLOAD / MENU MODE as the
fallback workflow for an older setup. With the project skill's host helper:

```sh
python scripts/everdrive.py --port EXPLICIT_PORT verify-lab --duration 10
```

The protocol is compatible with the SGDK Hardware Lab helper, with no SGDK
runtime dependency. Twelve-byte `HLV1` packets contain a big-endian 16-bit kind,
32-bit fresh nonzero nonce, and CRC16-CCITT (polynomial 1021, initial FFFF) over
the first ten bytes. Request=1, response=2 with nonce XOR A55A5AA5, host ACK=3,
guest DONE=4. Host success requires DONE; guest liveness lasts five seconds
after ACK and requires renewal. The UI shows NOT RUN before the first exchange,
PASS/LIVE while the lease is current, and PASS/IDLE afterward. PASS/IDLE records
the last successful check without claiming a current host session. Stopping the
verifier does not mean USB disconnected. These are cooperative liveness
evidence, not authentication.

Only detected EDIO is polled, at idle Main/Sub job boundaries. Each iteration
consumes at most 32 available FIFO bytes from `$A130D1`, using the count at
`$A130D2 & 07FF`. Replies use bounded CMD_USB_WR `2B D4 22 DD`, length 000C,
and the packet, never a busy wait. The existing resident VBlank counter supplies
time; no second clock or ISR is installed. Pending/partial messages expire in
one second. The reloadable room calls the resident service only after CD completion.
No `ed_init`, FIFO flush, disk init, mount, save access or Sub-side USB service.

[Pinned guest definitions](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/edio/everdrive.h)
and [implementation](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/edio/everdrive.c)
define the registers and commands. The MCU also serves CD reads: idle checks do
not qualify concurrent streaming, PCM or CD-DA. There are no CPU breakpoints,
single-step execution or arbitrary debugger operations in this Lab protocol.
