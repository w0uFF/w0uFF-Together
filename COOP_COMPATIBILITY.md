# Co-op wording correction — next source build

The user confirms that the separately supplied, previously working DLL supported gameplay in co-op, including Fly. This is a **confirmed user observation** and must not be misreported as “co-op unsupported”.

Current w0uFF Together v0.4.2 has been observed to load its UI but fail to locate the local character. No co-op test of this fork is yet successful. Current checkpoint logic targets the controlled pawn; behavior of the rope and other clients is not verified. Do not claim checkpoint/rope synchronization is absent or present without direct testing.

The source-only patch in `patches/coop-copy-correction.patch` corrects the outdated wording in the Training screen and the bundled README. Apply it when preparing the next source build, then verify the build and multiplayer behavior. This patch **does not repair the Unreal tick/local-pawn problem**.

Next diagnostic in the v0.4.2 DLL: INSERT → Settings → Runtime diagnostics. Provide the values for Unreal tick callbacks, snapshot read failures, local player controller, and local pawn. Check whether callbacks increase over 3–5 seconds. Features should remain OFF by default.
