# Integration regressions

The live suite reuses the streaming/audio adapters from `megadev-audio`; it does
not invent another CD driver, mailbox protocol, allocator or frame publisher.
One resident Main callback services input and frame publication, and the existing
Sub loop services audio and CD work.

The visible sequence requests track 2, loads the other background, checks that
music resumes, requests track 3, deliberately loads a nonexistent file, then
loads a valid background again. Missing content is recoverable; the suite must
finish with three accepted checks and no service error. B reduces the upload
allowance to 64 bytes per VBlank; C cancels using the existing drain/retire path.
Switching to the replay page does not suspend live service coordination.

## What the execution harness controls

The original-68000 harness executes both compiled adapters. It models mailbox
register views, Word RAM ownership, directory cache entries, CDC writes and BIOS
audio command completion. It varies reader delays, omits some worker observations,
changes upload chunk sizes and injects missing files, oversized entries, reader
failure, corrupt payloads, cancellation and late results. Partial uploads must
never be published as ready; the buffer remains leased until retirement.

The combined regression initializes the actual PCM/audio adapters, starts music,
then drives streaming through its normal stop/read/resume path. The BIOS model
rejects a music command while the data reader is active. After a successful load,
the selected music intent must resume. Separate audio tests cover PCM register
and sample behavior. The suite coordinator is also executed with controlled
service completions to check retry, waiting, missing-file recovery and deadline
failure. Neither model asserts real seek latency or audible quality.

## A reproduced handoff race

A new adversarial case commits a Sub reply immediately before Main reads its
reply register. The old streaming examples sampled Word RAM mode first, then
read the reply token. That could pair a new committed reply with old Sub ownership
and falsely enter ST_FAULT. The old compiled adapter fails this regression.

The corrected adapters read the committed reply first, then take a fresh ownership
sample before validating metadata or accessing returned RAM. The correction is
shared by the streaming, audio, transition and testing fixtures. System/scaling
already teach the same observation rule; no wire format or ownership policy changed.
This case runs in the existing streaming execution harness for all affected skills.

Do not turn an observed race into sleeps or a longer arbitrary deadline. Check the
publication order and resource contract directly. Keep a non-returning peer's
resource unavailable; a test timeout is a failure report, not permission to force
an ownership change.
