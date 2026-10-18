# Allocation lifetimes

Choose storage by its last user, not only by the scene that requested it.

- **Resident:** vectors, communication state, shared art descriptors or audio state that survive scene changes.
- **Scene:** objects/maps/tables reset together after the scene and its deferred users finish.
- **Scratch:** short-lived decode/build space whose consumers finish before reuse.
- **In flight:** buffers still referenced by a queued VDP transfer, CPU job, drive/CDC operation or ASIC task. Their original scene may already be leaving.

The Lab's arena is an example for one scene lifetime. It is not a general Megadev heap, garbage collector or allocator recommendation for every object.

## Small bounded arena

`ma_take` validates nonzero size and power-of-two alignment up to 256, accounts for padding, and rejects overflow/full capacity without changing either the arena or output handle. Allocations do not move. Peak use includes padding. There is no individual free, so no arbitrary holes to coalesce; a reset releases the whole lifetime.

A handle contains an offset, size, pool identity and generation. Resolve it against the correct arena only after `ma_valid`; then use `backing + handle.offset`. It is not a pointer that another CPU can dereference. Pool IDs must be distinct within the application. Initialize an arena once; reinitializing it over live handles defeats the generation contract.

Reset advances the generation and invalidates old handles even if a later allocation reuses the same offset. Generation exhaustion fails closed rather than wrapping an old handle back into validity. These are advisory C checks: retaining a raw pointer bypasses them, and forged/corrupted handles are not a supported security boundary.

## Lease the actual bytes

A lease records that a deferred user still needs the allocation. Pin before publishing that work, release only after the consumer is finished, then reset. Submission is not completion. Copying a descriptor into a queue does not extend its backing lifetime unless the ownership policy does.

The Lab refuses an unload while a lease is active. Completing that simulated deferred user releases it; the next unload succeeds and the old handle is rejected. No real asynchronous VDP/CDC job is created by that particular lifetime page.

Lease records have one owner and must not be copied or independently released twice. The active flag catches reuse of the same released record; it cannot enforce linear ownership against arbitrary C struct copies. Interrupt and foreground access to the same allocator also needs the game's synchronization policy. The Lab changes its arena only in Main foreground.

## Mapping lifetime is separate

A Word RAM pointer may keep the same numeric value while the assigned physical bank changes. The single `word_view` owner advances its generation before a mode/bank transition and refuses transitions while readers hold leases. A pending or faulted view cannot be borrowed.

Keep a view lease for the entire access, including a deferred DMA if its source is Word RAM. This Lab instead verifies/copies into Main RAM and renders that resident snapshot, so no pending VDP transfer reads a bank being exchanged. The two CPUs and any loader/ASIC/audio integration must agree on the same resource policy; separate view managers for the same physical Word RAM would not provide isolation.
