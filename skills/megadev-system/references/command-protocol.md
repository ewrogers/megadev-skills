# A small command protocol

This is one optional, single-slot teaching protocol. Reuse a game's existing channel if it already provides these invariants. Queues or double buffers require their own capacity/backpressure and lifetime rules.

## Register allocation

All fields are 16-bit words. Only the named CPU writes them.

| Main → Sub | Meaning | Sub → Main | Meaning |
| --- | --- | --- | --- |
| COMCMD0 | Nonzero request token, written last; zero retires request | COMSTAT0 | Matching completion token, written last; zero retires reply |
| COMCMD1 | Opcode | COMSTAT1 | Result: 1 OK, 2 cancelled, 3 bad request, 4 wrong owner, 5 corrupt data |
| COMCMD2 | Seed | COMSTAT2 | Observational progress in words |
| COMCMD3 | Fixed count, 256 | COMSTAT3 | INT2 heartbeat, independent of job state |
| COMCMD4 | Cancellation token, or zero | COMSTAT4 | Sub startup check |
| COMCMD5 | Optional delayed-service test flag, 0/1 | COMSTAT5 | Output checksum |
| COMCMD6–7 | Unused | COMSTAT6 | Returned result tag |
| | | COMSTAT7 | Observational worker phase |

Tokens run 2–65535 and wrap to 2 only after the previous exchange is retired. With one outstanding request and no queued packets, this is enough to avoid confusing successive identical opcodes. A different transport that can retain old messages across resets needs a session/generation policy; a token alone is not universal replay protection.

COMSTAT2/7 may change while a job runs. They are hints for the display, never proof that output is complete or memory is safe to read.

## Exchange

1. Admit a request only when both commit words are zero and Main owns 2M Word RAM.
2. Main prepares a fixed 522-byte buffer: four header words, 256 data words and one guard. It writes opcode/seed/count/options with COMCMD0 still zero.
3. Main requests the 2M handoff once. It commits COMCMD0 only after observing Sub ownership. Once committed, header and buffer remain untouched by Main.
4. Sub captures the token and header, rechecks the token, validates opcode/count/options, then performs at most 16 words per foreground slice. Every slice checks ownership before touching Word RAM.
5. Sub finishes or cancels. It requests ownership return once and waits without accessing the buffer. Only after observing Main ownership does it publish result/checksum/tag and finally COMSTAT0.
6. Main accepts only its matching token with Main ownership. For successful, non-discarded work, it verifies header, every output word, guard and checksum. It then clears cancellation and COMCMD0.
7. Sub observes COMCMD0 zero and clears COMSTAT0. Main observes that zero with its buffer still owned before making the slot available.

Neither side starts another exchange during retirement. A matching reply without ownership is insufficient. Completion values stay stable until acknowledged.

The double-read header pattern assumes a cooperating sender keeps metadata immutable after publishing its commit. It does not make arbitrary concurrent header mutation safe. Never split a commit into byte stores.

## Bounded work, not a general job API

The transformation validates each input word and replaces it with a deterministic output. It needs no dynamic allocation or pointers in the mailbox. Rejected or cancelled work can leave partial output, which Main never consumes as valid.

Only 522 bytes of Word RAM are used. The rest is not free to share with another loader, ASIC operation or module while this job owns the **whole 2M region**. A 2M ownership handoff applies to the region, not just this small slice. Memory placement policy remains outside this skill.
