# Safe 1M / 2M Word RAM handling

## Views and units

2M and 1M mean **megabits**: 256 KiB total, or two 128 KiB banks. Main's disc-target linear window starts at `WORD_RAM` (200000); Sub uses `WORD_RAM_2M` (080000) in 2M mode and `WORD_RAM_1M` (0C0000) in 1M mode.

Megadev's Main header names `WORD_RAM_1M_BANK1` and `WORD_RAM_1M_BANK2` can mislead: 220000 is a **cell-mapped view of Main's assigned bank**, not independent access to Sub's other bank. The 1M layout interleaves physical words; do not assume the banks are contiguous halves of a former 2M linear image.

Primary implementation evidence: ares v148 [Main mapping](https://github.com/ares-emulator/ares/blob/v148/ares/md/mcd/bus-external.cpp), [Sub mapping](https://github.com/ares-emulator/ares/blob/v148/ares/md/mcd/bus-internal.cpp), [Main registers](https://github.com/ares-emulator/ares/blob/v148/ares/md/mcd/io-external.cpp) and [Sub registers](https://github.com/ares-emulator/ares/blob/v148/ares/md/mcd/io-internal.cpp). These are also the emulator model used for qualification; they do not replace tests on a console.

## Verified Megadev seam

See the selected [Main Gate Array wrapper](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/gate_arr.h) and [Sub wrapper](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/gate_arr.h). `wait_2m`, `grant_2m`, `set_1m` and `set_2m` are synchronous helpers with confirmation loops. They are not a general asynchronous memory manager. The Lab uses bounded single register requests plus later observations during runtime, keeping synchronous bootstrap loading separate.

MODE is bit 2, DMNA bit 1 and RET bit 0. In 2M, the Lab accesses Main only at low bits 001 and Sub at 010. In 1M, RET identifies Main's assigned bank; Sub sees the other bank. DMNA has exchange-handshake semantics, not the same whole-region ownership meaning as 2M.

## Lab transitions

The mailbox uses the single-writer token/commit/retirement pattern taught by `megadev-system`. Its commands are project-owned operations, not SDK APIs.

- **Prepare in 2M:** Main invalidates its view and grants the region. Sub fills in 16-word foreground slices, returns ownership, then publishes completion. Main checks actual ownership and every returned word.
- **Enter 1M:** both users are quiescent and runtime code/stacks remain in CPU-local RAM. Sub owns 2M first, selects 1M/RET=0 and prepares its linear bank. Once confirmed, Main rebuilds its own bank. Old linear data is not assumed valid.
- **Prepare back bank:** Sub fills its assigned 1M linear view while Main continues reading/verifying its different front bank. No bank identity changes.
- **Swap:** after Main's readers finish, Main invalidates the old view and requests exchange by clearing DMNA. Sub checks the request and expected RET, then flips RET. Main waits for MODE=1, changed RET and cleared request before using the newly assigned bank. Both RET directions are tested; setting RET every time is not a complete swap operation.
- **Return to 2M:** stop both users, invalidate views and have Sub clear MODE. If the resulting 2M owner is Main, Main explicitly grants the region in actual 2M mode. Sub waits for ownership, rebuilds the 2M payload, returns it and publishes completion. Code does not guess ownership from the mode-change write.

After a swap the new Sub bank contains old front data, not automatically prepared future content. The Lab marks it not-ready until another prepare completes.

## Failure and coexistence

Metadata precedes aligned 16-bit commit words; output is stable until retired. A reply alone never proves mapping. Held readers block a transition before MMIO changes. If a grant/mode/reply fails or the five-second observation deadline expires, this diagnostic quarantines its view and requires explicit disc reset. It does not clear local state and continue through an uncertain mapping. Broader recoverable command cancellation belongs to system policy.

Do not change mode/banks while executing code or a stack from Word RAM, while CDC/VDP DMA references it, or while the graphics ASIC uses it. The Lab performs no active runtime disc read, direct Word RAM VDP DMA or ASIC work. Keeping the Sub CD access loop alive is not proof that its existing 2M transfer configuration is suitable for 1M streaming.

Mode changes do not erase RAM automatically. Rebuilding here is a deliberately simple content-validity policy; preserving data with a known layout conversion is a separate advanced technique.
