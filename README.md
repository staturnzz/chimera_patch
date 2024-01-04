# chimera_patch

This patch was created to improve the success rate on 4k devices. The exploit used is a improved version of [time_waste](https://github.com/jakeajames/time_waste) by Jake James for 4k devices, I'm calling *time_saved*. While is this mainly for 4k devices it *should* work on any device supported by Chimera but won't improve success for them. Use at your own risk, no support will be given, **DO NOT** ask/submit issues/etc about this patch to the original creators of Chimera.

### Building (macOS only):
```bash
git clone --recursive https://github.com/staturnzz/chimera_patch.git
cd chimera_patch
make all
```

### Patching:
```bash
git clone --recursive https://github.com/staturnzz/chimera_patch.git
cd chimera_patch
./patch_ipa <stock.ipa> <patched.ipa>
```

Then sideload your `<patched.ipa>` and enjoy. 
**I will never release an IPA so don't ask.**

### Credits:
- [Jake James](https://github.com/jakeajames) for time_waste
- [Coolstar](https://github.com/coolstar) for Chimera
- [Odyssey Team, et al.](https://twitter.com/odysseyteam_) for Chimera

