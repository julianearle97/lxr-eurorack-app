# IMPORTANT: Ride sample bank mismatch

The repository's `Core/Src/Soundengine/UserSampleBank.c` still contains the original 1.50-second `live ride2.wav` in slot 8.

The user replaced it in their second uploaded archive `lxr drums(1).zip` with a shorter, ~1.06-second ride. Previous versions v1.57.1 and v1.57.2 with this change were produced by patching the compiled binary; **the new ride was never committed to source**.

Before supplying a new firmware image to the user, patch slot 8 in the compiled image using the verified ride replacement from the previous v1.57.1 image or rebuild `UserSampleBank.c` using the *second* archive, leaving the other eleven samples identical.

This is an experimental firmware branch; do not imply that the latest ride has been incorporated merely because the source compiles. 
